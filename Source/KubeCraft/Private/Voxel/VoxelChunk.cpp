#include "Voxel/VoxelChunk.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Voxel/BlockCatalog.h"
#include "Voxel/BlockDefinition.h"
#include "Voxel/VoxelWorld.h"

using namespace KubeCraft::Voxel;

namespace
{
	/** The six face directions of a cube. */
	const FIntVector NeighborOffsets[] = {
		FIntVector(1, 0, 0), FIntVector(-1, 0, 0),
		FIntVector(0, 1, 0), FIntVector(0, -1, 0),
		FIntVector(0, 0, 1), FIntVector(0, 0, -1),
	};
}

AVoxelChunk::AVoxelChunk()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

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

bool AVoxelChunk::IsKnownBlock(FBlockId Id) const
{
	const UBlockCatalog* Catalog = VoxelWorld ? VoxelWorld->GetCatalog() : nullptr;
	return Id == AirBlockId || (Catalog && Catalog->GetDefinition(Id));
}

void AVoxelChunk::RebuildMesh()
{
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
				const FBlockId Id = GetBlock(Local);
				if (Id != AirBlockId && IsExposed(Local))
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
		if (UInstancedStaticMeshComponent* Mesh = FindOrCreateMesh(Pair.Key))
		{
			Mesh->AddInstances(Pair.Value, /*bShouldReturnIndices*/ false);
		}
	}
}

bool AVoxelChunk::IsExposed(const FIntVector& Local) const
{
	for (const FIntVector& Offset : NeighborOffsets)
	{
		if (!IsSolidNeighbor(Local + Offset))
		{
			return true;
		}
	}
	return false;
}

bool AVoxelChunk::IsSolidNeighbor(const FIntVector& Local) const
{
	// Nothing can be seen from below the world, so the bottom faces are never drawn.
	if (Local.Z < 0)
	{
		return true;
	}
	// Above the height limit there is only Air, so the top faces of the highest layer are always drawn.
	if (Local.Z >= ChunkHeight)
	{
		return false;
	}
	if (IsInsideChunk(Local))
	{
		return GetBlock(Local) != AirBlockId;
	}
	// Across the chunk border, ask the world. Beyond the edge of the world it answers Air,
	// so the outer walls of the world stay drawn.
	return VoxelWorld && VoxelWorld->GetBlock(ChunkToBlockOrigin(ChunkCoord) + Local) != AirBlockId;
}

UInstancedStaticMeshComponent* AVoxelChunk::FindOrCreateMesh(FBlockId Id)
{
	if (TObjectPtr<UInstancedStaticMeshComponent>* Existing = MeshPerBlockId.Find(Id))
	{
		return *Existing;
	}

	// SetBlock only stores known ids and the world checks its block mesh before creating chunks, so this can
	// only fail after a code change. ensure() reports it once instead of on every rebuild.
	const UBlockCatalog* Catalog = VoxelWorld ? VoxelWorld->GetCatalog() : nullptr;
	const UBlockDefinition* Definition = Catalog ? Catalog->GetDefinition(Id) : nullptr;
	UStaticMesh* BlockMesh = VoxelWorld ? VoxelWorld->GetBlockMesh() : nullptr;
	if (!ensureMsgf(Definition && BlockMesh, TEXT("%s: cannot draw block id %d (not in the catalog, or no block mesh)"),
		*GetName(), Id))
	{
		return nullptr;
	}

	UInstancedStaticMeshComponent* Mesh = NewObject<UInstancedStaticMeshComponent>(this);
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetStaticMesh(BlockMesh);
	Mesh->SetMaterial(0, Definition->Material);
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Mesh->RegisterComponent();

	MeshPerBlockId.Add(Id, Mesh);
	return Mesh;
}
