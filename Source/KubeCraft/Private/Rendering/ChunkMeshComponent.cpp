#include "Rendering/ChunkMeshComponent.h"

#include "Blocks/BlockCatalog.h"
#include "Blocks/BlockDefinition.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "KubeCraft.h"
#include "Rendering/BlockInstancesComponent.h"
#include "World/VoxelChunk.h"
#include "World/VoxelWorld.h"

using namespace KubeCraft::Voxel;

DECLARE_CYCLE_STAT(TEXT("Chunk Rebuild"), STAT_ChunkRebuild, STATGROUP_KubeCraft);
DECLARE_CYCLE_STAT(TEXT("Block Redraw"), STAT_BlockRedraw, STATGROUP_KubeCraft);

void UChunkMeshComponent::Rebuild()
{
	SCOPE_CYCLE_COUNTER(STAT_ChunkRebuild);

	const AVoxelChunk* Chunk = FindInitializedChunk();
	if (!Chunk)
	{
		return;
	}

	// 1. Collect every visible cell, grouped by block type, so each type is updated in a single call.
	TMap<FBlockId, TArray<FIntVector>> CellsPerBlockId;
	for (int32 Z = 0; Z < ChunkHeight; ++Z)
	{
		for (int32 Y = 0; Y < ChunkSize; ++Y)
		{
			for (int32 X = 0; X < ChunkSize; ++X)
			{
				const FIntVector Local(X, Y, Z);
				const FBlockId Id = GetVisibleBlock(*Chunk, Local);
				if (Id != AirBlockId)
				{
					CellsPerBlockId.FindOrAdd(Id).Add(Local);
				}
			}
		}
	}

	// 2. Every type that shows needs its mesh.
	for (const TPair<FBlockId, TArray<FIntVector>>& Pair : CellsPerBlockId)
	{
		FindOrCreateMesh(*Chunk->GetVoxelWorld(), Pair.Key);
	}

	// 3. Hand each mesh its cells. A type that no longer appears in the chunk gets an empty list.
	for (const TPair<uint8, TObjectPtr<UBlockInstancesComponent>>& Pair : MeshPerBlockId)
	{
		const TArray<FIntVector>* Cells = CellsPerBlockId.Find(Pair.Key);
		Pair.Value->SetBlocks(Cells ? *Cells : TArray<FIntVector>());
	}
}

void UChunkMeshComponent::RedrawBlock(const FIntVector& Local)
{
	// Mining and placing call this seven times per changed block, so its cost decides whether that hitches.
	SCOPE_CYCLE_COUNTER(STAT_BlockRedraw);

	const AVoxelChunk* Chunk = FindInitializedChunk();
	if (!Chunk)
	{
		return;
	}

	// Most of the seven redraws after a change end here: the neighbor looks the same as before.
	const FBlockId Visible = GetVisibleBlock(*Chunk, Local);
	const FBlockId Drawn = FindDrawnBlock(Local);
	if (Visible == Drawn)
	{
		return;
	}

	// A cell shows at most one type: take the old cube out, then add the new one.
	if (UBlockInstancesComponent* OldMesh = MeshPerBlockId.FindRef(Drawn))
	{
		OldMesh->RemoveBlock(Local);
	}
	if (Visible != AirBlockId)
	{
		if (UBlockInstancesComponent* NewMesh = FindOrCreateMesh(*Chunk->GetVoxelWorld(), Visible))
		{
			NewMesh->AddBlock(Local);
		}
	}
}

FBlockId UChunkMeshComponent::FindDrawnBlock(const FIntVector& Local) const
{
	// Asking the meshes keeps one record of what is drawn: theirs. A chunk has one mesh per type, so only a few.
	for (const TPair<uint8, TObjectPtr<UBlockInstancesComponent>>& Pair : MeshPerBlockId)
	{
		if (Pair.Value->HasBlock(Local))
		{
			return Pair.Key;
		}
	}
	return AirBlockId;
}

FBlockId UChunkMeshComponent::GetVisibleBlock(const AVoxelChunk& Chunk, const FIntVector& Local) const
{
	const FBlockId Id = Chunk.GetBlock(Local);
	return (Id != AirBlockId && IsExposed(Chunk, Local)) ? Id : AirBlockId;
}

bool UChunkMeshComponent::IsExposed(const AVoxelChunk& Chunk, const FIntVector& Local) const
{
	for (const FIntVector& Offset : FaceNeighborOffsets)
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
	// Across a chunk border the chunk asks the world. A chunk that is not loaded counts as Air, which is why
	// the streaming loads all four neighbors before it draws a chunk.
	return Chunk.GetBlockOrNeighbor(Local) != AirBlockId;
}

const AVoxelChunk* UChunkMeshComponent::FindInitializedChunk() const
{
	const AVoxelChunk* Chunk = GetOwner<AVoxelChunk>();
	if (!ensureMsgf(Chunk && Chunk->GetVoxelWorld(), TEXT("%s: must belong to an initialized AVoxelChunk."), *GetName()))
	{
		return nullptr;
	}
	return Chunk;
}

UBlockInstancesComponent* UChunkMeshComponent::FindOrCreateMesh(const AVoxelWorld& World, FBlockId Id)
{
	if (TObjectPtr<UBlockInstancesComponent>* Existing = MeshPerBlockId.Find(Id))
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

	UBlockInstancesComponent* Mesh = NewObject<UBlockInstancesComponent>(GetOwner());
	Mesh->SetupAttachment(this);
	Mesh->SetStaticMesh(BlockMesh);
	Mesh->SetMaterial(0, Definition->Material);
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Mesh->RegisterComponent();

	MeshPerBlockId.Add(Id, Mesh);
	return Mesh;
}
