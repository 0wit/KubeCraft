#include "Misc/AutomationTest.h"
#include "Tests/TestVoxelWorld.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** A save's changes by block, so two saves can be compared whatever their order. */
	TMap<FIntVector, const UBlockDefinition*> ChangesByBlock(const FVoxelWorldSave& Save)
	{
		TMap<FIntVector, const UBlockDefinition*> Changes;
		for (const FBlockChange& Change : Save.Changes)
		{
			Changes.Add(Change.Block, Change.Type);
		}
		return Changes;
	}
}

/**
 * Checks AVoxelWorld::MakeSave and GenerateFromSave: a save holds the seed and every change, also of unloaded chunks.
 * A new world built from it has that seed and those blocks, is drawn around the player's location instead of the
 * start, and leaves out a change whose block type is not in its catalog.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveLoadTest, "KubeCraft.Integration.SaveLoad",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSaveLoadTest::RunTest(const FString& Parameters)
{
	using namespace KubeCraft::Voxel;

	const FTestVoxelWorld Test(FTestVoxelWorld::EChunks::None);
	if (!TestNotNull(TEXT("Test world"), Test.World))
	{
		return false;
	}
	constexpr int32 View = 2;	// Small, so both worlds build fast.
	AVoxelWorld& Original = *Test.World;
	FTestVoxelWorld::SetViewDistance(Original, View);
	Original.Generate();

	// 1. Three changes: a hole and a snow block in chunk (0, 0), and a hole in chunk (-1, 0).
	const int32 Top = FTestVoxelWorld::GroundHeight - 1;
	const FIntVector Hole(3, 3, Top);
	const FIntVector Placed(5, 5, Top + 1);
	const FIntVector OtherHole(-1, 2, Top);
	TestTrue(TEXT("Mine"), Original.MineBlock(Hole));
	TestTrue(TEXT("Place"), Original.PlaceBlock(Placed, Test.Snow));
	TestTrue(TEXT("Mine in the next chunk"), Original.MineBlock(OtherHole));

	// The player walks away: both chunks unload, but their changes must still be saved.
	FTestVoxelWorld::StreamAround(Original, FIntPoint(20, 0));
	TestFalse(TEXT("Changed chunk unloaded"), Original.IsChunkLoaded(FIntPoint::ZeroValue));

	// 2. Save.
	FVoxelWorldSave Save = Original.MakeSave();
	const TMap<FIntVector, const UBlockDefinition*> Expected = {
		{ Hole, nullptr }, { Placed, Test.Snow }, { OtherHole, nullptr } };
	TestTrue(TEXT("Save holds every change, Air as null"), ChangesByBlock(Save).OrderIndependentCompareEqual(Expected));

	// The test worlds always use seed 0; another seed in the save shows that the loaded world takes the saved one.
	constexpr int32 SavedSeed = 1234;
	Save.Seed = SavedSeed;

	// A block type that is not in the catalog (deleted since saving): left out with a warning, the terrain stays.
	const FIntVector Unknown(1, 1, Top);
	FBlockChange& UnknownChange = Save.Changes.AddDefaulted_GetRef();
	UnknownChange.Block = Unknown;
	UnknownChange.Type = NewObject<UBlockDefinition>();
	AddExpectedMessage(TEXT("not in the catalog"), ELogVerbosity::Warning);

	// 3. Load into a new world, with the player standing in chunk (1, 0).
	AVoxelWorld& Loaded = *Test.SpawnVoxelWorld();
	FTestVoxelWorld::SetViewDistance(Loaded, View);
	const FIntPoint PlayerChunk(1, 0);
	Loaded.GenerateFromSave(Save, BlockToWorldCenter(ChunkToBlockOrigin(PlayerChunk) + FIntVector(2, 2, Top + 1)));

	TestEqual(TEXT("Seed from the save"), Loaded.MakeSave().Seed, SavedSeed);
	TestTrue(TEXT("Same changes as the original"),
		ChangesByBlock(Loaded.MakeSave()).OrderIndependentCompareEqual(Expected));
	TestEqual(TEXT("Hole is Air"), Loaded.GetBlock(Hole), AirBlockId);
	TestTrue(TEXT("Snow is back"), Loaded.GetBlockDefinition(Placed) == Test.Snow);
	TestEqual(TEXT("Hole in the next chunk is Air"), Loaded.GetBlock(OtherHole), AirBlockId);
	TestTrue(TEXT("Unknown type left out, the terrain stays"), Loaded.GetBlockDefinition(Unknown) == Test.Stone);

	// Chunk (1 + View, 0) is ViewDistance from the player, but one more from the start: drawn only around the player.
	TestTrue(TEXT("Drawn around the player, not the start"), Loaded.IsChunkDrawn(PlayerChunk + FIntPoint(View, 0)));

	return true;
}

/**
 * Checks AVoxelWorld::SetSeed, used for a new world with a typed-in seed: it wins over the random seed, and once the
 * world is built it changes nothing (the terrain is already made from the first seed).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCustomSeedTest, "KubeCraft.Integration.CustomSeed",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCustomSeedTest::RunTest(const FString& Parameters)
{
	const FTestVoxelWorld Test(FTestVoxelWorld::EChunks::None);
	if (!TestNotNull(TEXT("Test world"), Test.World))
	{
		return false;
	}
	AVoxelWorld& World = *Test.World;
	FTestVoxelWorld::SetViewDistance(World, 2);	// Small, so the world builds fast.

	// Like in the game: a new random seed on every start, unless one was typed in.
	FTestVoxelWorld::SetRandomSeed(World, true);
	constexpr int32 TypedSeed = 1234;
	World.SetSeed(TypedSeed);
	World.Generate();
	TestEqual(TEXT("Generate uses the typed-in seed"), World.MakeSave().Seed, TypedSeed);

	AddExpectedMessage(TEXT("SetSeed after the world was built is ignored"), ELogVerbosity::Warning);
	World.SetSeed(TypedSeed + 1);
	TestEqual(TEXT("A seed after building changes nothing"), World.MakeSave().Seed, TypedSeed);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
