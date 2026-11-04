#include "Rendering/BlockInstancesComponent.h"

#include "World/VoxelTypes.h"

using namespace KubeCraft::Voxel;

UBlockInstancesComponent::UBlockInstancesComponent()
{
	// Without this, removing instance 3 of 10 would shift instances 4..9 down by one,
	// and every one of them would need a new entry in InstancePerCell.
	SetRemoveSwap();
}

void UBlockInstancesComponent::SetBlocks(const TArray<FIntVector>& Cells)
{
	ClearInstances();
	CellPerInstance.Reset();
	InstancePerCell.Reset();

	TArray<FTransform> Transforms;
	Transforms.Reserve(Cells.Num());
	for (const FIntVector& Cell : Cells)
	{
		InstancePerCell.Add(Cell, CellPerInstance.Add(Cell));
		Transforms.Emplace(LocalToRelativeCenter(Cell));
	}
	// One call for all cubes is much faster than one AddInstance per cube.
	AddInstances(Transforms, /*bShouldReturnIndices*/ false);
}

void UBlockInstancesComponent::AddBlock(const FIntVector& Cell)
{
	if (InstancePerCell.Contains(Cell))
	{
		return;
	}
	// New instances are always appended, so the new index is the current count.
	const int32 Instance = AddInstance(FTransform(LocalToRelativeCenter(Cell)), /*bWorldSpace*/ false);
	ensure(Instance == CellPerInstance.Num());
	CellPerInstance.Add(Cell);
	InstancePerCell.Add(Cell, Instance);
}

void UBlockInstancesComponent::RemoveBlock(const FIntVector& Cell)
{
	int32 Instance = INDEX_NONE;
	if (!InstancePerCell.RemoveAndCopyValue(Cell, Instance))
	{
		return;
	}
	// The record always matches the instance list unless this class has a bug: report that instead of crashing.
	if (!ensureMsgf(CellPerInstance.IsValidIndex(Instance), TEXT("%s: cell %s points to missing instance %d"),
		*GetName(), *Cell.ToString(), Instance))
	{
		return;
	}
	// Swap-removal: the last instance moves into the freed index. Do the same with the cell list,
	// then tell the moved cell its new index.
	//   before: [A B C D]   remove B   after: [A D C]   D: 3 -> 1
	RemoveInstance(Instance);
	CellPerInstance.RemoveAtSwap(Instance, EAllowShrinking::No);
	if (CellPerInstance.IsValidIndex(Instance))
	{
		InstancePerCell[CellPerInstance[Instance]] = Instance;
	}
}
