#include "Blocks/BlockRules.h"

#include "Blocks/BlockDefinition.h"
#include "Engine/World.h"
#include "World/VoxelTypes.h"
#include "World/VoxelWorld.h"

using namespace KubeCraft::Voxel;

namespace
{
	/** How far (cm) a player may reach into a cell and still not block placing there. */
	constexpr float PlayerTouchTolerance = 1.f;
}

bool FBlockRules::CanMine(const FIntVector& Block) const
{
	// Air has no definition. Unbreakable types such as bedrock are the floor of the world.
	const UBlockDefinition* Definition = World.GetBlockDefinition(Block);
	return Definition && Definition->bBreakable;
}

bool FBlockRules::CanPlace(const FIntVector& Block) const
{
	if (!World.IsInsideWorld(Block) || World.GetBlock(Block) != AirBlockId)
	{
		return false;
	}
	// A block must not trap a player. The test box is a bit smaller than the cell,
	// so a player who only touches one of its faces may still build there.
	const FCollisionShape CellShape = FCollisionShape::MakeBox(FVector(0.5f * BlockSize - PlayerTouchTolerance));
	const UWorld* PhysicsWorld = World.GetWorld();
	return PhysicsWorld && !PhysicsWorld->OverlapAnyTestByObjectType(BlockToWorldCenter(Block), FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn), CellShape);
}
