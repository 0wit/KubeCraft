#include "World/VoxelWorld.h"

#include "Blocks/BlockCatalog.h"
#include "Blocks/BlockRules.h"
#include "Engine/World.h"
#include "KubeCraft.h"
#include "World/VoxelChunk.h"

using namespace KubeCraft::Voxel;

AVoxelWorld::AVoxelWorld()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AVoxelWorld::Generate()
{
	if (!ensureMsgf(Chunks.IsEmpty(), TEXT("%s: Generate() was called twice."), *GetName()))
	{
		return;
	}
	if (!Catalog || !BlockMesh)
	{
		UE_LOG(LogKubeCraft, Error, TEXT("%s: Catalog and BlockMesh must both be assigned; the world stays empty."),
			*GetName());
		return;
	}

	if (bRandomSeed)
	{
		// The clock counts in 100 ns steps, so its lowest 32 bits are different on every start.
		Seed = static_cast<int32>(FDateTime::Now().GetTicks());
	}
	UE_LOG(LogKubeCraft, Log, TEXT("World seed: %d"), Seed);
	const FTerrainGenerator Generator(Seed, Terrain);

	// The block type depends only on the height, so look it up once per height instead of once per block.
	TArray<FBlockId> BlockIdPerHeight;
	BlockIdPerHeight.SetNumUninitialized(ChunkHeight);
	for (int32 Z = 0; Z < ChunkHeight; ++Z)
	{
		BlockIdPerHeight[Z] = Catalog->GetIdForHeight(Z);
	}

	// Chunks from -Size/2 to Size/2 - 1 on each axis, so block (0, 0) is at the center of the world.
	const int32 FirstChunk = -WorldSizeInChunks / 2;
	for (int32 Y = 0; Y < WorldSizeInChunks; ++Y)
	{
		for (int32 X = 0; X < WorldSizeInChunks; ++X)
		{
			SpawnChunk(FIntPoint(FirstChunk + X, FirstChunk + Y), Generator, BlockIdPerHeight);
		}
	}

	// Draw only after every chunk is filled, so the faces on chunk borders can look into the neighbor.
	for (const TPair<FIntPoint, TObjectPtr<AVoxelChunk>>& Pair : Chunks)
	{
		Pair.Value->RebuildMesh();
	}
}

void AVoxelWorld::SpawnChunk(const FIntPoint& ChunkCoord, const FTerrainGenerator& Generator,
	const TArray<FBlockId>& BlockIdPerHeight)
{
	const FIntVector Origin = ChunkToBlockOrigin(ChunkCoord);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	AVoxelChunk* Chunk = GetWorld()->SpawnActor<AVoxelChunk>(BlockToWorldCorner(Origin), FRotator::ZeroRotator, SpawnParams);
	if (!ensureMsgf(Chunk, TEXT("%s: could not spawn chunk %s"), *GetName(), *ChunkCoord.ToString()))
	{
		return;
	}
	Chunk->Initialize(*this, ChunkCoord);

	// Fill each column from the bottom up to the ground height; the height bands pick the block type.
	for (int32 Y = 0; Y < ChunkSize; ++Y)
	{
		for (int32 X = 0; X < ChunkSize; ++X)
		{
			const int32 Height = Generator.GetSurfaceHeight(Origin.X + X, Origin.Y + Y);
			for (int32 Z = 0; Z < Height; ++Z)
			{
				Chunk->SetBlock(FIntVector(X, Y, Z), BlockIdPerHeight[Z]);
			}
		}
	}

	Chunks.Add(ChunkCoord, Chunk);
}

AVoxelChunk* AVoxelWorld::FindChunk(const FIntPoint& ChunkCoord) const
{
	const TObjectPtr<AVoxelChunk>* Chunk = Chunks.Find(ChunkCoord);
	// The entry can also be null: the engine clears it when a chunk actor is destroyed.
	return Chunk ? Chunk->Get() : nullptr;
}

FBlockId AVoxelWorld::GetBlock(const FIntVector& Block) const
{
	const AVoxelChunk* Chunk = FindChunk(BlockToChunk(Block));
	return Chunk ? Chunk->GetBlock(BlockToLocal(Block)) : AirBlockId;
}

const UBlockDefinition* AVoxelWorld::GetBlockDefinition(const FIntVector& Block) const
{
	return Catalog ? Catalog->GetDefinition(GetBlock(Block)) : nullptr;
}

FVector AVoxelWorld::GetSurfaceLocation(const FIntPoint& Column) const
{
	// Walk down from the ceiling to the first solid block. An empty column ends at Z = -1 (the world floor).
	int32 Z = ChunkHeight - 1;
	while (Z >= 0 && GetBlock(FIntVector(Column.X, Column.Y, Z)) == AirBlockId)
	{
		--Z;
	}
	return BlockToWorldTopCenter(FIntVector(Column.X, Column.Y, Z));
}

bool AVoxelWorld::IsInsideWorld(const FIntVector& Block) const
{
	// The chunks are exactly as tall as the world, so the chunk's own bounds are the floor and the ceiling.
	return IsInsideChunk(BlockToLocal(Block)) && FindChunk(BlockToChunk(Block));
}

bool AVoxelWorld::MineBlock(const FIntVector& Block)
{
	if (!FBlockRules(*this).CanMine(Block))
	{
		return false;
	}
	return SetBlockAndRedraw(Block, AirBlockId);
}

bool AVoxelWorld::PlaceBlock(const FIntVector& Block, const UBlockDefinition* Type)
{
	// CanPlace needs a chunk, and chunks only exist when Generate found a catalog.
	if (!FBlockRules(*this).CanPlace(Block))
	{
		return false;
	}
	// Air means the type is null or not in the catalog: a chunk could not draw it.
	const FBlockId Id = Catalog->GetId(Type);
	if (Id == AirBlockId)
	{
		return false;
	}
	return SetBlockAndRedraw(Block, Id);
}

bool AVoxelWorld::SetBlockAndRedraw(const FIntVector& Block, FBlockId Id)
{
	AVoxelChunk* Chunk = FindChunk(BlockToChunk(Block));
	const FIntVector Local = BlockToLocal(Block);
	if (!Chunk || !Chunk->SetBlock(Local, Id))
	{
		return false;
	}

	// Only the block itself and the six blocks touching it can appear or disappear.
	Chunk->RedrawBlock(Local);
	for (const FIntVector& Offset : FaceNeighborOffsets)
	{
		RedrawBlock(Block + Offset);
	}
	return true;
}

void AVoxelWorld::RedrawBlock(const FIntVector& Block)
{
	// Neighbors below the floor, above the ceiling or past the edge of the world have nothing to draw.
	// (Same test as IsInsideWorld, but the chunk is needed afterwards, so it is looked up only once.)
	AVoxelChunk* Chunk = FindChunk(BlockToChunk(Block));
	const FIntVector Local = BlockToLocal(Block);
	if (Chunk && IsInsideChunk(Local))
	{
		Chunk->RedrawBlock(Local);
	}
}
