#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "World/VoxelTypes.h"
#include "ChunkMeshComponent.generated.h"

class AVoxelChunk;
class AVoxelWorld;
class UBlockInstancesComponent;

/**
 *  Draws one chunk. It keeps one mesh per block type and draws only the blocks you can see.
 */
UCLASS()
class KUBECRAFT_API UChunkMeshComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	/** Redraws the whole chunk. It runs once, after the chunk is filled. */
	void Rebuild();

	/**
	 * Fixes one cell after mining or placing.
	 * Compares what should be drawn with what is drawn.
	 * Changes the cube only if they differ.
	 */
	void RedrawBlock(const FIntVector& Local);

private:
	/** The block type to draw in this cell, or air if nothing is drawn (the cell is air or buried). */
	FBlockId GetVisibleBlock(const AVoxelChunk& Chunk, const FIntVector& Local) const;

	/** Gives the type that has a cube in a cell right now, or air if no mesh has one there. */
	FBlockId FindDrawnBlock(const FIntVector& Local) const;

	/** Is true if at least one of the cell's 6 neighbors is not solid, so you can see a face. */
	bool IsExposed(const AVoxelChunk& Chunk, const FIntVector& Local) const;

	/** True if the cell at this local coordinate hides the face next to it. (may lie in a neighbor chunk) */
	bool IsSolidNeighbor(const AVoxelChunk& Chunk, const FIntVector& Local) const;

	/** The owning chunk, or null if the owner is not an AVoxelChunk connected to its world. */
	const AVoxelChunk* FindInitializedChunk() const;

	/** Gives the mesh for a block type and creates it on first use. Gives null if the type isn't in the catalog. */
	UBlockInstancesComponent* FindOrCreateMesh(const AVoxelWorld& World, FBlockId Id);

	/** One mesh per block type present in the chunk. Key is an FBlockId (UHT needs the raw uint8). */
	UPROPERTY(Transient)
	TMap<uint8, TObjectPtr<UBlockInstancesComponent>> MeshPerBlockId;
};
