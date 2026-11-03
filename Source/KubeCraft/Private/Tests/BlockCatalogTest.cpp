#include "Misc/AutomationTest.h"
#include "Voxel/BlockCatalog.h"
#include "Voxel/BlockDefinition.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Checks how the catalog turns ids into definitions and heights into block types,
 * which terrain generation (and later placing) relies on. Uses a catalog built in memory, not DA_BlockCatalog.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBlockCatalogHeightBandsTest, "KubeCraft.Voxel.HeightBands",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBlockCatalogHeightBandsTest::RunTest(const FString& Parameters)
{
	UBlockCatalog* Catalog = NewObject<UBlockCatalog>();
	UBlockDefinition* Stone = NewObject<UBlockDefinition>();
	UBlockDefinition* Grass = NewObject<UBlockDefinition>();
	UBlockDefinition* Snow = NewObject<UBlockDefinition>();

	// No bands yet: nothing to place.
	TestEqual(TEXT("No bands -> Air"), Catalog->GetIdForHeight(0), AirBlockId);

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
	TestEqual(TEXT("Z 0 -> Stone"), Catalog->GetIdForHeight(0), FBlockId(1));
	TestEqual(TEXT("Z 2 -> Stone"), Catalog->GetIdForHeight(2), FBlockId(1));
	TestEqual(TEXT("Z 3 -> Grass"), Catalog->GetIdForHeight(3), FBlockId(2));
	TestEqual(TEXT("Z 4 -> Grass"), Catalog->GetIdForHeight(4), FBlockId(2));
	TestEqual(TEXT("Z 5 -> Snow"), Catalog->GetIdForHeight(5), FBlockId(3));
	TestEqual(TEXT("Above the last band -> last band"), Catalog->GetIdForHeight(100), FBlockId(3));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
