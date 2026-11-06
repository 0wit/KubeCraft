---
name: run-game
description: Run KubeCraft and drive it - start the Unreal editor, play in the viewport (PIE) via MCP, move the player, take screenshots of the editor window and check the log. Use when the user says "run the app/game", "play it", "show me", or a change must be seen working in the real game (not just tests).
---

# Run and drive KubeCraft

The game runs inside the editor (PIE). MCP can start/stop PIE and move actors, but **cannot send keys or mouse
input** - mining, placing, walking and the hotbar are still play-tested by the user.

## 1. Editor up, MCP up
- Running? `tasklist //FI "IMAGENAME eq UnrealEditor.exe"`.
- C++ changed since the last build → build first (`ue-build`), with the editor closed.
- Start it (PowerShell): `Start-Process "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" -ArgumentList "`"$(Resolve-Path KubeCraft.uproject)`""` (from the repo root)
- Wait: `bash .claude/skills/run-game/scripts/wait-mcp.sh`. The first MCP call after start can still time out
  while assets load - just retry. If the `mcp__unreal__*` tools stay unavailable, ask the user to run `/mcp`.

## 2. Play
```
call_tool EditorToolset.EditorAppToolset StartPIE
  {"options": {"bSimulate": false, "playMode": "PlayMode_InViewPort", "warmupSeconds": 3}}
```
- Player pawn: `/Game/KubeCraft/Maps/UEDPIE_0_L_World.L_World:PersistentLevel.BP_KubeCraftCharacter_C_0`
- Where is it: `editor_toolset.toolsets.actor.ActorTools` `get_actor_transform {"actor": {"refPath": ...}}`
- Move it: `set_actor_transform {"actor": {...}, "xform": {"location": {"x":..,"y":..,"z":..}}}` (cm; 1 block = 100,
  1 chunk = 1600). Drop it from high up (z ≈ 6000) and read z again after a few seconds: standing on ground ≈ 1000–6000,
  far below 0 = fell through the world.
- The editor runs slowly (≈ 3 fps) while it is in the background - give streaming time before judging.
- Stop: `StopPIE {}`.

## 3. Look at it
```
powershell -NoProfile -ExecutionPolicy Bypass -File ".claude\skills\run-game\scripts\capture-editor.ps1" -Out "<scratchpad>\pie.png" -Scale 0.5
```
then Read the PNG. **Only capture the editor window** (this script) - a full-screen grab can show the user's other,
private windows. `CaptureViewport` / Slate `Screenshot` don't work during PIE (editor world / empty image).
A blank or sky-only frame means the game did not start - say so.

## 4. Check the log
`Saved/Logs/KubeCraft.log`, from the last `World seed:` line on:
`sed -n '/World seed/,$p' Saved/Logs/KubeCraft.log | grep -iE "error|ensure|warning|LogKubeCraft" | grep -v ModelContextProtocol`
- `LogUtils: Error: The Editor is currently in a play mode` / `GetCurrentLevel ... no world` come from MCP actor
  tools during PIE, not from the game.

## 5. Report
What you saw (screenshots by full scratchpad path), positions, log result - and the list of things the user must
still test by hand (input, feel, hitches with `stat KubeCraft`).
