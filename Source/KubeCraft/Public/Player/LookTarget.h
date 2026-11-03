#pragma once

#include "CoreMinimal.h"

class APawn;
class AVoxelWorld;

/**
 * The block a player is looking at, and the empty cell in front of the face they look at.
 * A short-lived answer: use it right away, don't keep it across frames (World is a plain pointer).
 */
struct FLookTarget
{
	/** World that owns the block. Never null in a target returned by TraceLookTarget. */
	AVoxelWorld* World = nullptr;

	/** The cube that was hit; mining removes this one. */
	FIntVector Block = FIntVector::ZeroValue;

	/** The cell in front of the hit face; placing fills this one. */
	FIntVector PlaceCell = FIntVector::ZeroValue;
};

namespace KubeCraft::Player
{
	/**
	 * Looks from the pawn's view point (the camera, for a player) up to Reach cm and returns the first block hit.
	 * Empty if nothing is in reach, the first hit is not part of a voxel world, or the pawn has no controller.
	 */
	TOptional<FLookTarget> TraceLookTarget(const APawn& Viewer, float Reach);
}
