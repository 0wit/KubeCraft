[Overview](ARCHITECTURE.md#contents) · [World](WORLD.md#contents) · **Player**

# KubeCraft – The Player

What the player does with blocks (mining, placing, the inventory) and what they see on screen.
Code: `Player/`, `Blocks/BlockRules`. Blueprints: `BP_KubeCraftCharacter`, `BP_InventoryComponent`, `WBP_PlayerHUD`.

## Contents

[Mining and placing](#mining-and-placing) ([Rules](#rules), [Where the line hits](#where-the-line-hits))
· [Inventory](#inventory-blueprint) ([Number keys](#number-keys))
· [What the player sees](#what-the-player-sees-blueprint) ([Outline](#outline-m_blockoutline), [Cracks](#cracks-m_blockcrack), [HUD](#hud-wbp_playerhud))

## Mining and placing

```
Left mouse held → Mining component, every frame (a stopwatch):
   1. "Which cube am I looking at?"  (LookTarget component: a line from the camera, max 5 cubes)
   2. Same cube as last frame? → add the time. Other cube → start at 0.
   3. Time ≥ the cube's MineTime → World.MineBlock → the cube becomes air
   4. OnBlockMined(type) → the inventory adds one
Left mouse released → stopwatch off and back to 0

Right mouse pressed → character Blueprint, once:
   1. Inventory: "which block is in the selected slot?"
   2. Placing component: "which face am I looking at?" → the empty cell in front of it
   3. World.PlaceBlock(cell, block) → the cell gets that block
   4. Placed? → the inventory takes one away. Not placed → nothing is used up.
```

- Mining times: stone 1.5 s, grass 0.75 s, snow 0.3 s. Bedrock can't be mined.
- The mining component only runs while the button is held. The rest of the time it costs nothing.
- The redraw after a change: [World → Redrawing after a change](WORLD.md#redrawing-after-a-change).

### Rules

The world asks `FBlockRules` before **every** change. The components can't skip it.
`PlaceBlock` also refuses an empty slot (no block) and a block that is not in the catalog.
`Place` refuses an empty slot even earlier, before it looks for a target.

| Rule | Why |
|---|---|
| Bedrock (`bBreakable` off) can't be mined | The floor. You can't dig out of the world. |
| No placing at height 64 or more | The ceiling. |
| No placing outside the 8 × 8 chunks | There is no chunk to hold the cube. |
| No placing where a player stands | You'd be stuck inside the cube. |

### Where the line hits

Exactly **on** the surface – the border between the cube and the air in front of it.
So the code moves half a cube away from the border, into the middle of one cell:

```
            air cell  │  cube
  camera ─────────────● hit point (on the border)
          ◀── 50 cm ──┼── 50 cm ──▶
          place here  │  mine this
      (HitToFaceNeighbor)   (HitToBlock)
```

[↑ Contents](#contents) · [Overview](ARCHITECTURE.md#contents)

## Inventory (Blueprint)

The inventory is Blueprint only. C++ gives it two hooks: `OnBlockMined` (mining) and `Place(type)` (placing).

`BP_InventoryComponent` (on the character) keeps 8 slots. A slot is `S_InventorySlot` = block + count.
An empty slot has no block. `SelectedSlot` says which slot the frame is on (0 = first).

```
slot:   0      1      2      3    …    7
      [Stone][Grass][     ][    ]    [    ]
        5      2
               ▲ SelectedSlot = 1
```

| Function | When | What it does |
|---|---|---|
| `Add(Block)` | A block was mined | Same type in a slot → +1. Else the first empty slot gets 1. All 8 full → nothing. |
| `GetSelectedBlock` | Right mouse | The block in the selected slot, or none |
| `RemoveOneFromSelected` | A block was placed | −1. At 0 the slot becomes empty. |
| `Select(Index)` | Keys 1–8 | Frame jumps to that slot. Other numbers do nothing. |
| `SelectNext(Step)` | Mouse wheel | Frame moves by one, 8 → 1 and 1 → 8 wrap around |

Every function that changes something calls `OnChanged` at the end. The [HUD](#hud-wbp_playerhud) listens to it and redraws the 8 slots.

### Number keys

Unreal names a key by the character it types. On a Czech keyboard the keys 1–8 type `+ ě š č ř ž ý á`,
and Unreal has no normal key for most of them. So the character Blueprint reacts to **Any Key** and looks the key's name up
in the map `HotbarKeys`:

| Key name | Slot |
|---|---|
| `1` or `+` | 0 |
| `2` or `ě` | 1 |
| … | … |
| `8` or `á` | 7 |

A key that is not in the map does nothing. The wheel uses the normal input action `IA_SelectSlot`.

[↑ Contents](#contents) · [Overview](ARCHITECTURE.md#contents)

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

### Outline (`M_BlockOutline`)

Every cube face has coordinates 0..1. The material shows a pixel only near the border:

```
distance from face centre = max(|U − 0.5|, |V − 0.5|)     0 in the middle, 0.5 at the edge
shown if distance ≥ 0.48
```

### Cracks (`M_BlockCrack`)

`T_Crack` is a grayscale picture. A pixel's gray value says **when** it appears:

```
gray 0.1 → appears at 10 % progress      white → never
shown if Progress ≥ gray value
```

### HUD (`WBP_PlayerHUD`)

Created by the character at the start. The character hands it the inventory
(the widget's `Inventory` pin, "Expose on Spawn").
- Crosshair: a "+" text, anchored to the screen centre. The look trace also goes through the screen centre, so the "+" and the outline agree.
- `InventoryBar`: a row of 8 `WBP_InventorySlot`, at the bottom centre. On `OnChanged`, slot N is redrawn from inventory slot N.
- `WBP_InventorySlot.Refresh`: picture = the block's `Texture` (`T_Block_*`), count as a number, white frame if selected. Empty slot = frame only.
- The 8 slots are placed by hand in the HUD. If `SlotCount` ever changes, add or remove slots there too.

[↑ Contents](#contents) · [Overview](ARCHITECTURE.md#contents)

---

[Overview](ARCHITECTURE.md#contents) · [World](WORLD.md#contents) · **Player**
