#include "Game/KubeCraftUserSettings.h"

UKubeCraftUserSettings* UKubeCraftUserSettings::GetKubeCraftUserSettings()
{
	return Cast<UKubeCraftUserSettings>(UGameUserSettings::GetGameUserSettings());
}

void UKubeCraftUserSettings::SetViewDistance(int32 Chunks)
{
	ViewDistance = FMath::Clamp(Chunks, MinViewDistance, MaxViewDistance);
}

void UKubeCraftUserSettings::SetChunksPerFrame(int32 Chunks)
{
	ChunksPerFrame = FMath::Clamp(Chunks, MinChunksPerFrame, MaxChunksPerFrame);
}

void UKubeCraftUserSettings::GetStreamingRanges(int32& OutMinViewDistance, int32& OutMaxViewDistance,
	int32& OutMinChunksPerFrame, int32& OutMaxChunksPerFrame)
{
	OutMinViewDistance = MinViewDistance;
	OutMaxViewDistance = MaxViewDistance;
	OutMinChunksPerFrame = MinChunksPerFrame;
	OutMaxChunksPerFrame = MaxChunksPerFrame;
}

void UKubeCraftUserSettings::SetToDefaults()
{
	// The engine resets the last kept resolution and display too; keep them for RevertVideoMode.
	const uint32 KeptSizeX = LastUserConfirmedResolutionSizeX;
	const uint32 KeptSizeY = LastUserConfirmedResolutionSizeY;
	const FString KeptDisplayID = LastUserConfirmedDisplayID;
	const int32 KeptDisplayIndex = LastUserConfirmedDisplayIndex;

	Super::SetToDefaults();

	LastUserConfirmedResolutionSizeX = KeptSizeX;
	LastUserConfirmedResolutionSizeY = KeptSizeY;
	LastUserConfirmedDisplayID = KeptDisplayID;
	LastUserConfirmedDisplayIndex = KeptDisplayIndex;

	// 0 x 0 means "pick one later" to the engine; the desktop size is a real choice the settings page can show.
	const FIntPoint DesktopResolution = GetDesktopResolution();
	if (DesktopResolution.X > 0 && DesktopResolution.Y > 0)
	{
		SetScreenResolution(DesktopResolution);
	}
	SetOverallScalabilityLevel(DefaultQualityLevel);
	ViewDistance = DefaultViewDistance;
	ChunksPerFrame = DefaultChunksPerFrame;
}

void UKubeCraftUserSettings::ValidateSettings()
{
	Super::ValidateSettings();
	SetViewDistance(ViewDistance);
	SetChunksPerFrame(ChunksPerFrame);
}

void UKubeCraftUserSettings::ApplyNonResolutionSettings()
{
	Super::ApplyNonResolutionSettings();
	OnApplied.Broadcast(*this);
}
