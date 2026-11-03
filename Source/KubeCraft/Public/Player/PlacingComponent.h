#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "World/VoxelTypes.h"
#include "PlacingComponent.generated.h"

/**
 * Press-to-place for the pawn it is attached to: puts a block against the face the player looks at.
 * The world decides whether that is allowed and which block type it becomes.
 */
UCLASS(ClassGroup = (KubeCraft), meta = (BlueprintSpawnableComponent))
class KUBECRAFT_API UPlacingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Places one block in the empty cell in front of the targeted face, if there is a target in reach. */
	void Place();

private:
	/** How far from the camera the targeted face can be. */
	UPROPERTY(EditAnywhere, Category = "KubeCraft|Placing", meta = (ClampMin = "0", Units = "cm"))
	float Reach = 5.f * KubeCraft::Voxel::BlockSize;
};
