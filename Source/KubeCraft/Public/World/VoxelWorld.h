#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Terrain/TerrainGenerator.h"
#include "World/VoxelTypes.h"
#include "VoxelWorld.generated.h"

class AVoxelChunk;
class UBlockCatalog;
class UBlockDefinition;
class UStaticMesh;

/**
 * The whole block world: owns all chunks and answers questions about blocks in world block coordinates.
 * Generate() picks a seed, creates a square of chunks centered on block (0, 0) and fills them with terrain.
 * After that, MineBlock() / PlaceBlock() are the only way blocks change; they ask FBlockRules first.
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

	/** Block type at a world block coordinate, or null for Air. */
	const UBlockDefinition* GetBlockDefinition(const FIntVector& Block) const;

	/** World position (cm) on top of the highest solid block in a column, i.e. where a player can stand. */
	FVector GetSurfaceLocation(const FIntPoint& Column) const;

	/** True if the block lies between the floor and the ceiling of a generated chunk, i.e. it can hold a block. */
	bool IsInsideWorld(const FIntVector& Block) const;

	/** Removes a block if FBlockRules allows it and redraws what changed. Returns true if the block was removed. */
	bool MineBlock(const FIntVector& Block);

	/**
	 * Puts a block of the given type here if FBlockRules allows it and redraws what changed.
	 * Returns true if a block was placed; false also for a type that is null or not in the catalog.
	 */
	bool PlaceBlock(const FIntVector& Block, const UBlockDefinition* Type);

	/** All block types and the height rule; shared by every chunk. */
	const UBlockCatalog* GetCatalog() const { return Catalog; }

	/** Mesh drawn for every cube; shared by every chunk. */
	UStaticMesh* GetBlockMesh() const { return BlockMesh; }

private:
#if WITH_DEV_AUTOMATION_TESTS
	/** Builds small worlds with a catalog made in code for the automation tests (Private/Tests/TestVoxelWorld.h). */
	friend struct FTestVoxelWorld;
#endif

	/**
	 * Creates one chunk and fills it with blocks from the generator. Does not draw it yet.
	 * BlockIdPerHeight holds the block type for each height Z (ChunkHeight entries), taken from the catalog.
	 */
	void SpawnChunk(const FIntPoint& ChunkCoord, const FTerrainGenerator& Generator,
		const TArray<FBlockId>& BlockIdPerHeight);

	/** Chunk at a chunk coordinate, or null outside the generated area. */
	AVoxelChunk* FindChunk(const FIntPoint& ChunkCoord) const;

	/**
	 * Changes one block and redraws it and its six neighbors - nothing else can change. On a chunk border
	 * some of those neighbors belong to the next chunk; they are redrawn there.
	 * Returns true if the block changed; if not (no chunk there, or the chunk refused the id), nothing is redrawn.
	 */
	bool SetBlockAndRedraw(const FIntVector& Block, FBlockId Id);

	/** Redraws the cube of one block in whichever chunk holds it. Does nothing outside the world. */
	void RedrawBlock(const FIntVector& Block);

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
