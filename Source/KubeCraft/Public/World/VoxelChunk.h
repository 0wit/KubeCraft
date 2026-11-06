#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/VoxelTypes.h"
#include "VoxelChunk.generated.h"

class AVoxelWorld;
class UChunkMeshComponent;

/**
 * One column of the world.
 * Stores which block sits in each cell, using one byte per cube (FBlockId).
 * Its mesh component turns those bytes into visible cubes.
 */
UCLASS(NotPlaceable)
class KUBECRAFT_API AVoxelChunk : public AActor
{
	GENERATED_BODY()

public:
	/** Creates an empty (all air) chunk with its mesh component as the root. */
	AVoxelChunk();

	/** Connects the chunk to its world (catalog, block mesh, neighbors) and tells it where it is. Call once, right after spawning. */
	void Initialize(AVoxelWorld& InVoxelWorld, const FIntPoint& InChunkCoord);

	/** The world this chunk belongs to, e.g. to find the world from a trace hit on the chunk. */
	AVoxelWorld* GetVoxelWorld() const { return VoxelWorld; }

	/** Returns the block at a local cell. If the cell is outside the chunk, it returns air. */
	FBlockId GetBlock(const FIntVector& Local) const;

	/**
	 * Same, but the cell may be just outside the chunk.
	 * Then asks the world, which reads the neighbor chunk.
	 * Above or below the world it returns air, because the chunk is as tall as the world.
	 * The mesh needs this to decide whether a cube face on the chunk border is hidden.
	 */
	FBlockId GetBlockOrNeighbor(const FIntVector& Local) const;

	/**
	 * Changes one cell. Returns true only if the block really changed.
	 * Returns false if the cell already held that id.
	 */
	bool SetBlock(const FIntVector& Local, FBlockId Id);

	/**
	 * Replaces the whole array at once (for example with freshly generated terrain)
	 * The array must have exactly BlocksPerChunk entries and only known ids, or nothing changes.
	 */
	void SetAllBlocks(TArray<FBlockId>&& NewBlocks);

	/** Redraws every cube from the array, then marks the chunk as drawn. */
	void RebuildMesh();

	/** true once RebuildMesh() has run. */
	bool IsDrawn() const { return bDrawn; }

	/** Redraws a single cell. Use it after that cell or one of its neighbors changed. */
	void RedrawBlock(const FIntVector& Local);

private:
	/** True for Air or an id the catalog has a definition for. */
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

	/** See IsDrawn(). */
	bool bDrawn = false;
};
