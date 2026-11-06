#pragma once

#include "CoreMinimal.h"
#include "World/VoxelTypes.h"
#include "TerrainGenerator.generated.h"

class UBlockCatalog;

/**
 * The shape of the hills, edited on the voxel world (BP_VoxelWorld).
 * The noise is built from layers ("octaves"): the first layer makes the big hills,
 * each next layer adds smaller and flatter bumps on top of them.
 */
USTRUCT(BlueprintType)
struct FTerrainSettings
{
	GENERATED_BODY()

	/** Average ground height in blocks: the height where the noise is 0. */
	UPROPERTY(EditAnywhere, Category = "KubeCraft|Terrain", meta = (ClampMin = "1"))
	int32 BaseHeight = 20;

	/** How far hills can rise above and valleys sink below BaseHeight, in blocks. */
	UPROPERTY(EditAnywhere, Category = "KubeCraft|Terrain", meta = (ClampMin = "0"))
	float HeightVariation = 32.f;

	/** Width of the biggest hills in blocks (one wave of the first layer). Bigger = gentler land. */
	UPROPERTY(EditAnywhere, Category = "KubeCraft|Terrain", meta = (ClampMin = "1"))
	float HillWidth = 64.f;

	/** Number of noise layers. More layers = more small detail. */
	UPROPERTY(EditAnywhere, Category = "KubeCraft|Terrain", meta = (ClampMin = "1", ClampMax = "8"))
	int32 Octaves = 4;

	/** Height of each layer compared to the one before it (0.5 = half as tall). */
	UPROPERTY(EditAnywhere, Category = "KubeCraft|Terrain", meta = (ClampMin = "0", ClampMax = "1"))
	float Persistence = 0.5f;

	/** How much narrower each layer's bumps are than the one before it (2 = half as wide). */
	UPROPERTY(EditAnywhere, Category = "KubeCraft|Terrain", meta = (ClampMin = "1"))
	float Lacunarity = 2.f;
};

/**
 * Decides what the land looks like. Same seed always makes the same world.
 */
struct KUBECRAFT_API FTerrainGenerator
{
	/** Prepares the noise for one world. A different seed reads a different part of the noise. */
	FTerrainGenerator(int32 Seed, const FTerrainSettings& InSettings);

	/**
	 * Adds up the noise layers (big hills plus smaller bumps) to get the ground height of one column.
	 * Kept between 1 and ChunkHeight - 1.
	 */
	int32 GetSurfaceHeight(int32 BlockX, int32 BlockY) const;

	/**
	 * Fills a whole new chunk: each column is solid up to its height.
	 * The catalog picks the block type by height; the top cube of a column may get its own type (grass on dirt).
	 */
	TArray<FBlockId> GenerateChunk(const FIntPoint& ChunkCoord, const UBlockCatalog& Catalog) const;

	/**
	 * Gives the same answer as GenerateChunk, but for one block.
	 * Air above the ground, otherwise the type for that height (or the top type for the top cube).
	 */
	FBlockId GetTerrainBlock(const FIntVector& Block, const UBlockCatalog& Catalog) const;

private:
	FTerrainSettings Settings;

	/** Random shift of each noise layer, chosen from the seed. */
	TArray<FVector2D> OctaveOffsets;
};
