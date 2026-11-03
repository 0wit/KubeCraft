# KubeCraft – How It Works

✅ done · 🔜 planned · ⚠️ temporary

## The idea

- The world is a grid of cubes.
- A cube is **just a number**: 0 = empty, 1 = stone, 2 = grass, 3 = snow.
- The world is cut into **chunks** (16 × 16 × 64 cubes). Each chunk keeps its numbers and draws them.
- What a number means (color, mining time) is set in **data assets** in the editor.

```
Player ──mine/place──▶ World 🔜 ──▶ Chunk, Chunk, Chunk ...
                                      │
                                      └─ "what is 2?" ──▶ Block catalog ──▶ Stone, Grass, Snow
```

## The pieces

| Piece | Job | |
|---|---|---|
| `VoxelTypes.h` | "A cube is a number" + converts positions (cm ↔ cube ↔ chunk) | ✅ |
| `UBlockDefinition` | One block type: material ✅, mining time + breakable (read by mining 🔜) | ✅ |
| `UBlockCatalog` | List of block types + which block goes at which height | ✅ |
| `AVoxelChunk` | One piece of the world: stores its numbers, draws visible cubes | ✅ ⚠️ flat test slab |
| `AKubeCraftCharacter` | Player: camera, walk, look, jump | ✅ |
| `AVoxelWorld` | Owns all chunks; mine, place, limits | 🔜 |
| `FTerrainGenerator` | Hills from Perlin noise | 🔜 |
| `UBlockInteractionComponent` | Hold to mine, press to place | 🔜 |

## Why chunks?

Like pages of a road atlas: when one street changes, you reprint **one page**, not the whole book.
Mining a cube redraws only its chunk, not the whole world. Chunks out of sight aren't drawn 🔜.

## Positions (`VoxelTypes.h`)

The same spot, asked by different code:

| | Example |
|---|---|
| Unreal, in cm | (1750, -30, 420) |
| Cube in the world | (17, -1, 4) |
| Chunk | (1, -1) |
| Place in the chunk's list | 1265 |

All conversions are only in `VoxelTypes.h`.

## A chunk (`AVoxelChunk`)

Side view of a tiny chunk:

```
 . . . .    0 = Air
 3 3 3 3    3 = Snow    ← all drawn (air above)
 2 2 2 2    2 = Grass   ← only the edge ones drawn
 1 1 1 1    1 = Stone   ← middle ones are hidden → skipped
```

- `GetBlock` / `SetBlock` read and write a number.
- `RebuildMesh` draws only cubes that touch air, one instanced mesh per block type.
- `SetBlock` refuses numbers that aren't in the catalog – otherwise you'd get an invisible solid cube.

Known limits ⚠️:
- One changed cube redraws the **whole chunk**. Fine for now; made smarter if mining stutters 🔜.
- A chunk thinks everything beyond its sides is air, so its outer walls are always drawn.
  Fixed once the world can tell a chunk about its neighbors 🔜.

## Blocks and height

- Block number = place in `DA_BlockCatalog` list + 1. **Add new blocks at the end.**
- Height bands: below 3 → stone, below 5 → grass, above → snow ⚠️ (retuned with real terrain).
- Top of the world = 64 (ceiling). Bottom = 0.

## Files

```
Source/KubeCraft/Public/   .h     Voxel/, Player/
Source/KubeCraft/Private/  .cpp   Voxel/, Player/, Tests/
Content/KubeCraft/         Blocks/, Blueprints/, Input/, Materials/, Maps/L_World
```

Blueprints hold only settings. Logic is C++.

## How do I…

- **Add a block:** new `BlockDefinition` asset + material, add it at the end of `DA_BlockCatalog`.
- **Change mining time:** `MineTime` in the block asset 🔜 (no mining yet).
- **Change player size / jump:** defaults are in blocks at the top of `KubeCraftCharacter.cpp`;
  per-Blueprint overrides go on the capsule, camera and movement components of `BP_KubeCraftCharacter`.
- **Change keys:** `IMC_Default`.
- **Run the tests:** Tools → Session Frontend → Automation → tick `KubeCraft` → Start.
  `KubeCraft.Voxel.Coordinates` checks the position conversions (incl. negative positions).
