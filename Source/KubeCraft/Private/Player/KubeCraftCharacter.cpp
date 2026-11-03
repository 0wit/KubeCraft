#include "Player/KubeCraftCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "KubeCraft.h"

namespace
{
	// Player is 0.6 x 1.8 blocks, like in Minecraft: fits through a 1-wide, 2-high gap.
	constexpr float CapsuleRadius = 30.f;
	constexpr float CapsuleHalfHeight = 90.f;

	/** Camera height above the capsule center (eyes at ~1.6 m). */
	constexpr float EyeHeightAboveCenter = 70.f;

	/** Enough to jump onto a block that is one level higher (peak ~1.27 m with default gravity). */
	constexpr float JumpZVelocity = 500.f;
}

AKubeCraftCharacter::AKubeCraftCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(CapsuleRadius, CapsuleHalfHeight);
	GetCharacterMovement()->JumpZVelocity = JumpZVelocity;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetCapsuleComponent());
	Camera->SetRelativeLocation(FVector(0.f, 0.f, EyeHeightAboveCenter));
	Camera->bUsePawnControlRotation = true;
}

void AKubeCraftCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return; // Not controlled by a player (e.g. just unpossessed): nothing to map.
	}
	if (!DefaultMappingContext)
	{
		UE_LOG(LogKubeCraft, Error, TEXT("%s: DefaultMappingContext is not assigned."), *GetName());
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
	{
		InputSubsystem->AddMappingContext(DefaultMappingContext, /*Priority*/ 0);
	}
}

void AKubeCraftCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInput)
	{
		UE_LOG(LogKubeCraft, Error, TEXT("%s requires the Enhanced Input component."), *GetName());
		return;
	}
	if (!MoveAction || !LookAction || !JumpAction)
	{
		UE_LOG(LogKubeCraft, Error, TEXT("%s: Move, Look and Jump input actions must all be assigned."), *GetName());
		return;
	}

	EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AKubeCraftCharacter::Move);
	EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &AKubeCraftCharacter::Look);
	EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
	EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
}

void AKubeCraftCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D Input = Value.Get<FVector2D>();
	AddMovementInput(GetActorForwardVector(), Input.Y);
	AddMovementInput(GetActorRightVector(), Input.X);
}

void AKubeCraftCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D Input = Value.Get<FVector2D>();
	AddControllerYawInput(Input.X);
	AddControllerPitchInput(Input.Y);
}
