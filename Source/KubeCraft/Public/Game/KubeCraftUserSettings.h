#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "KubeCraftUserSettings.generated.h"

/**
 * Engine's settings class, plus two game settings.
 * View distance (2–12 chunks, default 4) and chunks per frame (1–8, default 2).
 * Everything is saved in GameUserSettings.ini.
 */
UCLASS(BlueprintType, config = GameUserSettings, configdonotcheckdefaults)
class KUBECRAFT_API UKubeCraftUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:
	/** Lowest and highest view distance in chunks. More chunks cost more memory and drawing time. */
	static constexpr int32 MinViewDistance = 2;
	static constexpr int32 MaxViewDistance = 12;
	static constexpr int32 DefaultViewDistance = 4;

	/** Lowest and highest chunk work per frame. Higher fills the world in faster, but frames may take longer. */
	static constexpr int32 MinChunksPerFrame = 1;
	static constexpr int32 MaxChunksPerFrame = 8;
	static constexpr int32 DefaultChunksPerFrame = 2;

	/** Overall quality after Defaults: 0 = Low, 1 = Medium, 2 = High, 3 = Epic. */
	static constexpr int32 DefaultQualityLevel = 3;

	/** Fires after the settings are applied, chunk streaming then uses the new values while playing. */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnApplied, const UKubeCraftUserSettings & /*Settings*/);
	FOnApplied OnApplied;

	/** The game's settings object. */
	UFUNCTION(BlueprintPure, Category = "KubeCraft|Settings")
	static UKubeCraftUserSettings *GetKubeCraftUserSettings();

	/** How far the player sees, in chunks. */
	UFUNCTION(BlueprintPure, Category = "KubeCraft|Settings")
	int32 GetViewDistance() const { return ViewDistance; }

	/** Sets the view distance, clamped to its range. Takes effect on ApplySettings. */
	UFUNCTION(BlueprintCallable, Category = "KubeCraft|Settings")
	void SetViewDistance(int32 Chunks);

	/** The most chunk loads, draws and unloads per frame. */
	UFUNCTION(BlueprintPure, Category = "KubeCraft|Settings")
	int32 GetChunksPerFrame() const { return ChunksPerFrame; }

	/** Sets the chunk work per frame, clamped to its range. Takes effect on ApplySettings. */
	UFUNCTION(BlueprintCallable, Category = "KubeCraft|Settings")
	void SetChunksPerFrame(int32 Chunks);

	/** The ranges of both values, for the settings sliders. */
	UFUNCTION(BlueprintPure, Category = "KubeCraft|Settings")
	static void GetStreamingRanges(int32 &OutMinViewDistance, int32 &OutMaxViewDistance, int32 &OutMinChunksPerFrame,
								   int32 &OutMaxChunksPerFrame);

	/**
	 * Resets everything to defaults, with the screen resolution set to the desktop size.
	 * Graphics quality set to Epic.
	 * It keeps the last confirmed resolution so that it can be reverted.
	 */
	virtual void SetToDefaults() override;

	/** Clamps values that were edited by hand in the .ini file. */
	virtual void ValidateSettings() override;

	/** Applies the settings, then fires OnApplied so the chunk streaming updates right away. */
	virtual void ApplyNonResolutionSettings() override;

private:
	UPROPERTY(config)
	int32 ViewDistance = DefaultViewDistance;

	UPROPERTY(config)
	int32 ChunksPerFrame = DefaultChunksPerFrame;
};
