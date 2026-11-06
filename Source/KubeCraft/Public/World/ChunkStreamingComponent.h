#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ChunkStreamingComponent.generated.h"

/**
 * The component keeps the chunks around the player loaded and drawn.
 */
UCLASS(ClassGroup = (KubeCraft))
class KUBECRAFT_API UChunkStreamingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Ticks only after AVoxelWorld has built the world (Generate or GenerateFromSave) and switched it on. */
	UChunkStreamingComponent();

	/** Loads and draws everything around a chunk, used at game start. */
	void LoadAllAround(const FIntPoint& CenterChunk);

	/**
	 * Each frame it finds the player's chunk. If the player moved chunks, it does a few chunks of work (MaxChunksPerFrame).
	 * It stops once everything is done until the player changes chunk again.
	 */
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

	/** A cached list of the offsets inside the view circle, sorted by distance. */
	const TArray<FIntPoint>& GetOffsetsNearestFirst();

	/**
	 * How far the player sees, in chunks.
	 * Chunks inside this circle get drawn.
	 * Chunks beyond it + 2 get unloaded.
	 */
	UPROPERTY(EditAnywhere, Category = "KubeCraft|Streaming", meta = (ClampMin = "2"))
	int32 ViewDistanceInChunks = 4;

	/**
	 * The most loads, draws and unloads allowed per frame.
	 * Higher means the world fills in faster but can cause stutters.
	 */
	UPROPERTY(EditAnywhere, Category = "KubeCraft|Streaming", meta = (ClampMin = "1"))
	int32 MaxChunksPerFrame = 2;

	/** The player's chunk the last time all work was finished; nothing to do until the player leaves it. */
	TOptional<FIntPoint> FinishedCenter;

	/**
	 * A cached copy of the view-circle offsets and the view distance it was built for.
	 * If that distance changes, the list is rebuilt.
	 * */
	TArray<FIntPoint> OffsetsNearestFirst;
	int32 OffsetsRadius = INDEX_NONE;
};
