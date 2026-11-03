#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlacingComponent.generated.h"

class ULookTargetComponent;

/**
 * Press-to-place for the pawn it is attached to: puts a block against the face the player looks at
 * (the target comes from the owner's ULookTargetComponent).
 * The world decides whether that is allowed and which block type it becomes.
 */
UCLASS(ClassGroup = (KubeCraft), meta = (BlueprintSpawnableComponent))
class KUBECRAFT_API UPlacingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Places one block in the empty cell in front of the targeted face, if there is a target in reach. */
	void Place();

protected:
	/** Finds the owner's aim component. */
	virtual void BeginPlay() override;

private:
	/** The owner's aim; tells which face is targeted. Null if the owner has none (then nothing is placed). */
	UPROPERTY(Transient)
	TObjectPtr<ULookTargetComponent> LookTarget;
};
