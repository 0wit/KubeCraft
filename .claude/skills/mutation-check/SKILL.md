---
name: mutation-check
description: Prove a KubeCraft test really catches bugs - break the code on purpose (one small change), build, run the test, see it fail, restore and rebuild. Use after writing or changing a test, when a review asks "would this test catch X?", or when the user asks for mutation checks.
---

# Mutation check

A test that passes proves little until it has been seen to **fail** on the bug it is meant to catch.

## Pick mutations
One per promise of the test - the bug a future change could really bring in. Examples from this project:
| Test | Mutation | Expected |
| --- | --- | --- |
| `Integration.ChunkStreaming` | changes not re-applied in `LoadChunk` | "Hole is still Air" fails |
| `Integration.ChunkStreaming` | neighbors not loaded before drawing | walls ("draws 436 cubes") |
| `Integration.IncrementalRedraw` | chunk borders counted as solid | cube count differs |
| `Integration.MineAndPlace` | neighbor chunk not redrawn | count off by one |

## Run one
Editor closed. From the repo root:
```bash
bash .claude/skills/mutation-check/scripts/mutate.sh Source/KubeCraft/Private/World/VoxelWorld.cpp \
  's/\t\t\tChunk->SetBlock(Change.Key, Change.Value);/\t\t\t\/\/ MUTATION/' KubeCraft.Integration.ChunkStreaming
```
The script shows the diff, builds, runs the test, then **always** restores the file and rebuilds clean (also on
Ctrl+C or a failed build). Use a timeout of ~600000 ms.

- Sources are CRLF: match text inside the line, don't anchor a pattern on `$`.
- Pick a pattern that occurs once (the script refuses an expression that changes nothing; check the diff for more).

## Judge
- Test **fails** with a clear message → good; note it ("Mutation X → fails at Y").
- Test **passes** → the test misses that bug: strengthen it (and re-run the mutation), or tell the user why it's OK.
- A **crash** instead of a fail → add an ensure/guard so a bug gives a clean fail.
- Afterwards `git diff` must show no trace of the mutation. Tell the user the results (mutation → where the test failed).
