#pragma once

#include "CoreMinimal.h"
#include "Stats/Stats.h"

DECLARE_LOG_CATEGORY_EXTERN(LogKubeCraft, Log, All);

/** Timings of the game's own code; type "stat KubeCraft" in the console to see them. */
DECLARE_STATS_GROUP(TEXT("KubeCraft"), STATGROUP_KubeCraft, STATCAT_Advanced);
