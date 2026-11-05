#include "World/VoxelWorld.h"

#include "Blocks/BlockCatalog.h"
#include "Blocks/BlockRules.h"
#include "Engine/World.h"
#include "KubeCraft.h"
#include "World/ChunkStreamingComponent.h"
#include "World/VoxelChunk.h"

using namespace KubeCraft::Voxel;

DECLARE_CYCLE_STAT(TEXT("Chunk Load"), STAT_ChunkLoad, STATGROUP_KubeCraft);

AVoxelWorld::AVoxelWorld()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Streaming = CreateDefaultSubobject<UChunkStreamingComponent>(TEXT("Streaming"));
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

	PrepareTerrain();

	// Everything around the start at once, so the player has ground under their feet; the rest follows them.
	Streaming->LoadAllAround(FIntPoint::ZeroValue);
	Streaming->SetComponentTickEnabled(true);
}

void AVoxelWorld::PrepareTerrain()
{
	if (bRandomSeed)
	{
		// The clock counts in 100 ns steps, so its lowest 32 bits are different on every start.
		Seed = static_cast<int32>(FDateTime::Now().GetTicks());
	}
	UE_LOG(LogKubeCraft, Log, TEXT("World seed: %d"), Seed);
	Generator.Emplace(Seed, Terrain);
}

void AVoxelWorld::LoadChunk(const FIntPoint& ChunkCoord)
{
	SCOPE_CYCLE_COUNTER(STAT_ChunkLoad);

	// Generate makes the generator only once the catalog and the block mesh are known to be set.
	if (!ensureMsgf(Generator, TEXT("%s: LoadChunk before Generate"), *GetName()) || FindChunk(ChunkCoord))
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	AVoxelChunk* Chunk = GetWorld()->SpawnActor<AVoxelChunk>(
		BlockToWorldCorner(ChunkToBlockOrigin(ChunkCoord)), FRotator::ZeroRotator, SpawnParams);
	if (!ensureMsgf(Chunk, TEXT("%s: could not spawn chunk %s"), *GetName(), *ChunkCoord.ToString()))
	{
		return;
	}
	Chunk->Initialize(*this, ChunkCoord);

	// The same seed always makes the same terrain, so only the player's changes need to be kept.
	Chunk->SetAllBlocks(Generator->GenerateChunk(ChunkCoord, *Catalog));
	if (const TMap<FIntVector, FBlockId>* Changes = ChangedBlocks.Find(ChunkCoord))
	{
		for (const TPair<FIntVector, FBlockId>& Change : *Changes)
		{
			Chunk->SetBlock(Change.Key, Change.Value);
		}
	}

	Chunks.Add(ChunkCoord, Chunk);
}

void AVoxelWorld::DrawChunk(const FIntPoint& ChunkCoord)
{
	if (AVoxelChunk* Chunk = FindChunk(ChunkCoord))
	{
		Chunk->RebuildMesh();
	}
}

void AVoxelWorld::UnloadChunk(const FIntPoint& ChunkCoord)
{
	if (AVoxelChunk* Chunk = FindChunk(ChunkCoord))
	{
		Chunk->Destroy();
	}
	Chunks.Remove(ChunkCoord);
}

bool AVoxelWorld::IsChunkDrawn(const FIntPoint& ChunkCoord) const
{
	const AVoxelChunk* Chunk = FindChunk(ChunkCoord);
	return Chunk && Chunk->IsDrawn();
}

TArray<FIntPoint> AVoxelWorld::GetLoadedChunks() const
{
	TArray<FIntPoint> Coords;
	Chunks.GetKeys(Coords);
	return Coords;
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
	if (!Chunk || !Chunk->SetBlock(BlockToLocal(Block), Id))
	{
		return false;
	}
	RememberChange(Block, Id);

	// Only the block itself and the six blocks touching it can appear or disappear.
	RedrawBlock(Block);
	for (const FIntVector& Offset : FaceNeighborOffsets)
	{
		RedrawBlock(Block + Offset);
	}
	return true;
}

void AVoxelWorld::RememberChange(const FIntVector& Block, FBlockId Id)
{
	// A loaded chunk means Generate has made the generator.
	const FIntPoint ChunkCoord = BlockToChunk(Block);
	const FIntVector Local = BlockToLocal(Block);
	if (Id != Generator->GetTerrainBlock(Block, *Catalog))
	{
		ChangedBlocks.FindOrAdd(ChunkCoord).Add(Local, Id);
		return;
	}
	if (TMap<FIntVector, FBlockId>* Changes = ChangedBlocks.Find(ChunkCoord))
	{
		Changes->Remove(Local);
		if (Changes->IsEmpty())
		{
			ChangedBlocks.Remove(ChunkCoord);
		}
	}
}

void AVoxelWorld::RedrawBlock(const FIntVector& Block)
{
	// Neighbors below the floor, above the ceiling or past the loaded chunks have nothing to draw.
	// A loaded chunk that is not drawn yet is skipped too: it draws all its cubes once its neighbors are loaded.
	AVoxelChunk* Chunk = FindChunk(BlockToChunk(Block));
	const FIntVector Local = BlockToLocal(Block);
	if (Chunk && Chunk->IsDrawn() && IsInsideChunk(Local))
	{
		Chunk->RedrawBlock(Local);
	}
}
