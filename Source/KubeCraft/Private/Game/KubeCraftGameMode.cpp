#include "Game/KubeCraftGameMode.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "KubeCraft.h"
#include "World/VoxelWorld.h"

void AKubeCraftGameMode::StartPlay()
{
	// At this point the players have already joined and got their pawns, but nothing has begun play yet.
	// So the world is built first and the pawns are moved onto it before they start falling.
	if (AVoxelWorld* VoxelWorld = FindVoxelWorld())
	{
		StartWorld(VoxelWorld);
	}

	Super::StartPlay();
}

void AKubeCraftGameMode::StartWorld_Implementation(AVoxelWorld* VoxelWorld)
{
	VoxelWorld->Generate();

	const FVector Ground = VoxelWorld->GetSurfaceLocation(FIntPoint::ZeroValue);
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* Player = It->Get();
		APawn* Pawn = Player ? Player->GetPawn() : nullptr;
		if (!Pawn)
		{
			continue;
		}
		// The pawn's location is its center, so lift it by half its height to put the feet on the ground.
		const FVector FeetOnGround = Ground + FVector(0.0, 0.0, Pawn->GetSimpleCollisionHalfHeight());
		Pawn->TeleportTo(FeetOnGround, Pawn->GetActorRotation());
	}
}

AVoxelWorld* AKubeCraftGameMode::FindVoxelWorld() const
{
	TActorIterator<AVoxelWorld> It(GetWorld());
	if (!It)
	{
		UE_LOG(LogKubeCraft, Error, TEXT("%s: no VoxelWorld in the level; there is no terrain."), *GetName());
		return nullptr;
	}
	return *It;
}
