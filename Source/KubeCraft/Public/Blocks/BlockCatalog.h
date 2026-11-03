#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "World/VoxelTypes.h"
#include "BlockCatalog.generated.h"

class UBlockDefinition;

/** "Below this height, use this block." One row of the height-to-block table. */
USTRUCT(BlueprintType)
struct FBlockHeightBand
{
	GENERATED_BODY()

	/** The band covers block heights below this value (and at or above the previous band's MaxZ). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KubeCraft|Blocks")
	int32 MaxZ = 0;

	/** Block type used inside this band. Must also be listed in the catalog's Blocks. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KubeCraft|Blocks")
	TObjectPtr<UBlockDefinition> Block = nullptr;
};

/**
 * All block types of the game and the height rule, edited as one data asset (DA_BlockCatalog).
 *
 *   Blocks:       [DA_Block_Stone, DA_Block_Grass, DA_Block_Snow, DA_Block_Bedrock]
 *                    id 1            id 2            id 3          id 4             (id 0 = Air)
 *   HeightBands:  [{MaxZ 1: Bedrock}, {MaxZ 16: Stone}, {MaxZ 26: Grass}, {MaxZ 64: Snow}]
 *
 * Chunks store only the id (one byte per cube); this catalog turns an id back into its definition.
 * The height bands decide the block type by height, both for terrain generation and for placing.
 */
UCLASS(BlueprintType)
class KUBECRAFT_API UBlockCatalog : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Definition for an id, or nullptr for Air and unknown ids. */
	const UBlockDefinition* GetDefinition(FBlockId Id) const;

	/** Id of a definition listed in Blocks, or AirBlockId if it isn't listed. */
	FBlockId GetId(const UBlockDefinition* Definition) const;

	/** Id of the block that belongs at block height Z according to HeightBands. */
	FBlockId GetIdForHeight(int32 Z) const;

#if WITH_EDITOR
	/** Reports configuration mistakes (missing materials, unsorted bands, ...) when the asset is saved. */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

protected:
	/** The automation tests fill Blocks and HeightBands directly; in the game they are only edited in the asset. */
	friend class FBlockCatalogHeightBandsTest;
	friend struct FTestVoxelWorld;

	/**
	 * Every block type in the game. A block's id is its position in this list + 1 (0 is Air).
	 * Append new blocks at the end so existing ids don't change.
	 */
	UPROPERTY(EditAnywhere, Category = "KubeCraft|Blocks")
	TArray<TObjectPtr<UBlockDefinition>> Blocks;

	/** Sorted by MaxZ, lowest first. Heights above the last band use the last band's block. */
	UPROPERTY(EditAnywhere, Category = "KubeCraft|Blocks")
	TArray<FBlockHeightBand> HeightBands;
};
