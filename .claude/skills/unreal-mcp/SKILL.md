---
name: unreal-mcp
description: How to use the Unreal MCP tools in KubeCraft without trial and error - toolset names, exact parameter shapes, saving assets, checking Blueprints/widgets/materials the user built, and known gotchas. Use before any mcp__unreal__ call that edits or inspects assets, Blueprints, materials, widgets or PIE.
---

# Unreal MCP in KubeCraft

Meta-tools: `list_toolsets`, `describe_toolset`, `call_tool`. **Toolset names need the full prefix** - copy them
from this table (or `list_toolsets`):

| Toolset | For |
| --- | --- |
| `editor_toolset.toolsets.object.ObjectTools` | `get_properties` / `set_properties` / `list_properties` on any object |
| `editor_toolset.toolsets.asset.AssetTools` | `duplicate`, `move`, `delete`, `save_assets`, `is_dirty`, `find_assets`, `exists` |
| `editor_toolset.toolsets.blueprint.BlueprintTools` | `read_graph_dsl`, `write_graph_dsl`, `create_node`, `connect_pins`, compile |
| `editor_toolset.toolsets.material.MaterialTools` | `add_expression`, `connect_expressions`, `get_expression_inputs`, `get_property_input` |
| `editor_toolset.toolsets.actor.ActorTools` | actor transforms, components |
| `editor_toolset.toolsets.programmatic.ProgrammaticToolset` | `execute_tool_script` - batch many calls in one Python script |
| `EditorToolset.EditorAppToolset` | `StartPIE` / `StopPIE`, viewport camera, selection |
| `EditorToolset.LogsToolset` | Output Log |
| `UMGToolSet.UMGToolSet` | `GetWidgetDescription` (widget tree of a WBP) |
| `AutomationTestToolset.AutomationTestToolset` | run tests inside the open editor |

## Parameter shapes that work
- Object refs: `{"refPath": "/Game/KubeCraft/Blocks/DA_Block_Stone.DA_Block_Stone"}` (package **.** name).
- `get_properties {"instance": {...}, "properties": ["MineTime"]}` → returns **camelCase** keys in a JSON string.
- `set_properties` takes **PascalCase** names; `values` is a JSON **string**.
- BP component template: `<BP path>_C:<Name>_GEN_VARIABLE` (find names: `grep -a _GEN_VARIABLE <file>.uasset`).
- `read_graph_dsl {"graph": {"refPath": "/Game/.../BP_X.BP_X:EventGraph"}}`.
- `GetWidgetDescription {"widgetBlueprint": {"refPath": "/Game/KubeCraft/UI/WBP_PlayerHUD.WBP_PlayerHUD"}}`.
- `StartPIE {"options": {"bSimulate": false, "playMode": "PlayMode_InViewPort", "warmupSeconds": 3}}`.
- `execute_tool_script`: the script defines `run()` returning a dict; `execute_tool(name, json.dumps(args))` returns
  a **dict** (don't `json.loads` it).

## Saving
- `save_assets` skips assets that aren't dirty, and compiling does not mark dirty. Make it dirty first: set a property
  to its own value (data assets) or `set_node_position` of a node to where it is (Blueprints), then save.
- After any MCP session: `git status` - closing the editor can re-save Blueprints the user only opened.

## No `mcp__unreal__*` tools in this session (server failed to connect at start)
Call it over HTTP: `MCP_STATE_DIR=<scratchpad> bash .claude/skills/unreal-mcp/scripts/call.sh <toolset> <tool> '<args-json>'`
(toolset `""` for `list_toolsets` / `describe_toolset`). Needs `node`. The session id is cached in
`$MCP_STATE_DIR/mcp.sid` - **delete it after every editor restart**.
- "MCP up after 3 s" right after starting the editor = **another editor was already running** (the user's). Check
  `tasklist` first; two editors on one project overwrite each other's saves.

## Importing Blueprints/widgets whose C++ parent doesn't exist here (e.g. copied from another project)
1. Back up the files. Add `[CoreRedirects]` `+ClassRedirects=(OldName="/Script/OldModule.OldClass",NewName="/Script/UMG.UserWidget")`
   to `Config/DefaultEngine.ini` **before** the editor starts (a running editor doesn't re-read it).
2. Designer values of old C++ properties come back if the Blueprint gets a variable with the **same name and type**
   (add it, compile, save) **before** the widgets that use it are loaded.
3. Clear old logic: `write_graph_dsl` with empty code does **not** clear a graph - `find_nodes {"title": ""}` +
   `delete_node` each; `remove_function_graph` for functions.
4. `AssetTools.move` re-saves the asset (drops old imports) and leaves no redirector. Then remove the redirects,
   restart, check every asset loads + compiles. Textures keep their import source path (may be a temp folder).

## Editing graphs the user arranged (keep their layout)
- `write_graph_dsl` rewrites the **whole** graph (all nodes made again, positions lost) - only use it on new, empty
  graphs. **Once the user has opened or moved nodes in a graph Claude wrote, it is theirs too**: a second
  `write_graph_dsl` there leaves their moved nodes in place, cut off from the event, and wires a new copy in a
  straight line. To add one event to an existing graph, write it into a **new function graph** with DSL and call
  it, or use `create_node` + `connect_pins` - never rewrite the graph. In existing graphs: `create_node` +
  `connect_pins`, `add_node_pin` (e.g. one more Sequence output), `break_pins` only to insert a node into one link. Compare node name + type + position before/after (find_nodes +
  get_node_infos) and show the user that nothing moved.
- `read_graph_dsl` output can show wrong names (another class, nodes twice) - check real links with get_node_infos.

## Gotchas
- DSL picks the first match of an ambiguous id: `Appearance|SetBrushfromTexture` = Border (use `Class|Image|SetBrush`
  + MakeSlateBrush), `Pawn|GetControlRotation` = Controller (pass `Pawn|GetController`). Impure nodes
  (CreateSaveGameObject) cannot sit inline in `(return ...)` - bind them first. Bool vars: setter is
  `Variables|Default|SetHasX` (no b). Enum values in `select` fail - use `if`.
- Make-struct nodes with many fields (MakePostProcessSettings) show no pins: set `ShowPinForProperties[i].bShowPin`
  via set_properties (whole array, same size), then compile. Big values: `call.sh tool @file.json`.
- MCP `AssetTools.delete` can return true but leave a redirector the editor has loaded - delete it with the editor closed.
- Call to Parent: `find_node_types` does not list it, but the type id is `<Category>|Parent:<Function>` (e.g.
  `KubeCraft|Game|Parent:StartWorld`) - try `create_node` with it before asking the user.
- `write_graph_dsl`: widget variables need explicit getters - `(Variables|WBP_X|GetText_Label)`, not a bare name.
- `set_properties` on the Blueprint asset itself fails (`BlueprintDescription` is not on the CDO); to make a widget
  dirty for saving, set a widget property to its own value, or move/rename it.
- Arrays in `set_properties`: changing size **and** contents in one call fails ("insertion points are ambiguous").
  Pass the existing elements exactly as `get_properties` returned them, append first, then set the new ones.
  Works for an object array (catalog `Blocks`): call 1 = old elements as `{"refPath": ...}` objects + `"None"`;
  call 2 = old elements + the new `{"refPath": ...}`. Same-size arrays of structs (`HeightBands`) can be set in one call.
- `save_assets {"asset_paths": ["/Game/KubeCraft/Blocks/DA_X", ...]}` (package paths, no `.Name`);
  `describe_toolset {"toolset_name": ...}`.
- Every material on block ISMs needs `bUsedWithInstancedStaticMeshes = true` (MIs of `M_Block` inherit it).
- `write_graph_dsl`: no `(return …)` in a new function graph; integer `%` is `Math|Integer|%(Integer)`; ambiguous type
  ids pick a wrong class → use `create_node` with `declaring_class`.
- `Set Members` with no field ticked does nothing - tick fields via `ShowPinForProperties` (`bShowPin`).
- During PIE, `CaptureViewport` shows the editor world (sky only) - use the `run-game` capture script instead.
- Live `set_properties` on a PIE Blueprint actor re-runs its construction script (Begin Play state is lost).
- `CloseMainWindow()` while the user is in PIE targets the PIE window - stop PIE first.
- The `unreal` MCP doesn't reconnect by itself if the editor started later → ask the user to run `/mcp`.
  Until then raw HTTP works: POST `initialize` → read `Mcp-Session-Id` header → `tools/call` with `call_tool`.
- First call after editor start may time out (ECONNRESET) while assets load - retry.
- `find_node_types` needs `"context_pins": []`; `AssetTools.is_dirty` / `save_assets` take plain path strings.
- Function outputs: `add_function_param` (`input_param: false`) creates the Return node; more Return nodes (e.g. an
  early `return false`) via `create_node` type `|AddReturnNode...`. A Return node ends the whole function, also
  inside a Sequence.
- `execute_tool_script` is not atomic: calls before an error stay done - check the graph before retrying.
- No tool can set (or read) a Blueprint **function description** / node comments: `ObjectTools` can't read K2Node
  properties (`MetaData`, `NodeComment`), the DSL has no syntax for it. Ask the user (select the function → Details →
  Graph → Description). To *read* comments, grep the saved `.uasset`: `LC_ALL=C grep -aoE "[ -~]{25,}" file | grep " "`.

## Building widget logic
- New WBP: `UMGToolSet.CreateWidgetBlueprint {folderPath, assetName, parentClass: /Script/UMG.UserWidget}`, then
  `AddWidget {widgetBlueprint, widgetClass, widgetDisplayName, parentWidget, childIndex}` (first one = root). Own
  widgets as children: class `/Game/.../WBP_Row.WBP_Row_C`. Their instance-editable vars: `set_properties` on
  `<WBP>:WidgetTree.<Name>` (Text = plain string, Text array = list of strings).
- Added / designer widgets are **not variables** - `ToggleWidgetAsVariable` before any getter or event uses them.
- Bound widget events (`(event OnClicked(Button_X) ...)`, slider `OnValueChanged`) only work after
  `BindToEventProperty {eventName, propertyName, propertyClass}` created the node (`/Script/UMG.Slider`,
  `/Script/UMG.Button`, or `/Game/.../WBP_MenuButton.WBP_MenuButton_C` for its dispatcher). Then the DSL rewrite keeps it.
- Members first, graphs second: `add_variable` (`container_type: ARRAY`, `graph` = local var),
  `add_struct_variable` (`/Script/CoreUObject.IntPoint`), `add_event_dispatcher` + `add_function_param` on the
  returned graph, `add_function_graph` + params, compile; then `write_graph_dsl` per graph.
- Calls on another object (`Class|Slider|...`, `Class|WBPSettingSelector|...`, `Settings|...`): pass the target
  as `:self x` and every other pin **by name** (`:StepSize 1.0`) - positional args go to `self` and fail.
  Pin names: `get_node_type_pins`. Dispatcher calls: `(Default|CallOnX :Value v)`.
- Multi-output nodes bind in **pin order** (`get_node_type_pins`): `GetSupportedFullscreenResolutions` = (Resolutions ReturnValue).
- Negative / some literal ints don't stick (`(CallFunction|Step -1)` became 0) - `set_pin_value` afterwards.
- Enum pins take the value name as a string: `:InFullscreenMode "WindowedFullscreen"`, `:InVisibility "Collapsed"`.
- A pure check (IsScreenResolutionDirty) is evaluated where it is used: store it in a local var **before** the call
  that changes it (ApplySettings).
- Widget `EventTick` is `AddEvent|UserInterface|EventTick (MyGeometry InDeltaTime)`; it ticks while the game is
  paused (world timers / Delay don't).
- Check the real links with `scripts/graph_links.py` (paste into execute_tool_script) - read_graph_dsl names lie.

## Checking a Blueprint the user built (they build graphs themselves; Claude checks)
1. `read_graph_dsl` on each graph (EventGraph, functions) / `GetWidgetDescription` for widgets /
   `get_expression_inputs` for materials.
2. Compare with what was agreed: names, pins connected, no leftover nodes, categories, Expose on Spawn, defaults.
3. Report short: what is right, what is missing - the user fixes it in the editor.
- `ObjectTools.set_properties` takes `values` as a **JSON string** (`json.dumps({...})`), not a `properties` dict.
- A component's value can be overridden in the owning Blueprint (`<BP>_C:<Component>_GEN_VARIABLE`) - check there
  when a CDO value looks right but the game acts wrong (inventory MaxStackSize was 0 on the character).
- Calls to another widget's function use its display name: `Class|WBPLanguageSettings|LoadfromSettings` (lower f).
