#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlacingComponent.generated.h"

class UBlockDefinition;
class ULookTargetComponent;

/**
 * Placing for the pawn it is attached to: puts a block of a given type against the face the player looks at
 * (the target comes from the owner's ULookTargetComponent). The world decides whether that is allowed.
 * Which type to place, and whether the player has one, is decided by the caller (the inventory in Blueprint).
 */
UCLASS(ClassGroup = (KubeCraft), meta = (BlueprintSpawnableComponent))
class KUBECRAFT_API UPlacingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/**
	 * Places one block of this type in the empty cell in front of the targeted face.
	 * Returns false if Type is null (empty slot), nothing is in reach or the world refused it,
	 * so the caller only uses up a block on success.
	 */
	UFUNCTION(BlueprintCallable, Category = "KubeCraft|Placing", meta = (ReturnDisplayName = "Placed"))
	bool Place(const UBlockDefinition* Type);

protected:
	/** Finds the owner's aim component. */
	virtual void BeginPlay() override;

private:
	/** The owner's aim; tells which face is targeted. Null if the owner has none (then nothing is placed). */
	UPROPERTY(Transient)
	TObjectPtr<ULookTargetComponent> LookTarget;
};
