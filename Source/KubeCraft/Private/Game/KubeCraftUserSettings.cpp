#include "Game/KubeCraftUserSettings.h"

#include "Engine/World.h"
#include "Internationalization/Culture.h"
#include "Internationalization/TextLocalizationManager.h"
#include "Misc/ConfigCacheIni.h"

namespace
{
	/** The engine's own place for the game language; it reads it at startup. */
	const TCHAR* LanguageSection = TEXT("Internationalization");
	const TCHAR* LanguageKey = TEXT("Culture");

	/** The language the game is written in ("en") */
	FString GetNativeLanguage()
	{
		return FTextLocalizationManager::Get().GetNativeCultureName(ELocalizedTextSourceCategory::Game);
	}

	/** The languages the game has text for (compiled .locres files) */
	TArray<FString> GetTranslatedLanguages()
	{
		const FString Native = GetNativeLanguage();
		TArray<FString> Codes = FTextLocalizationManager::Get().GetLocalizedCultureNames(ELocalizationLoadFlags::Game);
		Codes.Sort([&Native](const FString& A, const FString& B) { return (A == Native) != (B == Native) ? A == Native : A < B; });
		return Codes;
	}
}

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

void UKubeCraftUserSettings::SetLanguage(const FString& Code)
{
	Language = GetTranslatedLanguages().Contains(Code) ? Code : GetNativeLanguage();
}

void UKubeCraftUserSettings::GetLanguages(TArray<FString>& OutCodes, TArray<FText>& OutNames)
{
	OutCodes.Reset();
	OutNames.Reset();
	for (const FString& Code : GetTranslatedLanguages())
	{
		const FCulturePtr Culture = FInternationalization::Get().GetCulture(Code);
		if (!Culture.IsValid())
		{
			continue;
		}
		// Native names are lower case in some languages, a menu option starts with a capital.
		const FString Name = Culture->GetNativeName();
		const FString First = FText::AsCultureInvariant(Name.Left(1)).ToUpper().ToString();
		OutCodes.Add(Code);
		OutNames.Add(FText::AsCultureInvariant(First + Name.RightChop(1)));
	}
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
	Language = GetNativeLanguage();
}

void UKubeCraftUserSettings::LoadSettings(bool bForceReload)
{
	Super::LoadSettings(bForceReload);
	SetLanguage(GConfig->GetStr(LanguageSection, LanguageKey, GGameUserSettingsIni));
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
	ApplyLanguage();
	OnApplied.Broadcast(*this);
}

#if WITH_EDITOR
void UKubeCraftUserSettings::PostInitProperties()
{
	Super::PostInitProperties();
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		FWorldDelegates::OnPIEStarted.AddWeakLambda(this, [this](UGameInstance *)
													{ ApplyLanguage(); });
	}
}
#endif

void UKubeCraftUserSettings::ApplyLanguage() const
{
	// The engine reads this key at the next start; SaveSettings writes the file.
	GConfig->SetString(LanguageSection, LanguageKey, *Language, GGameUserSettingsIni);

#if WITH_EDITOR
	if (GIsEditor)
	{
		// Switching the culture would change the editor's own language. While playing in the editor, the engine shows
		// game text through its "game localization preview", so switch that instead. Outside Play there is no game text.
		if (FTextLocalizationManager::Get().ShouldGameLocalizationPreviewAutoEnable())
		{
			FTextLocalizationManager::Get().EnableGameLocalizationPreview(Language);
		}
		return;
	}
#endif
	FInternationalization::Get().SetCurrentCulture(Language);
}
