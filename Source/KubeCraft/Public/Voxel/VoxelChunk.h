#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Voxel/VoxelTypes.h"
#include "VoxelChunk.generated.h"

class UBlockCatalog;
class UInstancedStaticMeshComponent;
class UStaticMesh;

/**
 * One ChunkSize x ChunkSize x ChunkHeight column of the world. It has two jobs:
 *
 *  1. Remember its blocks: a flat array of FBlockId (one byte per cube), see LocalToIndex.
 *  2. Draw them (RebuildMesh): one UInstancedStaticMeshComponent per block type - each type has its
 *     own material - drawing many cubes in one draw call. Blocks buried on all six sides are skipped,
 *     so underground terrain costs nothing to render.
 *
 * The actor sits at the chunk's world origin, so local block coordinates map directly to
 * relative positions of the instances.
 */
UCLASS()
class KUBECRAFT_API AVoxelChunk : public AActor
{
	GENERATED_BODY()

public:
	/** Creates an empty (all Air) chunk with a root component for the instanced meshes. */
	AVoxelChunk();

	/** Block at a local coordinate. Returns Air for coordinates outside the chunk. */
	FBlockId GetBlock(const FIntVector& Local) const;

	/**
	 * Changes one block. Ignored (with an ensure) for coordinates outside the chunk and ids not in the catalog.
	 * Does not update the visuals; call RebuildMesh() after a batch of changes.
	 */
	void SetBlock(const FIntVector& Local, FBlockId Id);

	/** Recreates all cube instances from the block array. */
	void RebuildMesh();

protected:
	virtual void BeginPlay() override;

private:
	/** Temporary content until the terrain generator exists: a flat slab, block types taken from the height bands. */
	void FillFlatForTesting();

	/** True for Air and for ids that have a definition in the catalog - the only ids a chunk may store. */
	bool IsKnownBlock(FBlockId Id) const;

	/** True if a face of this block can be seen, i.e. at least one neighbor is not solid. */
	bool IsExposed(const FIntVector& Local) const;

	/** True if the neighbor at this local coordinate hides the face next to it. */
	bool IsSolidNeighbor(const FIntVector& Local) const;

	/** Returns the instanced mesh for a block type, creating it on first use. Null if the id is not in the catalog. */
	UInstancedStaticMeshComponent* FindOrCreateMesh(FBlockId Id);

	/** All block types and the height rule. */
	UPROPERTY(EditAnywhere, Category = "KubeCraft|Chunk")
	TObjectPtr<UBlockCatalog> Catalog;

	/** Mesh drawn for every cube; must be BlockSize wide with its pivot in the center. */
	UPROPERTY(EditAnywhere, Category = "KubeCraft|Chunk")
	TObjectPtr<UStaticMesh> BlockMesh;

	/** Height in blocks of the temporary test slab (see FillFlatForTesting); capped at ChunkHeight. */
	UPROPERTY(EditAnywhere, Category = "KubeCraft|Chunk", meta = (ClampMin = "0"))
	int32 FlatTestHeight = 6;

	/** One mesh component per block type present in this chunk. Key is an FBlockId (UHT needs the raw uint8). */
	UPROPERTY(Transient)
	TMap<uint8, TObjectPtr<UInstancedStaticMeshComponent>> MeshPerBlockId;

	/** All blocks of the chunk, indexed by KubeCraft::Voxel::LocalToIndex. */
	TArray<FBlockId> Blocks;
};
