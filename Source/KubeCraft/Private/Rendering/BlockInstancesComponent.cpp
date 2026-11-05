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
	CellPerInstance.Reserve(Cells.Num());
	InstancePerCell.Reserve(Cells.Num());

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
	if (InstancePerCell.Contains(Cell) || !CheckRecordMatchesInstances())
	{
		return;
	}
	// New instances are always appended, so the new index is the current count.
	const int32 Instance = AddInstance(FTransform(LocalToRelativeCenter(Cell)), /*bWorldSpace*/ false);
	if (!ensureMsgf(Instance == CellPerInstance.Num(), TEXT("%s: cube for cell %s got index %d, expected %d"),
		*GetName(), *Cell.ToString(), Instance, CellPerInstance.Num()))
	{
		return;
	}
	CellPerInstance.Add(Cell);
	InstancePerCell.Add(Cell, Instance);
}

void UBlockInstancesComponent::RemoveBlock(const FIntVector& Cell)
{
	const int32* FoundInstance = InstancePerCell.Find(Cell);
	if (!FoundInstance || !CheckRecordMatchesInstances())
	{
		return;
	}
	const int32 Instance = *FoundInstance;
	// The two lists must agree on this cube, and the engine must really remove it. If not, change nothing:
	// a wrong swap-removal below would move another cell's cube.
	if (!ensureMsgf(CellPerInstance.IsValidIndex(Instance) && CellPerInstance[Instance] == Cell,
		TEXT("%s: cell %s points to instance %d, which is not its cube"), *GetName(), *Cell.ToString(), Instance))
	{
		return;
	}
	if (!ensureMsgf(RemoveInstance(Instance), TEXT("%s: could not remove instance %d"), *GetName(), Instance))
	{
		return;
	}
	// Swap-removal: the last instance moves into the freed index. Do the same with the cell list,
	// then tell the moved cell its new index.
	//   before: [A B C D]   remove B   after: [A D C]   D: 3 -> 1
	InstancePerCell.Remove(Cell);
	CellPerInstance.RemoveAtSwap(Instance, EAllowShrinking::No);
	if (CellPerInstance.IsValidIndex(Instance))
	{
		InstancePerCell[CellPerInstance[Instance]] = Instance;
	}
}

bool UBlockInstancesComponent::CheckRecordMatchesInstances() const
{
	return ensureMsgf(GetInstanceCount() == CellPerInstance.Num(),
		TEXT("%s: %d cubes drawn but %d recorded - change cubes only through SetBlocks / AddBlock / RemoveBlock"),
		*GetName(), GetInstanceCount(), CellPerInstance.Num());
}
