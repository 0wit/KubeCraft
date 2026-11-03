#pragma once

#include "CoreMinimal.h"

/**
 * What a chunk stores for each cube: one byte saying which block type is there.
 * 0 means Air (empty); any other value is a position in the block catalog (see UBlockCatalog).
 */
using FBlockId = uint8;

/** The id of an empty cell: never drawn, never collides, not listed in the catalog. */
inline constexpr FBlockId AirBlockId = 0;

/** Highest id a block type can have, so the catalog supports at most this many block types. */
inline constexpr int32 MaxBlockId = TNumericLimits<FBlockId>::Max();

/**
 * World grid constants and the only place where coordinates are converted.
 *
 * Three coordinate spaces are used:
 *  - World: FVector in centimeters (Unreal units).
 *  - Block: FIntVector, one unit = one cube. Block (0,0,0) spans world [0, BlockSize) on each axis.
 *  - Local: FIntVector inside one chunk, X/Y in [0, ChunkSize), Z in [0, ChunkHeight).
 *
 * Example - the same spot in all spaces:
 *   World (1750, -30, 420) cm
 *   -> Block (17, -1, 4)          WorldToBlock: divide by 100, round DOWN (-0.3 -> -1, not 0)
 *   -> Chunk (1, -1)              BlockToChunk: divide by 16, round DOWN (FloorDiv)
 *   -> Local (1, 15, 4)           BlockToLocal: block minus the chunk's first block
 *   -> Index 1265                 LocalToIndex: 1 + 15*16 + 4*16*16
 */
namespace KubeCraft::Voxel
{
	/** Edge length of one cube in cm. Matches the engine's /Engine/BasicShapes/Cube mesh. */
	inline constexpr float BlockSize = 100.f;

	/** Number of blocks along X and Y in one chunk. */
	inline constexpr int32 ChunkSize = 16;

	/** Number of blocks along Z in one chunk. This is also the world's height limit. */
	inline constexpr int32 ChunkHeight = 64;

	/** Number of cells in one chunk, i.e. the length of its flat block array. */
	inline constexpr int32 BlocksPerChunk = ChunkSize * ChunkSize * ChunkHeight;

	/**
	 * Integer division that rounds towards negative infinity (-1 / 16 == -1, not 0).
	 * Divisor must be positive - it is always a grid size here.
	 */
	inline int32 FloorDiv(int32 Value, int32 Divisor)
	{
		check(Divisor > 0);
		// C++ rounds towards zero. A negative remainder means it rounded up, so step one down.
		const int32 Quotient = Value / Divisor;
		return (Value % Divisor < 0) ? Quotient - 1 : Quotient;
	}

	/** Block that contains the given world position. */
	inline FIntVector WorldToBlock(const FVector& WorldLocation)
	{
		return FIntVector(
			FMath::FloorToInt32(WorldLocation.X / BlockSize),
			FMath::FloorToInt32(WorldLocation.Y / BlockSize),
			FMath::FloorToInt32(WorldLocation.Z / BlockSize));
	}

	/** World position of a block's minimum corner, e.g. where a chunk actor whose first block this is sits. */
	inline FVector BlockToWorldCorner(const FIntVector& Block)
	{
		return FVector(Block) * BlockSize;
	}

	/** World position of the center of a block. */
	inline FVector BlockToWorldCenter(const FIntVector& Block)
	{
		return (FVector(Block) + FVector(0.5)) * BlockSize;
	}

	/** World position of the middle of a block's top face, e.g. where a player standing on it has their feet. */
	inline FVector BlockToWorldTopCenter(const FIntVector& Block)
	{
		return (FVector(Block) + FVector(0.5, 0.5, 1.0)) * BlockSize;
	}

	/**
	 * Center of a block relative to its chunk actor, which sits at the chunk's first block.
	 * Same math as BlockToWorldCenter, because the local grid starts at the actor's origin.
	 */
	inline FVector LocalToRelativeCenter(const FIntVector& Local)
	{
		return BlockToWorldCenter(Local);
	}

	/** Chunk (column) that contains the given block. */
	inline FIntPoint BlockToChunk(const FIntVector& Block)
	{
		return FIntPoint(FloorDiv(Block.X, ChunkSize), FloorDiv(Block.Y, ChunkSize));
	}

	/** Block coordinate of a chunk's first (minimum corner) block. */
	inline FIntVector ChunkToBlockOrigin(const FIntPoint& Chunk)
	{
		return FIntVector(Chunk.X * ChunkSize, Chunk.Y * ChunkSize, 0);
	}

	/** Position of a block inside its own chunk. */
	inline FIntVector BlockToLocal(const FIntVector& Block)
	{
		return Block - ChunkToBlockOrigin(BlockToChunk(Block));
	}

	/** True if a local coordinate lies inside the chunk bounds. */
	inline bool IsInsideChunk(const FIntVector& Local)
	{
		return Local.X >= 0 && Local.X < ChunkSize
			&& Local.Y >= 0 && Local.Y < ChunkSize
			&& Local.Z >= 0 && Local.Z < ChunkHeight;
	}

	/**
	 * Index into a chunk's flat block array. The caller must ensure IsInsideChunk(Local).
	 * Like a book: X = letter in a line, Y = line on a page, Z = page.
	 */
	inline int32 LocalToIndex(const FIntVector& Local)
	{
		return Local.X + Local.Y * ChunkSize + Local.Z * ChunkSize * ChunkSize;
	}
}
