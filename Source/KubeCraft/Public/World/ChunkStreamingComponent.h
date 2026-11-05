#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ChunkStreamingComponent.generated.h"

/**
 * Keeps the chunks around the player loaded and drawn, so the world has no edge. It lives on AVoxelWorld and tells
 * it which chunks to load, draw and unload - at most MaxChunksPerFrame in all per frame, so crossing a border does
 * not hitch.
 *
 * Distance is measured in chunks from the player's chunk, as a circle:
 *   up to ViewDistance        drawn, nearest first; its four neighbors are loaded just before, for its border cubes
 *   beyond ViewDistance + 2   unloaded (one ring of slack: walking back and forth over a border reloads nothing)
 * A chunk that was drawn stays drawn until it is unloaded.
 */
UCLASS(ClassGroup = (KubeCraft))
class KUBECRAFT_API UChunkStreamingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Ticks only after AVoxelWorld::Generate has picked the seed and switched it on. */
	UChunkStreamingComponent();

	/** Loads and draws everything around a chunk at once, ignoring MaxChunksPerFrame. For the start of the game. */
	void LoadAllAround(const FIntPoint& CenterChunk);

	/** Follows the first player: does the next few chunks of work around the chunk they stand in. */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
#if WITH_DEV_AUTOMATION_TESTS
	/** Calls UpdateAround with its own center and limit (Private/Tests/ChunkStreamingTest.cpp). */
	friend class FChunkStreamingTest;
	/** Sets the view distance and streams around a chunk for the other tests (Private/Tests/TestVoxelWorld.h). */
	friend struct FTestVoxelWorld;
#endif

	/**
	 * Loads and draws the missing chunks nearest first, then unloads the far ones - at most MaxChunks of these in all.
	 * Returns true when nothing is left to do around this center.
	 */
	bool UpdateAround(const FIntPoint& CenterChunk, int32 MaxChunks);

	/** Every chunk offset within ViewDistanceInChunks (a circle), nearest first. Made again only if the distance changes. */
	const TArray<FIntPoint>& GetOffsetsNearestFirst();

	/** How many chunks far the player sees. At least 2, so the chunks next to the player's chunk are always drawn. */
	UPROPERTY(EditAnywhere, Category = "KubeCraft|Streaming", meta = (ClampMin = "2"))
	int32 ViewDistanceInChunks = 4;

	/** Most chunks loaded, drawn or unloaded in one frame. Higher = the world appears faster, but a frame can take longer. */
	UPROPERTY(EditAnywhere, Category = "KubeCraft|Streaming", meta = (ClampMin = "1"))
	int32 MaxChunksPerFrame = 2;

	/** The player's chunk the last time all work was finished; nothing to do until the player leaves it. */
	TOptional<FIntPoint> FinishedCenter;

	/** Kept by GetOffsetsNearestFirst, for the view distance in OffsetsRadius. */
	TArray<FIntPoint> OffsetsNearestFirst;
	int32 OffsetsRadius = INDEX_NONE;
};
