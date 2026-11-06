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

	/**
	 * The block type, or null for air (mined block).
	 * The type itself, not its id, a reordered catalog still loads.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "KubeCraft|World")
	TObjectPtr<const UBlockDefinition> Type = nullptr;
};

/**
 * Everything needed to build a voxel world again: the seed makes the same terrain with the changes on top.
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
 * Actor that holds the whole block world.
 * Keeps the loaded chunks.
 * Answers "what block is at X?"
 * The only place where blocks get changed.
 */
UCLASS()
class KUBECRAFT_API AVoxelWorld : public AActor
{
	GENERATED_BODY()

public:
	/** Creates the world actor with its streaming component; no visuals of its own. */
	AVoxelWorld();

	/**
	 * Picks the seed, then loads and draws the chunks around block (0, 0).
	 * Called once, before the player is placed.
	 */
	void Generate();

	/**
	 * Makes Generate use this seed instead of a random one, one typed in for a new world.
	 * Call before the world is built.
	 */
	UFUNCTION(BlueprintCallable, Category = "KubeCraft|World")
	void SetSeed(int32 NewSeed);

	/**
	 * Reads a seed the player typed in: a whole number that fits in an int32, spaces around it allowed.
	 * Returns false for anything else, random seed is picked in that case.
	 */
	UFUNCTION(BlueprintPure, Category = "KubeCraft|World")
	static bool TryParseSeed(const FString& Text, int32& OutSeed);

	/**
	 * Builds a saved world instead of a new one: its seed, its changes, and the chunks around the player's saved location.
	 * Called once instead of Generate, before the player is placed.
	 */
	UFUNCTION(BlueprintCallable, Category = "KubeCraft|World")
	void GenerateFromSave(const FVoxelWorldSave& Save, const FVector& PlayerLocation);

	/**
	 * The seed and every player change to store in a SaveGame.
	 * Not pure, copies every change.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "KubeCraft|World")
	FVoxelWorldSave MakeSave() const;

	/** Block at a world block coordinate. Air where no chunk is loaded and above or below the world. */
	FBlockId GetBlock(const FIntVector& Block) const;

	/** Block type at a world block coordinate, or null for air. */
	const UBlockDefinition* GetBlockDefinition(const FIntVector& Block) const;

	/** World position on top of the highest solid block in a column, where a player can stand. */
	FVector GetSurfaceLocation(const FIntPoint& Column) const;

	/** True if the block lies between the floor and the ceiling of a loaded chunk. */
	bool IsInsideWorld(const FIntVector& Block) const;

	/**
	 * Removes a block if FBlockRules allows it and redraws what changed.
	 * Returns true if the block was removed.
	 */
	bool MineBlock(const FIntVector& Block);

	/**
	 * Puts a block of the given type here if FBlockRules allows it, redraws what changed.
	 * Returns true if a block was placed.
	 */
	bool PlaceBlock(const FIntVector& Block, const UBlockDefinition* Type);

	/** All block types and the height rule; shared by every chunk. */
	const UBlockCatalog* GetCatalog() const { return Catalog; }

	/** Mesh drawn for every cube, shared by every chunk. */
	UStaticMesh* GetBlockMesh() const { return BlockMesh; }

	// Chunk loading called by UChunkStreamingComponent, which decides which chunks should exist.

	/**
	 * Spawns a chunk actor, fills it with terrain from the seed.
	 * Puts the remembered player changes on top.
	 * Doesn't draw it.
	 */
	void LoadChunk(const FIntPoint& ChunkCoord);

	/** Draws a loaded chunk. Its four neighbors should be loaded first. */
	void DrawChunk(const FIntPoint& ChunkCoord);

	/** Destroys a chunk. Its player changes stay remembered for the next LoadChunk. */
	void UnloadChunk(const FIntPoint& ChunkCoord);

	/** True if the chunk is loaded. */
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

	/** Makes the terrain generator from Seed. The first step of both Generates; everything after it may use Generator. */
	void PrepareTerrain();

	/** Loads and draws the chunks around a chunk at once, then lets the streaming follow the player. The last step of both Generates. */
	void GenerateAround(const FIntPoint& CenterChunk);

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

	/** Pick a new seed on every start. */
	UPROPERTY(EditAnywhere, Category = "KubeCraft|World")
	bool bRandomSeed = true;

	/**
	 * The number the terrain is made from: same seed, same terrain.
	 * Overwritten on start when bRandomSeed is on, by SetSeed, and by a loaded save.
	 */
	UPROPERTY(EditAnywhere, Category = "KubeCraft|World", meta = (EditCondition = "!bRandomSeed"))
	int32 Seed = 0;

	/** Shape of the hills. */
	UPROPERTY(EditAnywhere, Category = "KubeCraft|World")
	FTerrainSettings Terrain;

	/**
	 * The terrain maker, created once from Seed and Terrain..
	 * It also works as a flag: set = the world is built.
	 */
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
