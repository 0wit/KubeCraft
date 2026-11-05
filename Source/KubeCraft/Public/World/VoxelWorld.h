#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Terrain/TerrainGenerator.h"
#include "World/VoxelTypes.h"
#include "VoxelWorld.generated.h"

class AVoxelChunk;
class UBlockCatalog;
class UBlockDefinition;
class UChunkStreamingComponent;
class UStaticMesh;

/** One block the player changed: where it is and what it is now. */
USTRUCT(BlueprintType)
struct FBlockChange
{
	GENERATED_BODY()

	/** World block coordinate. */
	UPROPERTY(BlueprintReadOnly, Category = "KubeCraft|World")
	FIntVector Block = FIntVector::ZeroValue;

	/** The block type, or null for Air (a mined block). The type itself, not its id, so a reordered catalog still loads. */
	UPROPERTY(BlueprintReadOnly, Category = "KubeCraft|World")
	TObjectPtr<const UBlockDefinition> Type = nullptr;
};

/**
 * Everything needed to build a voxel world again: the seed makes the same terrain, and the changes go on top.
 * Made by AVoxelWorld::MakeSave, given back to AVoxelWorld::GenerateFromSave; a SaveGame Blueprint just stores it.
 */
USTRUCT(BlueprintType)
struct FVoxelWorldSave
{
	GENERATED_BODY()

	/** Seed of the terrain. */
	UPROPERTY(BlueprintReadOnly, Category = "KubeCraft|World")
	int32 Seed = 0;

	/** Every block the player changed, in loaded and unloaded chunks. */
	UPROPERTY(BlueprintReadOnly, Category = "KubeCraft|World")
	TArray<FBlockChange> Changes;
};

/**
 * The whole block world: owns the loaded chunks and answers questions about blocks in world block coordinates.
 * Its UChunkStreamingComponent decides which chunks are loaded and drawn; this actor only does what it is asked.
 * MineBlock() / PlaceBlock() are the only way blocks change; they ask FBlockRules first, and the changes are
 * remembered, so a chunk that is unloaded and loaded again still has them. MakeSave() hands out the seed and these
 * changes for a save game; GenerateFromSave() builds that world again.
 *
 * Chunks are placed in absolute world space (block (0, 0, 0) is always at the world origin),
 * so moving this actor in the level does not move the terrain.
 */
UCLASS()
class KUBECRAFT_API AVoxelWorld : public AActor
{
	GENERATED_BODY()

public:
	/** Creates the world actor with its streaming component; it has no visuals of its own. */
	AVoxelWorld();

	/** Picks the seed, then loads and draws the chunks around block (0, 0) at once. Call once, before the player is placed. */
	void Generate();

	/**
	 * Makes Generate use this seed instead of a random one, e.g. one typed in for a new world.
	 * Call before the world is built; afterwards the terrain is already made, so it is ignored (with a warning).
	 */
	UFUNCTION(BlueprintCallable, Category = "KubeCraft|World")
	void SetSeed(int32 NewSeed);

	/**
	 * Builds a saved world instead of a new one: its seed, its changes, and the chunks around the player's saved
	 * location at once, so the player lands on ground. Call once instead of Generate, before the player is placed.
	 */
	UFUNCTION(BlueprintCallable, Category = "KubeCraft|World")
	void GenerateFromSave(const FVoxelWorldSave& Save, const FVector& PlayerLocation);

	/** The seed and every player change, for a SaveGame to store. */
	UFUNCTION(BlueprintPure, Category = "KubeCraft|World")
	FVoxelWorldSave MakeSave() const;

	/** Block at a world block coordinate. Air where no chunk is loaded and above or below the world. */
	FBlockId GetBlock(const FIntVector& Block) const;

	/** Block type at a world block coordinate, or null for Air. */
	const UBlockDefinition* GetBlockDefinition(const FIntVector& Block) const;

	/** World position (cm) on top of the highest solid block in a column, i.e. where a player can stand. */
	FVector GetSurfaceLocation(const FIntPoint& Column) const;

	/** True if the block lies between the floor and the ceiling of a loaded chunk, i.e. it can hold a block. */
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

	// Chunk loading - called by UChunkStreamingComponent, which decides which chunks should exist.

	/**
	 * Creates a chunk and fills it: terrain from the seed, then the remembered player changes. Does not draw it,
	 * because its border cubes need the neighbor chunks. Does nothing if it is already loaded or Generate has not run.
	 */
	void LoadChunk(const FIntPoint& ChunkCoord);

	/** Draws a loaded chunk. Its four neighbors should be loaded first, or its border cubes are drawn as if facing Air. */
	void DrawChunk(const FIntPoint& ChunkCoord);

	/** Destroys a chunk. Its player changes stay remembered for the next LoadChunk. */
	void UnloadChunk(const FIntPoint& ChunkCoord);

	/** True if the chunk is loaded (filled with blocks). */
	bool IsChunkLoaded(const FIntPoint& ChunkCoord) const { return FindChunk(ChunkCoord) != nullptr; }

	/** True if the chunk is loaded and drawn. */
	bool IsChunkDrawn(const FIntPoint& ChunkCoord) const;

	/** Coordinates of every loaded chunk. */
	TArray<FIntPoint> GetLoadedChunks() const;

private:
#if WITH_DEV_AUTOMATION_TESTS
	/** Builds small worlds with a catalog made in code for the automation tests (Private/Tests/TestVoxelWorld.h). */
	friend struct FTestVoxelWorld;
#endif

	/** Chunk at a chunk coordinate, or null if it is not loaded. */
	AVoxelChunk* FindChunk(const FIntPoint& ChunkCoord) const;

	/** True if the world can be built: not built yet, and Catalog and BlockMesh are set (else it logs why not). */
	bool CanGenerate() const;

	/** Makes the terrain generator from Seed, then loads and draws the chunks around a chunk. The end of both Generates. */
	void GenerateAround(const FIntPoint& CenterChunk);

	/** Makes the terrain generator from Seed. Part of GenerateAround. */
	void PrepareTerrain();

	/**
	 * Remembers a player change for when its chunk loads again. A block set back to what the terrain had there
	 * is forgotten instead, because the terrain makes it again by itself.
	 */
	void RememberChange(const FIntVector& Block, FBlockId Id);

	/**
	 * Changes one block, remembers the change and redraws the block and its six neighbors - nothing else can change.
	 * On a chunk border some of those neighbors belong to the next chunk; they are redrawn there.
	 * Returns true if the block changed; if not (no chunk there, or the chunk refused the id), nothing is redrawn.
	 */
	bool SetBlockAndRedraw(const FIntVector& Block, FBlockId Id);

	/** Redraws the cube of one block in whichever chunk holds it. Does nothing outside drawn chunks. */
	void RedrawBlock(const FIntVector& Block);

	/** Decides which chunks are loaded and drawn around the player. */
	UPROPERTY(VisibleAnywhere, Category = "KubeCraft|World")
	TObjectPtr<UChunkStreamingComponent> Streaming;

	/** All block types and the height rule. */
	UPROPERTY(EditAnywhere, Category = "KubeCraft|World")
	TObjectPtr<UBlockCatalog> Catalog;

	/** Mesh drawn for every cube; must be BlockSize wide with its pivot in the center. */
	UPROPERTY(EditAnywhere, Category = "KubeCraft|World")
	TObjectPtr<UStaticMesh> BlockMesh;

	/** Pick a new seed on every start. Turn off to always get the world of Seed. */
	UPROPERTY(EditAnywhere, Category = "KubeCraft|World")
	bool bRandomSeed = true;

	/**
	 * Seed of the terrain. Overwritten on start when bRandomSeed is on, by SetSeed, and by a loaded save.
	 * The seed in use is written to the log.
	 */
	UPROPERTY(EditAnywhere, Category = "KubeCraft|World", meta = (EditCondition = "!bRandomSeed"))
	int32 Seed = 0;

	/** Shape of the hills. */
	UPROPERTY(EditAnywhere, Category = "KubeCraft|World")
	FTerrainSettings Terrain;

	/** Made once from Seed and Terrain by Generate, so every chunk gets the same land - even if they are edited later. */
	TOptional<FTerrainGenerator> Generator;

	/** Every loaded chunk, by chunk coordinate. */
	UPROPERTY(Transient)
	TMap<FIntPoint, TObjectPtr<AVoxelChunk>> Chunks;

	/**
	 * Every block the player changed, by chunk, then by local coordinate: the new id.
	 * Kept for unloaded chunks too, and applied on top of the terrain when a chunk loads.
	 */
	TMap<FIntPoint, TMap<FIntVector, FBlockId>> ChangedBlocks;
};
