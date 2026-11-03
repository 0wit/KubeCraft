#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/VoxelTypes.h"
#include "VoxelChunk.generated.h"

class AVoxelWorld;
class UChunkMeshComponent;

/**
 * One ChunkSize x ChunkSize x ChunkHeight column of the world, created and filled by AVoxelWorld.
 * It remembers its blocks in a flat array of FBlockId (one byte per cube, see LocalToIndex);
 * its UChunkMeshComponent turns them into visible cubes.
 */
UCLASS(NotPlaceable)
class KUBECRAFT_API AVoxelChunk : public AActor
{
	GENERATED_BODY()

public:
	/** Creates an empty (all Air) chunk with its mesh component as the root. */
	AVoxelChunk();

	/** Connects the chunk to its world (catalog, block mesh, neighbors) and tells it where it is. Call once, right after spawning. */
	void Initialize(AVoxelWorld& InVoxelWorld, const FIntPoint& InChunkCoord);

	/** The world this chunk belongs to, e.g. to find the world from a trace hit on the chunk. */
	AVoxelWorld* GetVoxelWorld() const { return VoxelWorld; }

	/** Block at a local coordinate. Returns Air for coordinates outside the chunk. */
	FBlockId GetBlock(const FIntVector& Local) const;

	/**
	 * Block at a local coordinate that may lie just outside this chunk (e.g. X = -1): such blocks are read
	 * from the neighbor chunk through the world. Air above and below the world and beyond its edge.
	 */
	FBlockId GetBlockOrNeighbor(const FIntVector& Local) const;

	/**
	 * Changes one block. Ignored (with an ensure) for coordinates outside the chunk and ids not in the catalog.
	 * Does not update the visuals; call RebuildMesh() after a batch of changes.
	 */
	void SetBlock(const FIntVector& Local, FBlockId Id);

	/** Redraws all cubes from the block array. */
	void RebuildMesh();

private:
	/** True for Air and for ids that have a definition in the catalog - the only ids a chunk may store. */
	bool IsKnownBlock(FBlockId Id) const;

	/** Draws the blocks; also the root, so it sits at the chunk's first block. */
	UPROPERTY(VisibleAnywhere, Category = "KubeCraft|Chunk")
	TObjectPtr<UChunkMeshComponent> Mesh;

	/** The world this chunk belongs to; provides the catalog, the block mesh and the neighbor chunks. */
	UPROPERTY(Transient)
	TObjectPtr<AVoxelWorld> VoxelWorld;

	/** Position of this chunk in chunk coordinates (see BlockToChunk). */
	FIntPoint ChunkCoord = FIntPoint::ZeroValue;

	/** All blocks of the chunk, indexed by KubeCraft::Voxel::LocalToIndex. */
	TArray<FBlockId> Blocks;
};
