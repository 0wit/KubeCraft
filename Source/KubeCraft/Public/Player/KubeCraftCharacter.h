#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "KubeCraftCharacter.generated.h"

class UCameraComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

/**
 * First-person player: a capsule sized to fit through 1x2 block gaps, a camera at eye height,
 * and Enhanced Input bindings for walking, looking and jumping.
 * Input assets are assigned in the Blueprint subclass (BP_KubeCraftCharacter).
 */
UCLASS()
class KUBECRAFT_API AKubeCraftCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AKubeCraftCharacter();

protected:
	/** Adds the input mapping context once a local player controls this pawn. */
	virtual void NotifyControllerChanged() override;

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	UPROPERTY(VisibleAnywhere, Category = "KubeCraft|Camera")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(EditDefaultsOnly, Category = "KubeCraft|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "KubeCraft|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "KubeCraft|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "KubeCraft|Input")
	TObjectPtr<UInputAction> JumpAction;
};
