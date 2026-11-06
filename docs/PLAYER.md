[Overview](ARCHITECTURE.md#contents) · [World](WORLD.md#contents) · **Player** · [Menus](MENUS.md#contents)

# KubeCraft – The Player

## Contents

[Mining and placing](#mining-and-placing) · [Inventory](#inventory-blueprint)
· [What the player sees](#what-the-player-sees-in-the-world) ([HUD](#hud))

## Mining and placing

```
Hold left mouse     look at a cube long enough → it breaks → goes into the inventory
Press right mouse   the selected block is put on the face you look at
```

- Looking at another cube resets the timer. Each block type has its own mining time.
- The player reaches 5 cubes far.

### Rules

| Rule                                | Why                    |
| ----------------------------------- | ---------------------- |
| Bedrock can't be mined              | The floor of the world |
| No placing at height 64 or more     | The ceiling            |
| No placing outside loaded chunks    | There is nothing there |
| No placing where a player stands    | You'd be stuck         |

## Inventory (Blueprint)

8 slots. A slot holds one block type and a count, at most `MaxStackSize` (128).

```
[Stone 5][Grass 2][   ][   ] … [   ]
            ▲ selected
```

A mined block goes to the first slot that fits (`FindSlotFor`):

| Order | Slot                                          |
| ----- | --------------------------------------------- |
| 1     | The selected slot, if same block and not full |
| 2     | Any slot with the same block, not full        |
| 3     | The selected slot, if empty                   |
| 4     | The first empty slot                          |
| –     | None fits: the block is lost                  |

Example: Grass is selected. Mining Grass gives `Grass 3`; mining Stone goes to the first empty slot.

- Placed block → the selected slot gets −1. At 0 it is empty.
- Keys 1–8 and the mouse wheel choose the slot.
- After every change the HUD redraws.

## What the player sees in the world

- **Outline:** a slightly bigger cube around the block you look at. The material draws only the edges.
- **Cracks:** another slightly bigger cube on the block you mine. The cracks grow with the mining progress.
  In `T_Crack`, darker pixels appear earlier.

### HUD

`WBP_PlayerHUD`

- A "+" in the middle of the screen. (crosshair)
- The row of slots at the bottom.
- Each slot shows the block's picture (`T_Block_*`), the count, and a white frame if it is selected.

[↑ Contents](#contents) · [Overview](ARCHITECTURE.md#contents)

---

[Overview](ARCHITECTURE.md#contents) · [World](WORLD.md#contents) · **Player** · [Menus](MENUS.md#contents)
