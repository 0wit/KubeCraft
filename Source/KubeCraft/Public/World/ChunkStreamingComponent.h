#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Game/KubeCraftUserSettings.h"
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

	/** Takes the view distance and chunks per frame from the player's settings, and again whenever they are applied. */
	virtual void BeginPlay() override;

	/** Stops listening to the player's settings. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 * Each frame it finds the player's chunk. If the player moved chunks, it does a few chunks of work (MaxChunksPerFrame).
	 * It stops once everything is done until the player changes chunk again.
	 */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
#if WITH_DEV_AUTOMATION_TESTS
	/** Calls UpdateAround with its own center and limit (Private/Tests/ChunkStreamingTest.cpp). */
	friend class FChunkStreamingTest;
	/** Applies settings and reads the finished state (Private/Tests/ChunkStreamingTest.cpp). */
	friend class FStreamingSettingsTest;
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

	/** Copies both values from the player's settings; if the view distance changed, the next tick streams again. */
	void ApplyUserSettings(const UKubeCraftUserSettings& Settings);

	/**
	 * How far the player sees, in chunks (player setting).
	 * Chunks inside this circle get drawn.
	 * Chunks beyond it + 2 get unloaded.
	 */
	int32 ViewDistanceInChunks = UKubeCraftUserSettings::DefaultViewDistance;

	/**
	 * The most loads, draws and unloads allowed per frame (player setting).
	 * Higher means the world fills in faster but can cause stutters.
	 */
	int32 MaxChunksPerFrame = UKubeCraftUserSettings::DefaultChunksPerFrame;

	/** Listening to UKubeCraftUserSettings::OnApplied while playing. */
	FDelegateHandle SettingsAppliedHandle;

	/** The player's chunk the last time all work was finished; nothing to do until the player leaves it. */
	TOptional<FIntPoint> FinishedCenter;

	/**
	 * A cached copy of the view-circle offsets and the view distance it was built for.
	 * If that distance changes, the list is rebuilt.
	 * */
	TArray<FIntPoint> OffsetsNearestFirst;
	int32 OffsetsRadius = INDEX_NONE;
};
