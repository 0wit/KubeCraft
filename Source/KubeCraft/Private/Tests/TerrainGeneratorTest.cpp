#include "Misc/AutomationTest.h"
#include "Terrain/TerrainGenerator.h"
#include "Tests/TestVoxelWorld.h"
#include "World/VoxelTypes.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Checks the promises of FTerrainGenerator: a seed always gives the same land, another seed gives other land,
 * the land is not flat, and the ground always stays inside the world.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTerrainGeneratorTest, "KubeCraft.Terrain.Generator",
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

/**
 * Checks that FTerrainGenerator::GetTerrainBlock (one block) gives the same answer as GenerateChunk (a whole chunk) on
 * hilly land, so the voxel world can trust it to tell whether a player change put a block back to its terrain value.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTerrainBlockTest, "KubeCraft.Terrain.TerrainBlock",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTerrainBlockTest::RunTest(const FString& Parameters)
{
	using namespace KubeCraft::Voxel;

	// Only for its catalog: bedrock at the bottom, stone above it.
	const FTestVoxelWorld Test(FTestVoxelWorld::EChunks::None);
	if (!TestNotNull(TEXT("Test world"), Test.World))
	{
		return false;
	}
	const UBlockCatalog& Catalog = *Test.World->GetCatalog();
	const FTerrainGenerator Generator(42, FTerrainSettings());

	// A chunk on each side of the origin, so negative coordinates are covered too.
	for (const FIntPoint& ChunkCoord : { FIntPoint(0, 0), FIntPoint(-1, -2) })
	{
		const TArray<FBlockId> Blocks = Generator.GenerateChunk(ChunkCoord, Catalog);
		const FIntVector Origin = ChunkToBlockOrigin(ChunkCoord);
		int32 Mismatches = 0;
		for (int32 Z = 0; Z < ChunkHeight; ++Z)
		{
			for (int32 Y = 0; Y < ChunkSize; ++Y)
			{
				for (int32 X = 0; X < ChunkSize; ++X)
				{
					const FIntVector Local(X, Y, Z);
					Mismatches += Blocks[LocalToIndex(Local)] != Generator.GetTerrainBlock(Origin + Local, Catalog);
				}
			}
		}
		TestEqual(FString::Printf(TEXT("Chunk %s: GetTerrainBlock = GenerateChunk"), *ChunkCoord.ToString()),
			Mismatches, 0);
	}
	TestEqual(TEXT("Below the floor is Air"), Generator.GetTerrainBlock(FIntVector(0, 0, -1), Catalog), AirBlockId);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
