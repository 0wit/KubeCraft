#include "Misc/AutomationTest.h"
#include "Blocks/BlockCatalog.h"
#include "Blocks/BlockDefinition.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Checks how the catalog turns ids into definitions and heights into block types,
 * which terrain generation (and later placing) relies on. Uses a catalog built in memory, not DA_BlockCatalog.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBlockCatalogHeightBandsTest, "KubeCraft.Blocks.HeightBands",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBlockCatalogHeightBandsTest::RunTest(const FString& Parameters)
{
	UBlockCatalog* Catalog = NewObject<UBlockCatalog>();
	UBlockDefinition* Stone = NewObject<UBlockDefinition>();
	UBlockDefinition* Grass = NewObject<UBlockDefinition>();
	UBlockDefinition* Snow = NewObject<UBlockDefinition>();

	// No bands yet: nothing to place.
	TestEqual(TEXT("No bands -> Air"), Catalog->GetIdForHeight(0, /*bIsTop*/ false), AirBlockId);

	auto MakeBand = [](int32 MaxZ, UBlockDefinition* Block)
	{
		FBlockHeightBand Band;
		Band.MaxZ = MaxZ;
		Band.Block = Block;
		return Band;
	};
	Catalog->Blocks = { Stone, Grass, Snow };
	Catalog->HeightBands = { MakeBand(3, Stone), MakeBand(5, Grass), MakeBand(64, Snow) };

	// Id = position in the list + 1; 0 is Air.
	TestEqual(TEXT("Stone id"), Catalog->GetId(Stone), FBlockId(1));
	TestEqual(TEXT("Snow id"), Catalog->GetId(Snow), FBlockId(3));
	TestEqual(TEXT("Unlisted block -> Air"), Catalog->GetId(NewObject<UBlockDefinition>()), AirBlockId);
	TestTrue(TEXT("Id 2 -> Grass"), Catalog->GetDefinition(2) == Grass);
	TestNull(TEXT("Air has no definition"), Catalog->GetDefinition(AirBlockId));
	TestNull(TEXT("Unknown id has no definition"), Catalog->GetDefinition(4));

	// MaxZ is exclusive: height 3 already belongs to the next band.
	TestEqual(TEXT("Z 0 -> Stone"), Catalog->GetIdForHeight(0, false), FBlockId(1));
	TestEqual(TEXT("Z 2 -> Stone"), Catalog->GetIdForHeight(2, false), FBlockId(1));
	TestEqual(TEXT("Z 3 -> Grass"), Catalog->GetIdForHeight(3, false), FBlockId(2));
	TestEqual(TEXT("Z 4 -> Grass"), Catalog->GetIdForHeight(4, false), FBlockId(2));
	TestEqual(TEXT("Z 5 -> Snow"), Catalog->GetIdForHeight(5, false), FBlockId(3));
	TestEqual(TEXT("Above the last band -> last band"), Catalog->GetIdForHeight(100, false), FBlockId(3));

	// A band without TopBlock uses its Block for the top cube too.
	TestEqual(TEXT("Top cube, no TopBlock -> Block"), Catalog->GetIdForHeight(4, /*bIsTop*/ true), FBlockId(2));

	// A band with TopBlock: dirt inside, grass only on the top cube.
	UBlockDefinition* Dirt = NewObject<UBlockDefinition>();
	Catalog->Blocks.Add(Dirt);
	Catalog->HeightBands[1].Block = Dirt;
	Catalog->HeightBands[1].TopBlock = Grass;
	TestEqual(TEXT("Inside the band -> Dirt"), Catalog->GetIdForHeight(4, false), FBlockId(4));
	TestEqual(TEXT("Top cube -> TopBlock (Grass)"), Catalog->GetIdForHeight(4, true), FBlockId(2));
	TestEqual(TEXT("TopBlock only in its own band"), Catalog->GetIdForHeight(2, true), FBlockId(1));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
