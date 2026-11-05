#pragma once

#include "CoreMinimal.h"

class AVoxelWorld;

/**
 * The game's limits for changing blocks: which blocks may be mined and where a block may be placed.
 * All limits live here (floor, ceiling, only in loaded chunks, "not inside a player"), so they are easy to find and change.
 *
 *   FBlockRules Rules(World);
 *   if (Rules.CanMine(Block)) { ... }
 */
class KUBECRAFT_API FBlockRules
{
public:
	/** Rules for one world. Keep the object short-lived; it only holds a reference. */
	explicit FBlockRules(const AVoxelWorld& InWorld) : World(InWorld) {}

	/** True if this block may be removed: it is not Air and its type is breakable (bedrock is not). */
	bool CanMine(const FIntVector& Block) const;

	/**
	 * True if a block may be put here: the cell is inside the world (above the floor, below the ceiling,
	 * within the generated area), it is Air, and no pawn stands in it.
	 */
	bool CanPlace(const FIntVector& Block) const;

private:
	const AVoxelWorld& World;
};
