---
id: extensions
title: Source adapters and editor extensions
description: Source adapters and editor extensions
section: Integration
order: 60
revision: 1
parent: game-adapters
---

## Public seams {#contracts}

`IGameAdapter` provides `descriptor()`, `propertySchema(entity)`, `createEntity(id,name)`, `initialScene()`, `validateScene(entities)` and optional `buildTargets()`/codec declarations. Accept an empty entity for generic schema discovery. Keep parsing, validation and serialization independent of ImGui. Inject an implementation at `EditorCore` construction in a separate destination project.

`EditorApp` can register asset-editor definitions and viewport/mesh/material/texture callbacks. `IEditorPlugin` has source lifecycle hooks; activation receives `EditorCore&`, not an arbitrary dynamic UI object. The Diagnostics example is a source plugin, not an ABI-stable DLL loader. Callbacks must be retired before their captured data is destroyed. Assets cannot supply executable plugins.

## Sandbox assumptions to review {#assumptions}

The starter retains the initial `hero` ID, transform properties, StaticMesh entity convention, initial logic/material graphs and bounded scene/project limits. The default viewport projects Sandbox primitives; a game's renderer and picking must be connected explicitly. Real schemas, entity/component reflection, game persistence and tools need verified implementation, not renamed labels.

The shipped `ExampleGameAdapter.h` adds a bounded Health field and retains existing transforms. It is an example, not a game decoder. Its annotated public contract appears in the API tree; full source remains in the source distribution, not a source browser inside this manual.

Add implementation files to the CMake target, select CyUI modules explicitly and keep one compatible ImGui. Preserve English default/fallback, stable UI/data IDs and source notices. Read the repository's AI guide and game porting dossier when deriving an SDK; assistant instructions are separate from this human manual.

## Reference code examples {#code-examples}

Original source-guide examples. Paths, sample names and explanatory placeholders must be adapted; they are not a working codec for an arbitrary game.

```cpp
#include "../examples/adapters/ExampleGameAdapter.h"
// Replace the existing EditorCore core(root) construction, not the whole host:
EditorCore core(root, std::make_unique<cygame::examples::ExampleGameAdapter>());
EditorApp app(core);
```

```powershell
.\runtime\python\Scripts\python.exe app\build_manifest.py
.\runtime\python\Scripts\python.exe app\tool.py describe
```
