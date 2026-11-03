#include "Player/LookTarget.h"

#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "World/VoxelChunk.h"
#include "World/VoxelTypes.h"

TOptional<FLookTarget> KubeCraft::Player::TraceLookTarget(const APawn& Viewer, float Reach)
{
	const AController* Controller = Viewer.GetController();
	const UWorld* World = Viewer.GetWorld();
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
	const FCollisionQueryParams Params(SCENE_QUERY_STAT(TraceLookTarget), /*bTraceComplex*/ false, &Viewer);
	if (!World->LineTraceSingleByChannel(Hit, ViewLocation, TraceEnd, ECC_Visibility, Params))
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
