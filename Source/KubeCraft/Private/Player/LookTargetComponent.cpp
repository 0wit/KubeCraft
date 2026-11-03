#include "Player/LookTargetComponent.h"

#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "World/VoxelChunk.h"

TOptional<FLookTarget> ULookTargetComponent::Trace() const
{
	const APawn* Viewer = GetOwner<APawn>();
	const AController* Controller = Viewer ? Viewer->GetController() : nullptr;
	const UWorld* World = GetWorld();
	if (!Controller || !World)
	{
		return {};
	}

	// For a player this is the camera, so the target is whatever sits in the middle of the screen.
	FVector ViewLocation;
	FRotator ViewRotation;
	Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);

	FHitResult Hit;
	const FVector TraceEnd = ViewLocation + ViewRotation.Vector() * Reach;
	const FCollisionQueryParams Params(SCENE_QUERY_STAT(LookTarget), /*bTraceComplex*/ false, Viewer);
	if (!World->LineTraceSingleByChannel(Hit, ViewLocation, TraceEnd, ECC_Visibility, Params))
	{
		return {};
	}
	// A trace that starts inside a collision (e.g. a camera clipping into a block) hits no face: its normal is
	// only the way out, not a face direction, so stepping along it could pick the wrong cell.
	if (Hit.bStartPenetrating)
	{
		return {};
	}

	// Only a chunk counts as a target; it also tells us which world the block belongs to.
	const AVoxelChunk* Chunk = Cast<AVoxelChunk>(Hit.GetActor());
	AVoxelWorld* VoxelWorld = Chunk ? Chunk->GetVoxelWorld() : nullptr;
	if (!VoxelWorld)
	{
		return {};
	}

	FLookTarget Target;
	Target.World = VoxelWorld;
	Target.Block = KubeCraft::Voxel::HitToBlock(Hit.ImpactPoint, Hit.ImpactNormal);
	Target.PlaceCell = KubeCraft::Voxel::HitToFaceNeighbor(Hit.ImpactPoint, Hit.ImpactNormal);
	return Target;
}
