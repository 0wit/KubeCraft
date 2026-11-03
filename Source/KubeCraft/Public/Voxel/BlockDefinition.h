#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BlockDefinition.generated.h"

class UMaterialInterface;

/**
 * One block type (stone, grass, snow, ...) as a data asset, e.g. DA_Block_Stone.
 * Adding a new block type needs no code: create a new asset of this class and add it
 * to the block catalog (DA_BlockCatalog).
 */
UCLASS(BlueprintType)
class KUBECRAFT_API UBlockDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KubeCraft|Block")
	FText DisplayName;

	/** Material applied to every cube of this type. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KubeCraft|Block")
	TObjectPtr<UMaterialInterface> Material = nullptr;

	/** How long the mine button must be held to break one cube of this type. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KubeCraft|Block", meta = (ClampMin = "0.05", Units = "s"))
	float MineTime = 1.f;

	/** False for blocks the player can never remove. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KubeCraft|Block")
	bool bBreakable = true;
};
