#include "Player/PlacingComponent.h"

#include "GameFramework/Pawn.h"
#include "Player/LookTarget.h"
#include "World/VoxelWorld.h"

void UPlacingComponent::Place()
{
	const APawn* Pawn = GetOwner<APawn>();
	if (const TOptional<FLookTarget> Target = Pawn ? KubeCraft::Player::TraceLookTarget(*Pawn, Reach) : NullOpt)
	{
		Target->World->PlaceBlock(Target->PlaceCell);
	}
}
