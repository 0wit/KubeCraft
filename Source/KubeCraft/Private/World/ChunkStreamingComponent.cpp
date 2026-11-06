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
		FIntPoint(0, 0),
		FIntPoint(1, 0),
		FIntPoint(-1, 0),
		FIntPoint(0, 1),
		FIntPoint(0, -1),
	};

	/**
	 * Chunks past ViewDistance that stay loaded: one ring for the neighbors of the drawn chunks (their border cubes
	 * look into them), and one ring of slack, so walking back and forth over a border reloads nothing.
	 */
	constexpr int32 UnloadMarginInChunks = 2;

	/** Every chunk offset within Radius chunks of (0, 0) (a circle), nearest first. */
	TArray<FIntPoint> MakeOffsetsNearestFirst(int32 Radius)
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
		Offsets.Sort([](const FIntPoint& A, const FIntPoint& B)
					 { return A.SizeSquared() < B.SizeSquared(); });
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

	// Every load, draw and unload uses up one chunk of this call's budget.
	int32 ChunksLeft = MaxChunks;
	auto TakeOneChunk = [&ChunksLeft]()
	{
		if (ChunksLeft <= 0)
		{
			return false;
		}
		--ChunksLeft;
		return true;
	};

	// 1. Draw, nearest first, the ground under the player comes first. A chunk's border cubes look into its
	//    four neighbors, so the chunk and its neighbors are loaded right before it is drawn.
	for (const FIntPoint& Offset : GetOffsetsNearestFirst())
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
				if (!TakeOneChunk())
				{
					return false;
				}
				World->LoadChunk(Coord + Step);
			}
		}
		if (!TakeOneChunk())
		{
			return false;
		}
		World->DrawChunk(Coord);
	}

	// 2. Unload the far chunks after drawing, so new ground never waits for old chunks to go.
	const int32 UnloadDistance = ViewDistanceInChunks + UnloadMarginInChunks;
	for (const FIntPoint& Coord : World->GetLoadedChunks())
	{
		if ((Coord - CenterChunk).SizeSquared() > UnloadDistance * UnloadDistance)
		{
			if (!TakeOneChunk())
			{
				return false;
			}
			World->UnloadChunk(Coord);
		}
	}
	return true;
}

const TArray<FIntPoint>& UChunkStreamingComponent::GetOffsetsNearestFirst()
{
	if (OffsetsRadius != ViewDistanceInChunks)
	{
		OffsetsNearestFirst = MakeOffsetsNearestFirst(ViewDistanceInChunks);
		OffsetsRadius = ViewDistanceInChunks;
	}
	return OffsetsNearestFirst;
}
