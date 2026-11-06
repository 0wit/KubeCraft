[Overview](ARCHITECTURE.md#contents) · **World** · [Player](PLAYER.md#contents) · [Menus](MENUS.md#contents)

# KubeCraft – The World

## Contents

[Why chunks?](#why-chunks) · [Positions](#positions) · [Chunk](#chunk) ([Chunk borders](#chunk-borders))
· [Redrawing after a change](#redrawing-after-a-change) · [Streaming](#streaming) · [Terrain](#terrain)
· [Blocks and height](#blocks-and-height)

## Why chunks?

Performance. The world is split into small pieces, so a change touches only one piece.
A new chunk draws all its cubes once. After that, a change in the chunk redraws only **7 cubes**.
Only the chunks near the player exist.

## Positions

`World/VoxelTypes.h`

|                           | Example          |
| ------------------------- | ---------------- |
| Unreal, in cm             | (1750, -30, 420) |
| Cube in the world         | (17, -1, 4)      |
| Chunk                     | (1, -1)          |
| Place in the chunk's list | 1265             |

## Chunk

`AVoxelChunk` + `Rendering/`

Side view of a tiny chunk:

```
 . . . .    0 = Air
 3 3 3 3    3 = Snow    ← drawn (air above)
 2 2 2 2    2 = Grass   ← only the edge ones drawn
 1 1 1 1    1 = Stone   ← middle ones hidden → not drawn
 4 4 4 4    4 = Bedrock ← only the edge ones drawn (nothing seen from below)
```

- A chunk keeps one number per cube.
- Only cubes that touch air are drawn.
- All stone cubes of a chunk are drawn as one object, all grass cubes as another.
- Few objects draw fast.

### Chunk borders

A cube at the edge of a chunk has a neighbor in the next chunk. The chunk asks the world about it.

```
 chunk A   │   chunk B
 … 14  15  │  0  1 …

 cube 15: "is cube 0 solid?" → yes → the side between them is hidden
```

- A chunk is drawn only after its 4 neighbors are filled, so cube 0 already exists when cube 15 asks.
- Where no chunk is loaded, there is no neighbor. It counts as air.
- A change at a chunk's edge also redraws the touching cube in the next chunk.

## Redrawing after a change

A changed cube can only affect itself and the 6 cubes touching it. So the world redraws those 7.

```
          above   back
            │   ╱
   left ── CUBE ── right
          ╱ │
     front  below
```

For each of the 7, the chunk compares what **should** be drawn with what **is** drawn now:

| Cell is…                     | Should be drawn? |
| ---------------------------- | ---------------- |
| Air                          | No               |
| Solid, touches air           | Yes, its type    |
| Solid, buried on all 6 sides | No               |

| Should be | Is drawn now | What happens                    |
| --------- | ------------ | ------------------------------- |
| grass     | grass        | Nothing                         |
| air       | grass        | The grass mesh removes its cube |
| stone     | air          | The stone mesh adds a cube      |

**Removing one cube.** A mesh numbers its cubes 0, 1, 2, … To remove one, the **last** cube takes its number:

```
before     0:A  1:B  2:C  3:D
remove B
after      0:A  1:D  2:C
```

Only D got a new number, so the mesh updates just one note ("D is number 1").

- A new chunk draws all its cubes at once.
- Type `stat KubeCraft` in the game console. Read the **Max** column for a speed measurement.

## Streaming

`UChunkStreamingComponent` (on `BP_VoxelWorld`)

Only the chunks near the player exist. Distance is counted in chunks from the player's chunk, as a circle.
Example with `ViewDistanceInChunks` = 2:

```
 . . . F . . .     P = the player's chunk
 . . F D F . .     D = drawn
 . F D D D F .     F = filled, not drawn (next to a drawn chunk)
 F D D P D D F     . = not loaded
 . F D D D F .
 . . F D F . .
 . . . F . . .
```

| Distance           | What happens                                                              |
| ------------------ | ------------------------------------------------------------------------- |
| Up to ViewDistance | Drawn. Its 4 neighbors are filled just before, for its borders            |
| ViewDistance + 2   | Kept if it is there. Walking back and forth over a border reloads nothing |
| More               | Unloaded (destroyed)                                                      |

- When the player walks into another chunk, the missing chunks are drawn nearest first, so the ground under the
  player comes first.

- At most `MaxChunksPerFrame` chunks are filled, drawn or unloaded in one frame. Unloading comes last, new ground never waits for old chunks to go.

- At the start, everything around the middle loads at once, before the player is placed.
- `ViewDistanceInChunks` (2 – 12) and `MaxChunksPerFrame` (1 – 8) are player settings
  ([Menus → Settings](MENUS.md#settings)). The start uses 4 and 2; the player's values arrive when play begins,
  and again on Apply. A bigger distance then fills in over the next frames.

**Player's changes.** A new chunk gets its cubes from the terrain again: same seed → same cubes.
The world remembers every cube you changed. When a chunk loads again, player's changes are put back on top.
A cube you set back to what the terrain had there (mine stone, place stone) is forgotten: the terrain makes it anyway.
Saving a world stores exactly these changes and the seed ([Menus → Saving](MENUS.md#saving)).

```
from the seed     your changes          loaded chunk
 3 3 3 3          2nd cube: air          3 . 3 3
 2 2 2 2                                 2 2 2 2
```

## Terrain

`FTerrainGenerator`

- **Perlin noise** gives a smooth random value (-1 to 1) for any (X, Y). Nearby spots get similar values.
- The **seed** shifts where the noise is sampled. New seed → new terrain. Same seed → same terrain.
- **Octaves:** the noise is added several times, each time finer and weaker. Result: big hills + small detail.
- The value becomes a height: (it is always between 1 and 63)

```
height = 20 + noise × 24
noise 0.25  →  20 + 6 = 26 cubes high
```

- For a new chunk, the generator makes all its cubes: each column is solid up to its height.
- It is made once, when the game starts. So all chunks get the same land, even if the settings are edited while playing.

## Blocks and height

- Block number = place in the `DA_BlockCatalog` list + 1.
- Below 1 → bedrock. Below 16 → stone. Below 26 → dirt. Above → snow.
- A band can have a **top block**: the top cube of a column in that band uses it instead.
- The dirt band has grass as its top block. So grass is only the top cube, with dirt under it:

```
 height 20 column:
 Z 19      grass    ← top cube, in the dirt band → top block
 Z 16–18   dirt
 Z 1–15    stone
 Z 0       bedrock
```

- A column whose top is in the stone band stays bare stone (that band has no top block).
- The top block is chosen when the land is made. Mining the grass leaves the dirt under it as it is.
- This rule is only for the generated terrain. A placed cube is the type of the selected slot.
- Bottom of the world = 0.
- Top of the world = 64.

[↑ Contents](#contents) · [Overview](ARCHITECTURE.md#contents)

---

[Overview](ARCHITECTURE.md#contents) · **World** · [Player](PLAYER.md#contents) · [Menus](MENUS.md#contents)
