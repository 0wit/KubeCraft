#include "Terrain/TerrainGenerator.h"

#include "Blocks/BlockCatalog.h"
#include "Math/RandomStream.h"

using namespace KubeCraft::Voxel;

namespace
{
	/** FMath::PerlinNoise2D repeats every 256 units, so shifting a layer further than that changes nothing. */
	constexpr double NoisePeriod = 256.0;

	/** Lowest allowed ground: one block, so there is always a floor to stand on. */
	constexpr int32 MinSurfaceHeight = 1;

	/** Highest allowed ground: one block below the ceiling, so there is always room to build. */
	constexpr int32 MaxSurfaceHeight = ChunkHeight - 1;
}

FTerrainGenerator::FTerrainGenerator(int32 Seed, const FTerrainSettings& InSettings)
	: Settings(InSettings)
{
	FRandomStream Random(Seed);
	const int32 NumOctaves = FMath::Max(1, Settings.Octaves);
	for (int32 Octave = 0; Octave < NumOctaves; ++Octave)
	{
		OctaveOffsets.Emplace(Random.FRandRange(0.0, NoisePeriod), Random.FRandRange(0.0, NoisePeriod));
	}
}

int32 FTerrainGenerator::GetSurfaceHeight(int32 BlockX, int32 BlockY) const
{
	// Fractal noise (fBm): add the layers, each one narrower (Lacunarity) and flatter (Persistence).
	const FVector2D Column(BlockX, BlockY);
	double Frequency = 1.0 / FMath::Max(Settings.HillWidth, 1.f);
	double Amplitude = 1.0;
	double Sum = 0.0;
	double MaxSum = 0.0;
	for (const FVector2D& Offset : OctaveOffsets)
	{
		Sum += Amplitude * FMath::PerlinNoise2D(Column * Frequency + Offset);
		MaxSum += Amplitude;
		Amplitude *= Settings.Persistence;
		Frequency *= Settings.Lacunarity;
	}

	// Each layer is in [-1, 1]; dividing by the sum of the amplitudes brings the total back to [-1, 1].
	const double Noise = Sum / MaxSum;
	const int32 Height = Settings.BaseHeight + FMath::RoundToInt32(Noise * Settings.HeightVariation);
	return FMath::Clamp(Height, MinSurfaceHeight, MaxSurfaceHeight);
}

TArray<FBlockId> FTerrainGenerator::GenerateChunk(const FIntPoint& ChunkCoord, const UBlockCatalog& Catalog) const
{
	// The block type depends only on the height, so look it up once per height instead of once per block.
	// The top cube of a column can differ (grass on top of dirt), so keep a second table for it.
	FBlockId BlockIdPerHeight[ChunkHeight];
	FBlockId TopIdPerHeight[ChunkHeight];
	for (int32 Z = 0; Z < ChunkHeight; ++Z)
	{
		BlockIdPerHeight[Z] = Catalog.GetIdForHeight(Z, /*bIsTop*/ false);
		TopIdPerHeight[Z] = Catalog.GetIdForHeight(Z, /*bIsTop*/ true);
	}

	TArray<FBlockId> Blocks;
	Blocks.Init(AirBlockId, BlocksPerChunk);
	const FIntVector Origin = ChunkToBlockOrigin(ChunkCoord);
	for (int32 Y = 0; Y < ChunkSize; ++Y)
	{
		for (int32 X = 0; X < ChunkSize; ++X)
		{
			const int32 Height = GetSurfaceHeight(Origin.X + X, Origin.Y + Y);
			for (int32 Z = 0; Z < Height; ++Z)
			{
				Blocks[LocalToIndex(FIntVector(X, Y, Z))] = (Z == Height - 1) ? TopIdPerHeight[Z] : BlockIdPerHeight[Z];
			}
		}
	}
	return Blocks;
}

FBlockId FTerrainGenerator::GetTerrainBlock(const FIntVector& Block, const UBlockCatalog& Catalog) const
{
	const int32 Height = GetSurfaceHeight(Block.X, Block.Y);
	if (Block.Z < 0 || Block.Z >= Height)
	{
		return AirBlockId;
	}
	return Catalog.GetIdForHeight(Block.Z, /*bIsTop*/ Block.Z == Height - 1);
}
