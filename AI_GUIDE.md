# AI guide — create a game-specific SDK/editor from CyGameSDK

This is the starting document for an AI adapting this foundation to another game.
It covers source development. An AI operating an already-built editor instead
uses its existing CyToolsCore MCP/SDK catalogue; it does not need a second server.

## Read first and identify the active baseline

1. Read [AGENTS.md](AGENTS.md), [app/AGENTS.md](app/AGENTS.md) and
   [README.md](README.md).
2. Check [runtime/active-release.json](runtime/active-release.json), the version in
   [app/CyTool.json](app/CyTool.json) and `editor_status` if the editor is running.
3. Read [GameAdapterGuide](app/Docs/GameAdapterGuide.md),
   [EditorArchitecture](app/Docs/EditorArchitecture.md) and
   [CapabilityCoverage](app/Docs/CapabilityCoverage.md).
4. Copy [GamePortingTemplate](app/Docs/GamePortingTemplate.md) into the new game's
   documentation and fill it with observed facts, unknowns and evidence.

Current baseline: active 0.28.0, 175 editor operations / 196 tools including Core
controls. Read [CompletionMilestones](app/Docs/CompletionMilestones.md) and
[Native3DAssets](app/Docs/Native3DAssets.md) for current implementations and bounds.
The 0.16.1 UI audit remains historical evidence for the charcoal palette/docking.
The preserved 0.19 candidate binary is historical; its import source was selectively
adopted, rebuilt and tested in 0.20.0. Do not mix historical catalogues/executables.

## What is reusable and what the game integration must supply

| Reusable foundation | Work specific to the target game |
|---|---|
| Docking, panels, workspaces, command search | Product identity, editor workflows and specialized panels |
| Descriptor-based property editing | Real entity types, schemas, defaults, constraints and migrations |
| Scene commands and document Undo | Conversion between the document and real game/level data |
| Asset browser and editor registry | Format decoders/encoders, dependencies and specialized editors |
| Viewport callback and per-view cameras | Engine rendering, coordinate conversion, picking and gizmo integration |
| Typed logic/material graphs and debugger | Bindings to the game's runtime and supported compilation semantics |
| Build target/provider seams | Actual compiler, packaging format, diagnostics and validation in the game |
| CyToolsCore MCP/SDK/jobs/workers | Typed operations and trusted source providers for the integration |

An extension filter is not an importer. A custom node is not an engine binding.
The DX11 material preview is an editor preview, not a game material compiler.
Sandbox ZIP packaging contains document data, not a cooked or runnable game.

## Shared portable 3D assets

Read [CommonAssetFormat](app/Docs/CommonAssetFormat.md) before inventing a game-specific
interchange mesh/animation format. The independent `portable_assets` 0.1 component
provides `.cypack` v1 (JSON envelope + unchanged GLB), a conversion capability planner,
CLI and optional official Core worker handlers. Use it for asset transfer between
derived SDKs. Native 0.21 adds bounded .cy3d documents, mesh/skeleton/animation
editing and GLB/CyPack import/export through the existing Core workers. Reference-pose retargeting, static LODs/collisions and expanded graph runtime are
implemented in [AdvancedAuthoring](app/Docs/AdvancedAuthoring.md). Real game codecs
and full production authoring remain separate work. The optional
standalone portable_* operations remain opt-in; native operations use model_* and
asset_import/reimport.

## Recommended delivery order

1. Establish the game's exact version, files, tools and a small known-good fixture.
   Record coordinate system, units, object IDs and the format evidence available.
2. Create a separate game-editor project at the destination requested by the user.
   Keep source under `app`, user data under `data`, and private runtime/build files
   under `runtime`. Install dependencies at their final path; do not copy the venv.
   Preserve third-party notices and user data when deriving the project.
3. Implement a minimal source adapter and prove scene/schema/persistence contracts.
   [ExampleGameAdapter.h](app/examples/adapters/ExampleGameAdapter.h) is a checked
   starter for the existing entity convention; it does not decode a real game.
4. Implement one small read-only asset/level decoder and an editor for its result.
   Make format failures explicit; retain originals and unknown fields where needed.
5. Add a renderer/picking bridge and verified coordinate conversion. Test multiple
   viewports and revision-checked transformations in the actual graphical host.
6. Add one exporter/build provider and validate its output with the real game or
   its official validator. Keep diagnostics navigable and cancellation real.
7. Expose the same functions to AI through the manifest and native dispatcher.
   Add engine/runtime graph bindings only when their semantics are implemented.
8. Ship the filled game dossier, exact setup/build instructions and evidence of
   what works. Separate implemented, tested, unavailable and planned capabilities.

Start with a small end-to-end path for the game. A generic-looking editor window
alone is not a finished game SDK.

## Source map

| Need | Entry points |
|---|---|
| Adapter contract and injection | `app/native/Core.h`, `Core.cpp`, `main.cpp` |
| Scene/selection/transactions | `app/native/Scene.cpp`, `Core.cpp` |
| Properties and outliner | `app/native/Editor.cpp`, `Outliner.cpp` |
| Viewport integration | `app/native/Editor.h`, `Viewport.cpp`, `Gizmo.cpp` |
| Asset routing and references | `app/native/Assets.cpp`, `Content.cpp`, `AssetReferences.cpp` |
| Node definitions/runtime/functions | `app/native/Graph.h`, `Graph.cpp`, `GraphRuntime.cpp`, `Functions.cpp` |
| Material CPU/GPU preview | `app/native/MaterialProgram.cpp`, `MaterialPreview.cpp`, `app/shader_codegen.py` |
| Portable 3D contract | `app/portable_assets`, `app/portable_asset_worker.py`, `app/schemas`, [format guide](app/Docs/CommonAssetFormat.md) |
| Game build jobs | `app/native/Build.cpp`, `app/build_service.py`, `app/package_worker.py` |
| AI operation declarations | `app/build_manifest.py`, `app/milestone_contracts.py`, `app/next_contracts.py` |
| SDK/MCP bridge | `app/tool.py`, `app/build_bridge.py`, `app/mcp.example.json` |
| Extract pure graph regions | `app/native/GraphCollapse.cpp`, [collapse guide](app/Docs/GraphCollapse.md) |
| Expand pure calls into editable nodes | `app/native/GraphExpand.cpp`, [expansion guide](app/Docs/GraphExpand.md) |
| Local enum/record migrations | `app/native/TypeMigration.cpp`, [type migration guide](app/Docs/TypeMigration.md) |
| Localization | `app/locales/ui.json`, [Localization](app/Docs/Localization.md) |

## Prompt to give the next AI

```text
Create a game-specific SDK/editor from the CyGameSDK foundation.
Target game and exact version: <GAME / VERSION>.
Destination project directory: <ABSOLUTE DESTINATION>.
Available game files and representative fixtures: <PATHS>.
Known format/toolchain documentation: <PATHS OR SOURCES, OR UNKNOWN>.
First useful workflow: <FOR EXAMPLE: OPEN, EDIT AND EXPORT ONE LEVEL>.

Read <FOUNDATION_ROOT>/AGENTS.md and AI_GUIDE.md first. Follow
app/Docs/GameAdapterGuide.md and fill app/Docs/GamePortingTemplate.md in the
new project. Verify the active baseline and its current catalogue; do not copy
operations or binaries from historical archives. Reuse CyUI and the existing CyToolsCore MCP/SDK/jobs.
Do not modify sibling dependencies or the original game files. Keep English
as the default UI language and preserve selectable translations.

Implement and verify the smallest real end-to-end game workflow first. Declare
operations in the manifest and implement the same native dispatcher path used
by the UI. Document unsupported formats, remaining Sandbox assumptions and
actual verification results. Ask only for missing game-specific facts that
cannot be recovered from the authorized files/sources.
```

## Verification and supporting contracts

Read [GameAdapterGuide](app/Docs/GameAdapterGuide.md) for the current adapter's
limitations before implementing new entity types. Build/provider semantics:
[BuildProviders](app/Docs/BuildProviders.md). Graph/material boundaries:
[MaterialPipeline](app/Docs/MaterialPipeline.md). Worker/lifetime/AI contracts:
[Workers](app/Docs/Workers.md), [TaskFiles](app/Docs/TaskFiles.md),
[MultiClient](app/Docs/MultiClient.md), [MCP](app/Docs/MCP.md).

The generic [AgentGuide](app/Docs/AgentGuide.md) describes standalone CyToolsCore
products. Its model/CUDA/download/mesh requirements are conditional on a real need
in the target product; they are not additional implemented CyGameSDK features.

## Advanced authoring and codecs

Read [AdvancedAuthoring](app/Docs/AdvancedAuthoring.md) and
[GameCodecs](app/Docs/GameCodecs.md). Native model_process prepares a bounded
snapshot, official Core workers compute it, and a hash-guarded commit creates one
Undo transaction. Use model_process for static LOD/collision/retarget operations.
Read exact type/rig/format limits; the implementation is not full Unreal parity.
For a game export, register GameCodecProvider in the derived SDK's trusted source
factory. Native assetCodecs declarations are optional discovery, not executable
codecs. Verify actual game outputs; a profile or synthetic fixture is insufficient.

Native 0.22 animation tools: read app/Docs/AnimationTools.md. Keep fixed-root IK
scale/length bounds, sampled root-motion fingerprints and explicit loss reporting.
No game movement/physics binding follows merely from extracting a motion clip.

Native 0.23 graph types: read app/Docs/NamedGraphTypes.md. Keep libraries local,
nominal pin identity, context-aware schemas and function/macro/runtime inheritance.
Type edits must validate all uses atomically; never silently migrate user values.

Native 0.24 interfaces: read app/Docs/GraphInterfaces.md (Docs/GraphInterfaces.md
from app). Keep contracts local, signatures nominal and class bindings validated.
Interface calls dispatch through shared runtime objects; include every possible
implementation when detecting recursion. Carry the interface library through
function views, macros, clipboard merges and runtime children without duplicating
it in serialized function bodies. --test-interfaces controls only its own ImGui.

Native 0.25 signature migration: read app/Docs/SignatureMigration.md (Docs from app).
Migration is explicit preview/apply on one owning graph. Never migrate on load.
Recompute the candidate from the request; require its exact planHash and explicit
acceptLoss when connections/fallbacks are removed or reset. Keep closure across
interfaces, method overrides and shared bindings, one Undo and unknown metadata.
Do not change mode, kind, self, or foreach callback item/index/break contracts.
--test-migration uses only an isolated host's own ImGui input.

Native 0.26 type migrations: read app/Docs/TypeMigration.md (Docs from app).
Migrate one local enum/structure explicitly, retaining nominal ID/kind and unknown
metadata. Normalize dependent declaration defaults before resolving node descriptors.
Recompute/validate the candidate, require the exact planHash and explicit loss
acceptance, and keep one Undo. Do not migrate on load, cross-asset registries or
live runtime objects. A removed field referenced by struct.set blocks the plan.
--test-type-migration controls only its isolated host's own ImGui input.

Native 0.27 graph extraction: read app/Docs/GraphCollapse.md (Docs from app).
Collapse only supported pure dependency regions. Keep variable/boundary/execution
nodes outside and check dependencies per output pin, including independent UV
branches. Every export must use every selected node and incoming wire. Never prune
nodes or evaluate unused external inputs merely to construct a helper. Preserve
original node/link IDs, fallback values and metadata, deterministic generated IDs,
exact source/request hashes and whole-owner Undo. --test-collapse uses only its
isolated ImGui host's own canvas/modal/keyboard input. No engine codec is implied.

Native 0.28 call expansion: read app/Docs/GraphExpand.md (Docs from app).
Expand only pure local calls, retain their definition and every interior node.
Keep typed boundary reroutes, external wire IDs/defaults and original call JSON in
comment provenance. Every connected function output must read every wired argument;
include nested macro laziness and per-output UV dependencies. Recompute/validate the
candidate and runtime macro expansion, require the exact hash and keep one Undo.
--test-expand injects only its isolated host's own ImGui canvas/modal/key input.
