#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MiningComponent.generated.h"

class AVoxelWorld;
class UBlockDefinition;
class ULookTargetComponent;

/** A block was really removed by this mining component; Type is what it was (e.g. DA_Block_Stone). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBlockMined, const UBlockDefinition*, Type);

/**
 * Hold-to-mine for the pawn it is attached to: while the mine button is held, it times how long the player
 * keeps looking at the same block and asks the world to remove it once the block type's MineTime is reached.
 * Looking at another block (or letting go) starts the timer over. The target comes from the owner's ULookTargetComponent.
 */
UCLASS(ClassGroup = (KubeCraft), meta = (BlueprintSpawnableComponent))
class KUBECRAFT_API UMiningComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Ticks only while the mine button is held. */
	UMiningComponent();

	/** Clears old progress and turns ticking on. */
	void StartMining();

	/** Turns ticking off and clears progress. */
	void StopMining();

	/** Times how long you look at the same block and mines it once MineTime is reached. */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Gives Blueprints the mined block's centre and its progress from 0 to 1. */
	UFUNCTION(BlueprintPure, Category = "KubeCraft|Mining", meta = (ReturnDisplayName = "Is Mining"))
	bool GetMiningProgress(FVector& BlockCenter, float& Progress) const;

	/** Fires once for each block broken, with its type (in Blueprints) */
	UPROPERTY(BlueprintAssignable, Category = "KubeCraft|Mining")
	FOnBlockMined OnBlockMined;

protected:
	/** Finds the owner's aim component. */
	virtual void BeginPlay() override;

private:
	/** Forgets the current block and sets the timer back to 0 */
	void ResetProgress();

	/** The owner's aim; tells which block is targeted. Null if the owner has none (then nothing is mined). */
	UPROPERTY(Transient)
	TObjectPtr<ULookTargetComponent> LookTarget;

	/** World of the block being mined; empty when nothing is being mined. */
	TWeakObjectPtr<AVoxelWorld> MinedWorld;

	/** Block being mined. Only valid while MinedWorld is set. */
	FIntVector MinedBlock = FIntVector::ZeroValue;

	/** How long the current block has been mined so far. */
	float MinedSeconds = 0.f;
};
