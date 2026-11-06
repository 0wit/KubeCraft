[Overview](ARCHITECTURE.md#contents) · [World](WORLD.md#contents) · [Player](PLAYER.md#contents) · **Menus**

# KubeCraft – Menus and Saving

## Contents

[From menu to world](#from-menu-to-world) · [Seed](#seed) · [Starting the world](#starting-the-world)
· [Saving](#saving) · [Loading](#loading) · [Pause menu](#pause-menu) · [Settings](#settings) · [Language](#language)

Everything here are Blueprints. C++ only gives Blueprint the seed and the changed cubes.
(`AVoxelWorld`: `MakeSave`, `GenerateFromSave`, `SetSeed`, `TryParseSeed`).

## From menu to world

```
 L_MainMenu ──Create / Load──▶ L_World ──Save & Quit──▶ L_MainMenu
                │
                └─ game instance: "play this world" (it survives the level change)
```

- Opening a level destroys everything in the old one. **Game instance** stays.

| Asset                      | Job                                                                   |
| -------------------------- | --------------------------------------------------------------------- |
| `BP_KubeCraftGameInstance` | Remembers the world being played. Creates, loads, saves, deletes them |
| `BP_MainMenuGameMode`      | Shows the main menu                                                   |
| `BP_KubeCraftGameMode`     | Builds a new or a saved world                                         |
| `WBP_MainMenu`             | Start, New World, Load World and Settings pages                       |
| `WBP_WorldEntry`           | One saved world on the Load page                                      |
| `WBP_PauseMenu`            | Esc menu                                                              |
| `WBP_MenuButton`           | The button of all menus                                               |
| `WBP_Settings`             | Settings page                                                         |
| `WBP_VideoSettings`        | Reading / applying video settings                                     |
| `WBP_LanguageSettings`     | Reading / applying the language                                       |
| `WBP_SettingSlider`        | Slider widget                                                         |
| `WBP_SettingSelector`      | Selector widget                                                       |
| `BP_WorldSave`             | One saved world                                                       |
| `BP_WorldListSave`         | Saved worlds list                                                     |

## Seed

| Field typed in                 | Result           |
| ------------------------------ | ---------------- |
| Name empty                     | "New World"      |
| Seed `1234` or `-42`           | Seed 1234 or -42 |
| Seed empty, `abc`, `12.5`      | A random seed    |
| Seed too big (over 2147483647) | A random seed    |

## Starting the world

`BP_KubeCraftGameMode` → Start World

| The game instance has… | What happens                                               |
| ---------------------- | ---------------------------------------------------------- |
| A loaded save          | Built from the save. The player stands and looks as before |
| A typed-in seed        | That seed, then the normal start                           |
| Nothing                | A random seed, then the normal start                       |

- The inventory is loaded too (`BP_InventoryComponent`, BeginPlay).

## Saving

`BP_KubeCraftGameInstance` → SaveWorld (pause menu: Save, Save & Quit)

| `BP_WorldSave` | Example                      |
| -------------- | ---------------------------- |
| WorldName      | "My hills"                   |
| SavedAt        | 5. 11. 2026 17:20            |
| World          | Seed 1234 + 12 changed cubes |
| PlayerLocation | (1750, -30, 2650)            |
| PlayerRotation | Where the player looks       |
| Inventory      | [Stone 5][Grass 2][ ] …      |
| SelectedSlot   | 1                            |

- Only the changed cubes are saved. ([World → Streaming](WORLD.md#streaming)).
  Saving moves the world to the front.
- **Picture** (SaveThumbnail): a scene capture at the camera renders what the player sees, without the HUD, It is saved as a PNG next to the save.
- A world started with Play in `L_World` has no slot yet. Its first save gives it
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

- One `WBP_WorldEntry` per slot. Each row reads its own save, name and seed.
  Its picture comes from the PNG.
- **Load:** the game instance keeps the save and opens `L_World`, see [Starting the world](#starting-the-world).
- **Delete:** the save, its picture and its entry in "Worlds" are deleted.

  (Deleting a file needs the engine plugin "Blueprint File Utilities", turned on in `KubeCraft.uproject`.)

## Pause menu

`WBP_PauseMenu`

| Button      | What it does                     |
| ----------- | -------------------------------- |
| Resume      | Closes the menu                  |
| Save        | Saves                            |
| Settings    | Settings page                    |
| Save & Quit | Saves, then opens the main menu. |

- The game is paused while the menu is open.

## Settings

`WBP_Settings` → Video tab (`WBP_VideoSettings`)

| Row                    | Choices                                 | Stored by                    |
| ---------------------- | --------------------------------------- | ---------------------------- |
| Resolution             | The monitor's fullscreen sizes          | Engine (`UGameUserSettings`) |
| Window mode            | Fullscreen, Windowed fullscreen, Window | Engine                       |
| Quality                | Low, Medium, High, Epic                 | Engine (overall scalability) |
| View distance (chunks) | 2 – 12                                  | `UKubeCraftUserSettings`     |
| Chunks per frame       | 1 – 8                                   | `UKubeCraftUserSettings`     |
| Language (own tab)     | English, Čeština                        | `UKubeCraftUserSettings`     |

- All of them are saved in `Saved/Config/Windows/GameUserSettings.ini`, for every world.
- `UKubeCraftUserSettings` (C++) is the engine's settings class plus the two chunk values and the language. It keeps them
  valid, also when the ini is edited by hand. Set in `DefaultEngine.ini`.

| Button   | What it does                                                             |
| -------- | ------------------------------------------------------------------------ |
| Apply    | Uses and saves the rows. View distance and language change while playing |
| Defaults | Puts the default values in the rows. Apply uses them                     |
| Back     | Forgets rows that were not applied, returns                              |

- **Keep these settings?** After Apply changed resolution or window mode, a 10 s countdown asks
  (`ConfirmDuration`). Keep stays; Revert, or no answer, goes back to the last kept size.
  The countdown runs in the widget's Tick, so it also works while the game is paused.
  Leaving the page another way during the countdown (e.g. Esc closes the pause menu) reverts too (Event Destruct).

## Language

Settings → Language tab (`WBP_LanguageSettings`)

| Choice  | Text in           |
| ------- | ----------------- |
| English | English (default) |
| Čeština | Czech             |

- Apply switches all text at once. The choice is saved in `GameUserSettings.ini` (`[Internationalization] Culture`).
  The engine reads that key at the next start, before the menu appears.

Where the text is:

| What                | Where                                              |
| ------------------- | -------------------------------------------------- |
| English text        | The widgets and Blueprints themselves              |
| Czech text          | `Content/Localization/Game/cs/Game.po` (text file) |
| What the game reads | `Game.locres` next to it (made by Compile)         |

[↑ Contents](#contents) · [Overview](ARCHITECTURE.md#contents)

---

[Overview](ARCHITECTURE.md#contents) · [World](WORLD.md#contents) · [Player](PLAYER.md#contents) · **Menus**
