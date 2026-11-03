#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "World/VoxelTypes.h"
#include "ChunkMeshComponent.generated.h"

class AVoxelChunk;
class AVoxelWorld;
class UInstancedStaticMeshComponent;

/**
 * Draws the blocks of the chunk that owns it: one UInstancedStaticMeshComponent per block type (each type has
 * its own material), drawing many cubes in one draw call. Blocks buried on all six sides are skipped, so
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
	/** Recreates all cubes from the owning chunk's blocks. */
	void Rebuild();

private:
	/** True if a face of this block can be seen, i.e. at least one neighbor is not solid. */
	bool IsExposed(const AVoxelChunk& Chunk, const FIntVector& Local) const;

	/** True if the cell at this local coordinate hides the face next to it. May lie in a neighbor chunk. */
	bool IsSolidNeighbor(const AVoxelChunk& Chunk, const FIntVector& Local) const;

	/** Returns the instanced mesh for a block type, creating it on first use. Null if the id is not in the catalog. */
	UInstancedStaticMeshComponent* FindOrCreateMesh(const AVoxelWorld& World, FBlockId Id);

	/** One mesh per block type present in the chunk. Key is an FBlockId (UHT needs the raw uint8). */
	UPROPERTY(Transient)
	TMap<uint8, TObjectPtr<UInstancedStaticMeshComponent>> MeshPerBlockId;
};
