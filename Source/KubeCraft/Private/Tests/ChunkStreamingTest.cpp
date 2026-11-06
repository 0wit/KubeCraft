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
 * Checks UChunkStreamingComponent with AVoxelWorld: Generate draws the start and switches the streaming on, the right
 * chunks are loaded and drawn around a center, borders between chunks show no wall, the per-frame limit holds for
 * loads, draws and unloads, far chunks unload (with one ring of slack), and player changes come back when an unloaded
 * chunk loads again - while a block set back to its terrain value is forgotten.
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

	// 1. Start: Generate draws everything around chunk (0, 0) at once, then lets the streaming follow the player.
	const FIntPoint Start(0, 0);
	World.Generate();
	TestTrue(TEXT("Start: streaming ticks"), Streaming.IsComponentTickEnabled());
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
	TestEqual(TEXT("Both changes remembered"), Test.CountRememberedChanges(), 2);

	// A block on the far side of the outermost drawn chunk: its neighbor block lies in a chunk that is loaded but not
	// drawn yet. Mining it must not draw a lone cube there - that chunk draws everything at once, later.
	const FIntPoint Undrawn(View + 1, 0);
	const FIntVector PutBack(KubeCraft::Voxel::ChunkSize * Undrawn.X - 1, 3, Top);
	TestTrue(TEXT("Mine next to a chunk that is not drawn"), World.MineBlock(PutBack));
	TestEqual(TEXT("Chunk that is not drawn stays empty"), Test.CountDrawnCubes(Undrawn), 0);

	// The block mined and put back as the same type is terrain again, so there is nothing to remember.
	TestTrue(TEXT("Put the stone back"), World.PlaceBlock(PutBack, Test.Stone));
	TestEqual(TEXT("Put-back block forgotten"), Test.CountRememberedChanges(), 2);

	// 3. One chunk to the side: a neighbor of the old edge, now ViewDistance + 2 away, stays loaded (the slack ring).
	Streaming.UpdateAround(FIntPoint(1, 0), MAX_int32);
	TestTrue(TEXT("Slack ring stays loaded"), World.IsChunkLoaded(FIntPoint(-(View + 1), 0)));

	// 4. Far away, at most one chunk of work (load, draw or unload) per call.
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
		const TSet<FIntPoint> LoadedAfter = LoadedChunks(World);
		const int32 Work = LoadedAfter.Difference(LoadedBefore).Num() + LoadedBefore.Difference(LoadedAfter).Num()
			+ NewlyDrawn.Num();
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

/**
 * Checks the player's streaming settings: UKubeCraftUserSettings keeps view distance and chunks per frame inside their
 * ranges, and applying new settings while playing makes the streaming draw the new circle around the same player.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStreamingSettingsTest, "KubeCraft.Integration.StreamingSettings",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FStreamingSettingsTest::RunTest(const FString& Parameters)
{
	// 1. Ranges. A settings object of its own, so the player's real settings stay untouched.
	UKubeCraftUserSettings* Settings = NewObject<UKubeCraftUserSettings>();
	Settings->SetViewDistance(1000);
	TestEqual(TEXT("View distance too far -> max"), Settings->GetViewDistance(), UKubeCraftUserSettings::MaxViewDistance);
	Settings->SetViewDistance(0);
	TestEqual(TEXT("View distance too near -> min"), Settings->GetViewDistance(), UKubeCraftUserSettings::MinViewDistance);
	Settings->SetChunksPerFrame(1000);
	TestEqual(TEXT("Chunks per frame too many -> max"), Settings->GetChunksPerFrame(),
		UKubeCraftUserSettings::MaxChunksPerFrame);
	Settings->SetChunksPerFrame(-3);
	TestEqual(TEXT("Chunks per frame too few -> min"), Settings->GetChunksPerFrame(),
		UKubeCraftUserSettings::MinChunksPerFrame);
	// Defaults: a real resolution and one quality level for the settings page, and the kept resolution survives.
	const FIntPoint Kept(1234, 567);
	Settings->SetScreenResolution(Kept);
	Settings->ConfirmVideoMode();
	Settings->SetToDefaults();
	TestEqual(TEXT("Default view distance"), Settings->GetViewDistance(), UKubeCraftUserSettings::DefaultViewDistance);
	TestEqual(TEXT("Default chunks per frame"), Settings->GetChunksPerFrame(),
		UKubeCraftUserSettings::DefaultChunksPerFrame);
	TestEqual(TEXT("Default quality is one overall level"), Settings->GetOverallScalabilityLevel(),
		UKubeCraftUserSettings::DefaultQualityLevel);
	if (Settings->GetDesktopResolution().X > 0)	// No desktop size without a screen (e.g. -nullrhi).
	{
		TestEqual(TEXT("Default resolution is the desktop's"), Settings->GetScreenResolution(),
			Settings->GetDesktopResolution());
	}
	Settings->RevertVideoMode();
	TestEqual(TEXT("Revert after Defaults -> the kept resolution"), Settings->GetScreenResolution(), Kept);
	Settings->SetToDefaults();

	// 2. A built world, finished around the start with the smallest view distance.
	const FTestVoxelWorld Test(FTestVoxelWorld::EChunks::None);
	if (!TestNotNull(TEXT("Test world"), Test.World))
	{
		return false;
	}
	AVoxelWorld& World = *Test.World;
	UChunkStreamingComponent& Streaming = *Test.GetStreaming();
	const FIntPoint Start = FIntPoint::ZeroValue;
	Settings->SetViewDistance(UKubeCraftUserSettings::MinViewDistance);
	Streaming.ApplyUserSettings(*Settings);
	World.Generate();
	TestTrue(TEXT("Start finished"), Streaming.FinishedCenter == Start);

	// 3. The player sees one chunk further: the work around the start is open again, and the bigger circle gets drawn.
	const int32 Further = UKubeCraftUserSettings::MinViewDistance + 1;
	Settings->SetViewDistance(Further);
	Settings->SetChunksPerFrame(UKubeCraftUserSettings::MaxChunksPerFrame);
	Streaming.ApplyUserSettings(*Settings);
	TestFalse(TEXT("New view distance -> streams again"), Streaming.FinishedCenter.IsSet());
	TestEqual(TEXT("Chunks per frame taken"), Streaming.MaxChunksPerFrame, UKubeCraftUserSettings::MaxChunksPerFrame);
	Streaming.UpdateAround(Start, MAX_int32);
	TestTrue(TEXT("Bigger circle drawn"), SameChunks(DrawnChunks(World), ChunksWithin(Start, Further)));

	// The same settings again change nothing: the finished work stays finished.
	Streaming.LoadAllAround(Start);
	Streaming.ApplyUserSettings(*Settings);
	TestTrue(TEXT("Same view distance -> stays finished"), Streaming.FinishedCenter == Start);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
