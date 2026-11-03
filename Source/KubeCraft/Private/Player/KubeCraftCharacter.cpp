#include "Player/KubeCraftCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "KubeCraft.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "Player/MiningComponent.h"
#include "Player/PlacingComponent.h"
#include "World/VoxelTypes.h"

namespace
{
	// Default size in blocks, like in Minecraft: 0.6 wide and 1.8 tall fits through a 1-wide, 2-high gap.
	// These only seed the components; BP_KubeCraftCharacter can still override them on the components.
	constexpr float PlayerWidthInBlocks = 0.6f;
	constexpr float PlayerHeightInBlocks = 1.8f;
	constexpr float EyeHeightInBlocks = 1.6f;	// Measured from the feet.

	/** Peak of a jump. A bit more than one block, so the player can step up onto the next level. */
	constexpr float JumpHeightInBlocks = 1.25f;

	/** Enhanced Input subsystem of the local player behind a controller, or null for AI / no controller. */
	UEnhancedInputLocalPlayerSubsystem* GetInputSubsystem(const AController* Controller)
	{
		const APlayerController* PlayerController = Cast<APlayerController>(Controller);
		return PlayerController
			? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer())
			: nullptr;
	}
}

AKubeCraftCharacter::AKubeCraftCharacter()
{
	using KubeCraft::Voxel::BlockSize;

	PrimaryActorTick.bCanEverTick = false;

	const float CapsuleRadius = 0.5f * PlayerWidthInBlocks * BlockSize;
	const float CapsuleHalfHeight = 0.5f * PlayerHeightInBlocks * BlockSize;
	GetCapsuleComponent()->InitCapsuleSize(CapsuleRadius, CapsuleHalfHeight);

	// Launch speed that reaches the jump height: v = sqrt(2 * g * h).
	const float Gravity = FMath::Abs(UPhysicsSettings::Get()->DefaultGravityZ);
	GetCharacterMovement()->JumpZVelocity = FMath::Sqrt(2.f * Gravity * JumpHeightInBlocks * BlockSize);

	// The camera is attached to the capsule center, so the eye height is measured from there.
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetCapsuleComponent());
	Camera->SetRelativeLocation(FVector(0.f, 0.f, EyeHeightInBlocks * BlockSize - CapsuleHalfHeight));
	Camera->bUsePawnControlRotation = true;

	MiningComponent = CreateDefaultSubobject<UMiningComponent>(TEXT("Mining"));
	PlacingComponent = CreateDefaultSubobject<UPlacingComponent>(TEXT("Placing"));
}

void AKubeCraftCharacter::NotifyControllerChanged()
{
	// Super overwrites PreviousController, so take this pawn's keys away from the old player first.
	if (UEnhancedInputLocalPlayerSubsystem* OldInput = GetInputSubsystem(PreviousController))
	{
		OldInput->RemoveMappingContext(DefaultMappingContext);
	}

	Super::NotifyControllerChanged();

	UEnhancedInputLocalPlayerSubsystem* NewInput = GetInputSubsystem(GetController());
	if (!NewInput)
	{
		return; // Not controlled by a local player (e.g. just unpossessed): nothing to map.
	}
	if (!DefaultMappingContext)
	{
		UE_LOG(LogKubeCraft, Error, TEXT("%s: DefaultMappingContext is not assigned."), *GetName());
		return;
	}
	NewInput->AddMappingContext(DefaultMappingContext, /*Priority*/ 0);
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
	if (!MoveAction || !LookAction || !JumpAction || !MineAction || !PlaceAction)
	{
		UE_LOG(LogKubeCraft, Error, TEXT("%s: Move, Look, Jump, Mine and Place input actions must all be assigned."),
			*GetName());
		return;
	}

	EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AKubeCraftCharacter::Move);
	EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &AKubeCraftCharacter::Look);
	EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
	EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	EnhancedInput->BindAction(MineAction, ETriggerEvent::Started, MiningComponent.Get(), &UMiningComponent::StartMining);
	EnhancedInput->BindAction(MineAction, ETriggerEvent::Completed, MiningComponent.Get(), &UMiningComponent::StopMining);
	EnhancedInput->BindAction(PlaceAction, ETriggerEvent::Started, PlacingComponent.Get(), &UPlacingComponent::Place);
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
