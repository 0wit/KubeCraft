# KubeCraft

A voxel sandbox game made in **Unreal Engine 5.8** with C++ and Blueprints, Claude and Unreal MCP.

Features:

- Every new world is a random hilly landscape (Perlin noise) with trees. You can type in your own **seed**.
- The terrain's block type depends on height: **bedrock** at the very bottom, **stone** low, **dirt** under
  **grass** in the middle, **snow** high. Trees are made of **wood** and **leaves**.
- Hold the mouse button to **mine** a block. Each type takes its own time (snow fast, stone slow). Bedrock can't be mined.
- Mined blocks go into an **inventory** of 8 slots, shown at the bottom of the screen.
- Click to **place** the selected block on the face you look at. It snaps to the grid and uses up one block.
- Chunks load in front of you and unload behind you. It has a floor (bedrock) and a
  ceiling (64 blocks).
- A crosshair marks the middle of the screen. The block you look at gets an outline. While mining, cracks grow on it.

- **Main menu** and **pause menu**: create, save, load and delete worlds.
- **Settings**: resolution, window mode, quality, view distance, chunks per frame and language (English, Czech).
- **Sound**: menu music, ambient music

## Documentation

| Page                                        | What is in it                                                                                          |
| ------------------------------------------- | ------------------------------------------------------------------------------------------------------ |
| **[ARCHITECTURE.md](docs/ARCHITECTURE.md)** | All classes and assets, how a game starts, files, instructions how to change parts of the appplication |
| [WORLD.md](docs/WORLD.md)                   | Positions, chunks, redrawing, streaming, terrain, block heights                                        |
| [PLAYER.md](docs/PLAYER.md)                 | Mining, placing, rules, inventory, HUD, outline, cracks                                                |
| [MENUS.md](docs/MENUS.md)                   | Main menu, pause menu, saving, loading, settings, language                                             |

## Getting started

You need:

- **Unreal Engine 5.8** (Epic Games Launcher)
- **Visual Studio 2022** with the "Game development with C++" workload
- **Git LFS** – the `.uasset` / `.umap` files are stored with it

```
git lfs install
git clone https://github.com/0wit/KubeCraft.git
```

Open `KubeCraft.uproject`. Unreal asks to build the `KubeCraft` module – click **Yes**.
The editor opens the main menu map `L_MainMenu`. Press **Play**, then create or load a world.

Command-line build (optional):

```
"C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat" KubeCraftEditor Win64 Development -Project="<path>/KubeCraft.uproject" -WaitMutex
```

## Controls

| Key               | Action                                                 |
| ----------------- | ------------------------------------------------------ |
| W A S D           | Walk                                                   |
| Mouse             | Look                                                   |
| Space             | Jump                                                   |
| Left mouse (hold) | Mine the block you look at                             |
| Right mouse       | Place the selected block                               |
| Mouse wheel       | Select the next / previous slot                        |
| 1 – 8             | Select a slot (Czech keyboard also: `+ ě š č ř ž ý á`) |
| Esc               | Pause menu                                             |

## How the application works:

Start in: **[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)**.

## Sounds

All sounds are from [Freesound](https://freesound.org).

| Used for        | Source                                                                                          | Asset           | Licence | Format |
| --------------- | ----------------------------------------------------------------------------------------------- | --------------- | ------- | ------ |
| Main menu music | [LightMister – 761287](https://freesound.org/people/LightMister/sounds/761287/)                 | `menuMusic`     | CC0     | ogg    |
| Ambient music 1 | [Andrewkn – 752124](https://freesound.org/people/Andrewkn/sounds/752124/)                       | `ambientMusic`  | CC0     | wav    |
| Ambient music 2 | [ChristmasKrumble666 – 744098](https://freesound.org/people/ChristmasKrumble666/sounds/744098/) | `ambientMusic2` | CC0     | wav    |
| Button press    | [riel26 – 752012](https://freesound.org/people/riel26/sounds/752012/)                           | `buttonPress`   | CC0     | wav    |
