#pragma once

#include "CoreMinimal.h"
#include "World/VoxelTypes.h"
#include "TreeGenerator.generated.h"

class UBlockCatalog;
class UBlockDefinition;
struct FTerrainGenerator;

/**
 * Which blocks a tree is made of and how often trees grow, edited on the voxel world (BP_VoxelWorld).
 * Trees are off until Trunk, Leaves and GrowsOn are all set.
 */
USTRUCT(BlueprintType)
struct FTreeSettings
{
	GENERATED_BODY()

	/** Block of the trunk (wood). Must also be listed in the block catalog. */
	UPROPERTY(EditAnywhere, Category = "KubeCraft|Trees")
	TObjectPtr<UBlockDefinition> Trunk = nullptr;

	/** Block of the leaves. Must also be listed in the block catalog. */
	UPROPERTY(EditAnywhere, Category = "KubeCraft|Trees")
	TObjectPtr<UBlockDefinition> Leaves = nullptr;

	/** A tree only grows on a column whose top cube is this block (grass). */
	UPROPERTY(EditAnywhere, Category = "KubeCraft|Trees")
	TObjectPtr<UBlockDefinition> GrowsOn = nullptr;

	/**
	 * The world is split into square cells of this many columns; each cell has at most one tree.
	 * Bigger = trees further apart. At least 5, so a whole tree (5 wide) fits inside one cell.
	 */
	UPROPERTY(EditAnywhere, Category = "KubeCraft|Trees", meta = (ClampMin = "5"))
	int32 CellSize = 7;

	/** Chance that a cell has a tree (0 = never, 1 = every cell, if it has grass). */
	UPROPERTY(EditAnywhere, Category = "KubeCraft|Trees", meta = (ClampMin = "0", ClampMax = "1"))
	float Chance = 0.4f;

	/** Shortest trunk in blocks. At least 3, so some trunk shows under the leaves. */
	UPROPERTY(EditAnywhere, Category = "KubeCraft|Trees", meta = (ClampMin = "3"))
	int32 MinTrunkHeight = 4;

	/** Tallest trunk in blocks. Each tree picks a height between Min and Max. */
	UPROPERTY(EditAnywhere, Category = "KubeCraft|Trees", meta = (ClampMin = "3"))
	int32 MaxTrunkHeight = 6;
};

/**
 * Decides where trees stand and which cubes they are made of. Same seed always gives the same trees.
 * A tree never leaves its cell, so one block only ever needs to look at the tree of its own cell.
 */
struct KUBECRAFT_API FTreeGenerator
{
	/** Prepares the trees of one world. Another seed puts the trees somewhere else. */
	FTreeGenerator(int32 Seed, const FTreeSettings& InSettings);

	/** Adds the trees to a chunk that already has its ground. Trees only fill air, they never cut into a hill. */
	void AddTrees(const FIntPoint& ChunkCoord, TArray<FBlockId>& Blocks, const FTerrainGenerator& Terrain,
		const UBlockCatalog& Catalog) const;

	/** The tree cube at a block (trunk or leaves), or air if no tree is there. Same answer as AddTrees. */
	FBlockId GetTreeBlock(const FIntVector& Block, const FTerrainGenerator& Terrain, const UBlockCatalog& Catalog) const;

private:
	/** One tree: where its trunk starts (the cube on top of the grass) and how tall the trunk is. */
	struct FTree
	{
		FIntVector Base;
		int32 TrunkHeight = 0;
	};

	/** What a tree has at one spot. */
	enum class ETreePart : uint8
	{
		None,
		Trunk,
		Leaves,
	};

	/** The tree of one cell, or nothing if the dice said no or its column has no grass on top or no room above. */
	TOptional<FTree> FindTree(const FIntPoint& Cell, const FTerrainGenerator& Terrain, const UBlockCatalog& Catalog) const;

	/** The shape of every tree: what is at Offset from the trunk's base. The only place the shape is described. */
	static ETreePart GetPart(const FIntVector& Offset, int32 TrunkHeight);

	/** The catalog id for a tree part (air for None). */
	FBlockId GetPartId(ETreePart Part, const UBlockCatalog& Catalog) const;

	/** True if Trunk, Leaves and GrowsOn are set. */
	bool IsEnabled() const;

	int32 Seed = 0;
	FTreeSettings Settings;
};
