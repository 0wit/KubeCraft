#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "KubeCraftCharacter.generated.h"

class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class ULookTargetComponent;
class UMiningComponent;
class UPlacingComponent;
struct FInputActionValue;

/**
 * AKubeCraftCharacter is the first-person player.
 * It sets up the body, then passes on buttons.
 */
UCLASS()
class KUBECRAFT_API AKubeCraftCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	/** Sets the default capsule size, jump strength and camera height, all derived from the block size. */
	AKubeCraftCharacter();

protected:
	/** Ticks after mining, so the cracks show this frame's progress. */
	virtual void BeginPlay() override;

	/**
	 * Runs on the first possession, that's when the player gets their keys
	 */
	virtual void NotifyControllerChanged() override;

	/** Binds Move, Look and Jump to this character, and Mine to its mining component. (Place is handled by the Blueprint)
	 * TODO: fix?
	 **/
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	/** Walks relative to where the character faces (2D input: X = right, Y = forward). */
	void Move(const FInputActionValue& Value);

	/** Turns the view (2D input: X = yaw, Y = pitch). */
	void Look(const FInputActionValue& Value);

	/** First-person view at eye height; it turns with the mouse. */
	UPROPERTY(VisibleAnywhere, Category = "KubeCraft|Camera")
	TObjectPtr<UCameraComponent> Camera;

	/** Finds the block in the middle of the screen; mining and placing use it. Blueprints read it for the outline. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KubeCraft|Blocks", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULookTargetComponent> LookTargetComponent;

	/** The hold-to-mine timer for the targeted block. Blueprints read it for the cracks and the inventory. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KubeCraft|Blocks", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMiningComponent> MiningComponent;

	/** Places a block against the face you look at. Blueprint's place input calls it. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KubeCraft|Blocks", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPlacingComponent> PlacingComponent;

	/** Which keys trigger which input action (IMC_Default); given to the controlling player. */
	UPROPERTY(EditDefaultsOnly, Category = "KubeCraft|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	/** Walk (2D: forward/back, left/right). */
	UPROPERTY(EditDefaultsOnly, Category = "KubeCraft|Input")
	TObjectPtr<UInputAction> MoveAction;

	/** Turn the view (2D: yaw, pitch). */
	UPROPERTY(EditDefaultsOnly, Category = "KubeCraft|Input")
	TObjectPtr<UInputAction> LookAction;

	/** Pressed to jump; released to stop a held jump. */
	UPROPERTY(EditDefaultsOnly, Category = "KubeCraft|Input")
	TObjectPtr<UInputAction> JumpAction;

	/** Held to mine the targeted block. */
	UPROPERTY(EditDefaultsOnly, Category = "KubeCraft|Input")
	TObjectPtr<UInputAction> MineAction;
};
