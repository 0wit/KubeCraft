# KubeCraft – How It Works

✅ done · 🔜 planned · ⚠️ temporary

## The idea

- The world is made of cubes.
- A cube is **just a number**: 0 = air, 1 = stone, 2 = grass, 3 = snow.
- Cubes are grouped in **chunks**. One chunk = 16 × 16 cubes, 64 high.
- What a number looks like is set in **data assets** (editor files).

```
Game mode ──1. build──▶ World ──▶ Chunk, Chunk, Chunk ...   (8 × 8 chunks)
    │                    │  │       │
    │                    │  │       └─ "what is 2?" ──▶ Block catalog ──▶ Stone, Grass, Snow
    │                    │  └─ "how high is the ground here?" ──▶ Terrain generator
    └──2. place on top──▶ Player ──mine/place 🔜──▶ World
```

## The pieces

| Piece | Job | |
|---|---|---|
| `VoxelTypes.h` | Converts positions: cm ↔ cube ↔ chunk | ✅ |
| `UBlockDefinition` | One block type: its material. Mining time 🔜 | ✅ |
| `UBlockCatalog` | List of block types. Says which block goes at which height | ✅ |
| `FTerrainGenerator` | Says how high the ground is at one spot | ✅ |
| `AVoxelWorld` | Owns all chunks. Fills them. Mine/place 🔜 | ✅ |
| `AVoxelChunk` | One piece of the world. Keeps its numbers, draws its cubes | ✅ |
| `AKubeCraftGameMode` | Builds the world, then puts the player on it | ✅ |
| `AKubeCraftCharacter` | The player: walk, look, jump | ✅ |
| `UBlockInteractionComponent` | Hold to mine, press to place | 🔜 |

## Starting a game

1. The player gets a character. It waits, it doesn't move yet.
2. The game mode tells the world: "build".
3. The world picks a random **seed** (a number). It's shown in the Output Log.
4. The world makes 8 × 8 chunks and fills them with cubes.
5. Then every chunk draws itself. (Fill all first, draw second – see "Chunk borders".)
6. The game mode puts the player on the ground in the middle.
   It lifts the player by half its height. Why? A character's position is its **middle**, not its feet.
7. The game starts.

## Terrain (`FTerrainGenerator`)

- **Perlin noise** gives a smooth random value (-1 to 1) for any (X, Y). Nearby spots get similar values.
- The **seed** shifts where the noise is sampled. New seed → new terrain. Same seed → same terrain.
- **Octaves:** the noise is added several times, each time finer and weaker. Result: big hills + small detail.
- The value becomes a height:

```
height = 20 + noise × 24
noise 0.25  →  20 + 6 = 26 cubes high
```

- Height is always between 1 and 63. So there's always a floor and room to build.
- To change the hills: `BP_VoxelWorld` → **Terrain**.

## Why chunks?

Like pages in a map book.
One street changes → you reprint **one page**, not the whole book.
Mining a cube redraws only its chunk.

## Positions (`VoxelTypes.h`)

One spot, four ways to say it:

| | Example |
|---|---|
| Unreal, in cm | (1750, -30, 420) |
| Cube in the world | (17, -1, 4) |
| Chunk | (1, -1) |
| Place in the chunk's list | 1265 |

All conversions live only in `VoxelTypes.h`.

## A chunk (`AVoxelChunk`)

Side view of a tiny chunk:

```
 . . . .    0 = Air
 3 3 3 3    3 = Snow    ← drawn (air above)
 2 2 2 2    2 = Grass   ← only the edge ones drawn
 1 1 1 1    1 = Stone   ← middle ones hidden → not drawn
```

- `GetBlock` reads a number. `SetBlock` writes one.
- `RebuildMesh` draws only cubes that touch air.
- `SetBlock` refuses unknown numbers. (They'd be invisible but solid.)

### Chunk borders

A cube at the edge of a chunk has a neighbor in the **next** chunk.
So the chunk asks the world: "Is there a cube?"

```
 chunk (0,0)  │  chunk (1,0)
  … 14  15    │   0  1 …
         ▲
  cube 15: "What's on my right?" → not mine → ask the world → "a cube" → don't draw that side
```

- That's why all chunks are filled **before** any is drawn.
  Otherwise the neighbor would still be empty.
- At the edge of the world there's no neighbor → "air" → the outer wall is drawn.

Known limits ⚠️:
- One changed cube redraws the **whole chunk**. Fine for now 🔜.
- The world has a fixed size (128 × 128 cubes). You can walk off the edge.

## Blocks and height

- Block number = place in the `DA_BlockCatalog` list + 1.
- **Add new blocks at the end** of the list.
- Below 16 → stone. Below 26 → grass. Above → snow.
- Example: a column 26 high = stone at 0..15, grass at 16..25.
- Bottom of the world = 0. Top = 64.

## Files

```
Source/KubeCraft/Public/   .h     Game/, Voxel/, Player/
Source/KubeCraft/Private/  .cpp   Game/, Voxel/, Player/, Tests/
Content/KubeCraft/         Blocks/, Blueprints/, Input/, Materials/, Maps/L_World
```

Blueprints hold only settings. All logic is C++.

## How do I…

| I want to… | Do this |
|---|---|
| Add a block | New `BlockDefinition` asset + material. Add it at the end of `DA_BlockCatalog`. |
| Change mining time 🔜 | `MineTime` in the block asset. |
| Change player size / jump | Numbers at the top of `KubeCraftCharacter.cpp`. |
| Change the hills | `BP_VoxelWorld` → Terrain. |
| Change the world size | `BP_VoxelWorld` → WorldSizeInChunks. |
| Get the same world again | Copy the seed from the Output Log into `BP_VoxelWorld` → Seed. Turn off `bRandomSeed`. |
| Change keys | `IMC_Default`. |
| Run the tests | Tools → Session Frontend → Automation → tick `KubeCraft` → Start. |

| Test | Checks |
|---|---|
| `KubeCraft.Voxel.Coordinates` | Position conversions, also negative ones |
| `KubeCraft.Voxel.HeightBands` | Which block goes at which height |
| `KubeCraft.Voxel.Terrain` | Same seed → same hills. Other seed → other hills. Height stays in the world. |
