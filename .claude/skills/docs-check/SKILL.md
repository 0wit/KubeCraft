---
name: docs-check
description: Keep KubeCraft's docs/ (ARCHITECTURE, WORLD, PLAYER, MENUS) in sync with the code and in the project's style - what to update for which change, plus a script that checks table alignment, heading links, CRLF and emoji. Use after changing classes, assets, flows or tests, before session-end, or when the user edits the docs.
---

# Docs check

## What to update
| Change | Update |
| --- | --- |
| New / removed / renamed class | ARCHITECTURE "C++ Classes" row (folder, job, Details link) + the diagram if it talks to others |
| New / removed asset | ARCHITECTURE "Blueprints" row, "How do I…" rows that name it |
| New setting a user would tweak | ARCHITECTURE "How do I…" row (`BP_X` → Component → Property) |
| New test | ARCHITECTURE "Tests" row |
| Change in how blocks are stored, drawn, loaded | WORLD.md section (+ Contents line if a section is added) |
| Change in mining, placing, inventory, HUD, rules | PLAYER.md section |
| Change in menus, saving, loading, input mode | MENUS.md section |
| Start order | ARCHITECTURE "Starting a game" |

The docs describe **only the current code** - no plans or ideas, no work history, no AI tooling.

## Style
- Short and plain, for a beginner: one-sentence bullets, small examples, ASCII pictures, tables.
- The code name goes under the heading in backticks (`` `UChunkStreamingComponent` ``), not in every sentence.
- Table cells without a full stop at the end; tables aligned (pad cells with spaces).
- Every page: nav bar at top and bottom, a "Contents" line, `[↑ Contents](#contents)` at the bottom.
- No emoji status markers.

## Run the check
```bash
bash .claude/skills/docs-check/scripts/check-docs.sh
```
Reports misaligned table rows (counted in characters, so `→ – …` are fine), links to headings that don't exist
(also across pages, GitHub anchor rules), files that are not CRLF, and emoji. Fix everything it reports. After
renaming a heading, also search the other pages for links to the old anchor (the script finds them).
