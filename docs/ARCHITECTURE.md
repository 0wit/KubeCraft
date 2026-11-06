**Overview** · [World](WORLD.md#contents) · [Player](PLAYER.md#contents) · [Menus](MENUS.md#contents)

# KubeCraft – How It Works

| Read                             | When you want to know…                                                              |
| -------------------------------- | ----------------------------------------------------------------------------------- |
| **This page**                    | Classes definition, how a game starts, where files are, how to change things, tests |
| **[World](WORLD.md#contents)**   | How blocks are stored and drawn                                                     |
| **[Player](PLAYER.md#contents)** | What the player does and sees                                                       |
| **[Menus](MENUS.md#contents)**   | Main menu, pause menu, saving and loading worlds                                    |

## Contents

[World](#world) · [C++ Classes](#c-classes) · [Blueprints](#blueprints) · [Starting a game](#starting-a-game)
· [Files](#files) · [How do I…](#how-do-i) · [Tests](#tests)

## World

- The world is made of cubes.
- A cube is a number: 0 = air, 1 = stone, 2 = grass, 3 = snow, 4 = bedrock, 5 = dirt.
- Cubes are grouped in **chunks**. One chunk = 16 × 16 cubes, 64 high.
- What a number looks like is set in **data assets** (editor files).

```
Main menu ──"play this world"──▶ Game instance ◀──"which world?"── Game mode        (Blueprint, see Menus)

Game mode ──1. build──▶ World ──▶ Chunk, Chunk, Chunk ...   (only around the player)
    │                    │ │ │ │    │
    │                    │ │ │ │    ├─ "what is 2?" ──▶ Block catalog ──▶ Stone, Grass, Snow, Bedrock, Dirt
    │                    │ │ │ │    └─ Chunk mesh ──▶ Block instances (one per type): draw the cubes
    │                    │ │ │ └─ "which chunks should exist?" ──▶ Chunk streaming (follows the player)
    │                    │ │ └─ "which cubes does a new chunk have?" ──▶ Terrain generator
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
|                                                      | `AVoxelWorld`              | Owns the loaded chunks. Loads, draws, unloads one when asked. Changes cubes, remembers the changes  | [World → Redrawing](WORLD.md#redrawing-after-a-change)      |
|                                                      |                            | Gives Blueprint the seed and the changes for saving, builds a saved world again                     | [Menus → Saving](MENUS.md#saving)                           |
|                                                      | `UChunkStreamingComponent` | Decides which chunks are loaded and drawn around the player, a few per frame                        | [World → Streaming](WORLD.md#streaming)                     |
|                                                      | `AVoxelChunk`              | One piece of the world. Keeps its numbers                                                           | [World → Chunk](WORLD.md#chunk)                             |
| `Rendering/` – how blocks are drawn                  | `UChunkMeshComponent`      | Decides which cubes of one chunk are visible, and of which type                                     | [World → Redrawing](WORLD.md#redrawing-after-a-change)      |
|                                                      | `UBlockInstancesComponent` | Draws all cubes of one type in one chunk. Adds or removes a single cube, knows which cells it draws | [World → Redrawing](WORLD.md#redrawing-after-a-change)      |
| `Terrain/` – how the hills are made                  | `FTerrainGenerator`        | Says how high the ground is at one spot. Makes the cubes of a new chunk                             | [World → Terrain](WORLD.md#terrain)                         |
| `Player/` – the player, and what they do with blocks | `AKubeCraftCharacter`      | Walk, look, jump                                                                                    | [Player → Mining and placing](PLAYER.md#mining-and-placing) |
|                                                      | `ULookTargetComponent`     | Which cube am I looking at?                                                                         | [Player → Mining and placing](PLAYER.md#mining-and-placing) |
|                                                      | `UMiningComponent`         | Mines cube after its mining time                                                                    | [Player → Mining and placing](PLAYER.md#mining-and-placing) |
|                                                      | `UPlacingComponent`        | Places cube on the face you look at                                                                 | [Player → Mining and placing](PLAYER.md#mining-and-placing) |
| `Game/` – start order of a game                      | `AKubeCraftGameMode`       | Builds the world,puts the player on it. Blueprint builds a saved world instead                      | [Starting a game](#starting-a-game)                         |
|                                                      | `UKubeCraftUserSettings`   | The player's settings, graphcis + view distance, chunks per frame and language                      | [Menus → Settings](MENUS.md#settings)                       |

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
| `M_Block` + `MI_Block_*`   | Look of a block: top, side and bottom texture, times a colour    | [Player → What the player sees](PLAYER.md#what-the-player-sees-in-the-world) |
| `BP_KubeCraftGameInstance` | Remembers which world is played. Creates, loads, saves worlds    | [Menus → From menu to world](MENUS.md#from-menu-to-world)                    |
| `BP_KubeCraftGameMode`     | Builds a new or a saved world (Start World)                      | [Menus → Starting the world](MENUS.md#starting-the-world)                    |
| `BP_MainMenuGameMode`      | Menu level: no player, shows the main menu                       | [Menus → From menu to world](MENUS.md#from-menu-to-world)                    |
| `BP_WorldSave` + list      | One saved world, and which worlds are saved                      | [Menus → Saving](MENUS.md#saving)                                            |
| `WBP_MainMenu` + parts     | Main menu, saved-world rows, settings page, menu button          | [Menus → From menu to world](MENUS.md#from-menu-to-world)                    |
| `WBP_Settings` + parts     | Settings page, Video and Language tabs, slider and selector rows | [Menus → Settings](MENUS.md#settings)                                        |
| `WBP_PauseMenu`            | Esc menu: Resume, Save, Settings, Save & Quit                    | [Menus → Pause menu](MENUS.md#pause-menu)                                    |

## Starting a game

1. `L_MainMenu` is opened.
   Create or Load tells the game instance which world to play, then opens `L_World`.
2. The game mode builds the world. New one gets a random **seed** (or the typed-in one),
   a saved one gets its seed and changes back. All chunks around the player load and draw at once.
3. The game mode puts the player on the ground, or back where they saved.
4. The character shows the HUD.
5. While playing, the chunk streaming loads new chunks in front of the player and unloads the ones behind.

Playing `L_World` straight from the editor skips step 1: a new world with a random seed.

## Files

```
Source/KubeCraft/Public/   .h     Blocks/, World/, Rendering/, Terrain/, Player/, Game/
Source/KubeCraft/Private/  .cpp   the same folders + Tests/
Content/KubeCraft/         Blocks/, Blueprints/ (+ Components/, SaveGames/, Structures/, World/), Input/, Materials/,
                           Textures/, UI/, Maps/ (L_MainMenu, L_World)
Content/Localization/Game/  the translations: one folder per language (cs/Game.po = the Czech text)
Config/Localization/       the steps of the Localization Dashboard (Gather, Export, Import, Compile)
SourceArt/Textures/        the PNG files the textures are imported from
docs/                      ARCHITECTURE.md, WORLD.md, PLAYER.md, MENUS.md
Saved/SaveGames/           saved worlds
```

## How do I…

| I want to…                                | Do this                                                                                      |
| ----------------------------------------- | -------------------------------------------------------------------------------------------- |
| Add a block                               | Make a new block asset with a material and a picture. Add it to the end of `DA_BlockCatalog` |
| Change a block's inventory picture        | Draw a new `T_Block_*` (or set texture in the block asset's `Texture`)                       |
| Give a block textures                     | In its `MI_Block_*`: set `TopTexture`, `SideTexture`, `BottomTexture`, and `Color` to white  |
| Change the number of slots                | `SlotCount` on `InventoryComponent` in `BP_KubeCraftCharacter`                               |
| Change which key picks which slot         | `HotbarKeys` in `BP_KubeCraftCharacter`                                                      |
| Change mining time                        | `MineTime` in the block asset                                                                |
| Make a block unbreakable                  | Turn off `bBreakable`                                                                        |
| Change how far the player reaches         | `BP_KubeCraftCharacter` → LookTarget → Reach                                                 |
| Change a mining/placing limit             | `Blocks/BlockRules.cpp`                                                                      |
| Change player size / jump                 | Numbers at the top of `Player/KubeCraftCharacter.cpp`                                        |
| Change the hills                          | `BP_VoxelWorld` → Terrain                                                                    |
| Change which block is at which height     | `DA_BlockCatalog` → HeightBands (`Block`, and `TopBlock` for the top cube, e.g. grass)       |
| Change how far the player sees            | Settings → Video → View distance                                                             |
| Change how fast new chunks appear         | Settings → Video → Chunks per frame (higher = faster, frames may take longer)                |
| Translate new or changed text             | Localization Dashboard → Game: Gather, Export, edit `cs/Game.po`, Import, Compile            |
| Add a language                            | Localization Dashboard → Game: Add New Culture, then as above. Add it to `CulturesToStage`   |
| Change keys                               | `IMC_Default`                                                                                |
| Change a texture                          | Edit its PNG in `SourceArt/Textures/`, then Reimport                                         |
| Make the outline thicker / another colour | `M_BlockOutline` change the colour node                                                      |
| Change the cracks                         | Draw a new `T_Crack` (shade of gray = when a pixel appears)                                  |
| Change the crosshair or the hotbar look   | `WBP_PlayerHUD`, `WBP_InventorySlot` (one slot)                                              |
| See how long a redraw takes               | While playing, type `stat KubeCraft` in the console (`~`)                                    |
| Run the tests                             | Tools → Session Frontend → Automation → `KubeCraft` → Start                                  |

## Tests

`Private/Tests/`:

- **Unit tests** check one piece alone, with no world.
- **Integration tests** need real chunks. A small temporary world is built (`TestVoxelWorld.h`).

| Test                                      | Tests code in                | Checks                                                                                                      |
| ----------------------------------------- | ---------------------------- | ----------------------------------------------------------------------------------------------------------- |
| `KubeCraft.World.Coordinates`             | `World/VoxelTypes.h`         | Position conversions, also negative ones. Which cube a hit means                                            |
| `KubeCraft.World.ParseSeed`               | `World/VoxelWorld`           | Which typed-in text is a seed: `-42` yes; `12.5`, `abc` or a too big number no                              |
| `KubeCraft.Blocks.HeightBands`            | `Blocks/BlockCatalog`        | Which block goes at which height, and the top block of a band                                               |
| `KubeCraft.Terrain.Generator`             | `Terrain/TerrainGenerator`   | Same seed → same hills. Other seed → other hills                                                            |
| `KubeCraft.Terrain.TerrainBlock`          | `Terrain/TerrainGenerator`   | The cube at one spot is the same as in the whole generated chunk; only the top cube gets the top block      |
| `KubeCraft.Integration.BlockRules`        | `Blocks/BlockRules`          | What may be mined, where a block may be placed                                                              |
| `KubeCraft.Integration.MineAndPlace`      | `World/VoxelWorld`           | Mining and placing can change blocks. Placed type is the given type. A border change redraws the next chunk |
| `KubeCraft.Integration.IncrementalRedraw` | `Rendering/`                 | After many changes, only cubes that touch air are drawn, each with its right type                           |
| `KubeCraft.Integration.ChunkStreaming`    | `World/ChunkStreaming…`      | Start draws the middle. Right chunks load, draw, unload. No walls. A few per call. Changes come back        |
| `KubeCraft.Integration.StreamingSettings` | `Game/KubeCraftUserSettings` | Settings stay in range, defaults. A new view distance while playing draws the bigger circle                 |
| `KubeCraft.Integration.SaveLoad`          | `World/VoxelWorld`           | A save has the seed and all changes. A loaded world has them, drawn around the player. Unneeded ones go     |
| `KubeCraft.Integration.CustomSeed`        | `World/VoxelWorld`           | A typed-in seed wins over the random one. After building, a new seed changes nothing                        |

[↑ Contents](#contents)

---

**Overview** · [World](WORLD.md#contents) · [Player](PLAYER.md#contents) · [Menus](MENUS.md#contents)
