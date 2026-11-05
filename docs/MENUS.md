[Overview](ARCHITECTURE.md#contents) · [World](WORLD.md#contents) · [Player](PLAYER.md#contents) · **Menus**

# KubeCraft – Menus and Saving

## Contents

[From menu to world](#from-menu-to-world) · [Main menu](#main-menu) · [Starting the world](#starting-the-world)
· [Saving](#saving) · [Loading](#loading) · [Pause menu](#pause-menu) · [Input mode](#input-mode)

Everything on this page is Blueprint. C++ only gives Blueprint the seed and the changed cubes
(`AVoxelWorld`: `MakeSave`, `GenerateFromSave`, `SetSeed`).

## From menu to world

```
 L_MainMenu ──Create / Load──▶ L_World ──Save & Quit──▶ L_MainMenu
                │
                └─ game instance: "play this world" (it survives the level change)
```

- Opening a level destroys everything in the old one. Only the **game instance** stays.
- So the menu tells the game instance which world to play, and the world level asks it.

| Asset                           | Job                                                                   |
| ------------------------------- | --------------------------------------------------------------------- |
| `BP_KubeCraftGameInstance`      | Remembers the world being played. Creates, loads, saves, deletes them |
| `BP_MainMenuGameMode`           | Menu level: no player, shows the main menu                            |
| `BP_KubeCraftGameMode`          | World level: builds a new or a saved world                            |
| `WBP_MainMenu`                  | Start, New World, Load World and Settings pages                       |
| `WBP_WorldEntry`                | One saved world on the Load page                                      |
| `WBP_PauseMenu`                 | Esc menu                                                              |
| `WBP_MenuButton`                | The button of all menus: a label and an OnClicked event               |
| `WBP_Settings`                  | Settings page: tabs and buttons, no settings yet                      |
| `BP_WorldSave`                  | One saved world                                                       |
| `BP_WorldListSave`              | Which worlds are saved                                                |

## Main menu

`WBP_MainMenu` (in `L_MainMenu`)

| Page       | Buttons                                 |
| ---------- | --------------------------------------- |
| Start      | New World, Load World, Settings, Quit   |
| New World  | Name, Seed, Create, Back                |
| Load World | One row per saved world, Back           |
| Settings   | Back returns to Start                   |

| Field typed in       | Result                |
| -------------------- | --------------------- |
| Name empty           | "New World"           |
| Seed `1234`          | Seed 1234             |
| Seed empty or `abc`  | A random seed         |

## Starting the world

`BP_KubeCraftGameMode` → Start World

| The game instance has… | What happens                                                                      |
| ---------------------- | --------------------------------------------------------------------------------- |
| A loaded save          | Built from the save, around the saved spot. The player stands and looks as before |
| A typed-in seed        | That seed, then the normal start: chunks around the middle, player on the ground  |
| Nothing                | A random seed, then the normal start                                              |

- The inventory comes back from the save too (`BP_InventoryComponent`, BeginPlay).

## Saving

`BP_KubeCraftGameInstance` → SaveWorld (pause menu: Save, Save & Quit)

| `BP_WorldSave` holds | Example                       |
| -------------------- | ----------------------------- |
| WorldName            | "My hills"                    |
| SavedAt              | 5. 11. 2026 17:20             |
| World                | Seed 1234 + 12 changed cubes  |
| PlayerLocation       | (1750, -30, 2650)             |
| PlayerRotation       | Where the player looks        |
| Inventory            | [Stone 5][Grass 2][ ] …       |
| SelectedSlot         | 1                             |

- Only the changed cubes are saved. The seed makes all the others again ([World → Streaming](WORLD.md#streaming)).
- Each world has its own **slot** with a random name, so two worlds may have the same name.
- Blueprint can't list the save files, so one more slot, "Worlds" (`BP_WorldListSave`), keeps the slot names.
  Saving moves the world to the front.
- **Picture** (SaveThumbnail): a scene capture at the camera renders what the player sees, without the HUD,
  320 × 180 (`ThumbnailWidth`, `ThumbnailHeight`). It is saved as a PNG next to the save.
  The capture keeps a rendering state (Always Persist Rendering State), otherwise it ignores exposure settings. Its
  exposure is fixed: `ThumbnailExposure` (EV100, -1.5 = as bright as the game; higher = darker).
- A world started with Play in `L_World` (not through the main menu) has no slot yet. Its first save gives it
  one, named "New World".

```
Saved/SaveGames/
  Worlds.sav         [3F2A…, 91BC…]      ← last saved first
  3F2A….sav          "My hills", seed 1234, …
  3F2A….png          its picture
  91BC….sav          "New World", seed 87, …
  91BC….png
```

## Loading

`WBP_MainMenu` → Load World

- One `WBP_WorldEntry` per slot in "Worlds". Each row reads its own save: the name and "Seed 1234 | date".
  Its picture comes from the PNG. A world without one shows a grey box.
- **Load:** the game instance keeps the save and opens `L_World`, see [Starting the world](#starting-the-world).
- **Delete:** the save, its picture and its entry in "Worlds" are deleted, the row disappears.
  (Deleting a file needs the engine plugin "Blueprint File Utilities", turned on in `KubeCraft.uproject`.)

## Pause menu

`WBP_PauseMenu` (Esc = `IA_Pause`)

| Button      | What it does                              |
| ----------- | ----------------------------------------- |
| Resume      | Closes the menu (Esc does the same)       |
| Save        | Saves, the game stays paused              |
| Settings    | Settings page, Back returns               |
| Save & Quit | Saves, then opens the main menu           |

- The game is paused while the menu is open. `IA_Pause` has "Trigger when Paused" on, so Esc still works.
- `BP_KubeCraftCharacter` makes the menu on the first Esc and keeps it (TogglePauseMenu).

## Input mode

Who gets the keys and the mouse:

| Where      | Input mode  | Cursor | Why                                       |
| ---------- | ----------- | ------ | ----------------------------------------- |
| Main menu  | UI only     | Shown  | Only buttons                              |
| Playing    | Game only   | Hidden | The mouse turns the camera all the time   |
| Pause menu | Game and UI | Shown  | Buttons, and Esc still reaches the game   |

- The input mode stays on the window when the level changes. So leaving the main menu sets **Game only** first
  (game instance, OpenWorldLevel).

[↑ Contents](#contents) · [Overview](ARCHITECTURE.md#contents)

---

[Overview](ARCHITECTURE.md#contents) · [World](WORLD.md#contents) · [Player](PLAYER.md#contents) · **Menus**
