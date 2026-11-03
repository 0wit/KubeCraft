#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Voxel/TerrainGenerator.h"
#include "Voxel/VoxelTypes.h"
#include "VoxelWorld.generated.h"

class AVoxelChunk;
class UBlockCatalog;
class UStaticMesh;

/**
 * The whole block world: owns all chunks and answers questions about blocks in world block coordinates.
 * Generate() picks a seed, creates a square of chunks centered on block (0, 0) and fills them with terrain.
 *
 * Chunks are placed in absolute world space (block (0, 0, 0) is always at the world origin),
 * so moving this actor in the level does not move the terrain.
 */
UCLASS()
class KUBECRAFT_API AVoxelWorld : public AActor
{
	GENERATED_BODY()

public:
	/** Creates the world actor; it has no visuals of its own. */
	AVoxelWorld();

	/** Picks the seed, creates and fills all chunks, then draws them. Call once, before players are placed. */
	void Generate();

	/** Block at a world block coordinate. Air outside the generated area and above or below the world. */
	FBlockId GetBlock(const FIntVector& Block) const;

	/** World position (cm) on top of the highest solid block in a column, i.e. where a player can stand. */
	FVector GetSurfaceLocation(const FIntPoint& Column) const;

	/** All block types and the height rule; shared by every chunk. */
	const UBlockCatalog* GetCatalog() const { return Catalog; }

	/** Mesh drawn for every cube; shared by every chunk. */
	UStaticMesh* GetBlockMesh() const { return BlockMesh; }

private:
	/**
	 * Creates one chunk and fills it with blocks from the generator. Does not draw it yet.
	 * BlockIdPerHeight holds the block type for each height Z (ChunkHeight entries), taken from the catalog.
	 */
	void SpawnChunk(const FIntPoint& ChunkCoord, const FTerrainGenerator& Generator,
		const TArray<FBlockId>& BlockIdPerHeight);

	/** All block types and the height rule. */
	UPROPERTY(EditAnywhere, Category = "KubeCraft|World")
	TObjectPtr<UBlockCatalog> Catalog;

	/** Mesh drawn for every cube; must be BlockSize wide with its pivot in the center. */
	UPROPERTY(EditAnywhere, Category = "KubeCraft|World")
	TObjectPtr<UStaticMesh> BlockMesh;

	/** Number of chunks along X and along Y. */
	UPROPERTY(EditAnywhere, Category = "KubeCraft|World", meta = (ClampMin = "1"))
	int32 WorldSizeInChunks = 8;

	/** Pick a new seed on every start. Turn off to always get the world of Seed. */
	UPROPERTY(EditAnywhere, Category = "KubeCraft|World")
	bool bRandomSeed = true;

	/** Seed of the terrain. Overwritten on start when bRandomSeed is on; the seed in use is written to the log. */
	UPROPERTY(EditAnywhere, Category = "KubeCraft|World", meta = (EditCondition = "!bRandomSeed"))
	int32 Seed = 0;

	/** Shape of the hills. */
	UPROPERTY(EditAnywhere, Category = "KubeCraft|World")
	FTerrainSettings Terrain;

	/** Every chunk of the world, by chunk coordinate. */
	UPROPERTY(Transient)
	TMap<FIntPoint, TObjectPtr<AVoxelChunk>> Chunks;
};
