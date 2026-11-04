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
 * First-person player: a capsule sized to fit through 1x2 block gaps, a camera at eye height,
 * and Enhanced Input bindings for walking, looking, jumping and mining.
 * Mining and placing are done by two components that share one aim component (ULookTargetComponent).
 * The character passes the mine button on; the place button is handled by the Blueprint subclass
 * (BP_KubeCraftCharacter), because it first asks the Blueprint inventory which block to place.
 * Input assets are assigned in that Blueprint subclass.
 */
UCLASS()
class KUBECRAFT_API AKubeCraftCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	/** Sets the default capsule size, jump strength and camera height, all derived from the block size. */
	AKubeCraftCharacter();

protected:
	/** Makes the character tick after its mining component, so the Blueprint visuals read up-to-date progress. */
	virtual void BeginPlay() override;

	/**
	 * Moves the input mapping context from the previous player (if any) to the one now controlling this pawn,
	 * and stops mining, because the old player's button release can no longer arrive.
	 */
	virtual void NotifyControllerChanged() override;

	/** Binds Move, Look and Jump to this character, and Mine to its mining component. */
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	/** Walks relative to where the character faces (2D input: X = right, Y = forward). */
	void Move(const FInputActionValue& Value);

	/** Turns the view (2D input: X = yaw, Y = pitch). */
	void Look(const FInputActionValue& Value);

	UPROPERTY(VisibleAnywhere, Category = "KubeCraft|Camera")
	TObjectPtr<UCameraComponent> Camera;

	/** Which block is in the middle of the screen; shared by mining and placing. Readable in Blueprints for visual feedback. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KubeCraft|Blocks", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULookTargetComponent> LookTargetComponent;

	/** Hold-to-mine timer for the targeted block. Readable in Blueprints for visual feedback and the inventory. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KubeCraft|Blocks", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMiningComponent> MiningComponent;

	/** Places a block against the face the player looks at. Called from the Blueprint's place input. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KubeCraft|Blocks", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPlacingComponent> PlacingComponent;

	UPROPERTY(EditDefaultsOnly, Category = "KubeCraft|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "KubeCraft|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "KubeCraft|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "KubeCraft|Input")
	TObjectPtr<UInputAction> JumpAction;

	/** Held to mine the targeted block. */
	UPROPERTY(EditDefaultsOnly, Category = "KubeCraft|Input")
	TObjectPtr<UInputAction> MineAction;
};
