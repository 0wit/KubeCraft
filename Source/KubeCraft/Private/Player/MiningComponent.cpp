#include "Player/MiningComponent.h"

#include "Blocks/BlockDefinition.h"
#include "Blocks/BlockRules.h"
#include "KubeCraft.h"
#include "Player/LookTargetComponent.h"
#include "World/VoxelWorld.h"

UMiningComponent::UMiningComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UMiningComponent::BeginPlay()
{
	Super::BeginPlay();

	LookTarget = GetOwner()->FindComponentByClass<ULookTargetComponent>();
	if (!LookTarget)
	{
		UE_LOG(LogKubeCraft, Error, TEXT("%s: the owner has no LookTargetComponent; mining is off."), *GetName());
	}
}

void UMiningComponent::StartMining()
{
	ResetProgress();
	SetComponentTickEnabled(true);
}

void UMiningComponent::StopMining()
{
	SetComponentTickEnabled(false);
	ResetProgress();
}

void UMiningComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const TOptional<FLookTarget> Target = LookTarget ? LookTarget->Trace() : NullOpt;
	if (!Target || !FBlockRules(*Target->World).CanMine(Target->Block))
	{
		ResetProgress();
		return;
	}

	// Looking at a different block starts over.
	if (MinedWorld.Get() != Target->World || MinedBlock != Target->Block)
	{
		MinedWorld = Target->World;
		MinedBlock = Target->Block;
		MinedSeconds = 0.f;
	}

	// CanMine passed, so the block is not Air and has a definition. (The check above only decides whether
	// the timer runs; MineBlock asks the rules again itself.)
	MinedSeconds += DeltaTime;
	if (MinedSeconds >= Target->World->GetBlockDefinition(Target->Block)->MineTime)
	{
		Target->World->MineBlock(Target->Block);
		// Still holding: carry on with whatever block is behind it on the next tick.
		ResetProgress();
	}
}

void UMiningComponent::ResetProgress()
{
	MinedWorld.Reset();
	MinedSeconds = 0.f;
}
