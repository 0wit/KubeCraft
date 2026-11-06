---
name: add-block-type
description: Add a new block type to KubeCraft (e.g. sand, wood) with no C++ - material instance, hotbar texture, block data asset, catalog entry (and a height band if the terrain should use it) - via the Unreal MCP. Use when the user wants a new block, block colour or block texture.
---

# Add a block type

A block type is data only (`UBlockDefinition`): never hardcode it in C++. Its id = place in `DA_BlockCatalog.Blocks`
+ 1, so **append at the end** - inserting would change the ids of existing blocks (and saved changes).

Ask the user first: name, colour, mine time, breakable, and whether the terrain should place it (height band).
Read the `unreal-mcp` skill for parameter shapes. Editor open, PIE stopped.

## Steps (example: Sand)
1. **Material** - `AssetTools.duplicate` `/Game/KubeCraft/Materials/MI_Block_Stone` → `MI_Block_Sand`.
   Set its `Color` vector parameter (linear RGB, e.g. `0.76, 0.70, 0.50`). The parent `M_Block` already has
   `bUsedWithInstancedStaticMeshes`; a material that does not derive from it needs that flag set.
2. **Hotbar picture** - a 16 × 16 PNG of the same colour, written with System.Drawing (PowerShell) **straight into
   `SourceArt/Textures/T_Block_Sand.png`** (tracked; the asset remembers its import path - importing from the
   scratchpad leaves a temp path in the .uasset). Convert linear → sRGB first, or the picture looks darker than the cube:
   `c < 0.0031308 ? 12.92·c : 1.055·c^(1/2.4) − 0.055`. Import it with
   `editor_toolset.toolsets.texture.TextureTools` `import_file {"folder_path": "/Game/KubeCraft/Textures",
   "asset_name": "T_Block_Sand", "source_file": "<repo root>/SourceArt/Textures/T_Block_Sand.png"}` (absolute path) and set
   `LODGroup = TEXTUREGROUP_Pixels2D` like the others. `import_file` refuses to overwrite an existing asset: to redo
   it, clear the block's `Texture`, `delete` the texture, import again, set `Texture` back.
   Check: `LC_ALL=C grep -aoE "Factory_[ -~]*png" Content/KubeCraft/Textures/T_Block_Sand.uasset` shows the SourceArt path.
3. **Block asset** - `AssetTools.duplicate` `/Game/KubeCraft/Blocks/DA_Block_Stone` → `DA_Block_Sand`; set
   `Material`, `Texture`, `MineTime` (s), `bBreakable`.
   **See-through block** (holes, like leaves): set `bSeeThrough` and duplicate `MI_Block_Leaves` instead of
   `MI_Block_Stone` (its parent `M_Leaves` is Masked; the holes are the alpha of its `LeafMask` texture).
4. **Catalog** - `get_properties` `DA_BlockCatalog` `Blocks`, then `set_properties` with the existing elements
   **exactly as returned** plus the new one at the end (see the array gotcha in `unreal-mcp`).
   For terrain: add a `HeightBands` entry (`MaxZ`, `Block`, optional `TopBlock` = the top cube of a column, like
   grass on dirt), keeping the list sorted by `MaxZ`. Or set it as an existing band's `Block`/`TopBlock`.
5. **Save** all touched assets (`save_assets`; make them dirty first if needed). Saving the catalog runs its
   `IsDataValid` - read the log for warnings (missing material / texture, unsorted bands).
6. **Check** - `git status`: only the new `.uasset` files and `DA_BlockCatalog` changed. Run the game (`run-game`)
   if the block is in the terrain; otherwise the user mines/places it by hand.
7. **Docs** - ARCHITECTURE "World" bullet listing the numbers (`0 = air, 1 = stone …`), WORLD "Blocks and height"
   if a band changed. Run `docs-check`.

## Hand test for the user
Mine the new block (does the mining time feel right?), see its picture in the hotbar, place it again.
