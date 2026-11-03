#include "Misc/AutomationTest.h"
#include "Voxel/TerrainGenerator.h"
#include "Voxel/VoxelTypes.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Checks the promises of FTerrainGenerator: a seed always gives the same land, another seed gives other land,
 * the land is not flat, and the ground always stays inside the world.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTerrainGeneratorTest, "KubeCraft.Voxel.Terrain",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTerrainGeneratorTest::RunTest(const FString& Parameters)
{
	const FTerrainSettings Settings;
	const FTerrainGenerator Generator(42, Settings);
	const FTerrainGenerator SameSeed(42, Settings);
	const FTerrainGenerator OtherSeed(43, Settings);

	// Sample an area around the origin, including negative columns.
	constexpr int32 HalfArea = 64;
	bool bSameSeedMatches = true;
	bool bOtherSeedDiffers = false;
	bool bInsideWorld = true;
	int32 Lowest = MAX_int32;
	int32 Highest = MIN_int32;
	for (int32 Y = -HalfArea; Y < HalfArea; ++Y)
	{
		for (int32 X = -HalfArea; X < HalfArea; ++X)
		{
			const int32 Height = Generator.GetSurfaceHeight(X, Y);
			bSameSeedMatches &= Height == SameSeed.GetSurfaceHeight(X, Y);
			bOtherSeedDiffers |= Height != OtherSeed.GetSurfaceHeight(X, Y);
			bInsideWorld &= Height >= 1 && Height < KubeCraft::Voxel::ChunkHeight;
			Lowest = FMath::Min(Lowest, Height);
			Highest = FMath::Max(Highest, Height);
		}
	}

	TestTrue(TEXT("Same seed -> same terrain"), bSameSeedMatches);
	TestTrue(TEXT("Other seed -> other terrain"), bOtherSeedDiffers);
	TestTrue(TEXT("Ground between 1 and ChunkHeight - 1"), bInsideWorld);
	TestTrue(TEXT("Terrain has hills (not flat)"), Highest - Lowest > 1);

	// Extreme settings must still keep the ground inside the world.
	FTerrainSettings Extreme;
	Extreme.BaseHeight = 1000;
	TestEqual(TEXT("Too high -> clamped below the ceiling"), FTerrainGenerator(1, Extreme).GetSurfaceHeight(0, 0),
		KubeCraft::Voxel::ChunkHeight - 1);
	Extreme.BaseHeight = -1000;
	TestEqual(TEXT("Too low -> clamped to the floor"), FTerrainGenerator(1, Extreme).GetSurfaceHeight(0, 0), 1);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
