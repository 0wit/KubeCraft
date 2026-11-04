#include "Misc/AutomationTest.h"
#include "Tests/TestVoxelWorld.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Checks that redrawing only the changed blocks gives exactly the picture a full rebuild of every chunk gives:
 * same cubes, in the same places, drawn by the same block type's mesh, none twice.
 * The edits cross chunk borders, remove cubes from the middle of a mesh's list and add a new block type.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIncrementalRedrawTest, "KubeCraft.Integration.IncrementalRedraw",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIncrementalRedrawTest::RunTest(const FString& Parameters)
{
	FTestVoxelWorld Test;	// Not const: the test rebuilds all chunks to compare.
	if (!TestNotNull(TEXT("Test world"), Test.World))
	{
		return false;
	}
	AVoxelWorld& World = *Test.World;
	const int32 Top = FTestVoxelWorld::GroundHeight - 1;

	// 1. A 2 x 2 x 2 pit on the corner where all four chunks meet: every redraw crosses a chunk border.
	for (int32 Z = Top; Z >= Top - 1; --Z)
	{
		for (int32 Y = -1; Y <= 0; ++Y)
		{
			for (int32 X = -1; X <= 0; ++X)
			{
				TestTrue(TEXT("Mine pit"), World.MineBlock(FIntVector(X, Y, Z)));
			}
		}
	}
	// 2. Fill part of the pit back in.
	TestTrue(TEXT("Refill pit"), World.PlaceBlock(FIntVector(0, 0, Top - 1)));
	TestTrue(TEXT("Refill pit"), World.PlaceBlock(FIntVector(-1, 0, Top - 1)));

	// 3. A trench inside one chunk: each removal takes a cube out of the middle of the mesh's list.
	for (int32 X = 3; X <= 7; ++X)
	{
		TestTrue(TEXT("Mine trench"), World.MineBlock(FIntVector(X, 5, Top)));
	}
	TestTrue(TEXT("Dig deeper"), World.MineBlock(FIntVector(5, 5, Top - 1)));
	TestTrue(TEXT("Refill trench"), World.PlaceBlock(FIntVector(4, 5, Top)));

	// 4. Floating snow: a block type the chunk did not draw before, then mined again.
	const int32 High = FTestVoxelWorld::SnowFromZ;
	TestTrue(TEXT("Place snow"), World.PlaceBlock(FIntVector(5, 5, High)));
	TestTrue(TEXT("Place snow"), World.PlaceBlock(FIntVector(6, 5, High)));
	TestTrue(TEXT("Mine snow"), World.MineBlock(FIntVector(5, 5, High)));

	const FTestVoxelWorld::FDrawnCubes Incremental = Test.GetDrawnCubes();
	Test.RebuildAllChunks();
	const FTestVoxelWorld::FDrawnCubes Full = Test.GetDrawnCubes();

	TestEqual(TEXT("No block drawn twice"), Incremental.MeshPerBlock.Num(), Incremental.CubeCount);
	TestEqual(TEXT("Same number of cubes as a full rebuild"), Incremental.CubeCount, Full.CubeCount);
	for (const TPair<FIntVector, const UInstancedStaticMeshComponent*>& Pair : Full.MeshPerBlock)
	{
		if (Incremental.MeshPerBlock.FindRef(Pair.Key) != Pair.Value)
		{
			AddError(FString::Printf(TEXT("Block %s is not drawn like after a full rebuild"), *Pair.Key.ToString()));
		}
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
