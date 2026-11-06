#include "Misc/AutomationTest.h"
#include "Blocks/BlockCatalog.h"
#include "Blocks/BlockDefinition.h"
#include "Terrain/TerrainGenerator.h"
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
 * Also checks that only the top cube of each column gets the band's TopBlock (like grass on top of dirt).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTerrainBlockTest, "KubeCraft.Terrain.TerrainBlock",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTerrainBlockTest::RunTest(const FString& Parameters)
{
	using namespace KubeCraft::Voxel;

	// Bedrock at the bottom, stone above it, and snow as the top cube of the stone band,
	// so the top cube of each column must differ from the cubes below it.
	UBlockDefinition* Bedrock = NewObject<UBlockDefinition>();
	UBlockDefinition* Stone = NewObject<UBlockDefinition>();
	UBlockDefinition* Snow = NewObject<UBlockDefinition>();
	UBlockCatalog* TestCatalog = NewObject<UBlockCatalog>();
	TestCatalog->Blocks = { Bedrock, Stone, Snow };
	TestCatalog->HeightBands.SetNum(2);
	TestCatalog->HeightBands[0].MaxZ = 1;
	TestCatalog->HeightBands[0].Block = Bedrock;
	TestCatalog->HeightBands[1].MaxZ = ChunkHeight;
	TestCatalog->HeightBands[1].Block = Stone;
	TestCatalog->HeightBands[1].TopBlock = Snow;
	const UBlockCatalog& Catalog = *TestCatalog;
	const FBlockId StoneId = Catalog.GetId(Stone);
	const FBlockId SnowId = Catalog.GetId(Snow);
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

		// Only the top cube of each column gets the TopBlock; the cube under it keeps the band's Block.
		int32 WrongTops = 0;
		for (int32 Y = 0; Y < ChunkSize; ++Y)
		{
			for (int32 X = 0; X < ChunkSize; ++X)
			{
				const int32 Height = Generator.GetSurfaceHeight(Origin.X + X, Origin.Y + Y);
				// Height 1 means the top cube is at Z 0, which is in the bedrock band (no TopBlock).
				WrongTops += Height >= 2 && Blocks[LocalToIndex(FIntVector(X, Y, Height - 1))] != SnowId;
				WrongTops += Height >= 3 && Blocks[LocalToIndex(FIntVector(X, Y, Height - 2))] != StoneId;
			}
		}
		TestEqual(FString::Printf(TEXT("Chunk %s: top cube = TopBlock, below = Block"), *ChunkCoord.ToString()),
			WrongTops, 0);
	}
	TestEqual(TEXT("Below the floor is Air"), Generator.GetTerrainBlock(FIntVector(0, 0, -1), Catalog), AirBlockId);
	return true;
}

/**
 * Checks the promises of FTreeGenerator on hilly land with a tree in every cell that has grass:
 * trunks are one block wide, as tall as the settings allow and stand on grass; trees only fill Air;
 * GetTerrainBlock agrees with GenerateChunk across chunk borders; the same seed gives the same trees;
 * and no trees grow when the ground has none of the block they grow on.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTreeGeneratorTest, "KubeCraft.Terrain.Trees",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTreeGeneratorTest::RunTest(const FString& Parameters)
{
	using namespace KubeCraft::Voxel;

	// Bare stone low down, dirt with grass on top higher up, so only some columns can have a tree.
	UBlockDefinition* Bedrock = NewObject<UBlockDefinition>();
	UBlockDefinition* Stone = NewObject<UBlockDefinition>();
	UBlockDefinition* Dirt = NewObject<UBlockDefinition>();
	UBlockDefinition* Grass = NewObject<UBlockDefinition>();
	UBlockDefinition* Wood = NewObject<UBlockDefinition>();
	UBlockDefinition* Leaves = NewObject<UBlockDefinition>();
	UBlockCatalog* TestCatalog = NewObject<UBlockCatalog>();
	TestCatalog->Blocks = { Bedrock, Stone, Dirt, Grass, Wood, Leaves };
	TestCatalog->HeightBands.SetNum(3);
	TestCatalog->HeightBands[0].MaxZ = 1;
	TestCatalog->HeightBands[0].Block = Bedrock;
	TestCatalog->HeightBands[1].MaxZ = 20;
	TestCatalog->HeightBands[1].Block = Stone;
	TestCatalog->HeightBands[2].MaxZ = ChunkHeight;
	TestCatalog->HeightBands[2].Block = Dirt;
	TestCatalog->HeightBands[2].TopBlock = Grass;
	const UBlockCatalog& Catalog = *TestCatalog;
	const FBlockId GrassId = Catalog.GetId(Grass);
	const FBlockId WoodId = Catalog.GetId(Wood);

	FTerrainSettings Settings;
	Settings.Trees.Trunk = Wood;
	Settings.Trees.Leaves = Leaves;
	Settings.Trees.GrowsOn = Grass;
	Settings.Trees.Chance = 1.f;
	const FTerrainGenerator Generator(42, Settings);
	FTerrainSettings NoTreeSettings = Settings;
	NoTreeSettings.Trees.Trunk = nullptr;
	const FTerrainGenerator NoTrees(42, NoTreeSettings);

	// 4 x 4 chunks around the origin, with negative coordinates and many chunk borders that trees cross.
	constexpr int32 FirstChunk = -2;
	constexpr int32 LastChunk = 1;
	TMap<FIntPoint, TArray<FBlockId>> Chunks;
	int32 Mismatches = 0;
	int32 ChangedGround = 0;
	for (int32 ChunkY = FirstChunk; ChunkY <= LastChunk; ++ChunkY)
	{
		for (int32 ChunkX = FirstChunk; ChunkX <= LastChunk; ++ChunkX)
		{
			const FIntPoint ChunkCoord(ChunkX, ChunkY);
			const TArray<FBlockId>& Blocks = Chunks.Add(ChunkCoord, Generator.GenerateChunk(ChunkCoord, Catalog));
			const TArray<FBlockId> Ground = NoTrees.GenerateChunk(ChunkCoord, Catalog);
			const FIntVector Origin = ChunkToBlockOrigin(ChunkCoord);
			for (int32 Index = 0; Index < BlocksPerChunk; ++Index)
			{
				const FIntVector Local(Index % ChunkSize, (Index / ChunkSize) % ChunkSize, Index / (ChunkSize * ChunkSize));
				Mismatches += Blocks[Index] != Generator.GetTerrainBlock(Origin + Local, Catalog);
				ChangedGround += Ground[Index] != AirBlockId && Ground[Index] != Blocks[Index];
			}
		}
	}
	TestEqual(TEXT("GetTerrainBlock = GenerateChunk, trees included"), Mismatches, 0);
	TestEqual(TEXT("Trees only fill Air"), ChangedGround, 0);

	auto GetBlock = [&Chunks](const FIntVector& Block)
	{
		const TArray<FBlockId>* Blocks = Chunks.Find(BlockToChunk(Block));
		return (Blocks && Block.Z >= 0 && Block.Z < ChunkHeight) ? (*Blocks)[LocalToIndex(BlockToLocal(Block))] : AirBlockId;
	};

	// Find every trunk from its lowest wood cube, then check it from bottom to top.
	const int32 FirstBlock = FirstChunk * ChunkSize;
	const int32 LastBlock = (LastChunk + 1) * ChunkSize - 1;
	int32 TreeCount = 0;
	int32 WideTrunks = 0;
	int32 WrongHeights = 0;
	int32 NotOnGrass = 0;
	for (int32 Y = FirstBlock; Y <= LastBlock; ++Y)
	{
		for (int32 X = FirstBlock; X <= LastBlock; ++X)
		{
			for (int32 Z = 1; Z < ChunkHeight; ++Z)
			{
				const FIntVector Block(X, Y, Z);
				if (GetBlock(Block) != WoodId)
				{
					continue;
				}
				for (int32 Side = 0; Side < 4; ++Side)	// The first four face neighbors are the sideways ones.
				{
					WideTrunks += GetBlock(Block + FaceNeighborOffsets[Side]) == WoodId;
				}
				if (GetBlock(Block - FIntVector(0, 0, 1)) == WoodId)
				{
					continue;	// Not the lowest cube of its trunk.
				}
				++TreeCount;
				NotOnGrass += GetBlock(Block - FIntVector(0, 0, 1)) != GrassId;
				int32 Height = 0;
				while (GetBlock(Block + FIntVector(0, 0, Height)) == WoodId)
				{
					++Height;
				}
				WrongHeights += Height < Settings.Trees.MinTrunkHeight || Height > Settings.Trees.MaxTrunkHeight;
			}
		}
	}
	TestTrue(TEXT("Some trees grew"), TreeCount > 0);
	TestEqual(TEXT("Trunks are one block wide"), WideTrunks, 0);
	TestEqual(TEXT("Trunk heights between Min and Max"), WrongHeights, 0);
	TestEqual(TEXT("Every trunk stands on grass"), NotOnGrass, 0);

	TestTrue(TEXT("Same seed -> same trees"),
		FTerrainGenerator(42, Settings).GenerateChunk(FIntPoint(0, 0), Catalog) == Chunks[FIntPoint(0, 0)]);

	// Trees that grow on a block the ground never has: none at all.
	FTerrainSettings NowhereSettings = Settings;
	NowhereSettings.Trees.GrowsOn = Leaves;
	const FTerrainGenerator Nowhere(42, NowhereSettings);
	int32 WoodWithoutGround = 0;
	for (const TPair<FIntPoint, TArray<FBlockId>>& Pair : Chunks)
	{
		for (const FBlockId Id : Nowhere.GenerateChunk(Pair.Key, Catalog))
		{
			WoodWithoutGround += Id == WoodId;
		}
	}
	TestEqual(TEXT("No grass -> no trees"), WoodWithoutGround, 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
