#include "Misc/AutomationTest.h"
#include "Tests/TestVoxelWorld.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Chunks within Radius chunks of Center (a circle), worked out here without the streaming code. */
	TSet<FIntPoint> ChunksWithin(const FIntPoint& Center, int32 Radius)
	{
		TSet<FIntPoint> Chunks;
		for (int32 Y = -Radius; Y <= Radius; ++Y)
		{
			for (int32 X = -Radius; X <= Radius; ++X)
			{
				if (X * X + Y * Y <= Radius * Radius)
				{
					Chunks.Add(Center + FIntPoint(X, Y));
				}
			}
		}
		return Chunks;
	}

	/** The chunks plus the four chunks next to each of them. */
	TSet<FIntPoint> WithNeighbors(const TSet<FIntPoint>& Chunks)
	{
		TSet<FIntPoint> Result = Chunks;
		for (const FIntPoint& Chunk : Chunks)
		{
			Result.Append({ Chunk + FIntPoint(1, 0), Chunk + FIntPoint(-1, 0), Chunk + FIntPoint(0, 1), Chunk + FIntPoint(0, -1) });
		}
		return Result;
	}

	/** True if both sets hold the same chunks. */
	bool SameChunks(const TSet<FIntPoint>& A, const TSet<FIntPoint>& B)
	{
		return A.Num() == B.Num() && A.Includes(B);
	}

	TSet<FIntPoint> LoadedChunks(const AVoxelWorld& World)
	{
		return TSet<FIntPoint>(World.GetLoadedChunks());
	}

	TSet<FIntPoint> DrawnChunks(const AVoxelWorld& World)
	{
		TSet<FIntPoint> Drawn;
		for (const FIntPoint& Coord : World.GetLoadedChunks())
		{
			if (World.IsChunkDrawn(Coord))
			{
				Drawn.Add(Coord);
			}
		}
		return Drawn;
	}
}

/**
 * Checks UChunkStreamingComponent with AVoxelWorld: the right chunks are loaded and drawn around a center, borders
 * between chunks show no wall, the per-frame limit holds, far chunks unload (with one ring of slack), and player
 * changes come back when an unloaded chunk loads again.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChunkStreamingTest, "KubeCraft.Integration.ChunkStreaming",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FChunkStreamingTest::RunTest(const FString& Parameters)
{
	const FTestVoxelWorld Test(FTestVoxelWorld::EChunks::None);
	if (!TestNotNull(TEXT("Test world"), Test.World))
	{
		return false;
	}
	AVoxelWorld& World = *Test.World;
	UChunkStreamingComponent& Streaming = *Test.GetStreaming();
	constexpr int32 View = 2;
	Streaming.ViewDistanceInChunks = View;

	// The flat ground is ChunkSize x ChunkSize columns per chunk. With all neighbors loaded, a chunk draws only
	// its top layer; a missing neighbor would add a wall of cubes on that side.
	constexpr int32 TopLayer = KubeCraft::Voxel::ChunkSize * KubeCraft::Voxel::ChunkSize;
	auto CheckNoWalls = [&]()
	{
		for (const FIntPoint& Coord : DrawnChunks(World))
		{
			if (Test.CountDrawnCubes(Coord) != TopLayer)
			{
				AddError(FString::Printf(TEXT("Chunk %s draws %d cubes, expected only its top layer (%d)"),
					*Coord.ToString(), Test.CountDrawnCubes(Coord), TopLayer));
			}
		}
	};

	// 1. Start: everything around chunk (0, 0) at once.
	const FIntPoint Start(0, 0);
	Streaming.LoadAllAround(Start);
	TestTrue(TEXT("Start: loaded = drawn + their neighbors"),
		SameChunks(LoadedChunks(World), WithNeighbors(ChunksWithin(Start, View))));
	TestTrue(TEXT("Start: drawn up to ViewDistance"), SameChunks(DrawnChunks(World), ChunksWithin(Start, View)));
	CheckNoWalls();

	// 2. Player changes in chunk (0, 0): a hole in the ground and a snow block on top.
	const int32 Top = FTestVoxelWorld::GroundHeight - 1;
	const FIntVector Hole(3, 3, Top);
	const FIntVector Placed(5, 5, Top + 1);
	TestTrue(TEXT("Mine"), World.MineBlock(Hole));
	TestTrue(TEXT("Place"), World.PlaceBlock(Placed, Test.Snow));
	const int32 DrawnWithChanges = Test.CountDrawnCubes(Start);

	// 3. One chunk to the side: a neighbor of the old edge, now ViewDistance + 2 away, stays loaded (the slack ring).
	Streaming.UpdateAround(FIntPoint(1, 0), MAX_int32);
	TestTrue(TEXT("Slack ring stays loaded"), World.IsChunkLoaded(FIntPoint(-(View + 1), 0)));

	// 4. Far away, at most one chunk of work per call.
	const FIntPoint Far(10, 0);
	bool bFinished = false;
	int32 Calls = 0;
	constexpr int32 MaxCalls = 1000;	// Far more than the work needed; stops the loop if streaming never finishes.
	TOptional<FIntPoint> FirstDrawn;
	while (!bFinished && Calls < MaxCalls)
	{
		const TSet<FIntPoint> LoadedBefore = LoadedChunks(World);
		const TSet<FIntPoint> DrawnBefore = DrawnChunks(World);
		bFinished = Streaming.UpdateAround(Far, 1);
		++Calls;
		const TSet<FIntPoint> NewlyDrawn = DrawnChunks(World).Difference(DrawnBefore);
		if (!FirstDrawn && !NewlyDrawn.IsEmpty())
		{
			FirstDrawn = *NewlyDrawn.CreateConstIterator();
		}
		const int32 Work = LoadedChunks(World).Difference(LoadedBefore).Num() + NewlyDrawn.Num();
		if (Work > 1)
		{
			AddError(FString::Printf(TEXT("Call %d did %d chunks of work, the limit is 1"), Calls, Work));
		}
	}
	TestTrue(TEXT("Far: finished"), bFinished);
	TestTrue(TEXT("Far: needed more than one call"), Calls > 1);
	TestTrue(TEXT("Far: the player's chunk is drawn first, so there is ground under them"), FirstDrawn == Far);
	TestTrue(TEXT("Far: loaded = drawn + their neighbors"),
		SameChunks(LoadedChunks(World), WithNeighbors(ChunksWithin(Far, View))));
	TestTrue(TEXT("Far: drawn up to ViewDistance"), SameChunks(DrawnChunks(World), ChunksWithin(Far, View)));
	TestFalse(TEXT("Start chunk unloaded"), World.IsChunkLoaded(Start));
	CheckNoWalls();

	// 5. Back to the start: the changes come back, and the chunk draws the same cubes as before it was unloaded.
	Streaming.UpdateAround(Start, MAX_int32);
	TestEqual(TEXT("Hole is still Air"), World.GetBlock(Hole), AirBlockId);
	TestEqual(TEXT("Snow is still there"), World.GetBlock(Placed), Test.IdOf(Test.Snow));
	TestEqual(TEXT("Drawn as before unloading"), Test.CountDrawnCubes(Start), DrawnWithChanges);
	const FTestVoxelWorld::FDrawnCubes Drawn = Test.GetDrawnCubes();
	TestTrue(TEXT("Snow is drawn"), Drawn.MeshPerBlock.Contains(Placed));
	TestFalse(TEXT("Hole is not drawn"), Drawn.MeshPerBlock.Contains(Hole));
	TestTrue(TEXT("Floor of the hole is drawn"), Drawn.MeshPerBlock.Contains(Hole - FIntVector(0, 0, 1)));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
