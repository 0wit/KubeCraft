#include "Misc/AutomationTest.h"
#include "Tests/TestVoxelWorld.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/**
	 * Blocks that should be drawn, worked out here from the block data alone, without the chunks' drawing code:
	 * every solid block with at least one face open to Air. Below the floor counts as solid (nobody can look
	 * from there); past the edge of the world and above its ceiling count as Air.
	 * A full rebuild uses the same drawing code as a redraw, so comparing only those two would miss a bug in it.
	 */
	TSet<FIntVector> FindBlocksThatShouldBeDrawn(const AVoxelWorld& World)
	{
		using namespace KubeCraft::Voxel;

		TSet<FIntVector> Expected;
		for (int32 Z = 0; Z < ChunkHeight; ++Z)
		{
			for (int32 Y = FTestVoxelWorld::FirstBlock; Y <= FTestVoxelWorld::LastBlock; ++Y)
			{
				for (int32 X = FTestVoxelWorld::FirstBlock; X <= FTestVoxelWorld::LastBlock; ++X)
				{
					const FIntVector Block(X, Y, Z);
					if (World.GetBlock(Block) == AirBlockId)
					{
						continue;
					}
					for (const FIntVector& Offset : FaceNeighborOffsets)
					{
						const FIntVector Neighbor = Block + Offset;
						if (Neighbor.Z >= 0 && World.GetBlock(Neighbor) == AirBlockId)
						{
							Expected.Add(Block);
							break;
						}
					}
				}
			}
		}
		return Expected;
	}
}

/**
 * Checks that redrawing only the changed blocks draws exactly the blocks that touch Air, each by the same block
 * type's mesh a full rebuild of every chunk uses, none twice.
 * The edits cross chunk borders, remove cubes from the middle of a mesh's list, add a new block type and
 * reach the edge and the ceiling of the world, where some neighbors lie outside every chunk.
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
	TestTrue(TEXT("Refill pit"), World.PlaceBlock(FIntVector(0, 0, Top - 1), Test.Stone));
	TestTrue(TEXT("Refill pit"), World.PlaceBlock(FIntVector(-1, 0, Top - 1), Test.Stone));

	// 3. A trench inside one chunk: each removal takes a cube out of the middle of the mesh's list.
	for (int32 X = 3; X <= 7; ++X)
	{
		TestTrue(TEXT("Mine trench"), World.MineBlock(FIntVector(X, 5, Top)));
	}
	TestTrue(TEXT("Dig deeper"), World.MineBlock(FIntVector(5, 5, Top - 1)));
	TestTrue(TEXT("Refill trench"), World.PlaceBlock(FIntVector(4, 5, Top), Test.Stone));

	// 4. Floating snow: a block type the chunk did not draw before, then mined again.
	constexpr int32 FloatingGap = 5;	// Air blocks between the ground and the snow, so it touches nothing.
	const int32 High = Top + 1 + FloatingGap;
	TestTrue(TEXT("Place snow"), World.PlaceBlock(FIntVector(5, 5, High), Test.Snow));
	TestTrue(TEXT("Place snow"), World.PlaceBlock(FIntVector(6, 5, High), Test.Snow));
	TestTrue(TEXT("Mine snow"), World.MineBlock(FIntVector(5, 5, High)));

	// 5. The edge of the world (one neighbor has no chunk) and its corner (two neighbors have none).
	//    Y = 2 and 3 keep the edge blocks away from the chunk border at Y = -1 | 0, so only the world edge is tested.
	constexpr int32 Last = FTestVoxelWorld::LastBlock;
	constexpr int32 First = FTestVoxelWorld::FirstBlock;
	TestTrue(TEXT("Mine at the edge"), World.MineBlock(FIntVector(Last, 2, Top)));
	TestTrue(TEXT("Place at the edge"), World.PlaceBlock(FIntVector(Last, 3, Top + 1), Test.Snow));	// Next to the hole.
	TestTrue(TEXT("Mine at the corner"), World.MineBlock(FIntVector(First, First, Top)));

	// 6. The ceiling: a 3 x 3 slab in the highest layer with one block under its centre. The centre block is then
	//    covered on every side except the top, so it is drawn only if "above the ceiling" counts as Air.
	//    Then a slab corner is mined: its neighbor above lies outside the world and must be skipped.
	constexpr int32 Ceiling = KubeCraft::Voxel::ChunkHeight - 1;
	const FIntVector SlabCentre(-5, 8, Ceiling);	// Middle of chunk (-1, 0), away from every chunk border.
	for (int32 Y = -1; Y <= 1; ++Y)
	{
		for (int32 X = -1; X <= 1; ++X)
		{
			TestTrue(TEXT("Place slab at the ceiling"), World.PlaceBlock(SlabCentre + FIntVector(X, Y, 0), Test.Stone));
		}
	}
	TestTrue(TEXT("Place under the slab"), World.PlaceBlock(SlabCentre - FIntVector(0, 0, 1), Test.Stone));
	TestTrue(TEXT("Mine slab corner"), World.MineBlock(SlabCentre + FIntVector(-1, -1, 0)));

	const FTestVoxelWorld::FDrawnCubes Incremental = Test.GetDrawnCubes();
	TestEqual(TEXT("No block drawn twice"), Incremental.MeshPerBlock.Num(), Incremental.CubeCount);

	// Which blocks are drawn: checked against the rule itself.
	const TSet<FIntVector> Expected = FindBlocksThatShouldBeDrawn(World);
	for (const FIntVector& Block : Expected)
	{
		if (!Incremental.MeshPerBlock.Contains(Block))
		{
			AddError(FString::Printf(TEXT("Block %s touches Air but is not drawn"), *Block.ToString()));
		}
	}
	for (const TPair<FIntVector, const UInstancedStaticMeshComponent*>& Pair : Incremental.MeshPerBlock)
	{
		if (!Expected.Contains(Pair.Key))
		{
			AddError(FString::Printf(TEXT("Block %s is drawn but should not be"), *Pair.Key.ToString()));
		}
	}

	// Which mesh draws each block, i.e. its type: checked against a full rebuild.
	Test.RebuildAllChunks();
	const FTestVoxelWorld::FDrawnCubes Full = Test.GetDrawnCubes();
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

/**
 * Checks that a see-through block (leaves) does not hide the cube under it, while a normal block does.
 * The ground is flat stone, so a stone with a cube placed on top of it is covered on all six sides.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeeThroughBlockTest, "KubeCraft.Integration.SeeThroughBlock",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSeeThroughBlockTest::RunTest(const FString& Parameters)
{
	const FTestVoxelWorld Test;
	if (!TestNotNull(TEXT("Test world"), Test.World))
	{
		return false;
	}
	Test.Snow->bSeeThrough = true;

	const FIntVector UnderSeeThrough(0, 0, FTestVoxelWorld::GroundHeight - 1);
	const FIntVector UnderStone(4, 4, FTestVoxelWorld::GroundHeight - 1);
	TestTrue(TEXT("Place a see-through block"), Test.World->PlaceBlock(UnderSeeThrough + FIntVector(0, 0, 1), Test.Snow));
	TestTrue(TEXT("Place a normal block"), Test.World->PlaceBlock(UnderStone + FIntVector(0, 0, 1), Test.Stone));

	const FTestVoxelWorld::FDrawnCubes Drawn = Test.GetDrawnCubes();
	TestTrue(TEXT("Cube under a see-through block stays drawn"), Drawn.MeshPerBlock.Contains(UnderSeeThrough));
	TestFalse(TEXT("Cube under a normal block is hidden"), Drawn.MeshPerBlock.Contains(UnderStone));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
