#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "World/VoxelTypes.h"
#include "MiningComponent.generated.h"

class AVoxelWorld;

/**
 * Hold-to-mine for the pawn it is attached to: while the mine button is held, it times how long the player
 * keeps looking at the same block and asks the world to remove it once the block type's MineTime is reached.
 * Looking at another block (or letting go) starts the timer over.
 */
UCLASS(ClassGroup = (KubeCraft), meta = (BlueprintSpawnableComponent))
class KUBECRAFT_API UMiningComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Ticks only while the mine button is held. */
	UMiningComponent();

	/** Mine button pressed: start timing the targeted block. */
	void StartMining();

	/** Mine button released: stop and forget the progress. */
	void StopMining();

	/** Advances the timer on the targeted block and mines it when the time is up. */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** Forgets the block being mined, so the next tick starts from zero. */
	void ResetProgress();

	/** How far from the camera a block can be mined. */
	UPROPERTY(EditAnywhere, Category = "KubeCraft|Mining", meta = (ClampMin = "0", Units = "cm"))
	float Reach = 5.f * KubeCraft::Voxel::BlockSize;

	/** World of the block being mined; empty when nothing is being mined. */
	TWeakObjectPtr<AVoxelWorld> MinedWorld;

	/** Block being mined. Only valid while MinedWorld is set. */
	FIntVector MinedBlock = FIntVector::ZeroValue;

	/** How long the current block has been mined so far. */
	float MinedSeconds = 0.f;
};
