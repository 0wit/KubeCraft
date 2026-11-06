#include "Terrain/TreeGenerator.h"

#include "Blocks/BlockCatalog.h"
#include "Math/RandomStream.h"
#include "Terrain/TerrainGenerator.h"

using namespace KubeCraft::Voxel;

namespace
{
	/** How far the leaves reach from the trunk, in blocks. The widest layer is 2 * 2 + 1 = 5 blocks. */
	constexpr int32 LeafRadius = 2;

	/** Smallest cell that still holds a whole tree, so trees of two cells never touch. */
	constexpr int32 MinCellSize = 2 * LeafRadius + 1;

	/** Number of wide leaf layers, around the top of the trunk. */
	constexpr int32 WideLeafLayers = 2;
}

FTreeGenerator::FTreeGenerator(int32 InSeed, const FTreeSettings& InSettings)
	: Seed(InSeed)
	, Settings(InSettings)
{
	Settings.CellSize = FMath::Max(Settings.CellSize, MinCellSize);
	Settings.MinTrunkHeight = FMath::Max(Settings.MinTrunkHeight, WideLeafLayers + 1);
	Settings.MaxTrunkHeight = FMath::Max(Settings.MaxTrunkHeight, Settings.MinTrunkHeight);
}

void FTreeGenerator::AddTrees(const FIntPoint& ChunkCoord, TArray<FBlockId>& Blocks, const FTerrainGenerator& Terrain,
	const UBlockCatalog& Catalog) const
{
	if (!IsEnabled())
	{
		return;
	}

	// Every cell that covers part of this chunk. A tree stays inside its cell, so no other cell can reach in.
	const FIntVector Origin = ChunkToBlockOrigin(ChunkCoord);
	const int32 FirstCellX = FloorDiv(Origin.X, Settings.CellSize);
	const int32 FirstCellY = FloorDiv(Origin.Y, Settings.CellSize);
	const int32 LastCellX = FloorDiv(Origin.X + ChunkSize - 1, Settings.CellSize);
	const int32 LastCellY = FloorDiv(Origin.Y + ChunkSize - 1, Settings.CellSize);
	for (int32 CellY = FirstCellY; CellY <= LastCellY; ++CellY)
	{
		for (int32 CellX = FirstCellX; CellX <= LastCellX; ++CellX)
		{
			const TOptional<FTree> Tree = FindTree(FIntPoint(CellX, CellY), Terrain, Catalog);
			if (!Tree)
			{
				continue;
			}
			// Walk the box around the tree; GetPart says what goes where.
			for (int32 Z = 0; Z <= Tree->TrunkHeight; ++Z)
			{
				for (int32 Y = -LeafRadius; Y <= LeafRadius; ++Y)
				{
					for (int32 X = -LeafRadius; X <= LeafRadius; ++X)
					{
						const FIntVector Offset(X, Y, Z);
						const FIntVector Local = Tree->Base + Offset - Origin;
						const FBlockId Id = GetPartId(GetPart(Offset, Tree->TrunkHeight), Catalog);
						if (Id != AirBlockId && IsInsideChunk(Local) && Blocks[LocalToIndex(Local)] == AirBlockId)
						{
							Blocks[LocalToIndex(Local)] = Id;
						}
					}
				}
			}
		}
	}
}

FBlockId FTreeGenerator::GetTreeBlock(const FIntVector& Block, const FTerrainGenerator& Terrain, const UBlockCatalog& Catalog) const
{
	if (!IsEnabled())
	{
		return AirBlockId;
	}
	const FIntPoint Cell(FloorDiv(Block.X, Settings.CellSize), FloorDiv(Block.Y, Settings.CellSize));
	const TOptional<FTree> Tree = FindTree(Cell, Terrain, Catalog);
	return Tree ? GetPartId(GetPart(Block - Tree->Base, Tree->TrunkHeight), Catalog) : AirBlockId;
}

TOptional<FTreeGenerator::FTree> FTreeGenerator::FindTree(const FIntPoint& Cell, const FTerrainGenerator& Terrain,
	const UBlockCatalog& Catalog) const
{
	// Every cell gets its own dice, made from the seed and the cell, so its tree is always the same.
	// Always roll in the same order: chance, column, height.
	FRandomStream Random(static_cast<int32>(HashCombineFast(GetTypeHash(Seed), GetTypeHash(Cell))));
	if (Random.FRand() >= Settings.Chance)
	{
		return {};
	}

	// Keep the trunk LeafRadius away from the cell's edges, so the leaves stay inside the cell.
	const int32 X = Cell.X * Settings.CellSize + Random.RandRange(LeafRadius, Settings.CellSize - 1 - LeafRadius);
	const int32 Y = Cell.Y * Settings.CellSize + Random.RandRange(LeafRadius, Settings.CellSize - 1 - LeafRadius);
	const int32 TrunkHeight = Random.RandRange(Settings.MinTrunkHeight, Settings.MaxTrunkHeight);

	const int32 GroundHeight = Terrain.GetSurfaceHeight(X, Y);
	const bool bOnGrass = Catalog.GetIdForHeight(GroundHeight - 1, /*bIsTop*/ true) == Catalog.GetId(Settings.GrowsOn);
	// The top leaves sit TrunkHeight above the base; they must stay below the ceiling.
	const bool bFitsUnderCeiling = GroundHeight + TrunkHeight < ChunkHeight;
	if (!bOnGrass || !bFitsUnderCeiling)
	{
		return {};
	}
	return FTree{ FIntVector(X, Y, GroundHeight), TrunkHeight };
}

FTreeGenerator::ETreePart FTreeGenerator::GetPart(const FIntVector& Offset, int32 TrunkHeight)
{
	//  side view, trunk height 4:        top view of a wide layer:
	//  Z 4   . . L . .  <- small layer      . L L L .
	//  Z 3   L L W L L  <- wide layers      L L L L L
	//  Z 2   L L W L L                     L L W L L
	//  Z 1       W                         L L L L L
	//  Z 0       W      <- Base             . L L L .
	const int32 AbsX = FMath::Abs(Offset.X);
	const int32 AbsY = FMath::Abs(Offset.Y);
	if (AbsX == 0 && AbsY == 0 && Offset.Z >= 0 && Offset.Z < TrunkHeight)
	{
		return ETreePart::Trunk;
	}
	const bool bInWideLayer = Offset.Z >= TrunkHeight - WideLeafLayers && Offset.Z < TrunkHeight;
	const bool bIsCorner = AbsX == LeafRadius && AbsY == LeafRadius;
	if (bInWideLayer && AbsX <= LeafRadius && AbsY <= LeafRadius && !bIsCorner)
	{
		return ETreePart::Leaves;
	}
	// The small layer on top: a plus shape, the middle and its four sides.
	if (Offset.Z == TrunkHeight && AbsX + AbsY <= 1)
	{
		return ETreePart::Leaves;
	}
	return ETreePart::None;
}

FBlockId FTreeGenerator::GetPartId(ETreePart Part, const UBlockCatalog& Catalog) const
{
	switch (Part)
	{
	case ETreePart::Trunk:
		return Catalog.GetId(Settings.Trunk);
	case ETreePart::Leaves:
		return Catalog.GetId(Settings.Leaves);
	default:
		return AirBlockId;
	}
}

bool FTreeGenerator::IsEnabled() const
{
	return Settings.Trunk && Settings.Leaves && Settings.GrowsOn;
}
