#include "Blocks/BlockCatalog.h"

#include "Blocks/BlockDefinition.h"
#include "Misc/DataValidation.h"

#define LOCTEXT_NAMESPACE "BlockCatalog"

const UBlockDefinition* UBlockCatalog::GetDefinition(FBlockId Id) const
{
	const int32 Index = static_cast<int32>(Id) - 1;
	return Blocks.IsValidIndex(Index) ? Blocks[Index].Get() : nullptr;
}

FBlockId UBlockCatalog::GetId(const UBlockDefinition* Definition) const
{
	const int32 Index = Blocks.IndexOfByKey(Definition);
	return (Definition && Index != INDEX_NONE) ? static_cast<FBlockId>(Index + 1) : AirBlockId;
}

FBlockId UBlockCatalog::GetIdForHeight(int32 Z, bool bIsTop) const
{
	if (HeightBands.IsEmpty())
	{
		return AirBlockId;
	}
	const FBlockHeightBand* Band = HeightBands.FindByPredicate([Z](const FBlockHeightBand& Each) { return Z < Each.MaxZ; });
	if (!Band)
	{
		Band = &HeightBands.Last();
	}
	return GetId(bIsTop && Band->TopBlock ? Band->TopBlock : Band->Block);
}

// runs only in the editor and checks the asset when you save it.
// possible errors: too many blocks, empty slots, missing materials, duplicates, and bad height bands.
#if WITH_EDITOR
EDataValidationResult UBlockCatalog::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	auto AddError = [&Context, &Result](const FText& Message)
	{
		Context.AddError(Message);
		Result = EDataValidationResult::Invalid;
	};

	if (Blocks.Num() > MaxBlockId)
	{
		AddError(FText::Format(LOCTEXT("TooManyBlocks", "At most {0} block types are supported."), MaxBlockId));
	}

	for (int32 Index = 0; Index < Blocks.Num(); ++Index)
	{
		const UBlockDefinition* Block = Blocks[Index];
		if (!Block)
		{
			AddError(FText::Format(LOCTEXT("EmptyBlock", "Blocks[{0}] is empty."), Index));
		}
		else if (!Block->Material)
		{
			AddError(FText::Format(LOCTEXT("MissingMaterial", "Block '{0}' has no material."), FText::FromString(Block->GetName())));
		}
		else if (Blocks.IndexOfByKey(Block) != Index)
		{
			AddError(FText::Format(LOCTEXT("DuplicateBlock", "Block '{0}' is listed twice."), FText::FromString(Block->GetName())));
		}
		if (Block && !Block->Texture)
		{
			// Not an error: the game works, the inventory slot just shows an empty picture for this block.
			Context.AddWarning(FText::Format(LOCTEXT("MissingTexture", "Block '{0}' has no texture; its inventory slot shows no picture."),
											 FText::FromString(Block->GetName())));
		}
	}

	for (int32 Index = 0; Index < HeightBands.Num(); ++Index)
	{
		const FBlockHeightBand& Band = HeightBands[Index];
		if (GetId(Band.Block) == AirBlockId)
		{
			AddError(FText::Format(LOCTEXT("BandBlockNotListed", "HeightBands[{0}] uses a block that is not in Blocks."), Index));
		}
		if (Band.TopBlock && GetId(Band.TopBlock) == AirBlockId)
		{
			AddError(FText::Format(LOCTEXT("BandTopBlockNotListed", "HeightBands[{0}] has a TopBlock that is not in Blocks."), Index));
		}
		if (Band.MaxZ <= 0 || Band.MaxZ > KubeCraft::Voxel::ChunkHeight)
		{
			AddError(FText::Format(LOCTEXT("BandOutsideWorld", "HeightBands[{0}].MaxZ must be between 1 and {1} (the world height)."),
								   Index, KubeCraft::Voxel::ChunkHeight));
		}
		if (Index > 0 && Band.MaxZ <= HeightBands[Index - 1].MaxZ)
		{
			AddError(FText::Format(LOCTEXT("BandsNotSorted", "HeightBands[{0}] must have a higher MaxZ than the band before it."), Index));
		}
	}

	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
