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

FBlockId UBlockCatalog::GetIdForHeight(int32 Z) const
{
	for (const FBlockHeightBand& Band : HeightBands)
	{
		if (Z < Band.MaxZ)
		{
			return GetId(Band.Block);
		}
	}
	return HeightBands.IsEmpty() ? AirBlockId : GetId(HeightBands.Last().Block);
}

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
	}

	for (int32 Index = 0; Index < HeightBands.Num(); ++Index)
	{
		const FBlockHeightBand& Band = HeightBands[Index];
		if (GetId(Band.Block) == AirBlockId)
		{
			AddError(FText::Format(LOCTEXT("BandBlockNotListed", "HeightBands[{0}] uses a block that is not in Blocks."), Index));
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
