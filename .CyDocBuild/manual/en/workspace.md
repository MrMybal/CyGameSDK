---
id: workspace
title: Workspace, layouts and help
description: Workspace, layouts and help
section: Getting started
order: 2
revision: 1
---

## Using this guide {#getting-started}

The startup Sandbox is a demonstration scene. Ctrl+O loads a saved project; Ctrl+S saves it. The example installer refuses to overwrite assets.

- Level: dock panels, select objects, inspect properties and use transform gizmos.

- Scripting: open a graph example, inspect typed connections and use the debugger.

- Materials: open Example_TexturedMaterial.cyasset and compile its DX11 preview. This is an editor shader, not a game material export.

- Examples3D: open StaticTriangle.cy3d or AnimatedTriangle.cy3d for mesh, skeleton, animation and GLB/CyPack workflows.

- Ctrl+P opens command search. Ctrl+Space opens the content drawer. The Content Browser has file filters, favorites and references.

## Continue reading {#next}

- [First editing session](doc:quickstart)
- [Browse assets](doc:content-browser)
- [Graph authoring](doc:graphs)
- [Materials](doc:materials)
- [3D assets](doc:assets-3d)
- [SDK operation reference](doc:operations)

```image
{"id": "workspace", "src": "assets/screenshots/workspace.png", "alt": "Sandbox workspace", "controls": "minimal"}
```


## Layouts, density and language {#layouts}

Use Window to show a panel or create another independent viewport. Level, Scripting and Materials presets arrange the same tools. Save/load a named layout explicitly; names accept letters, numbers, underscore and dash. Layouts and presentation preferences are not document Undo. Ctrl+Space opens a drawer that can dock back into the layout.

Options select theme, compact/comfortable density, text scale and language. English is the source/default/fallback, independent of Windows locale. An explicitly saved French choice is preserved. Stable data/window IDs do not change with translated captions. User captions and filenames are never translated. Escape cancels a property draft; a stale or hidden field must not replay old ImGui edits over new SDK state.

## Contextual help {#help}

Help > Documentation opens the local manual. F1 opens the guide for the focused editor panel, retaining the editor language. Focus a tool, press F1, and follow its stable section link. This launches a local file URL; it does not start a documentation server.
