# Editor architecture — 0.28.0

## Layers and ownership

1. EditorCore owns the live document (entities, graphs, models and JSON assets), selection, command
   registry and bounded undo/redo history. It has no ImGui dependency. Native
   operations run on one owner thread; failed edits restore the previous document.
2. EditorApp owns the panel registry, menus/search, cameras, graph canvases,
   property widgets, asset editor registry and persisted presentation preferences.
   Panel IDs stay stable through localization and docking.
3. IGameAdapter supplies a descriptor, entity factory, property schemas,
   initial scene, validation and optional build targets. Inject an implementation in EditorCore's constructor.
   Sandbox is the only implementation shipped.
4. CyToolsCore owns manifest validation, authenticated SDK sessions, private jobs,
   progress/cancellation infrastructure and MCP/CLI/JSONL/HTTP. tool.py registers
   declared operations and forwards each short operation to the native owner thread.
   No competing job scheduler or MCP implementation is introduced.
5. EditorIpc is a bounded, length-prefixed local named pipe. It rejects remote
   clients and uses a Windows ACL for the editor's current account. Requests have a
   1 MiB limit; responses 4 MiB. This is an internal transport, not a second AI SDK.

The native GUI uses the same validated command dispatcher as the IPC bridge.
Immediate document edits are local transactions. The CyToolsCore job envelope is
the AI-facing invocation/lifetime contract. Long builds/imports should execute as
Core ProcessWorker jobs, then submit a short commit operation to the owner thread.

## Source plugins

IEditorPlugin::activate/deactivate receives the native host and can register panels
and commands. Diagnostics is an executable example. No external DLL loading, hot
reload, sandboxing of plugin code or stable binary ABI is claimed. Source plugins
are trusted application code.

Declare operations in build_manifest.py and regenerate CyTool.json, then use
EditorCore::bindCommand for native handlers. CyToolsCore discovers those same
descriptors. Do not silently register an AI operation only in a menu.

EditorApp::registerAssetEditor takes an ID, recognized extensions and an open
callback. The built-in .cyasset handler opens independent graph documents/tabs;
unsupported formats open the metadata inspector. A game plugin can register its
own specialized UI. Asset documents never provide executable callbacks.

EditorApp::viewportRenderer receives ViewportContext (origin, size, camera and host).
The default renderer projects Sandbox boxes with CPU math into an ImGui draw list;
DX11 renders the interface. A real engine adapter supplies its texture/render
pipeline and picking here. The Sandbox host integrates pinned ImGuizmo sources
for local/world transforms and snapping; the view/projection matrices match the CPU renderer. Another platform host can replace Win32/DX11.

## Properties

IGameAdapter::propertySchema describes fields with JSON Schema types, titles,
enum/range metadata and x-editor-group / x-editor-widget. Details renders strings,
booleans, numbers, integers, enums, vectors and colors automatically.
Other values can use a JSON field. Dynamic groups are supported.

This is descriptor-driven editing, not automatic reflection of arbitrary native
memory or an unknown game's variables. Importing those descriptors belongs to its
adapter. Native validation implements the subset used by the editor descriptors;
CyToolsCore validates full Draft 2020-12 at its boundary. Add native validation
support when introducing new schema constructs.

## Nodal foundation

A graph serializes its domain, stable node IDs, type IDs, positions, explicit values
and directed typed links. One document owns scene objects and graph assets;
edits use the same undo snapshots as object changes. Each opened graph asset has
a separate graph ID and per-view selection, pan/zoom and preview state.

The common canvas provides searchable creation, selection, node dragging, zoom,
pan, typed pins, connection/removal, value editing and structural validation.
Click a pin to start a wire, then an opposite-direction pin to connect. Select a
node to remove one of its connections in its properties.

Validation rejects unknown node/pin IDs, incompatible domains/types, duplicate IDs,
occupied inputs, invalid values and cycles. Limits: 500 nodes / 2,000 links per
graph. The Sandbox runtime has a bounded evaluation budget.

Logic nodes include begin/print/branch/sequence, arithmetic, comparisons, typed
variable get/set and float-to-string. Material nodes include typed parameters,
arithmetic, color mixing/multiplication and material output. GraphRuntime.cpp owns
a bounded interpreter with independent runtime variables and debug sessions.
Breakpoints, stepping, continue, trace and observed values use the same interpreter
as graph_run. Program changes invalidate a debug snapshot; presentation changes
and breakpoint edits do not. See Milestones.md for limits and controls.

registerNodeType permits typed pure source-plugin nodes with a descriptor and a
callback receiving resolved input values and returning named outputs. Types:
exec, bool, float, string and color; pure callbacks use the four value types.
The registry is process-local and should be populated at startup.

MaterialPreview.cpp provides an analytical sphere/plane, lighting and a fallback
shader driven by a CPU center sample. A supported graph can replace the pixel shader
with DXBC compiled in an official CyToolsCore ProcessWorker (see MaterialPipeline.md).
Per-document render targets preserve independent previews; the renderer restores
the graphics pipeline before the interface renders. This is an editor preview backend.

Functions/subgraphs have typed signatures, nested calls and local libraries.
Missing mode preserves pure behavior; execution mode adds explicit control flow.
Functions.cpp derives call/boundary descriptors from the serialized signature;
GraphRuntime.cpp shares an evaluation budget and a stable library across nested calls.
A root validates every definition and rejects recursive call graphs. Function bodies
use the same graph commands with an optional functionName context.

Blueprint.cpp/Macros.cpp now implement typed containers, transient object references,
local classes/methods and scoped inline macro expansion. GraphRuntime adds typed
foreach, logical delays and custom events. Profiling, visual graph merging,
versioned type migrations and engine/compiler bindings remain work. Materials still need
world-space inputs, engine shader lowering, stage restrictions
and engine-specific compilation/caching. There is no Blueprint compatibility.

## Asset documents and compilation

data/assets is the current content root. The browser uses relative file IDs,
folders, field-based search, favorites, tiles/list and extension-based filters.
Copy/move/Trash/restore operate on real files without overwriting. These persistent
file operations stay separate from document Undo; references are updated in retained
snapshots so Undo cannot resurrect an obsolete asset path. Dirty graphs are protected. .cyasset stores versioned
graph JSON. New graph assets refuse existing names; opening graph files creates
independent tabs. Saving is explicit. Project snapshots store the open document
state, and asset files have their own Save asset action.

Native game formats, import/reimport, binary-format dependency providers, thumbnails,
game archives, source/generated output separation and filesystem watchers remain
provider work. Listing a file does not implement its decoder.

Sandbox announces game build/play as false and document package as true.
IGameAdapter::buildTargets declares provider IDs, target/platform/configurations
and an options schema. Build.cpp validates the live document and returns a bounded,
revision-stamped snapshot. build_service.py registers build_execute as an official
CyToolsCore job and dispatches to a trusted source provider using ProcessWorker.
The delivered sandbox.archive provider writes a document ZIP; it does not compile a game.
All snapshot, diagnostic and archive outputs use JobContext.path. See BuildProviders.md.

## Persistence and multi-client behavior

Project JSON, graph assets and recipes are versioned. Temporary-file replacement
protects explicit native saves. Project loads validate the incoming document before
replacing state; Undo restores the previous document. Startup shows the demonstration
scene; Ctrl+O reloads the saved project.

The document is shared live editor state. Job visibility remains per identity,
but callers do not receive private scene copies. The owner thread serializes
mutations. editor_status and scene_list expose a monotonic revision. Mutations may supply
expectedRevision; a mismatch rejects before editing. The gizmo submits one batch
when the drag ends and cancels if the live revision changes. Inspect current state
before retrying after disconnect, since the first call may already have committed.

Each standalone stdio bridge owns its runtime/data directory. Separate bridges
do not share a scheduler automatically. Core's authenticated HTTP service and MCP
bridge remain the existing solution for shared job scheduling.

## Workspace and scene modules

Workspace.cpp owns Level/Scripting/Materials presets, named layout documents and
Content Drawer. Presentation preferences and layouts are separate from game assets.
Capture/preview mode does not persist preference changes. Scene.cpp owns selection,
atomic transformations, locks, duplication and virtual outliner folders. Folders do
not imply transform parenting. UI and MCP share these command handlers.

## Authoring and declared asset dependencies (0.7)

GraphBatch.cpp implements atomic move/delete, portable fragments and static comment
frames. Clipboard definitions merge only when identical. Group drags commit one
revision-checked command on mouse release. GraphSmokeTest.h drives the real ImGui
input path in an isolated test root; it does not synthesize operating-system clicks.
Function boundary nodes are protected against copying or removal.

AssetReferences.cpp indexes explicit references in saved .cyasset files and live
documents. It conservatively combines both states until consumers are saved. The
index is bounded and refreshed on request; incomplete scans refuse file move/Trash.
Windows case-insensitive paths and canonical locations are used for reference keys.
Cycles are allowed for asset metadata and traversal uses a visited set. This is not
an external function linker, decoder for native game formats or an automatic
multi-file retargeting transaction. ReferenceView.cpp exposes direct relationships,
missing targets and editable declarations through the same native command contract.

## Graph navigation and material instances (0.10)

GraphNavigation.cpp implements graph_find across loaded roots and their local
functions, typed wire insertion, per-view bookmarks and comment-frame editing.
Search returns at most 2,000 entries with a truncated flag. GraphNavigatorView.cpp
refreshes its search when the document revision or active graph changes. Bookmarks
store logical pan coordinates and zoom, remain inside graph assets, and do not
invalidate a paused program. Reroutes preserve pin types and interpreter behavior.
Comment frames have editable text, position and dimensions; they do not parent nodes.

Materials.cpp resolves material-instance assets against live parents when loaded,
otherwise against saved .cyasset files. Relative parent paths remain confined to
the content root. Chains have at most eight parent links and reject cycles.
Overrides apply to a copy of the effective material variable declarations, then
pass the existing type validation/evaluator. Parent topology/defaults are never
modified by an override. Removing a stale override remains possible when a parent
parameter was renamed. MaterialInstanceView.cpp caches by document revision;
external disk changes require Refresh material. Parent links also participate in
the saved/live reference index and referenced-file protections.

## Desktop build client (0.10)

BuildClient starts a fixed local Python SDK client without a shell or console.
It creates only transport metadata under data/build-runs/<runId>: request.json,
state.json and cancel.request. build_bridge.py submits and observes one official
Core job. Core remains responsible for scheduling, state transitions, worker process
ownership and cancellation. The GUI client does not implement a compiler scheduler.

Desktop sessions use temporary=False; generated files remain in the owning Core
job workspace after the GUI closes. Closing an editor requests cancellation for
its active job. A process handle is closed without pretending that the worker has
already stopped. State files retain the job result, but GUI history/recovery after
restart is not implemented. The desktop job status is shared editor-session state;
independent SDK jobs retain Core's per-identity access controls.

## Execution functions and debugger frames (0.13)

A local function can declare mode="execution" in a logic asset. Its input boundary
adds a then output, its return boundary an exec input; calls have exec/then pins
in addition to their typed value signature. These reserved names cannot be used
for data ports. Material assets and pure functions reject execution calls. Function
mode is selected on creation; no in-place pure/execution conversion is provided.

Each GraphExecution frame owns arguments, initialized local variables, watched
values, its pending execution queue and cached results of completed execution calls.
A parent suspends its queue while a child frame runs. Returning evaluates and caches
all outputs, discards remaining work in that invocation and resumes the caller.
Reading an execution output before its call executes fails; it never lazily invokes
side effects. Ending an execution invocation without its return also fails.
Functions access root data through arguments/return values, not implicit global scope.

Frames share the existing 10,000-value-evaluation and 4,096-execution-step budgets,
message/trace buffers and immutable local library. The validated call graph forbids
recursion and nesting beyond 16 functions. Pure calls still evaluate lazily and do
not create paused debugger frames. graph_function_evaluate can run an execution
function in isolation, returning values/messages/trace/locals without modifying the
owning graph. Diagnostics inspect every local definition and flag an unconnected
execution return; they are not a full proof that every possible branch returns.

F10 steps over a call, F11 steps into an execution call and Shift+F11 steps out.
Step-over/out honor breakpoints encountered in callees; continue skips only the
breakpoint just stopped at. At the root, step-out runs to completion or a breakpoint.
status includes activeFunction, context-qualified trace entries, callStack,
rootVariables and active-frame variables. Each frame exposes arguments, watches and its suspended call site.
The GUI can inspect an older frame or follow the active one, navigates to the correct
subgraph after a stepping action and highlights only that frame's current node.
Breakpoint edits refresh both active frames and future calls. Semantic edits mark
the session stale; layout/comments/bookmarks do not. Runtime errors preserve the
remaining stack for inspection. There are no latent calls, recursion or engine callbacks.

FunctionSmokeTest.h feeds F10/F11/Shift+F11 through the real ImGui keyboard path in
an isolated host and verifies navigation, local values and a three-frame nested stack.

## Saved-asset comparison (0.13)

GraphDiff.cpp matches stable IDs and reports reordered ID sequences separately
when membership is unchanged, preserving review of event/fan-out execution ordering. GraphDiffView.cpp presents
added/removed/changed nodes, links, declarations, function bodies, comment groups,
bookmarks and remaining properties. It includes material-instance parent/overrides.
The optional presentation filter strips layout, node comments/breakpoints, bookmarks
and comment frames. It does not alter runtime graph signatures or saved files.

Assets.cpp resolves an actual saved .cyasset inside the content root and validates
it before comparing; a built-in graph without a file uses its savedGraphs_ session
baseline. It reports whether the disk contents diverged from the last load/save.
Refreshing is explicit for external disk changes. No file watcher is introduced.
The comparison does not apply/revert edits, change the revision, clear Undo or merge
files. The existing Save asset action remains explicit and refreshes the baseline.

Details are capped at 1,024 entries and 2 MiB, with a complete summary and truncated
flag. Value previews cap at 8,000 UTF-8 bytes without splitting a code point; full
before/after JSON is included only below 16 KiB per value. Larger structures remain
reviewable through their preview and native graph APIs. This is a structured saved/live
comparison, not Unreal's full visual graph diff or a version-control integration.


## Preview material jobs and image asset editor (0.16)

MaterialProgram.cpp resolves the live parent chain without requiring the center CPU
sample to succeed. The plan contains an effective graph, ordered float4 uniforms,
parameter names, a SHA-256 program signature and a fallback sample/error. Root
parameter defaults and presentation metadata are excluded from the signature;
semantic constants, bindings, signatures and function defaults remain included.
material_sample carries caller UV coordinates through nested pure function frames.

shader_service.py registers material_compile in the existing runtime. The native
owner thread produces a bounded snapshot; shader_worker.py generates trusted HLSL
expressions and invokes D3DCompiler inside a Core ProcessWorker. JobContext.path
owns all intermediate/output files. No asset-supplied source, include or shell command
is accepted. shader_codegen.py emits contextual node mappings and rejects unsupported
reachable expressions. The error report is available through error.technicalDetails.

The desktop shares BuildClient/build_bridge.py for build and preview jobs. It adds
no scheduler: Core owns execution, progress, identity checks and worker cancellation.
The GUI caps desktop shader requests at two active clients; exclusiveGroups applies
within each official runtime. An SDK compile does not automatically alter the GUI.
material_preview_apply validates the current signature, parameter ordering, owned
paths and source/bytecode hashes, then creates the DX11 shader on the owner thread.
The source and GPU object remain usable after the caller closes its task files.
Semantic changes temporarily select the fallback; parameter changes reuse the shader
with new uniforms. Refresh is explicit for external parent-file changes.

Image.cpp is a core WIC metadata/decoding service independent of ImGui. TextureView.cpp
is registered by extension; TexturePreview.cpp uploads bounded RGBA8 previews and
channel variants. The viewer decodes PNG/JPEG/BMP, limits input to 32 MiB, dimensions
to 8192 per side and the decoded surface to 4,194,304 pixels. An eight-entry LRU cache
retains one CPU image and one GPU channel view per entry. Decode/upload is synchronous
and bounded; there is no watcher or game texture codec. AssetImport.md and
MaterialPipeline.md describe the 0.20 importer and material texture sampling.
Platform adapters can replace the image renderer callback.

Preview caches and their COM references are scoped to end before releasing the DX11
device and window. Shutdown clears and flushes bindings after the ImGui backend closes,
then destroys the image/material resources and IPC before releasing global graphics.

## Independent portable asset contract (component 0.1)

portable_assets is a source library/CLI for a versioned .cypack envelope and
byte-preserved self-contained GLB. It is independent of the native document and UI.
The conversion registry checks declared feature/extension support and explicitly
reports blocked/loss-review/provider-required states. No game codec is inferred.
Optional service handlers reuse the official Core ProcessWorker, private job paths
and task/export lifecycle; shared source inputs are disjoint from runtime storage.
Native 0.20 now uses that contract through scene3d/model_service and the existing
Core import/export workers. .cy3d edit documents, MeshPreview, ModelView and JSON
asset panels extend the shared native dispatcher. Game converters remain provider
work. See CommonAssetFormat.md and Native3DAssets.md for validation scope, limits
and tests.

## Native model/JSON authoring (0.20)

Scene3D validates the bounded document and evaluates TRS hierarchy, quaternion
animation and linear blend skinning independently of ImGui. ModelCommands and
JsonDocuments mutate document collections with normal revision/rollback/Undo.
Disk baselines live outside Undo so saving, undoing and saving again does not
misidentify the editor's own write as an external modification. Hash guards reject
external changes; source-import commit additionally protects live model edits.

ModelView/JsonView own presentation state, register per-asset tabs and expose the
same operations to native GUI and SDK. MeshPreview owns bounded DX11 targets and
buffers; pose caching keys include revision/clip/time. Resources end before the
DX11 device, and targets for released documents are pruned after draw submission.
Official model_service/model_worker snapshot and export to job-owned outputs;
there is no native UI-thread game converter or replacement Core scheduler.

## Advanced authoring (0.21)

ModelAuthoring.cpp validates bounded editor metadata and hash-guards worker commits.
ModelAuthoringView.cpp supplies LOD/collision/retarget controls. model_authoring.py
contains neutral algorithms; model_process_service/worker use official Core jobs
and immutable inputs. Source geometry edits mark generated LODs stale and collisions
for review. scene3d.py carries bulk authoring metadata in glTF extras with a hashed
CyPack manifest reference. Read AdvancedAuthoring.md before extending this contract.

BlueprintView.cpp renders typed class declarations/runtime fields. Macros.cpp
expands source macros before GraphExecution with scoped locals/source mappings.
The simulation clock is separate from Core scheduling and uses a single execution
lane. New GUI smoke input is confined to isolated --test-blueprint roots.

GameCodecView.cpp queries the actual trusted game_codecs.py registry through the
desktop Core bridge. IGameAdapter::assetCodecs only declares capabilities; register
matching executable source providers. No real game provider is bundled.

## Native 0.22 animation authoring

[AnimationTools](AnimationTools.md) documents offline fixed-root chain IK, extracted
root-motion pairs with sampled reconstruction and trimmed/resampled clips. The
existing model_process/Core worker/commit path is reused, with real native preview,
Undo and GLB/CyPack export. MotionToolsView.cpp supplies translated property views.
The desktop bridge now publishes terminal readiness after Core closes its handles.
No joint limits/contact solver or engine movement/physics binding is supplied.
Evidence: data/reports/motion-validation.json and motion-captures.json.

## Native 0.23 graph types

[NamedGraphTypes](NamedGraphTypes.md) documents local nominal enums/structures,
validated values/pins, literal/make/break/set/equality/switch nodes, recursive
properties and named variables/signatures/class fields. TypeLibrary.cpp resolves
schemas and node definitions against the owner library; runtime children/macros
inherit the same context. TypesView.cpp supplies native designers. Clipboard
transfers root libraries into functions with conflict checks; semantic changes
invalidate debug sessions. These are local declarations, without standalone shared
type assets, automatic shape migrations, interfaces or complete Unreal parity.
Catalogue: 173 editor operations / 194 tools including Core. Verification:
data/reports/types-validation.json, types-tests.log and types-captures.json.

## 0.24 local class interfaces

Interfaces.cpp validates local contracts and resolves contextual node descriptors.
Blueprint.cpp resolves inherited implementations. GraphExecution shares the interface
library and object pool across pure/execution children; callTarget selects the actual
bound function while the debugger retains its concrete function name. Functions.cpp
adds every potential implementation edge to the bounded recursion check. InterfacesView.cpp
reuses typed property widgets. CyToolsCore remains the sole MCP/SDK/job runtime.
See [GraphInterfaces](GraphInterfaces.md) for persistence, limits and tested workflows.

## 0.25 explicit signature migration

SignatureMigration.cpp builds a temporary owner copy and signature-equivalence
closure through class overrides and interface bindings. It remaps boundaries,
call nodes, fallback values and links, then validates the complete owner. A hash
binds the exact request and source. Only a checked apply commits; one normal Undo
restores the owner. SignatureMigrationView.cpp shares the same dispatcher and
native property themes; no new scheduler or engine compatibility layer is added.
See [SignatureMigration](SignatureMigration.md) for loss/metadata semantics.

## Native 0.26 type migration

TypeMigration.cpp builds an isolated candidate from one local nominal enum/record.
A neutral internal shape validates recursion/expansion before authored parent
defaults are converted; it never replaces those authored defaults. Declaration
normalization precedes dynamic node descriptor resolution, then typed node values,
references and wires migrate before complete owner validation. A stale/changed hash,
invalid graph or unaccepted loss cannot commit. The same core dispatcher is used
by the EN/FR type designer, shared migration review and official Core operations.
Read TypeMigration.md for semantics, bounds and verification.

## Native 0.27 graph region extraction

GraphCollapse.cpp derives a pure helper and replacement call in a temporary owner.
Selected nodes and internal links move unchanged; crossing links retain IDs and
metadata in the caller. Per-output dependencies protect against newly eager inputs.
Generated IDs bind deterministically to the source/request; complete owner
validation precedes one atomic commit. GraphCollapseView.cpp supplies the translated
selection modal, typed boundary table and stale-preview guard. No scheduler or
engine compiler is added. Read GraphCollapse.md for contracts and limits.

## Native 0.28 reversible call expansion

GraphExpand.cpp builds one temporary owner from a selected pure call. It clones
interior nodes and comment groups, adds typed boundary reroutes and retains original
outer wire IDs/fallbacks. The complete removed call is retained in comment provenance.
Per-output dependencies guard eager arguments; runtime-expanded nested macros are
used only for analysis. Authored and runtime-expanded candidate validation precede
one atomic commit. GraphExpandView.cpp exposes the same plan/apply through translated
CyUI controls. Catalogue: 175 editor operations / 196 tools including Core.
Read GraphExpand.md for defaults, metadata, scope and runtime budget limitations.
