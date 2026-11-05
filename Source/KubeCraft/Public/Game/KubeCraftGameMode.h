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
	/** Calls StartWorld with the voxel world placed in the level, then lets everything begin play. */
	virtual void StartPlay() override;

protected:
	/**
	 * Builds the voxel world and puts the players on it. By default a new world, with every player on the ground at
	 * its center. A Blueprint can override it to load a save instead (AVoxelWorld::GenerateFromSave) and put the
	 * player back where they were. The pawns exist here, but nothing has begun play yet.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "KubeCraft|Game")
	void StartWorld(AVoxelWorld* VoxelWorld);

private:
	/** The voxel world placed in the level, or null (with an error in the log) if there is none. */
	AVoxelWorld* FindVoxelWorld() const;
};
