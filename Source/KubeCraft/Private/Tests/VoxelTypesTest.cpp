#include "Misc/AutomationTest.h"
#include "World/VoxelTypes.h"

#if WITH_DEV_AUTOMATION_TESTS

using namespace KubeCraft::Voxel;

/**
 * Checks the coordinate conversions in VoxelTypes.h, which mining, placing and terrain all rely on.
 * Focus: rounding for negative coordinates, exact block edges, and the worked example in the header.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVoxelCoordinatesTest, "KubeCraft.World.Coordinates",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FVoxelCoordinatesTest::RunTest(const FString& Parameters)
{
	// Rounding down, not toward zero: the classic voxel bug.
	TestEqual(TEXT("FloorDiv(15, 16)"), FloorDiv(15, 16), 0);
	TestEqual(TEXT("FloorDiv(16, 16)"), FloorDiv(16, 16), 1);
	TestEqual(TEXT("FloorDiv(-1, 16)"), FloorDiv(-1, 16), -1);
	TestEqual(TEXT("FloorDiv(-16, 16)"), FloorDiv(-16, 16), -1);
	TestEqual(TEXT("FloorDiv(-17, 16)"), FloorDiv(-17, 16), -2);
	// Extremes must not overflow.
	TestEqual(TEXT("FloorDiv(MIN_int32, 16)"), FloorDiv(MIN_int32, 16), MIN_int32 / 16);
	TestEqual(TEXT("FloorDiv(MIN_int32 + 1, 16)"), FloorDiv(MIN_int32 + 1, 16), MIN_int32 / 16);
	TestEqual(TEXT("FloorDiv(MAX_int32, 16)"), FloorDiv(MAX_int32, 16), MAX_int32 / 16);

	// Block edges: [0, 100) cm is block 0, -0.1 cm is already block -1.
	TestEqual(TEXT("Just below 0"), WorldToBlock(FVector(-0.1, 0, 0)), FIntVector(-1, 0, 0));
	TestEqual(TEXT("Just below 100"), WorldToBlock(FVector(99.9, 0, 0)), FIntVector(0, 0, 0));
	TestEqual(TEXT("Exactly 100"), WorldToBlock(FVector(100, 0, 0)), FIntVector(1, 0, 0));

	// The worked example from the VoxelTypes.h comment, step by step.
	const FIntVector Block = WorldToBlock(FVector(1750, -30, 420));
	TestEqual(TEXT("Example block"), Block, FIntVector(17, -1, 4));
	TestEqual(TEXT("Example chunk"), BlockToChunk(Block), FIntPoint(1, -1));
	TestEqual(TEXT("Example local"), BlockToLocal(Block), FIntVector(1, 15, 4));
	TestEqual(TEXT("Example index"), LocalToIndex(BlockToLocal(Block)), 1265);

	// A block's corner is its minimum; negative blocks lie below zero.
	TestEqual(TEXT("Corner of (-1, 2, 0)"), BlockToWorldCorner(FIntVector(-1, 2, 0)), FVector(-100, 200, 0));

	// A block's center lies inside that same block (round trip, including negatives).
	for (const FIntVector& B : { FIntVector(0, 0, 0), FIntVector(-1, -17, 3), FIntVector(31, -16, 63) })
	{
		TestEqual(*FString::Printf(TEXT("Center round trip %s"), *B.ToString()), WorldToBlock(BlockToWorldCenter(B)), B);
	}

	// The top face is one full block above the corner and centered in X/Y, also for negative blocks.
	TestEqual(TEXT("Top center of (0, 0, 0)"), BlockToWorldTopCenter(FIntVector(0, 0, 0)), FVector(50, 50, 100));
	TestEqual(TEXT("Top center of (-1, 2, -1)"), BlockToWorldTopCenter(FIntVector(-1, 2, -1)), FVector(-50, 250, 0));

	// Aiming: a hit on the top face of block (2, 3, 4) mines that block and places on top of it.
	const FVector TopHit(250, 350, 500);
	const FVector Up(0, 0, 1);
	TestEqual(TEXT("Hit top face -> block"), HitToBlock(TopHit, Up), FIntVector(2, 3, 4));
	TestEqual(TEXT("Hit top face -> place cell"), HitToFaceNeighbor(TopHit, Up), FIntVector(2, 3, 5));
	// A hit on the -X face of block (-1, 0, 0) at x = -100: the place cell is further into negative X.
	const FVector SideHit(-100, 50, 50);
	const FVector MinusX(-1, 0, 0);
	TestEqual(TEXT("Hit -X face -> block"), HitToBlock(SideHit, MinusX), FIntVector(-1, 0, 0));
	TestEqual(TEXT("Hit -X face -> place cell"), HitToFaceNeighbor(SideHit, MinusX), FIntVector(-2, 0, 0));

	// Chunk bounds and the ends of the flat array.
	const FIntVector LastLocal(ChunkSize - 1, ChunkSize - 1, ChunkHeight - 1);
	TestTrue(TEXT("Last corner inside"), IsInsideChunk(LastLocal));
	TestFalse(TEXT("X = ChunkSize outside"), IsInsideChunk(FIntVector(ChunkSize, 0, 0)));
	TestFalse(TEXT("Z = -1 outside"), IsInsideChunk(FIntVector(0, 0, -1)));
	TestEqual(TEXT("First index"), LocalToIndex(FIntVector(0, 0, 0)), 0);
	TestEqual(TEXT("Last index"), LocalToIndex(LastLocal), BlocksPerChunk - 1);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
