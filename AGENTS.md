# CyGameSDK instructions for AI developers

Start with [AI_GUIDE.md](AI_GUIDE.md), then [app/AGENTS.md](app/AGENTS.md).
This repository is a reusable native game-editor foundation. A game-specific SDK
requires an adapter, real format/toolchain knowledge and verification against that game.

- Check `runtime/active-release.json`, `app/CyTool.json` and the running
  `editor_status` before assuming a version or capability. Currently the active
  source/catalogue/executable is 0.28.0. The preserved 0.19.0 candidate is under
  `runtime/development-0.19.0`; its executable remains historical. Import source
  was adopted and verified in 0.20.0; read `app/Docs/CompletionMilestones.md`.
- For a new game, follow `app/Docs/GameAdapterGuide.md` and fill in
  `app/Docs/GamePortingTemplate.md`. Use a separate destination authorized by the user.
  Do not overwrite this reusable foundation or the user's installed game files.
- CyToolsCore provides MCP, the AI SDK, identities, jobs and ProcessWorker.
  Extend operations/providers; do not implement a replacement MCP server or scheduler.
- CyImgui_Interface supplies CyUI/ImGui. Read its `docs/AI_GUIDE.md` before native UI
  edits. Do not modify sibling repositories as part of a game adapter task.
- English is the source/default/fallback UI language, independent of Windows locale.
  Preserve an explicitly selected language and add other languages to the catalogue.
- `IGameAdapter` is a source-level extension seam, not automatic support for an
  arbitrary engine. Record remaining Sandbox assumptions before claiming support.
- Do not claim a working game parser/exporter/compiler/renderer because a menu,
  extension filter, graph node or capability flag exists. Verify real game output.
- Keep runtime and manifest/source versions matched. Preserve project/asset data.
  Do not rebuild or replace the user's running editor just to update documentation.

The generic CyToolsCore documents under app/Docs are reference material. Model
installers, CUDA, mesh generation and other product-specific requirements apply
only when the requested game integration actually uses those features. The
CyGameSDK-specific guides above describe this editor's current implementation.

Include root AGENTS.md and AI_GUIDE.md in future source distributions/checkpoints,
alongside the app documentation and examples. Historical archives remain unchanged.

Before designing cross-game mesh/skeleton/animation storage, read
app/Docs/CommonAssetFormat.md. Reuse the portable contract and preserve asset IDs
and unknown metadata. Conversion profiles are declarations, not game codecs.
The portable 0.1 component supplies interchange. Native 0.21 integrates a bounded
.cy3d editing/import/export profile; read app/Docs/Native3DAssets.md for its limits.

For 0.21 authoring, read app/Docs/AdvancedAuthoring.md and GameCodecs.md. Graph
simulation time is distinct from Core job scheduling. Object handles are transient;
keep type/budget checks, scoped macro expansion and source debugger mappings.
Honor generated-LOD stale flags and collision review markers after geometry edits.
No real game codec is bundled; require a chosen target, real fixtures and target
verification before claiming compatibility. Use the trusted source registry.

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
