#include "Player/MiningComponent.h"

#include "Blocks/BlockDefinition.h"
#include "Blocks/BlockRules.h"
#include "KubeCraft.h"
#include "Player/LookTargetComponent.h"
#include "World/VoxelTypes.h"
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
	const UBlockDefinition* Type = Target->World->GetBlockDefinition(Target->Block);
	MinedSeconds += DeltaTime;
	if (MinedSeconds >= Type->MineTime)
	{
		if (Target->World->MineBlock(Target->Block))
		{
			OnBlockMined.Broadcast(Type);
		}
		// Still holding: carry on with whatever block is behind it on the next tick.
		ResetProgress();
	}
}

bool UMiningComponent::GetMiningProgress(FVector& BlockCenter, float& Progress) const
{
	const AVoxelWorld* World = MinedWorld.Get();
	// The definition can be missing if the block changed since the last tick (e.g. mined by someone else).
	const UBlockDefinition* Definition = World ? World->GetBlockDefinition(MinedBlock) : nullptr;
	if (!Definition)
	{
		return false;
	}
	BlockCenter = KubeCraft::Voxel::BlockToWorldCenter(MinedBlock);
	Progress = FMath::Clamp(MinedSeconds / Definition->MineTime, 0.f, 1.f);
	return true;
}

void UMiningComponent::ResetProgress()
{
	MinedWorld.Reset();
	MinedSeconds = 0.f;
}
