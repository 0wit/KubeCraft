#pragma once

#include "CoreMinimal.h"

class AVoxelWorld;

/**
 * Decides what the player may change in the world. Only checks, never changes anything.
 * All limits are here (floor, ceiling, only in loaded chunks).
 */
class KUBECRAFT_API FBlockRules
{
public:
	/** Rules for one world. Keeps a reference to the world it checks. */
	explicit FBlockRules(const AVoxelWorld& InWorld) : World(InWorld) {}

	/** True if the block isn't air and its type is breakable. */
	bool CanMine(const FIntVector& Block) const;

	/**
	 * True if the cell is inside the world, is air and no player is standing in it.
	 */
	bool CanPlace(const FIntVector& Block) const;

private:
	const AVoxelWorld& World;
};
