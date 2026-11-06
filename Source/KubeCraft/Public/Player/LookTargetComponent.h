#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "World/VoxelTypes.h"
#include "LookTargetComponent.generated.h"

class AVoxelWorld;

/**
 * The block a player is looking at, and the empty cell in front of the face they look at.
 * A short-lived answer: use it right away, don't keep it across frames (World is a plain pointer).
 */
struct FLookTarget
{
	/** World that owns the block. Never null in a target returned by ULookTargetComponent::Trace. */
	AVoxelWorld* World = nullptr;

	/** The cube that was hit; mining removes this one. */
	FIntVector Block = FIntVector::ZeroValue;

	/** The cell in front of the hit face; placing fills this one. */
	FIntVector PlaceCell = FIntVector::ZeroValue;
};

/**
 * Aim of the pawn it is attached to: which block is in the middle of the screen, within Reach.
 * Mining and placing both ask this one component, so they always agree on the target and the reach.
 */
UCLASS(ClassGroup = (KubeCraft), meta = (BlueprintSpawnableComponent))
class KUBECRAFT_API ULookTargetComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/**
	 * Casts a line from the camera, up to Reach (5 blocks)
	 * Returns block that was hit, empty if nothing is in reach.
	 */
	TOptional<FLookTarget> Trace() const;

	/**
	 * Blueprint version used for the block outline.
	 * It runs Trace() once
	 * Gives back true plus the block's centre in cm
	 * Gives back false if no block is in reach.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "KubeCraft|Aim", meta = (ReturnDisplayName = "Found"))
	bool FindLookedAtBlockCenter(FVector& BlockCenter) const;

private:
	/** How far from the camera a block can be mined or placed against. */
	UPROPERTY(EditAnywhere, Category = "KubeCraft|Aim", meta = (ClampMin = "0", Units = "cm"))
	float Reach = 5.f * KubeCraft::Voxel::BlockSize;
};
