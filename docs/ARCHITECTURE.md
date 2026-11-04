**Overview** · [World](WORLD.md#contents) · [Player](PLAYER.md#contents)

# KubeCraft – How It Works

**Start here.** This page is the overview. Two more pages go into detail:

| Read | When you want to know… |
|---|---|
| **This page** | What the pieces are, how a game starts, where files are, how to change things, the tests |
| **[World](WORLD.md#contents)** | How blocks are stored and drawn: positions, chunks, chunk borders, redrawing, terrain, height bands |
| **[Player](PLAYER.md#contents)** | What the player does and sees: mining, placing, rules, inventory, hotbar, outline, cracks |

Read this page first. Then open the page for the part you work on.
Every page has the same bar at the top and the bottom, and every section ends with a link back.

## Contents

[The idea](#the-idea) · [The pieces](#the-pieces) · [Starting a game](#starting-a-game) · [Files](#files)
· [How do I…](#how-do-i) · [Tests](#tests)

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
                            ├─ Blueprint: inventory (8 slots) ◀── mined blocks / ──▶ which block to place
                            └─ Blueprint: HUD (crosshair, hotbar), outline, cracks   (only shows, changes nothing)
```

Top half (world, chunks, terrain) → [World](WORLD.md#contents). Bottom half (player, inventory, HUD) → [Player](PLAYER.md#contents).

[↑ Contents](#contents)

## The pieces

Grouped by folder. Each folder is one **topic**, so a change usually stays in one folder.

| Folder | Piece | Job | Details |
|---|---|---|---|
| `Blocks/` – what a block is, what you may do with it | `UBlockDefinition` | One block type: material, hotbar picture, mining time, breakable or not | [World → Blocks and height](WORLD.md#blocks-and-height) |
| | `UBlockCatalog` | List of block types. Says which block the terrain puts at which height | [World → Blocks and height](WORLD.md#blocks-and-height) |
| | `FBlockRules` | Says if a cube may be mined, or placed somewhere | [Player → Rules](PLAYER.md#rules) |
| `World/` – where blocks are | `VoxelTypes.h` | Converts positions: cm ↔ cube ↔ chunk | [World → Positions](WORLD.md#positions-worldvoxeltypesh) |
| | `AVoxelWorld` | Owns all chunks. Fills them. Changes cubes and redraws | [World → Redrawing](WORLD.md#redrawing-after-a-change) |
| | `AVoxelChunk` | One piece of the world. Keeps its numbers | [World → A chunk](WORLD.md#a-chunk-avoxelchunk--rendering) |
| `Rendering/` – how blocks are drawn | `UChunkMeshComponent` | Decides which cubes of one chunk are visible, and of which type | [World → Redrawing](WORLD.md#redrawing-after-a-change) |
| | `UBlockInstancesComponent` | Draws all cubes of one type in one chunk. Adds or removes a single cube | [World → Redrawing](WORLD.md#redrawing-after-a-change) |
| `Terrain/` – how the hills are made | `FTerrainGenerator` | Says how high the ground is at one spot | [World → Terrain](WORLD.md#terrain-fterraingenerator) |
| `Player/` – the player, and what they do with blocks | `AKubeCraftCharacter` | Walk, look, jump. Passes the mine button on | [Player → Mining and placing](PLAYER.md#mining-and-placing) |
| | `ULookTargetComponent` | Which cube am I looking at? One reach, shared by mining and placing | [Player → Where the line hits](PLAYER.md#where-the-line-hits) |
| | `UMiningComponent` | Hold the button → cube breaks after its mining time. Tells others what was mined (`OnBlockMined`) | [Player → Mining and placing](PLAYER.md#mining-and-placing) |
| | `UPlacingComponent` | Puts a cube of a given type on the face you look at | [Player → Mining and placing](PLAYER.md#mining-and-placing) |
| `Game/` – start order of a game | `AKubeCraftGameMode` | Builds the world, then puts the player on it | [Starting a game](#starting-a-game) |

Not C++, but part of the game:

| Asset | Job | Details |
|---|---|---|
| `BP_KubeCraftCharacter` | Settings of the player. Graphs: outline + cracks, the place button, choosing a slot | [Player → What the player sees](PLAYER.md#what-the-player-sees-blueprint), [Mining and placing](PLAYER.md#mining-and-placing), [Number keys](PLAYER.md#number-keys) |
| `BP_InventoryComponent` | The inventory: 8 slots, which one is selected | [Player → Inventory](PLAYER.md#inventory-blueprint) |
| `S_InventorySlot` | One slot: which block + how many | [Player → Inventory](PLAYER.md#inventory-blueprint) |
| `WBP_PlayerHUD` | Everything on screen: the "+" crosshair and the row of 8 slots | [Player → HUD](PLAYER.md#hud-wbp_playerhud) |
| `WBP_InventorySlot` | Draws one slot: frame, block picture, count | [Player → HUD](PLAYER.md#hud-wbp_playerhud) |
| `M_BlockOutline` | Draws only the edges of a cube | [Player → Outline](PLAYER.md#outline-m_blockoutline) |
| `M_BlockCrack` + `T_Crack` | Draws cracks that grow with mining progress | [Player → Cracks](PLAYER.md#cracks-m_blockcrack) |
| `T_Block_*` | Picture of each block type, shown in the hotbar (plain colours for now) | [Player → HUD](PLAYER.md#hud-wbp_playerhud) |

[↑ Contents](#contents)

## Starting a game

1. The player gets a character. It waits, it doesn't move yet.
2. The game mode tells the world: "build".
3. The world picks a random **seed** (a number). It's shown in the Output Log.
4. The world makes 8 × 8 chunks and fills them with cubes.
5. Then every chunk draws itself. (Fill all first, draw second – see [WORLD.md → Chunk borders](WORLD.md#chunk-borders).)
6. The game mode puts the player on the ground in the middle.
   It lifts the player by half its height. Why? A character's position is its **middle**, not its feet.
7. The character shows the HUD (crosshair + 8 empty slots).
8. The game starts.

[↑ Contents](#contents)

## Files

```
Source/KubeCraft/Public/   .h     Blocks/, World/, Rendering/, Terrain/, Player/, Game/
Source/KubeCraft/Private/  .cpp   the same folders + Tests/
Content/KubeCraft/         Blocks/, Blueprints/ (+ Components/, Structures/), Input/, Materials/, Textures/, UI/, Maps/L_World
docs/                      ARCHITECTURE.md (this page), WORLD.md, PLAYER.md
```

- Which file is in which folder: see **The pieces** above. A class `AVoxelWorld` lives in
  `Public/World/VoxelWorld.h` + `Private/World/VoxelWorld.cpp`.
- Folders are by **topic**, not by kind of class – so there is no `Components/` folder.
  `ChunkMeshComponent` is about drawing the world → `Rendering/`. `MiningComponent` is about the player → `Player/`.
- Content folders are by kind of asset (materials, textures, UI, ...), unlike the C++ folders.
- Game rules are C++. Blueprints hold settings, the inventory and the visual feedback
  (see [Player → Inventory](PLAYER.md#inventory-blueprint) and [Player → What the player sees](PLAYER.md#what-the-player-sees-blueprint)).

[↑ Contents](#contents)

## How do I…

| I want to… | Do this |
|---|---|
| Add a block | New `BlockDefinition` asset + material + `Texture` (hotbar picture). Add it at the end of `DA_BlockCatalog`. |
| Change a block's hotbar picture | Draw a new `T_Block_*` (or set another texture in the block asset's `Texture`). |
| Change the number of slots | `SlotCount` on `InventoryComponent` in `BP_KubeCraftCharacter` + the same number of slots in `WBP_PlayerHUD` → `InventoryBar`. |
| Change which key picks which slot | `HotbarKeys` in `BP_KubeCraftCharacter` (key name → slot, 0 = first). |
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
| Change the crosshair or the hotbar look | `WBP_PlayerHUD` (crosshair, position of the bar), `WBP_InventorySlot` (one slot). |
| See how long a redraw takes | Play, open the console (`~`), type `stat KubeCraft`. "Block Redraw" = one cube, "Chunk Rebuild" = a whole chunk. |
| Run the tests | Tools → Session Frontend → Automation → tick `KubeCraft` → Start. |

[↑ Contents](#contents)

## Tests

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
| `KubeCraft.Integration.MineAndPlace` | `World/VoxelWorld` | Mining and placing really change blocks. Placed type is the given type. No type / unknown type is refused. A border change redraws the next chunk. |
| `KubeCraft.Integration.IncrementalRedraw` | `Rendering/` | After many mines and places (across chunk borders, a new block type), the cube-by-cube picture equals a full redraw of every chunk. |

"No placing where a player stands" needs a real player, so it is checked by playing, not by a test.
The inventory and the HUD are Blueprint, so they are checked by playing too.

[↑ Contents](#contents)

---

**Overview** · [World](WORLD.md#contents) · [Player](PLAYER.md#contents)
