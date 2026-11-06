#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BlockDefinition.generated.h"

class UMaterialInterface;
class UTexture2D;

/**
 * One block type as a data asset, e.g. DA_Block_Stone.
 * Adding a new block type needs no code: create a new asset of this class and add it
 * to the block catalog (DA_BlockCatalog).
 */
UCLASS(BlueprintType)
class KUBECRAFT_API UBlockDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Material applied to every cube of this type. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KubeCraft|Block")
	TObjectPtr<UMaterialInterface> Material = nullptr;

	/**
	 * Picture of this block type, shown in the hotbar inventory. Meant to be the same texture the block's material shows,
	 * so the inventory and the world always match (the block materials use plain color for now).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KubeCraft|Block")
	TObjectPtr<UTexture2D> Texture = nullptr;

	/** How long the mine button must be held to break one cube of this type. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KubeCraft|Block", meta = (ClampMin = "0.05", Units = "s"))
	float MineTime = 1.f;

	/** False for blocks the player can never remove. (bedrock only for now) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KubeCraft|Block")
	bool bBreakable = true;

	/**
	 * True for blocks with holes in their material (leaves). Cubes next to them stay drawn, so you can see through
	 * the holes. Normal blocks hide a cube that is covered on all six sides.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KubeCraft|Block")
	bool bSeeThrough = false;
};
