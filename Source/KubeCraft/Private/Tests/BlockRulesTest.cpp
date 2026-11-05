#include "Misc/AutomationTest.h"
#include "Blocks/BlockRules.h"
#include "Tests/TestVoxelWorld.h"

#if WITH_DEV_AUTOMATION_TESTS

/*
 * Integration tests: unlike the other tests, these build a real (small, temporary) voxel world with chunks,
 * because the rules and edits ask the world about its blocks. See FTestVoxelWorld for the layout.
 * Not covered here (needs a player): "no placing where a pawn stands" - that one is play-tested.
 */

/** Checks every limit of FBlockRules: what may be mined, and where a block may be placed. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBlockRulesTest, "KubeCraft.Integration.BlockRules",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBlockRulesTest::RunTest(const FString& Parameters)
{
	const FTestVoxelWorld Test;
	if (!TestNotNull(TEXT("Test world"), Test.World))
	{
		return false;
	}
	const FBlockRules Rules(*Test.World);
	const int32 Top = FTestVoxelWorld::GroundHeight - 1;	// Highest solid block of every column.

	// Mining: only solid, breakable blocks.
	TestFalse(TEXT("Bedrock can't be mined"), Rules.CanMine(FIntVector(0, 0, 0)));
	TestTrue(TEXT("Stone can be mined"), Rules.CanMine(FIntVector(0, 0, Top)));
	TestFalse(TEXT("Air can't be mined"), Rules.CanMine(FIntVector(0, 0, Top + 1)));
	TestFalse(TEXT("Outside the world is Air"), Rules.CanMine(FIntVector(FTestVoxelWorld::LastBlock + 1, 0, Top)));

	// Placing: only into Air inside the world.
	TestTrue(TEXT("Air on the ground"), Rules.CanPlace(FIntVector(0, 0, Top + 1)));
	TestFalse(TEXT("Solid cell"), Rules.CanPlace(FIntVector(0, 0, Top)));
	TestTrue(TEXT("Highest cell"), Rules.CanPlace(FIntVector(0, 0, KubeCraft::Voxel::ChunkHeight - 1)));
	TestFalse(TEXT("Ceiling"), Rules.CanPlace(FIntVector(0, 0, KubeCraft::Voxel::ChunkHeight)));
	TestFalse(TEXT("Below the floor"), Rules.CanPlace(FIntVector(0, 0, -1)));
	constexpr int32 First = FTestVoxelWorld::FirstBlock;
	constexpr int32 Last = FTestVoxelWorld::LastBlock;
	TestTrue(TEXT("Last cell at -X edge"), Rules.CanPlace(FIntVector(First, 0, Top + 1)));
	TestFalse(TEXT("Past the -X edge"), Rules.CanPlace(FIntVector(First - 1, 0, Top + 1)));
	TestTrue(TEXT("Last cell at +X edge"), Rules.CanPlace(FIntVector(Last, 0, Top + 1)));
	TestFalse(TEXT("Past the +X edge"), Rules.CanPlace(FIntVector(Last + 1, 0, Top + 1)));
	TestFalse(TEXT("Past the +Y edge"), Rules.CanPlace(FIntVector(0, Last + 1, Top + 1)));

	return true;
}

/** Checks that MineBlock and PlaceBlock really change blocks, refuse what the rules forbid, and redraw borders. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMineAndPlaceTest, "KubeCraft.Integration.MineAndPlace",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMineAndPlaceTest::RunTest(const FString& Parameters)
{
	const FTestVoxelWorld Test;
	if (!TestNotNull(TEXT("Test world"), Test.World))
	{
		return false;
	}
	AVoxelWorld& World = *Test.World;
	const int32 Top = FTestVoxelWorld::GroundHeight - 1;
	const FIntVector Ground(3, 3, Top);

	// Mining turns a block into Air, once.
	TestTrue(TEXT("Mine stone"), World.MineBlock(Ground));
	TestEqual(TEXT("Mined block is Air"), World.GetBlock(Ground), AirBlockId);
	TestFalse(TEXT("Mine the same cell again"), World.MineBlock(Ground));
	TestFalse(TEXT("Mine bedrock"), World.MineBlock(FIntVector(3, 3, 0)));
	TestEqual(TEXT("Bedrock stays"), World.GetBlock(FIntVector(3, 3, 0)), Test.IdOf(Test.Bedrock));

	// Placing fills the cell with the given type, whatever the height.
	TestTrue(TEXT("Place snow back"), World.PlaceBlock(Ground, Test.Snow));
	TestEqual(TEXT("Placed block is snow"), World.GetBlock(Ground), Test.IdOf(Test.Snow));
	TestFalse(TEXT("Place into a solid cell"), World.PlaceBlock(Ground, Test.Stone));
	TestEqual(TEXT("Solid cell keeps its block"), World.GetBlock(Ground), Test.IdOf(Test.Snow));
	TestFalse(TEXT("Place outside the world"), World.PlaceBlock(FIntVector(FTestVoxelWorld::LastBlock + 1, 3, Top + 1), Test.Stone));

	// Placing needs a type the catalog knows; anything else would be an invisible solid block.
	const FIntVector Above(3, 3, Top + 1);
	TestFalse(TEXT("Place no type"), World.PlaceBlock(Above, nullptr));
	TestFalse(TEXT("Place a type not in the catalog"), World.PlaceBlock(Above, NewObject<UBlockDefinition>()));
	TestEqual(TEXT("Refused cell stays Air"), World.GetBlock(Above), AirBlockId);

	// Mining a buried block at X = 0 uncovers its neighbor at X = -1, which belongs to the next chunk.
	// That chunk must be redrawn too, so it draws one cube more than before.
	const FIntPoint NeighborChunk(-1, 0);
	const int32 DrawnBefore = Test.CountDrawnCubes(NeighborChunk);
	TestTrue(TEXT("Mine buried block on the border"), World.MineBlock(FIntVector(0, 3, Top - 1)));
	TestEqual(TEXT("Neighbor chunk shows the uncovered cube"), Test.CountDrawnCubes(NeighborChunk), DrawnBefore + 1);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
