#pragma once

#include "CoreMinimal.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "BlockInstancesComponent.generated.h"

/**
 * Draws all cubes of one block type in one chunk, as instances (copies) of one cube mesh.
 * It remembers which instance stands in which cell, so a single cube can be added or removed without
 * touching the others. Change its cubes only through the functions below, so that record stays right.
 * If the record ever disagrees with the instances, AddBlock / RemoveBlock change nothing (with an ensure)
 * instead of moving another cell's cube.
 *
 * Cells are local block coordinates of the chunk; the component sits at the chunk's first block.
 */
UCLASS()
class KUBECRAFT_API UBlockInstancesComponent : public UInstancedStaticMeshComponent
{
	GENERATED_BODY()

public:
	/** Turns on swap-removal (see RemoveBlock), which the cell record relies on. */
	UBlockInstancesComponent();

	/** Replaces all cubes with one cube per given cell, in a single update. Cells must not repeat. */
	void SetBlocks(const TArray<FIntVector>& Cells);

	/** Adds a cube in this cell. Does nothing if there already is one. */
	void AddBlock(const FIntVector& Cell);

	/** Removes the cube in this cell. Does nothing if there is none. */
	void RemoveBlock(const FIntVector& Cell);

private:
	/**
	 * True if the record has one cell per instance. False (with an ensure) if cubes were added or removed
	 * past this class, e.g. with AddInstance or ClearInstances called directly.
	 */
	bool CheckRecordMatchesInstances() const;

	/** Cell of each instance, by instance index. Always the same length as the instance list. */
	TArray<FIntVector> CellPerInstance;

	/** Instance index of each cell that has a cube: the reverse of CellPerInstance. */
	TMap<FIntVector, int32> InstancePerCell;
};
