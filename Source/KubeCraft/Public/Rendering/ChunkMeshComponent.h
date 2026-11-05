#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "World/VoxelTypes.h"
#include "ChunkMeshComponent.generated.h"

class AVoxelChunk;
class AVoxelWorld;
class UBlockInstancesComponent;

/**
 * Decides which blocks of the chunk that owns it are visible, and hands them to one UBlockInstancesComponent
 * per block type (each type has its own material). Blocks buried on all six sides are skipped, so
 * underground terrain costs nothing to render.
 *
 * It is the chunk's root, so it sits at the chunk's first block and local block coordinates map directly
 * to the relative positions of the cubes.
 */
UCLASS()
class KUBECRAFT_API UChunkMeshComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	/** Recreates all cubes from the owning chunk's blocks, e.g. once after the world is filled. */
	void Rebuild();

	/**
	 * Updates the cube of one cell after it or one of its neighbors changed: adds, removes or swaps it.
	 * Only this cell's cube changes; its six neighbors are only read (on a chunk border, through the world),
	 * so the neighbor chunks must already be filled. Much cheaper than Rebuild(), which looks at every cell.
	 * Does nothing if the cell already shows the right type, which is the usual case for a neighbor.
	 */
	void RedrawBlock(const FIntVector& Local);

private:
	/** The block type to draw in this cell, or Air if nothing is drawn (the cell is Air or buried). */
	FBlockId GetVisibleBlock(const AVoxelChunk& Chunk, const FIntVector& Local) const;

	/** The block type whose mesh has a cube in this cell right now, or Air if none has. */
	FBlockId FindDrawnBlock(const FIntVector& Local) const;

	/** True if a face of this block can be seen, i.e. at least one neighbor is not solid. */
	bool IsExposed(const AVoxelChunk& Chunk, const FIntVector& Local) const;

	/** True if the cell at this local coordinate hides the face next to it. May lie in a neighbor chunk. */
	bool IsSolidNeighbor(const AVoxelChunk& Chunk, const FIntVector& Local) const;

	/** The owning chunk, or null (with an ensure) if the owner is not an AVoxelChunk connected to its world. */
	const AVoxelChunk* FindInitializedChunk() const;

	/** Returns the cubes of a block type, creating them on first use. Null if the id is not in the catalog. */
	UBlockInstancesComponent* FindOrCreateMesh(const AVoxelWorld& World, FBlockId Id);

	/** One mesh per block type present in the chunk. Key is an FBlockId (UHT needs the raw uint8). */
	UPROPERTY(Transient)
	TMap<uint8, TObjectPtr<UBlockInstancesComponent>> MeshPerBlockId;
};
