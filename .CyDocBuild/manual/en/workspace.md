---
id: workspace
title: Workspace, layouts and help
description: Workspace, layouts and help
section: Getting started
order: 2
revision: 2
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

## Typed properties {#typed-properties}

Details now uses the common CyUI Inspector for scalar, vector, record, array, Set and Map fields. Double-click a numeric field or choose its expression action to enter arithmetic such as `Value*2`. The application still validates the proposed value against the game adapter or graph schema before committing it. Exact integer storage is preserved.

Collections keep their element schema when empty. Their rows are virtualized and support filtering and common collection actions. A graph's existing element-count, size and type limits still apply; a more capable widget does not expand the document format. Authored enum members retain their original text and nominal graph identity.

Press Escape to cancel an active draft. An external document change or a hidden field cancels a stale edit. A successful field or collection edit creates one document Undo transaction.

## Hierarchy and scene table {#scene-table}

The World Outliner switches between **Hierarchy** and **Table view**. The hierarchy supports keyboard navigation, Ctrl/Shift selection, F2 rename, visibility, locking and dragging scene objects into folders. **Focus selection** frames the current object in the viewport. Folder rows describe the existing scene organization; they are not engine entity parents.

The table shows the adapter's property columns, including typed vectors and colors. Sort or filter the visible rows, select a rectangular range, and use Ctrl+C/Ctrl+V for typed TSV. A paste validates the complete batch before applying any cell; read-only fields, locked objects or an invalid value reject it. One accepted paste is one Undo. These edits use the same `property_set_many` Core operation available to the SDK.

## Keyboard shortcuts {#shortcuts}

Options > **Keyboard shortcuts** lets you rebind the common save/load, Undo/Redo, search, content drawer and context-help commands. Same-context collisions are rejected. Escape cancels capture and Backspace clears a binding. Bindings are presentation preferences; menus and toolbar tooltips use the registry's current shortcut text. An explicit language choice does not change command identity.

Text editors keep their own text Undo while typing. The global document commands run through the existing EditorCore dispatcher.
