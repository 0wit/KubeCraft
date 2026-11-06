#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "KubeCraftGameMode.generated.h"

class AVoxelWorld;

/**
 * Sets the start order of a game: first build the voxel world, then put the players on it - before anything begins
 * play. Which pawn the players get is configured in BP_KubeCraftGameMode, which can also build a saved world instead.
 */
UCLASS()
class KUBECRAFT_API AKubeCraftGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	/** Finds the world, builds it, lets everything start. */
	virtual void StartPlay() override;

protected:
	/**
	 * Makes a new world and puts each player's feet on the ground at the center.
	 * BP_KubeCraftGameMode can replace it to load a save instead.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "KubeCraft|Game")
	void StartWorld(AVoxelWorld* VoxelWorld);

private:
	/** The voxel world placed in the level, or null (with an error in the log) if there is none. */
	AVoxelWorld* FindVoxelWorld() const;
};
