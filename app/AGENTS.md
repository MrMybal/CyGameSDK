# CyGameSDK developer guide

For a new game SDK/editor, start with ../AI_GUIDE.md, Docs/GameAdapterGuide.md
and the dossier in Docs/GamePortingTemplate.md. Check runtime/active-release.json
at the project root; preserved candidate sources are not active extensions.

Read the root README.md, Docs/EditorArchitecture.md, Docs/CapabilityCoverage.md,
CyTool.json and tool.py. The app began with cytools new --desktop, and replaces
the generated Python UI with a native C++17 CyUI / ImGui / DX11 host.

- Read ../CyImgui_Interface/docs/AI_GUIDE.md when editing native UI.
  Use palette roles, dp(), property tables, translated labels and stable ### IDs.
- CyToolsCore remains the MCP/SDK/job implementation. Do not copy its runtime,
  rewrite its scheduler/authentication/MCP parser, or modify sibling repositories.
- app/build_manifest.py is the source of the operation catalogue. Regenerate
  CyTool.json after editing it. The GUI, native IPC and MCP use EditorCore::execute.
- Keep game adapters and graph data independent of ImGui. Inject IGameAdapter;
  register asset editors, viewport callbacks, panels and pure graph node types.
- Declare source-plugin operations in the manifest and bind their native handlers.
  Do not claim an ABI-stable DLL/plugin loader, Blueprint compatibility, shader
  compilation or game build/play unless implemented and verified.
- Graph/scene edits validate before committing and participate in document undo.
  Long compiles/imports must become CyToolsCore jobs/workers, not UI-thread work.
- English is the source/default/fallback UI language, independent of Windows locale.
  Preserve an explicitly selected language and keep other languages selectable.
- Every visible label goes through the EN/FR catalogue app/locales/ui.json.
  Preserve IDs, filenames, user content and protocol values without translation.
- Write job outputs through JobContext.path. Explicit project/asset/configuration
  saves are persistent user operations. SDK task files follow Docs/TaskFiles.md.
- Keep stdout structured in CLI/JSONL/MCP. Native logs are bounded and selectable.
- Build: app/build_native.cmd (or app/build.ps1). Tests:
  runtime/build/CyGameSDKTests.exe <absolute project root>, then
  runtime/python/Scripts/python.exe app/tests/test_integration.py.
- Use --capture <absolute.bmp> --panel viewport|logic|material|options --lang en|fr
  --theme <CyUI key> for renderer QA. Capture mode does not save preferences/layout.
- Preserve notices in LICENCES. Distinguish tested code from planned capabilities.
  No game-specific model/CUDA/download/install flow applies to the current Sandbox.

- Functions are local to each graph asset. Missing mode means pure; execution mode
  adds exec/then pins, isolated locals and return semantics. Resolve dynamic descriptors
  with nodeDefinition; do not assume every node has a static nodeTypes descriptor.
  Keep recursive calls prohibited and share budgets across nested evaluations.
  Preserve execution-output caching, explicit return failures and shared step limits.
  Debug state includes context-qualified nodes and a call stack; never navigate a
  function node in its owner root by assuming IDs are globally unique.
- Reference scans combine saved/live declarations. Preserve protections against
  moving referenced files, and report incomplete index results explicitly.
- For graphical authoring QA, --test-graph and --test-gizmo operate only on an
  isolated --root. --test-functions exercises real ImGui keyboard navigation.
  --asset and --function select documents for capture mode.

- Material instances keep overrides separate from the parent topology. Resolve
  against loaded parents first, preserve path confinement/cycle/type checks and
  include implicit parent edges in the asset reference index.
- Build targets belong to IGameAdapter; trusted Python source providers run through
  build_service.py and official CyToolsCore ProcessWorker. build_bridge.py is a GUI
  SDK client, not a scheduler. Never accept compiler commands from asset JSON.
  Outputs/diagnostics/snapshots use JobContext.path; desktop sessions are persistent.
  Read Docs/BuildProviders.md for the provider contract and current archive scope.

- Graph comparisons read the saved asset on demand (or the session baseline for
  unsaved built-ins). They never apply changes or reset document Undo. Bound details,
  preserve full summary counts and UTF-8 when truncating previews. No filesystem
  watcher, arbitrary-version diff or visual graph merge is implemented yet.


- Preview shaders compile only through shader_service.py and the official Core
  ProcessWorker. Never put asset HLSL, includes or commands into a compiler request.
  Read Docs/MaterialPipeline.md. Keep parameter ordering consistent in native plans
  and Python lowering; root defaults are uniforms, function defaults are program data.
  Presentation edits must not invalidate a shader. Check signatures, confined paths
  and both hashes before installation; retained GPU objects must not depend on job files.
- UV sampling carries context through nested pure calls. A failing center sample must
  not block preparation of a valid GPU program or sampling of a different UV point.
- Image decoding is bounded WIC PNG/JPEG/BMP, separate from ImGui and its renderer.
  Preserve content-root confinement, file/dimension/pixel caps and the eight-entry
  cache. Native 0.20 includes bounded material samplers/importers; pixel editing
  and game texture codecs remain work. Read Docs/MaterialPipeline.md.
- Renderer QA may use --capture-after 40..1200 frames, --capture-ready <marker> and --panel texture|compiler.
  Copy locales into isolated test workspaces. Native view settings are presentation,
  not undoable document mutations.

- Shared asset interchange: read Docs/CommonAssetFormat.md and portable_assets.
  Preserve GLB bytes for identity transfer; game conversion must report losses and
  enforce coordinate/rig/format limits. Never treat an envelope integrity check as
  full glTF semantic or engine-output validation.
- portable_assets.service registers optional handlers into the existing Core runtime.
  Declare their descriptors, skip native proxies for those IDs, and bind the desktop
  job path before exposing GUI actions. Native 0.20 uses model_* and asset_import
  with scene3d/model_service workers; optional portable_* remains opt-in.
- Verify portable code with tests/test_portable_assets.py. Optional Khronos fixture
  QA uses tests/validate_portable_fixtures.cjs and the pinned development dependency.

- UI theme/inventory: Docs/InterfaceAudit.md, native/EditorTheme.h and interface_features.json.
  Use CyUI setPalette for app themes; preserve explicit language/theme preferences.
  test_interface.py and capture_interface.py use isolated native GUI hosts.

- Neutral documents: read Docs/Native3DAssets.md. Preserve UUIDs/source payload and
  unknown metadata. Do not expand the bounded glTF profile silently. model_* and
  json_* edits share document Undo; saves guard against external disk changes.
  test_scene3d.py, test_interface.py and capture_3d.py verify the native host.

- 0.21: read Docs/AdvancedAuthoring.md / GameCodecs.md. Blueprint.cpp/Macros.cpp
  extend the existing interpreter. Preserve per-advance shared budgets, GUID object
  handles, macro expansion scopes and source trace mappings. Classes/instances are
  local simulation data, not reflection of arbitrary engine memory.
- ModelAuthoring and model_process_service use official Core workers and hash-guarded
  commits. Geometry edits invalidate LODs / mark collision review; providers must
  honor these markers. Bulk authoring metadata uses glTF extras and a checked
  CyPack manifest reference; unknown namespaces and UUIDs remain preserved.
- game_codecs.default_codecs is empty. Only trusted source registration may enable
  a real codec; never accept a worker command from user asset/project JSON. The
  test fixture worker is synthetic and must not appear as a real game's provider.

- 0.22: Docs/AnimationTools.md and animation_tools.py implement bounded offline IK,
  root-motion pairs/reconstruction and trim/resample, using existing model_process.
  Keep the source clip/bind data unchanged, checked metadata clip indices and
  float32 pose fingerprints. A future clip reorder/delete must migrate pair indices.
- The bridge terminal state must follow Core runtime closure; completion cannot
  advertise files as ready for cleanup while its SQLite/worker handles remain open.
- --test-motion injects only isolated-host ImGui input; never global desktop input.

- 0.23: Docs/NamedGraphTypes.md / TypeLibrary.cpp describe graph-local nominal enums
  and records. Pass the type context into schemas, classes and functions. Runtime
  children inherit it; serialized function bodies must not duplicate root libraries.
- Clipboard merges types/functions/classes at the owner, with conflict rejection.
  Keep in-use type changes atomic and preserve unknown definition/field metadata.
- TypesView.cpp and record widgets use real properties; --test-types only injects
  isolated-host ImGui input. English remains source/default/fallback.

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
