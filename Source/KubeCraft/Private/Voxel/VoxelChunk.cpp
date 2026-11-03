#include "Voxel/VoxelChunk.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "KubeCraft.h"
#include "Voxel/BlockCatalog.h"
#include "Voxel/BlockDefinition.h"

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

void AVoxelChunk::BeginPlay()
{
	Super::BeginPlay();

	if (!Catalog || !BlockMesh)
	{
		UE_LOG(LogKubeCraft, Error, TEXT("%s: Catalog and BlockMesh must both be assigned; the chunk stays empty."),
			*GetName());
		return;
	}

	FillFlatForTesting();
	RebuildMesh();
}

bool AVoxelChunk::IsKnownBlock(FBlockId Id) const
{
	return Id == AirBlockId || (Catalog && Catalog->GetDefinition(Id));
}

void AVoxelChunk::FillFlatForTesting()
{
	const int32 Height = FMath::Clamp(FlatTestHeight, 0, ChunkHeight);
	for (int32 Z = 0; Z < Height; ++Z)
	{
		const FBlockId Id = Catalog->GetIdForHeight(Z);
		for (int32 Y = 0; Y < ChunkSize; ++Y)
		{
			for (int32 X = 0; X < ChunkSize; ++X)
			{
				SetBlock(FIntVector(X, Y, Z), Id);
			}
		}
	}
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
	// Outside this chunk we don't know the neighbor yet (no world), so treat it as open air.
	return GetBlock(Local) != AirBlockId;
}

UInstancedStaticMeshComponent* AVoxelChunk::FindOrCreateMesh(FBlockId Id)
{
	if (TObjectPtr<UInstancedStaticMeshComponent>* Existing = MeshPerBlockId.Find(Id))
	{
		return *Existing;
	}

	// SetBlock only stores known ids and BeginPlay checks BlockMesh, so this can only fail after a code
	// change. ensure() reports it once instead of on every rebuild.
	const UBlockDefinition* Definition = Catalog ? Catalog->GetDefinition(Id) : nullptr;
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
