# Porting CyGameSDK to a game — source integration guide

This guide describes the active 0.23 C++17/Win32/DX11 implementation. Start at
[../../AI_GUIDE.md](../../AI_GUIDE.md). Complete [GamePortingTemplate.md](GamePortingTemplate.md)
for the target game. The checked [example adapter](../examples/adapters/ExampleGameAdapter.h)
is a starter, not an implementation for an existing game.

## 1. Establish the destination and evidence

Work in the game-editor destination authorized by the user. Preserve the original
foundation and the installed game. Keep versioned test fixtures and known-good
outputs separate from user projects. Record game/platform/build IDs, available
format information, toolchain versions, coordinates/units and output validation.
Unknown facts stay explicit. Consult authorized local sources before asking for
missing information; verify external format/version claims against primary sources.

Derive the source project and notices, then install the private runtime at its final
location with `app/setup_mcp.ps1` (its `CyToolsCoreRoot` parameter selects the sibling
Core source). Do not copy `runtime/python`: it is a local virtual environment.
Keep `app`, `data`, `runtime`. Update product/adapter identities and paths in the
manifest generator, build scripts, launcher and MCP configuration together.

## 2. Implement and inject IGameAdapter

The exact contract is in [../native/Core.h](../native/Core.h):

| Method | Responsibility |
|---|---|
| `descriptor() const` | Stable adapter ID, display name, version and truthful capabilities |
| `propertySchema(const Json&) const` | Editable entity fields and constraints; also accept an empty object for generic discovery |
| `createEntity(id, name) const` | Return a valid `{id,type,properties}` entity with initialized defaults |
| `initialScene() const` | Initial editor entities; the current default host initially selects `hero` |
| `validateScene(entities) const` | Validate IDs, types and schema; throw on invalid persisted data |
| `buildTargets() const` | Optional targets; the base implementation returns an empty array |

In the derived project's `app/native/main.cpp`, after including the example header:

```cpp
#include "../examples/adapters/ExampleGameAdapter.h"
// Replace the existing EditorCore core(root) construction, not the whole host:
EditorCore core(root, std::make_unique<cygame::examples::ExampleGameAdapter>());
EditorApp app(core);
```

Register game UI/renderer hooks after EditorApp construction. Keep adapter parsing,
validation and serialization independent of ImGui. Adding implementation .cpp files
requires adding them to both `app/build_native.cmd` and `app/CMakeLists.txt`.
The example header is source-only and is not selected by the default application.

Capabilities describe implemented behavior; setting `build:true` does not add a
compiler. There is no runtime `adapter_load` operation or automatic DLL discovery.
`IEditorPlugin::activate/deactivate` receives `EditorCore&`, not `EditorApp&`.
UI registrations use an application reference with an explicit lifetime. Implement
callback retirement before claiming dynamic unload/hot reload.

## 3. Keep a compatibility bridge until Sandbox assumptions are removed

The interface is an extension seam, but the current host still has concrete
assumptions. Adapt these deliberately instead of changing only an adapter ID:

| Assumption / location | Required integration decision |
|---|---|
| `Core.cpp` initially selects `hero` | Supply it initially, as the starter does, or generalize initial selection consistently |
| Core seeds `logic` and `material` graphs; project loads require both | Retain them or update creation/load validation and corresponding UI together |
| Scene/UI use name, enabled, mobility, position/rotation/scale, tint | Keep a compatible editor representation or generalize commands, outliner, focus and gizmos |
| Scene duplication offsets a 3-component position; generic creation accepts a name | Add explicit game entity-type creation and mapped transforms before exposing arbitrary types |
| Scene command cap is 2,000 entities | Design/test a scaling strategy before claiming large-level editing |
| `project_save/load` use `data/projects/Sandbox.cyproject.json` | Replace/configure this path and product labels in a derived project |
| Project JSON includes format/schemaVersion/adapterId | Preserve identity checks; add explicit migrations instead of loading another game's project implicitly |
| `propertySchema({})` is used for discovery | Provide a valid general schema or adapt discovery for per-type schemas |
| Native validator implements a subset of JSON Schema | Extend native checks before using unsupported constraints accepted only by Core's full schema validation |

Native `validateValue` currently handles primitive type checks, enum, number bounds,
string byte-length bounds, array bounds/items, and object required/properties/
additionalProperties. Do not assume `$ref`, `oneOf`, `pattern` or formats are enforced.
For scene persistence, throw on invalid data: `project_save/load` currently call
`validateScene` without interpreting a returned `valid:false` result. The build
path also handles that return shape; persistence must still reject explicitly.

The example retains `StaticMesh` plus these conventional fields and adds `health`
as a bounded integer to demonstrate automatic property widgets. It validates its
factory output and rejects bad values/duplicate IDs. A different game's actual
entity layout, type registry and runtime reflection remain your implementation.

## 4. Implement one real asset path

For cross-SDK 3D transfer, start with [CommonAssetFormat.md](CommonAssetFormat.md)
and `portable_assets`. Keep the portable envelope/GLB convention common across games;
implement game -> common and common -> game in trusted providers. Preserve opaque
metadata and UUIDs, detect target identity conflicts and report conversion losses.
Animation retargeting is separate from transporting a clip with its original rig.


`EditorApp::registerAssetEditor(AssetEditorDefinition)` maps an ID and lowercase
extension list to an `open(const Json&)` callback. The browser passes a descriptor
containing a content-relative `path`. Register a panel with a stable ID in
`core.panels`; the callback can load/inspect through a declared operation and open
that panel using `app.open(panelId)`. See [../native/Editor.h](../native/Editor.h)
and the image/graph registrations in Editor.cpp.

A registration alone supplies no parser, editable document, save behavior,
revision tracking, dependencies or Undo. Add those contracts for the format.
Keep file paths confined to the intended content/input roots; bound file size and
decoding. Preserve original files and distinguish source data, editable documents
and generated game output. Start with read-only inspection, then prove a validated
roundtrip before exporting changes. Retain unknown fields when the format needs it.

Active 0.23 provides .cyasset graphs, WIC PNG/JPEG/BMP viewing/sampling,
image import/reimport/thumbnails, bounded .cy3d mesh/rig/animation editing and a
JSON tree editor. Read AssetImport.md and Native3DAssets.md. File lifecycle
operations remain outside document Undo; document authoring uses Undo. Native
game-format dependencies require a provider or extension of AssetReferences.cpp;
its index reads declared graph/material references, not arbitrary game archives.

## 5. Supply a real viewport

Set `app.viewportRenderer` to a callback receiving `ViewportContext&` from Editor.h:
canvas origin/size, the view's Camera and the live EditorCore. A non-Sandbox adapter
without it intentionally displays a missing-renderer message.

The callback replaces the default viewport draw path. It does not automatically
receive the Sandbox picking, middle-button pan implementation or gizmo handling
that occurs later in Viewport.cpp. Integrate those with the game's actual camera,
view/projection matrices and selection semantics. For GPU rendering, preserve
DX11 state used by ImGui and retain texture resources through draw submission.
Document units, handedness, axis orientation and rotation conventions. Test import,
render, picking, edits and export with the same fixture to catch conversion errors.

## 6. Bind game graphs and materials

`registerNodeType(id, descriptor, NodeEvaluator)` in Graph.h adds trusted, typed
pure source nodes. Register before constructing the editor. The evaluator receives
resolved inputs/default values and returns named outputs. `core_tests.cpp` has a
working custom.double example. Keep IDs stable across saves and declare migrations.

This registry does not supply arbitrary custom execution/latent node handlers,
object references, a game VM or graph-to-game compilation. Implement such runtime
semantics explicitly in the appropriate layer before advertising them. See
GraphRuntime.cpp and Functions.cpp for the bounded existing interpreter.

A CPU material evaluator is separate from GPU HLSL lowering. New custom pure nodes
do not automatically become supported by shader_codegen.py. Add/test supported
lowering or report the expression unsupported. Native engine material export and
shader compilation are distinct from the existing editor DX11 preview. See
[MaterialPipeline.md](MaterialPipeline.md).

## 7. Expose identical functions to the GUI and AI

Declare a namespaced operation ID and exact input/output schemas in the manifest
sources (`build_manifest.py`, milestone_contracts.py or next_contracts.py). Use an
existing operation as the metadata pattern, including permissions/resources,
progress/cancellation and exclusiveGroups. Then regenerate:

```powershell
.\runtime\python\Scripts\python.exe app\build_manifest.py
.\runtime\python\Scripts\python.exe app\tool.py describe
```

For example, after declaring a read-only `example_game_info` returning an object:

```cpp
core.bindCommand("example_game_info", [&core](const Json&) {
    return core.adapter().descriptor();
});
```

Bind after core construction. A GUI action calls `app.invoke`/`core.execute`; tool.py
forwards short SDK/MCP operations through the same dispatcher. A menu-only handler
is insufficient for AI access. Mutations need schema validation, atomic failure,
Undo policy and expectedRevision as appropriate. Do not bypass transactions by
mutating `core.entity()` from a worker. Independent view settings are not document
mutations. Discover current schemas through `cy_list_operations` /
`cy_describe_operation`, rather than hardcoding historical tool counts.

CyToolsCore remains the MCP/SDK/job implementation. For long work, use a trusted
ProcessWorker provider and a bounded native snapshot/commit operation. Do not hold
the ImGui thread while compiling or parsing a large archive. Respect Core task-file
privacy and lifetime; explicit asset/project saves are persistent user operations.
After a disconnect, inspect live revision/state before retrying a mutation.

## 8. Add the game build/export provider

Declare targets via `IGameAdapter::buildTargets()`, then a matching trusted Python
`BuildProvider` in build_service.py. Register defaults for the desktop bridge too;
passing a provider map in one SDK script does not configure the GUI automatically.
[BuildProviders.md](BuildProviders.md) gives the exact worker/report contract.

Existing request/result paths are oriented toward an archive (`archivePath`, report
and package metadata). A real compiler with a different result requires extending
the service/output schema and Build UI. Preserve fixed argv invocation, owned job
paths, diagnostics and cancellation. Never read executable commands from asset JSON.
Only announce game build/play after running the result or an appropriate game validator.

## 9. Verify and hand off

In the derived project, run the native contracts and integration tests against an
isolated workspace. Add meaningful game-specific tests for real fixture decoding,
roundtrip output, corruption, size/path bounds, unsupported versions, coordinates,
atomic edits/Undo, schema/API parity and worker cancellation. Confirm the actual
graphical host, English default, another language and representative themes.

Ship setup/build instructions, filled game dossier, version/capability matrix,
formats, commands/schemas, fixtures and actual evidence. Record unsupported engine
features and remaining Sandbox assumptions. Do not mark the SDK done with mocked
compiler output or screenshots alone. Existing foundation commands:

```powershell
.\app\build.ps1
.\runtime\build\CyGameSDKTests.exe <ABSOLUTE_DERIVED_PROJECT>
$env:CYGAME_TEST_GUI='1'
.\runtime\python\Scripts\python.exe app\tests\test_integration.py
```

A build replaces that project's executable; stop only its owned editor after
preserving user work. Documentation-only changes do not require replacing the
currently running editor. Adapt Sandbox assertions in the derived test suite to
the game while preserving the foundation's relevant behavioral checks.

## Advanced graphs and asset conversion

Read AdvancedAuthoring.md before binding graph objects/events to a real engine.
Current references are transient interpreter instances, logical time is explicit,
and the built-in runtime is not an Unreal Blueprint binary/compiler/runtime.

GameCodecs.md describes GameCodecProvider and the trusted default_codecs() source
registry used by both SDK and GUI. Optional IGameAdapter::assetCodecs() declarations
do not register executable handlers. Provide exact game versions/platforms, real
fixtures, honest losses and target validator evidence. Do not ship the synthetic
fixture_codec_worker as a game provider. Preserve UUIDs, unknown metadata, source
hashes, stale LOD/review flags and official Core job ownership/cancellation.

Read AnimationTools.md before binding offline IK or root-motion curves to an engine.
Define coordinate, root lock, scale, movement/physics and loss policies explicitly;
a sampled reconstruction test is not a real game animation-codec validation.

Read NamedGraphTypes.md for local enum/record contracts. Keep nominal IDs and
unknown metadata; an engine/shared type registry requires explicit binding and
migrations. The current values do not imply engine reflection or ABI compatibility.

Read TypeMigration.md for explicit local enum/record evolution in 0.26. Keep stable
nominal IDs, unknown metadata and reviewable loss policies. This workflow does not
migrate shared assets on load or bind values to a game's reflection/runtime ABI.

Read GraphCollapse.md before extending pure-region extraction. Future custom
output-specific evaluators must extend dependency analysis before being supported;
never assume every output reads every input. Preserve strict game/runtime boundaries.
