# KubeCraft – How It Works

## The idea

- The world is made of cubes.
- A cube is **just a number**: 0 = air, 1 = stone, 2 = grass, 3 = snow, 4 = bedrock.
- Cubes are grouped in **chunks**. One chunk = 16 × 16 cubes, 64 high.
- What a number looks like is set in **data assets** (editor files).

```
Game mode ──1. build──▶ World ──▶ Chunk, Chunk, Chunk ...   (8 × 8 chunks)
    │                    │ │ │      │
    │                    │ │ │      ├─ "what is 2?" ──▶ Block catalog ──▶ Stone, Grass, Snow, Bedrock
    │                    │ │ │      └─ Chunk mesh ──▶ Block instances (one per type): draw the cubes
    │                    │ │ └─ "how high is the ground here?" ──▶ Terrain generator
    │                    │ └─ "may I change this cube?" ──▶ Block rules
    │                    │
    └──2. place on top──▶ Player ──▶ Mining / Placing ──mine/place──▶ World
                            │
                            └─ Blueprint: crosshair, outline, cracks   (only shows, changes nothing)
```

## The pieces

Grouped by folder. Each folder is one **topic**, so a change usually stays in one folder.

| Folder | Piece | Job |
|---|---|---|
| `Blocks/` – what a block is, what you may do with it | `UBlockDefinition` | One block type: material, mining time, breakable or not |
| | `UBlockCatalog` | List of block types. Says which block goes at which height |
| | `FBlockRules` | Says if a cube may be mined, or placed somewhere |
| `World/` – where blocks are | `VoxelTypes.h` | Converts positions: cm ↔ cube ↔ chunk |
| | `AVoxelWorld` | Owns all chunks. Fills them. Changes cubes and redraws |
| | `AVoxelChunk` | One piece of the world. Keeps its numbers |
| `Rendering/` – how blocks are drawn | `UChunkMeshComponent` | Decides which cubes of one chunk are visible, and of which type |
| | `UBlockInstancesComponent` | Draws all cubes of one type in one chunk. Adds or removes a single cube |
| `Terrain/` – how the hills are made | `FTerrainGenerator` | Says how high the ground is at one spot |
| `Player/` – the player, and what they do with blocks | `AKubeCraftCharacter` | Walk, look, jump. Passes the mine/place buttons on |
| | `ULookTargetComponent` | Which cube am I looking at? One reach, shared by mining and placing |
| | `UMiningComponent` | Hold the button → cube breaks after its mining time |
| | `UPlacingComponent` | Press the button → a cube appears on the face you look at |
| `Game/` – start order of a game | `AKubeCraftGameMode` | Builds the world, then puts the player on it |

Not C++, but part of the game:

| Asset | Job |
|---|---|
| `BP_KubeCraftCharacter` | Settings of the player + the graphs that move the outline and the cracks |
| `M_BlockOutline` | Draws only the edges of a cube |
| `M_BlockCrack` + `T_Crack` | Draws cracks that grow with mining progress |
| `WBP_Crosshair` | A "+" in the middle of the screen |

## Starting a game

1. The player gets a character. It waits, it doesn't move yet.
2. The game mode tells the world: "build".
3. The world picks a random **seed** (a number). It's shown in the Output Log.
4. The world makes 8 × 8 chunks and fills them with cubes.
5. Then every chunk draws itself. (Fill all first, draw second – see "Chunk borders".)
6. The game mode puts the player on the ground in the middle.
   It lifts the player by half its height. Why? A character's position is its **middle**, not its feet.
7. The character shows the crosshair.
8. The game starts.

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

So the work is split into pieces of the same size.
Each chunk has its own list of numbers and its own picture.
At the start, every chunk draws all its cubes once.
After that, a change redraws only **7 cubes** – see "Redrawing after a change".

## Positions (`World/VoxelTypes.h`)

One spot, four ways to say it:

| | Example |
|---|---|
| Unreal, in cm | (1750, -30, 420) |
| Cube in the world | (17, -1, 4) |
| Chunk | (1, -1) |
| Place in the chunk's list | 1265 |

All conversions live only in `VoxelTypes.h`.

## A chunk (`AVoxelChunk` + `Rendering/`)

Side view of a tiny chunk:

```
 . . . .    0 = Air
 3 3 3 3    3 = Snow    ← drawn (air above)
 2 2 2 2    2 = Grass   ← only the edge ones drawn
 1 1 1 1    1 = Stone   ← middle ones hidden → not drawn
 4 4 4 4    4 = Bedrock ← only the edge ones drawn (nothing sees from below)
```

- The chunk keeps the numbers. `GetBlock` reads one. `SetBlock` writes one.
- `SetBlock` refuses unknown numbers. (They'd be invisible but solid.) It returns false then,
  and also when the number is already there. Nothing changed, so nothing is redrawn.
- The chunk's mesh component draws only cubes that touch air.
- It has one **block instances** component per block type. Each draws all cubes of its type as copies
  ("instances") of one cube mesh, in one draw call. Each type has its own material.
- The chunk is the **list**. `Rendering/` is the **picture** of the list.

### Chunk borders

A cube at the edge of a chunk has a neighbor in the **next** chunk.
So the chunk asks the world: "Is there a cube?" (`GetBlockOrNeighbor`)

```
 chunk (0,0)  │  chunk (1,0)
  … 14  15    │   0  1 …
         ▲
  cube 15: "What's on my right?" → not mine → ask the world → "a cube" → don't draw that side
```

- That's why all chunks are filled **before** any is drawn.
  Otherwise the neighbor would still be empty.
- At the edge of the world there's no neighbor → "air" → the outer wall is drawn.
- Mining cube 15 opens a hole next to cube 0 of the next chunk.
  So the world also redraws cube 0 – in the next chunk.

Known limits:
- The world has a fixed size (128 × 128 cubes). You can walk off the edge.

## Redrawing after a change

A changed cube can only affect itself and the 6 cubes touching its faces.
So `AVoxelWorld` redraws exactly those 7. Each one goes to the chunk that holds it.

```
        above
  left  CUBE  right      + front and back
        below
```

For one cube, the mesh component asks: "Should a cube show here, and which type?"

| Cell is… | Shown? |
|---|---|
| Air | No |
| Solid, touches air on some side | Yes, its type |
| Solid, buried on all 6 sides | No |

Then it fixes the picture: removes the cube from the wrong type, adds it to the right one. If nothing changed, nothing happens.

**How one cube is removed (`UBlockInstancesComponent`).** The engine keeps the cubes in a list.
Removing one moves the **last** cube into the gap (so nothing else has to shift).
The component keeps a note "which cell is at which place in the list" and fixes it:

```
list:  [A B C D]     remove B  →  [A D C]
note:  D is at 3                  D is at 1   ← the only fix
```

The note must always match the engine's list. So change cubes only through the component (`SetBlocks`,
`AddBlock`, `RemoveBlock`). If the note and the list ever disagree, the component changes nothing and
raises an **ensure** (an error in the log; the game keeps running). Otherwise it could remove the wrong cube.

- At the start, a chunk still draws everything at once (`Rebuild`): faster than one cube at a time.
- Cost: type `stat KubeCraft` in the game console. "Block Redraw" = one cube, "Chunk Rebuild" = a whole chunk.
  Read the **Max** column – a redraw happens in one frame only.

## Mining and placing

```
Left mouse held → Mining component, every frame (a stopwatch):
   1. "Which cube am I looking at?"  (LookTarget component: a line from the camera, max 5 cubes)
   2. Same cube as last frame? → add the time. Other cube → start at 0.
   3. Time ≥ the cube's MineTime → World.MineBlock → the cube becomes air
Left mouse released → stopwatch off and back to 0

Right mouse pressed → Placing component, once:
   1. "Which face am I looking at?" → the empty cell in front of it
   2. World.PlaceBlock → the cell gets the block for its height
```

- Mining times: stone 1.5 s, grass 0.75 s, snow 0.3 s. Bedrock can't be mined.
- The mining component only runs while the button is held. The rest of the time it costs nothing.
- The world asks `FBlockRules` before **every** change. The components can't skip it.

| Rule | Why |
|---|---|
| Bedrock (`bBreakable` off) can't be mined | The floor. You can't dig out of the world. |
| No placing at height 64 or more | The ceiling. |
| No placing outside the 8 × 8 chunks | There is no chunk to hold the cube. |
| No placing where a player stands | You'd be stuck inside the cube. |

Where does the line hit? Exactly **on** the surface – the border between the cube and the air in front of it.
So the code moves half a cube away from the border, into the middle of one cell:

```
            air cell  │  cube
  camera ─────────────● hit point (on the border)
          ◀── 50 cm ──┼── 50 cm ──▶
          place here  │  mine this
      (HitToFaceNeighbor)   (HitToBlock)
```

## What the player sees (Blueprint)

The rules are C++. The **feedback** is Blueprint + materials, in `BP_KubeCraftCharacter`.
The Blueprint only reads. It never changes a block.

C++ gives the Blueprint two functions:

| Function | On | Gives |
|---|---|---|
| `FindLookedAtBlockCenter` | LookTarget component | Found? + centre of the block you look at (cm) |
| `GetMiningProgress` | Mining component | Mining? + centre of the mined block + progress 0..1 |

Every frame (Event Tick):

```
FindLookedAtBlockCenter ──▶ outline cube: visible if found, move to the centre
GetMiningProgress       ──▶ crack cube:   visible if mining, move to the centre, set "Progress"
```

- The character ticks **after** its mining component. So the cracks show this frame's progress, not the last one.
- Outline and cracks are **extra cubes** on top of the block, a bit bigger (1.01 and 1.005). Two sizes so they don't flicker into each other.
- Why not change the block itself? All stone cubes of a chunk are **one** mesh. Changing its material would crack all of them.
- Both cubes have **no collision**. They are only pictures: nothing should bump into them or stand on them.
  (The look trace would skip them anyway – it ignores the player's own actor.)
- Bedrock: outline yes (you can look at it), cracks no (it can't be mined).

**Outline (`M_BlockOutline`).** Every cube face has coordinates 0..1. The material shows a pixel only near the border:

```
distance from face centre = max(|U − 0.5|, |V − 0.5|)     0 in the middle, 0.5 at the edge
shown if distance ≥ 0.48
```

**Cracks (`M_BlockCrack`).** `T_Crack` is a grayscale picture. A pixel's gray value says **when** it appears:

```
gray 0.1 → appears at 10 % progress      white → never
shown if Progress ≥ gray value
```

**Crosshair (`WBP_Crosshair`).** A widget with a "+" text, anchored to the screen centre. The look trace also goes through the screen centre, so the "+" and the outline agree.

## Blocks and height

- Block number = place in the `DA_BlockCatalog` list + 1.
- **Add new blocks at the end** of the list.
- Below 1 → bedrock. Below 16 → stone. Below 26 → grass. Above → snow.
- Example: a column 26 high = bedrock at 0, stone at 1..15, grass at 16..25.
- A placed cube follows the same rule: placed at height 20 → grass.
- Bottom of the world = 0. Top = 64.

## Files

```
Source/KubeCraft/Public/   .h     Blocks/, World/, Rendering/, Terrain/, Player/, Game/
Source/KubeCraft/Private/  .cpp   the same folders + Tests/
Content/KubeCraft/         Blocks/, Blueprints/, Input/, Materials/, Textures/, UI/, Maps/L_World
```

- Which file is in which folder: see **The pieces** above. A class `AVoxelWorld` lives in
  `Public/World/VoxelWorld.h` + `Private/World/VoxelWorld.cpp`.
- Folders are by **topic**, not by kind of class – so there is no `Components/` folder.
  `ChunkMeshComponent` is about drawing the world → `Rendering/`. `MiningComponent` is about the player → `Player/`.
- Game rules are C++. Blueprints hold settings and the visual feedback (see "What the player sees").

## How do I…

| I want to… | Do this |
|---|---|
| Add a block | New `BlockDefinition` asset + material. Add it at the end of `DA_BlockCatalog`. |
| Change mining time | `MineTime` in the block asset. |
| Make a block unbreakable | Turn off `bBreakable` in the block asset. |
| Change how far the player reaches | `BP_KubeCraftCharacter` → LookTarget → Reach. |
| Change a mining/placing limit | `Blocks/BlockRules.cpp`. |
| Change player size / jump | Numbers at the top of `Player/KubeCraftCharacter.cpp`. |
| Change the hills | `BP_VoxelWorld` → Terrain. |
| Change the world size | `BP_VoxelWorld` → WorldSizeInChunks. |
| Get the same world again | Copy the seed from the Output Log into `BP_VoxelWorld` → Seed. Turn off `bRandomSeed`. |
| Change keys | `IMC_Default`. |
| Make the outline thicker / another colour | `M_BlockOutline`: lower the 0.48 in **Step** / change the colour node. |
| Change the cracks | Draw a new `T_Crack` (gray = when a pixel appears). |
| Change the crosshair | `WBP_Crosshair`. |
| See how long a redraw takes | Play, open the console (`~`), type `stat KubeCraft`. "Block Redraw" = one cube, "Chunk Rebuild" = a whole chunk. |
| Run the tests | Tools → Session Frontend → Automation → tick `KubeCraft` → Start. |

Tests live in `Private/Tests/`. There are two kinds:
- **Unit tests** check one piece alone, with no world. Fast. Names follow the folders: `KubeCraft.<Folder>.<What>`.
- **Integration tests** need real chunks, so they build a small temporary world first (`TestVoxelWorld.h`).
  It has 2 × 2 chunks and flat ground: bedrock at height 0, stone at 1–3. Names: `KubeCraft.Integration.<What>`.

| Test | Tests code in | Checks |
|---|---|---|
| `KubeCraft.World.Coordinates` | `World/VoxelTypes.h` | Position conversions, also negative ones. Which cube a hit means. |
| `KubeCraft.Blocks.HeightBands` | `Blocks/BlockCatalog` | Which block goes at which height |
| `KubeCraft.Terrain.Generator` | `Terrain/TerrainGenerator` | Same seed → same hills. Other seed → other hills. Height stays in the world. |
| `KubeCraft.Integration.BlockRules` | `Blocks/BlockRules` | What may be mined. Where a block may be placed (floor, ceiling, edges, solid cells). |
| `KubeCraft.Integration.MineAndPlace` | `World/VoxelWorld` | Mining and placing really change blocks. Placed type follows the height. A border change redraws the next chunk. |
| `KubeCraft.Integration.IncrementalRedraw` | `Rendering/` | After many mines and places (across chunk borders, a new block type), the cube-by-cube picture equals a full redraw of every chunk. |

"No placing where a player stands" needs a real player, so it is checked by playing, not by a test.
