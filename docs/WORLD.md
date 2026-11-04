[Overview](ARCHITECTURE.md#contents) · **World** · [Player](PLAYER.md#contents)

# KubeCraft – The World

Where the blocks are, how they are stored and drawn, and how the hills are made.
Code: `World/`, `Rendering/`, `Terrain/`, `Blocks/BlockCatalog`.

## Contents

[Why chunks?](#why-chunks) · [Positions](#positions-worldvoxeltypesh) · [A chunk](#a-chunk-avoxelchunk--rendering)
([Chunk borders](#chunk-borders)) · [Redrawing after a change](#redrawing-after-a-change) · [Terrain](#terrain-fterraingenerator)
· [Blocks and height](#blocks-and-height)

## Why chunks?

So the work is split into pieces of the same size.
Each chunk has its own list of numbers and its own picture.
At the start, every chunk draws all its cubes once.
After that, a change redraws only **7 cubes** – see [Redrawing after a change](#redrawing-after-a-change).

[↑ Contents](#contents) · [Overview](ARCHITECTURE.md#contents)

## Positions (`World/VoxelTypes.h`)

One spot, four ways to say it:

| | Example |
|---|---|
| Unreal, in cm | (1750, -30, 420) |
| Cube in the world | (17, -1, 4) |
| Chunk | (1, -1) |
| Place in the chunk's list | 1265 |

All conversions live only in `VoxelTypes.h`.

[↑ Contents](#contents) · [Overview](ARCHITECTURE.md#contents)

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

[↑ Contents](#contents) · [Overview](ARCHITECTURE.md#contents)

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

[↑ Contents](#contents) · [Overview](ARCHITECTURE.md#contents)

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

[↑ Contents](#contents) · [Overview](ARCHITECTURE.md#contents)

## Blocks and height

- Block number = place in the `DA_BlockCatalog` list + 1.
- **Add new blocks at the end** of the list.
- Below 1 → bedrock. Below 16 → stone. Below 26 → grass. Above → snow.
- Example: a column 26 high = bedrock at 0, stone at 1..15, grass at 16..25.
- This rule is only for the generated terrain. A placed cube is the type from the selected slot, at any height
  (see [Player → Mining and placing](PLAYER.md#mining-and-placing)).
- Bottom of the world = 0. Top = 64.

[↑ Contents](#contents) · [Overview](ARCHITECTURE.md#contents)

---

[Overview](ARCHITECTURE.md#contents) · **World** · [Player](PLAYER.md#contents)
