---
name: localize
description: Update KubeCraft's translations (Czech) headless - gather new/changed text, export the .po, fill in translations, import and compile the .locres - without clicking through the Localization Dashboard. Use after adding or changing any player-facing text, or when the user asks to translate / add a language.
---

# Localize KubeCraft

Target **Game** (Dashboard settings in `Config/DefaultEditor.ini`, steps in `Config/Localization/Game_*.ini`,
written the same way the Dashboard writes them). Data: `Content/Localization/Game/<culture>/Game.po|.archive|.locres`.

## Steps (editor closed - the commandlets load the same DLLs)
```bash
for s in Gather Export; do "/c/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" \
  "$(git rev-parse --show-toplevel)/KubeCraft.uproject" -run=GatherText -config="Config/Localization/Game_$s.ini" \
  -unattended -nopause -nullrhi -nosplash -log=$s.log -stdout 2>&1 | grep -E "exit code|Error"; done
```
1. Gather + Export (above). New texts get `msgstr ""` in `cs/Game.po`.
2. Translate: add the English → Czech pairs to the table in `scripts/fill-po.js`, then
   `node .claude/skills/localize/scripts/fill-po.js Content/Localization/Game/cs/Game.po` (prints what is missing).
   Write the script to `$TEMP` first if bash heredocs mangle backslashes.
3. Import + Compile: same loop with `Import Compile`.
4. Check: `iconv -f UTF-16 -t UTF-8 Content/Localization/Game/Game.manifest | grep '"Text"'` lists all gathered texts.

## Gotchas
- Only **Text** is gathered. A Blueprint **String** variable/literal (e.g. a default name) is not - make it Text.
- Text joined with String Append can't be translated - use Format Text (`Seed {Seed}`).
- `#if WITH_EDITOR` LOCTEXTs are skipped (`ShouldGatherFromEditorOnlyData=false`).
- Engine "GameFeatureData" errors in the commandlet log come from the AllToolsets plugin - ignore.
- Editor Play: `UKubeCraftUserSettings` switches the game localization preview at PIE start (OnPIEStarted) and on
  Apply - never SetCurrentCulture in the editor (it would change the editor's own language).
- Kept English on purpose: "Seed" (msgstr = msgid) and the default world name "New World" (a String).
- Check in Play without the user: SlateInspectorToolset Snapshot/Click reaches UMG buttons (bring the level tab
  to front first: Click its `tab "L_MainMenu"` ref).
- New language: add the culture to `SupportedCulturesStatistics` (DefaultEditor.ini), `CulturesToGenerate` in all
  four `Game_*.ini`, and `+CulturesToStage` in DefaultGame.ini.
