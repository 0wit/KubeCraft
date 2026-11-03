#include "World/ChunkMeshComponent.h"

#include "Blocks/BlockCatalog.h"
#include "Blocks/BlockDefinition.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "KubeCraft.h"
#include "World/VoxelChunk.h"
#include "World/VoxelWorld.h"

using namespace KubeCraft::Voxel;

DECLARE_CYCLE_STAT(TEXT("Chunk Rebuild"), STAT_ChunkRebuild, STATGROUP_KubeCraft);

namespace
{
	/** The six face directions of a cube. */
	const FIntVector NeighborOffsets[] = {
		FIntVector(1, 0, 0), FIntVector(-1, 0, 0),
		FIntVector(0, 1, 0), FIntVector(0, -1, 0),
		FIntVector(0, 0, 1), FIntVector(0, 0, -1),
	};
}

void UChunkMeshComponent::Rebuild()
{
	// Mining and placing call this for every changed block, so its cost is what decides whether that hitches.
	SCOPE_CYCLE_COUNTER(STAT_ChunkRebuild);

	const AVoxelChunk* Chunk = GetOwner<AVoxelChunk>();
	const AVoxelWorld* World = Chunk ? Chunk->GetVoxelWorld() : nullptr;
	if (!ensureMsgf(World, TEXT("%s: must belong to an initialized AVoxelChunk."), *GetName()))
	{
		return;
	}

	// 1. Collect the position of every visible block, grouped by block type,
	//    so each mesh component is updated in a single call.
	TMap<FBlockId, TArray<FTransform>> InstancesPerBlockId;

	for (int32 Z = 0; Z < ChunkHeight; ++Z)
	{
		for (int32 Y = 0; Y < ChunkSize; ++Y)
		{
			for (int32 X = 0; X < ChunkSize; ++X)
			{
				const FIntVector Local(X, Y, Z);
				const FBlockId Id = Chunk->GetBlock(Local);
				if (Id != AirBlockId && IsExposed(*Chunk, Local))
				{
					InstancesPerBlockId.FindOrAdd(Id).Emplace(LocalToRelativeCenter(Local));
				}
			}
		}
	}

	// 2. Remove the old cubes (this also empties types that no longer appear in the chunk).
	for (const TPair<uint8, TObjectPtr<UInstancedStaticMeshComponent>>& Pair : MeshPerBlockId)
	{
		Pair.Value->ClearInstances();
	}

	// 3. Hand each type's list to its mesh component.
	for (const TPair<FBlockId, TArray<FTransform>>& Pair : InstancesPerBlockId)
	{
		if (UInstancedStaticMeshComponent* Mesh = FindOrCreateMesh(*World, Pair.Key))
		{
			Mesh->AddInstances(Pair.Value, /*bShouldReturnIndices*/ false);
		}
	}
}

bool UChunkMeshComponent::IsExposed(const AVoxelChunk& Chunk, const FIntVector& Local) const
{
	for (const FIntVector& Offset : NeighborOffsets)
	{
		if (!IsSolidNeighbor(Chunk, Local + Offset))
		{
			return true;
		}
	}
	return false;
}

bool UChunkMeshComponent::IsSolidNeighbor(const AVoxelChunk& Chunk, const FIntVector& Local) const
{
	// Nothing can be seen from below the world, so the bottom faces are never drawn.
	if (Local.Z < 0)
	{
		return true;
	}
	// Across a chunk border the chunk asks the world. Beyond the edge of the world that is Air,
	// so the outer walls of the world stay drawn.
	return Chunk.GetBlockOrNeighbor(Local) != AirBlockId;
}

UInstancedStaticMeshComponent* UChunkMeshComponent::FindOrCreateMesh(const AVoxelWorld& World, FBlockId Id)
{
	if (TObjectPtr<UInstancedStaticMeshComponent>* Existing = MeshPerBlockId.Find(Id))
	{
		return *Existing;
	}

	// The chunk only stores known ids and the world checks its block mesh before creating chunks, so this can
	// only fail after a code change. ensure() reports it once instead of on every rebuild.
	const UBlockCatalog* Catalog = World.GetCatalog();
	const UBlockDefinition* Definition = Catalog ? Catalog->GetDefinition(Id) : nullptr;
	UStaticMesh* BlockMesh = World.GetBlockMesh();
	if (!ensureMsgf(Definition && BlockMesh, TEXT("%s: cannot draw block id %d (not in the catalog, or no block mesh)"),
		*GetName(), Id))
	{
		return nullptr;
	}

	UInstancedStaticMeshComponent* Mesh = NewObject<UInstancedStaticMeshComponent>(GetOwner());
	Mesh->SetupAttachment(this);
	Mesh->SetStaticMesh(BlockMesh);
	Mesh->SetMaterial(0, Definition->Material);
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Mesh->RegisterComponent();

	MeshPerBlockId.Add(Id, Mesh);
	return Mesh;
}
