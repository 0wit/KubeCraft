**Overview** · [World](WORLD.md#contents) · [Player](PLAYER.md#contents)

# KubeCraft – How It Works

| Read                             | When you want to know…                                                              |
| -------------------------------- | ----------------------------------------------------------------------------------- |
| **This page**                    | Classes definition, how a game starts, where files are, how to change things, tests |
| **[World](WORLD.md#contents)**   | How blocks are stored and drawn                                                     |
| **[Player](PLAYER.md#contents)** | What the player does and sees                                                       |

## Contents

[World](#world) · [C++ Classes](#c-classes) · [Blueprints](#blueprints) · [Starting a game](#starting-a-game)
· [Files](#files) · [How do I…](#how-do-i) · [Tests](#tests)

## World

- The world is made of cubes.
- A cube is a number: 0 = air, 1 = stone, 2 = grass, 3 = snow, 4 = bedrock.
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

## C++ Classes

| Folder                                               | Class                      | Job                                                                                                 | Details                                                     |
| ---------------------------------------------------- | -------------------------- | --------------------------------------------------------------------------------------------------- | ----------------------------------------------------------- |
| `Blocks/` – what a block is and what to do with it   | `UBlockDefinition`         | Material, hotbar picture, mining time, breakable information                                        | [World → Blocks and height](WORLD.md#blocks-and-height)     |
|                                                      | `UBlockCatalog`            | List of block types. Says which block the terrain puts at which height                              | [World → Blocks and height](WORLD.md#blocks-and-height)     |
|                                                      | `FBlockRules`              | Says if a cube may be mined, or placed somewhere                                                    | [Player → Rules](PLAYER.md#rules)                           |
| `World/` – where blocks are                          | `VoxelTypes.h`             | Converts positions: cm ↔ cube ↔ chunk                                                               | [World → Positions](WORLD.md#positions)                     |
|                                                      | `AVoxelWorld`              | Owns all chunks. Fills them. Changes cubes and redraws                                              | [World → Redrawing](WORLD.md#redrawing-after-a-change)      |
|                                                      | `AVoxelChunk`              | One piece of the world. Keeps its numbers                                                           | [World → Chunk](WORLD.md#chunk)                             |
| `Rendering/` – how blocks are drawn                  | `UChunkMeshComponent`      | Decides which cubes of one chunk are visible, and of which type                                     | [World → Redrawing](WORLD.md#redrawing-after-a-change)      |
|                                                      | `UBlockInstancesComponent` | Draws all cubes of one type in one chunk. Adds or removes a single cube, knows which cells it draws | [World → Redrawing](WORLD.md#redrawing-after-a-change)      |
| `Terrain/` – how the hills are made                  | `FTerrainGenerator`        | Says how high the ground is at one spot                                                             | [World → Terrain](WORLD.md#terrain)                         |
| `Player/` – the player, and what they do with blocks | `AKubeCraftCharacter`      | Walk, look, jump                                                                                    | [Player → Mining and placing](PLAYER.md#mining-and-placing) |
|                                                      | `ULookTargetComponent`     | Which cube am I looking at?                                                                         | [Player → Mining and placing](PLAYER.md#mining-and-placing) |
|                                                      | `UMiningComponent`         | Mines cube after its mining time                                                                    | [Player → Mining and placing](PLAYER.md#mining-and-placing) |
|                                                      | `UPlacingComponent`        | Places cube on the face you look at                                                                 | [Player → Mining and placing](PLAYER.md#mining-and-placing) |
| `Game/` – start order of a game                      | `AKubeCraftGameMode`       | Builds the world and puts the player on it                                                          | [Starting a game](#starting-a-game)                         |

## Blueprints

| Asset                      | Job                                                              | Details                                                                      |
| -------------------------- | ---------------------------------------------------------------- | ---------------------------------------------------------------------------- |
| `BP_KubeCraftCharacter`    | Settings of the player. Outline + mining cracks, choosing a slot | [Player → What the player sees](PLAYER.md#what-the-player-sees-in-the-world) |
| `BP_InventoryComponent`    | Inventory                                                        | [Player → Inventory](PLAYER.md#inventory-blueprint)                          |
| `S_InventorySlot`          | Which block + how many of it                                     | [Player → Inventory](PLAYER.md#inventory-blueprint)                          |
| `WBP_PlayerHUD`            | Everything on screen                                             | [Player → HUD](PLAYER.md#hud)                                                |
| `WBP_InventorySlot`        | Frame, block picture, count                                      | [Player → HUD](PLAYER.md#hud)                                                |
| `M_BlockOutline`           | Edges of a cube material                                         | [Player → What the player sees](PLAYER.md#what-the-player-sees-in-the-world) |
| `M_BlockCrack` + `T_Crack` | Cracks that grow with mining progress                            | [Player → What the player sees](PLAYER.md#what-the-player-sees-in-the-world) |
| `T_Block_*`                | Picture of each block type                                       | [Player → HUD](PLAYER.md#hud)                                                |

## Starting a game

1. The game mode tells the world to build. The world picks a random **seed** (shown in the Output Log),
   fills 8 × 8 chunks, then draws them.
2. The game mode puts the player on the ground in the middle.
3. The character shows the HUD.

## Files

```
Source/KubeCraft/Public/   .h     Blocks/, World/, Rendering/, Terrain/, Player/, Game/
Source/KubeCraft/Private/  .cpp   the same folders + Tests/
Content/KubeCraft/         Blocks/, Blueprints/ (+ Components/, Structures/), Input/, Materials/, Textures/, UI/, Maps/L_World
docs/                      ARCHITECTURE.md (this page), WORLD.md, PLAYER.md
```

## How do I…

| I want to…                                | Do this                                                                                      |
| ----------------------------------------- | -------------------------------------------------------------------------------------------- |
| Add a block                               | Make a new block asset with a material and a picture. Add it to the end of `DA_BlockCatalog` |
| Change a block's hotbar picture           | Draw a new `T_Block_*` (or set another texture in the block asset's `Texture`)               |
| Change the number of slots                | `SlotCount` on `InventoryComponent` in `BP_KubeCraftCharacter`                               |
| Change which key picks which slot         | `HotbarKeys` in `BP_KubeCraftCharacter` (key name → slot, 0 = first)                         |
| Change mining time                        | `MineTime` in the block asset                                                                |
| Make a block unbreakable                  | Turn off `bBreakable` in the block asset                                                     |
| Change how far the player reaches         | `BP_KubeCraftCharacter` → LookTarget → Reach                                                 |
| Change a mining/placing limit             | `Blocks/BlockRules.cpp`                                                                      |
| Change player size / jump                 | Numbers at the top of `Player/KubeCraftCharacter.cpp`                                        |
| Change the hills                          | `BP_VoxelWorld` → Terrain                                                                    |
| Change the world size                     | `BP_VoxelWorld` → WorldSizeInChunks                                                          |
| Get the same world again                  | Copy the seed from the Output Log into `BP_VoxelWorld` → Seed. Turn off `bRandomSeed`        |
| Change keys                               | `IMC_Default`                                                                                |
| Make the outline thicker / another colour | `M_BlockOutline`: lower the 0.48 in **Step** / change the colour node                        |
| Change the cracks                         | Draw a new `T_Crack` (gray = when a pixel appears)                                           |
| Change the crosshair or the hotbar look   | `WBP_PlayerHUD` (crosshair, position of the bar), `WBP_InventorySlot` (one slot)             |
| See how long a redraw takes               | While playing, type `stat KubeCraft` in the console (`~`)                                    |
| Run the tests                             | Tools → Session Frontend → Automation → `KubeCraft` → Start                                  |

## Tests

`Private/Tests/`:

- **Unit tests** check one piece alone, with no world.
- **Integration tests** need real chunks. A small temporary world is built (`TestVoxelWorld.h`).

| Test                                      | Tests code in              | Checks                                                                                                      |
| ----------------------------------------- | -------------------------- | ----------------------------------------------------------------------------------------------------------- |
| `KubeCraft.World.Coordinates`             | `World/VoxelTypes.h`       | Position conversions, also negative ones. Which cube a hit means                                            |
| `KubeCraft.Blocks.HeightBands`            | `Blocks/BlockCatalog`      | Which block goes at which height                                                                            |
| `KubeCraft.Terrain.Generator`             | `Terrain/TerrainGenerator` | Same seed → same hills. Other seed → other hills                                                            |
| `KubeCraft.Integration.BlockRules`        | `Blocks/BlockRules`        | What may be mined, where a block may be placed                                                              |
| `KubeCraft.Integration.MineAndPlace`      | `World/VoxelWorld`         | Mining and placing can change blocks. Placed type is the given type. A border change redraws the next chunk |
| `KubeCraft.Integration.IncrementalRedraw` | `Rendering/`               | After many changes, only cubes that touch air are drawn, each with its right type                           |

[↑ Contents](#contents)

---

**Overview** · [World](WORLD.md#contents) · [Player](PLAYER.md#contents)
