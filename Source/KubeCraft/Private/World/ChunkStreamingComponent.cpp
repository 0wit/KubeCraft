#include "World/ChunkStreamingComponent.h"

#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "World/VoxelTypes.h"
#include "World/VoxelWorld.h"

using namespace KubeCraft::Voxel;

namespace
{
	/** Steps from a chunk to itself and to the four chunks that share a border with it. */
	const FIntPoint ChunkAndNeighbors[] = {
		FIntPoint(0, 0), FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1),
	};

	/** Every chunk offset within Radius chunks of (0, 0) (a circle), nearest first. */
	TArray<FIntPoint> GetOffsetsNearestFirst(int32 Radius)
	{
		TArray<FIntPoint> Offsets;
		for (int32 Y = -Radius; Y <= Radius; ++Y)
		{
			for (int32 X = -Radius; X <= Radius; ++X)
			{
				const FIntPoint Offset(X, Y);
				if (Offset.SizeSquared() <= Radius * Radius)
				{
					Offsets.Add(Offset);
				}
			}
		}
		Offsets.Sort([](const FIntPoint& A, const FIntPoint& B) { return A.SizeSquared() < B.SizeSquared(); });
		return Offsets;
	}
}

UChunkStreamingComponent::UChunkStreamingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UChunkStreamingComponent::LoadAllAround(const FIntPoint& CenterChunk)
{
	if (UpdateAround(CenterChunk, MAX_int32))
	{
		FinishedCenter = CenterChunk;
	}
}

void UChunkStreamingComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player)
	{
		return;
	}
	const FIntPoint Center = BlockToChunk(WorldToBlock(Player->GetActorLocation()));
	if (FinishedCenter == Center)
	{
		return;
	}
	if (UpdateAround(Center, MaxChunksPerFrame))
	{
		FinishedCenter = Center;
	}
	else
	{
		FinishedCenter.Reset();
	}
}

bool UChunkStreamingComponent::UpdateAround(const FIntPoint& CenterChunk, int32 MaxChunks)
{
	AVoxelWorld* World = GetOwner<AVoxelWorld>();
	if (!ensureMsgf(World, TEXT("%s: must be on an AVoxelWorld."), *GetName()))
	{
		return true;
	}
	// 1. Unload the far chunks. Destroying an actor is cheap, so all of them at once.
	const int32 UnloadDistance = ViewDistanceInChunks + 2;
	for (const FIntPoint& Coord : World->GetLoadedChunks())
	{
		if ((Coord - CenterChunk).SizeSquared() > UnloadDistance * UnloadDistance)
		{
			World->UnloadChunk(Coord);
		}
	}

	// 2. Draw, nearest first, so the ground under the player comes first. A chunk's border cubes look into its
	//    four neighbors, so the chunk and its neighbors are loaded right before it is drawn.
	int32 Done = 0;
	for (const FIntPoint& Offset : GetOffsetsNearestFirst(ViewDistanceInChunks))
	{
		const FIntPoint Coord = CenterChunk + Offset;
		if (World->IsChunkDrawn(Coord))
		{
			continue;
		}
		for (const FIntPoint& Step : ChunkAndNeighbors)
		{
			if (!World->IsChunkLoaded(Coord + Step))
			{
				if (Done == MaxChunks)
				{
					return false;
				}
				World->LoadChunk(Coord + Step);
				++Done;
			}
		}
		if (Done == MaxChunks)
		{
			return false;
		}
		World->DrawChunk(Coord);
		++Done;
	}
	return true;
}
