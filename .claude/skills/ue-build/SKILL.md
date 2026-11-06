---
name: ue-build
description: Build the KubeCraft Unreal C++ project from the command line (UnrealBuildTool), regenerate project files, and run automation tests. Use after changing C++ code, adding/removing source files, or when asked to compile/build/test.
---

# Build KubeCraft (UE 5.8, Windows)

Engine: `C:/Program Files/Epic Games/UE_5.8`
Project: `KubeCraft.uproject` in the repo root. The commands below run from the repo root in Git Bash and use
`ROOT="$(git rev-parse --show-toplevel)"` (gives a Windows-style path like `D:/git/KubeCraft`, which UBT needs).

## Important: editor open?
If the Unreal Editor has the project open, a CLI build of `KubeCraftEditor` fails
("Unable to build while Live Coding is active" or locked DLLs). Then either:
- ask the user to press **Ctrl+Alt+F11** (Live Coding) in the editor – fine for .cpp-only changes, or
- close the editor and build from CLI.
New/removed `UCLASS`/`UPROPERTY`/header changes → prefer closing the editor and doing a full build
(Live Coding can't reliably handle header/reflection changes).

To close it safely (shows its save dialog) - stop PIE first, then (PowerShell):
```powershell
Get-Process UnrealEditor -ErrorAction SilentlyContinue | ForEach-Object { $_.CloseMainWindow() | Out-Null }
$p = Get-Process UnrealEditor -ErrorAction SilentlyContinue; if ($p) { $p.WaitForExit(60000) | Out-Null }
if (Get-Process UnrealEditor -ErrorAction SilentlyContinue) { "still running" } else { "closed" }
```
"still running" = a save dialog is waiting for the user - tell them. Reopen after the build (see `run-game`).

## Editor build (most common)
Run via the Bash tool:
```bash
ROOT="$(git rev-parse --show-toplevel)"
"/c/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat" \
  KubeCraftEditor Win64 Development \
  -Project="$ROOT/KubeCraft.uproject" -WaitMutex -NoHotReloadFromIDE
```
Use a timeout of ~600000 ms. Check the tail of the output for `Result: Succeeded` / `error C`/`error LNK`.

## After adding/removing source files
Regenerate project files (optional for UBT, needed for Visual Studio/Rider to see them):
```bash
"/c/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat" \
  -projectfiles -project="$ROOT/KubeCraft.uproject" -game -engine
```

## Automation tests (headless)
```bash
"/c/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" \
  "$ROOT/KubeCraft.uproject" \
  -ExecCmds="Automation RunTests KubeCraft; Quit" -unattended -nopause -nullrhi -nosplash \
  -log -stdout
```
Look for `Test Completed. Result={Success}` / `{Fail}` lines. Tests live in
`Source/KubeCraft/Private/Tests/` and use the `KubeCraft.` test name prefix.
Short output: `| grep -E "Test Completed|LogAutomationController: Error|Ensure"`.
One test only: `RunTests KubeCraft.Integration.ChunkStreaming`. The headless run needs the editor closed too
(it loads the same DLLs); with the editor open use `AutomationTestToolset` via MCP instead.

## Line endings
Sources and docs are CRLF in the working copy. Files written by tools come out LF → convert before `git add`:
`sed -i 's/\r$//; s/$/\r/' <file>` (check with `file <file>`). Use the Edit tool for small changes - sed/perl
patterns that end in `\n` don't match CRLF lines. Exception: files under `.claude/` stay LF (`.gitattributes`).

## Logs
Editor/PIE logs: `Saved/Logs/KubeCraft.log`.
