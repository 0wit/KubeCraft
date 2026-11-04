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

bool UPlacingComponent::Place(const UBlockDefinition* Type)
{
	const TOptional<FLookTarget> Target = LookTarget ? LookTarget->Trace() : NullOpt;
	return Target && Target->World->PlaceBlock(Target->PlaceCell, Type);
}
