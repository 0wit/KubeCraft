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
	/** Sets the default capsule size, jump strength and camera height, all derived from the block size. */
	AKubeCraftCharacter();

protected:
	/** Moves the input mapping context from the previous player (if any) to the one now controlling this pawn. */
	virtual void NotifyControllerChanged() override;

	/** Binds the Move, Look and Jump actions to this character. */
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	/** Walks relative to where the character faces (2D input: X = right, Y = forward). */
	void Move(const FInputActionValue& Value);

	/** Turns the view (2D input: X = yaw, Y = pitch). */
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
