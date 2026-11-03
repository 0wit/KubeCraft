#include "Voxel/VoxelWorld.h"

#include "Engine/World.h"
#include "KubeCraft.h"
#include "Voxel/BlockCatalog.h"
#include "Voxel/VoxelChunk.h"

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

FBlockId AVoxelWorld::GetBlock(const FIntVector& Block) const
{
	const TObjectPtr<AVoxelChunk>* Chunk = Chunks.Find(BlockToChunk(Block));
	// The entry can also be null: the engine clears it when a chunk actor is destroyed.
	return (Chunk && *Chunk) ? (*Chunk)->GetBlock(BlockToLocal(Block)) : AirBlockId;
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
