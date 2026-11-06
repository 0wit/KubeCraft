#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlacingComponent.generated.h"

class UBlockDefinition;
class ULookTargetComponent;

/**
 * Puts a block where the player is looking.
 */
UCLASS(ClassGroup = (KubeCraft), meta = (BlueprintSpawnableComponent))
class KUBECRAFT_API UPlacingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/**
	 * Places one block of this type in the empty cell the player is looking it.
	 * The world may refuse it (e.g. if the cell is not empty), the player only uses up a block on success.
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
