#include "World/VoxelChunk.h"

#include "Blocks/BlockCatalog.h"
#include "World/ChunkMeshComponent.h"
#include "World/VoxelWorld.h"

using namespace KubeCraft::Voxel;

AVoxelChunk::AVoxelChunk()
{
	PrimaryActorTick.bCanEverTick = false;
	Mesh = CreateDefaultSubobject<UChunkMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

	Blocks.Init(AirBlockId, BlocksPerChunk);
}

void AVoxelChunk::Initialize(AVoxelWorld& InVoxelWorld, const FIntPoint& InChunkCoord)
{
	ensureMsgf(!VoxelWorld, TEXT("%s: Initialize() was called twice."), *GetName());
	VoxelWorld = &InVoxelWorld;
	ChunkCoord = InChunkCoord;
}

FBlockId AVoxelChunk::GetBlock(const FIntVector& Local) const
{
	return IsInsideChunk(Local) ? Blocks[LocalToIndex(Local)] : AirBlockId;
}

FBlockId AVoxelChunk::GetBlockOrNeighbor(const FIntVector& Local) const
{
	if (IsInsideChunk(Local))
	{
		return Blocks[LocalToIndex(Local)];
	}
	// The chunk is as tall as the world, so above or below it there is nothing to ask the world about.
	if (Local.Z < 0 || Local.Z >= ChunkHeight || !VoxelWorld)
	{
		return AirBlockId;
	}
	return VoxelWorld->GetBlock(ChunkToBlockOrigin(ChunkCoord) + Local);
}

void AVoxelChunk::SetBlock(const FIntVector& Local, FBlockId Id)
{
	if (!ensureMsgf(IsInsideChunk(Local), TEXT("SetBlock outside chunk: %s"), *Local.ToString()))
	{
		return;
	}
	// Every stored id must be drawable: an unknown id would be solid for its neighbors but invisible.
	if (!ensureMsgf(IsKnownBlock(Id), TEXT("SetBlock with block id %d, which is not in the catalog"), Id))
	{
		return;
	}
	Blocks[LocalToIndex(Local)] = Id;
}

void AVoxelChunk::RebuildMesh()
{
	Mesh->Rebuild();
}

bool AVoxelChunk::IsKnownBlock(FBlockId Id) const
{
	const UBlockCatalog* Catalog = VoxelWorld ? VoxelWorld->GetCatalog() : nullptr;
	return Id == AirBlockId || (Catalog && Catalog->GetDefinition(Id));
}
