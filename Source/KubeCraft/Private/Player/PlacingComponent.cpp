#include "Player/PlacingComponent.h"

#include "KubeCraft.h"
#include "Player/LookTargetComponent.h"
#include "World/VoxelWorld.h"

void UPlacingComponent::BeginPlay()
{
	Super::BeginPlay();

	LookTarget = GetOwner()->FindComponentByClass<ULookTargetComponent>();
	if (!LookTarget)
	{
		UE_LOG(LogKubeCraft, Error, TEXT("%s: the owner has no LookTargetComponent; placing is off."), *GetName());
	}
}

void UPlacingComponent::Place()
{
	if (const TOptional<FLookTarget> Target = LookTarget ? LookTarget->Trace() : NullOpt)
	{
		Target->World->PlaceBlock(Target->PlaceCell);
	}
}
