#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "KubeCraftGameMode.generated.h"

class AVoxelWorld;

/**
 * Sets the start order of a game: first build the voxel world, then put the players on its surface.
 * Which pawn the players get is configured in BP_KubeCraftGameMode.
 */
UCLASS()
class KUBECRAFT_API AKubeCraftGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	/** Generates the voxel world placed in the level and moves every player onto the ground at its center. */
	virtual void StartPlay() override;

private:
	/** The voxel world placed in the level, or null (with an error in the log) if there is none. */
	AVoxelWorld* FindVoxelWorld() const;
};
