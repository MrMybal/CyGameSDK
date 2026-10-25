# Capability coverage — CyGameSDK 0.28.0

| Capability | Delivered behavior | Validation / limits |
|---|---|---|
| Native desktop | C++17, Win32, DX11, Dear ImGui docking, CyUI | MSVC build and real graphical launches |
| Workspace | Level/Scripting/Materials, named layouts, dockable tabs, detached windows, Ctrl+Space drawer | Graphical save/load/switch test; capture mode preserves preferences |
| Multiple viewports | Independent cameras and up to 16 viewport instances | Extra viewport created on graphical host through SDK |
| Scene editing | Multi-selection, duplicate/delete, virtual folders, visibility and locks | Atomicity, locking, undo/redo, selection restoration checked |
| Transforms | ImGuizmo move/rotate/scale, local/world, snapping; group translation | Actual ImGui drag changes position, creates one Undo entry and restores initial position; group rotation/scale remain future work |
| Revision control | Monotonic live revision, optional expectedRevision on mutations | Stale batch rejection and no partial edits checked |
| Generic property UI | Schema-based strings, numbers, booleans, enums, vector2/vector3/vector4, color and JSON fallback | Sandbox types rendered; automatic reflection of unknown games belongs to adapters |
| Game adapters | IGameAdapter injection and capability/schema boundaries | Sandbox only; document archive available, game build/play unavailable |
| Viewport rendering | Projected boxes, grid, solid/wireframe, orbit, pan, zoom, picking and focus | EN/FR and theme captures; engine meshes/renderers remain provider work |
| Graph canvas | Typed wires, multi-selection/marquee, group drag, frame all, clipboard, alignment and comment groups | Actual ImGui marquee/group drag verifies one Undo entry and restoration; clipboard data/conflicts tested |
| Graph correctness | Direction/type/domain/ID/value checks, occupied-input and cycle rejection | Native and bridge tests verify rejection and rollback |
| Variables | float/bool/string/color, default/get/set/rename; material parameters | Invalid defaults/bindings rejected, rename updates references; examples checked |
| Logic execution | Events, print, branch/sequence, math/comparisons/conversion, variable access | Bounded interpreter; runtime variables do not overwrite declaration defaults |
| Debugger | Breakpoints, step, continue, trace, variables, observed values, program invalidation | Native bridge scenarios and official MCP jobs on graphical host |
| Diagnostics | Missing event/output, disconnected nodes, unused variables; navigate to node | Invalid runtime division yields a visible failure; structural drafts may save |
| Material preview | UV/procedural expressions, generated HLSL/DXBC, sphere/plane and live uniforms | Official worker compilation, native shader installation, stale/checksum rejection and EN/FR captures; supported editor expression subset, no engine shader export |
| Procedural material nodes | UV U/V, UV transform, checker/gradient, compose color, sine/fract/saturate | CPU samples, negative coordinates, pure function UV context and real shader compilation checked |
| Shader job ownership | Core ProcessWorker, private source/DXBC/reports, progress and cancellation | Worker termination, per-identity isolation, hashes and non-mutating GUI apply checked |
| Custom nodes | Pure source-plugin node registry/evaluator | Native test registers and executes custom.double |
| Asset browser | Real filesystem, folders, field queries, favorites, tiles/list | Search/filter/file lifecycle tested; bounded decoded image thumbnails |
| References | Declared .cyasset references, saved/live union, direct/transitive consumers and dependencies, missing targets | Cycles, confinement, incomplete scans and prevention of referenced-file moves/Trash checked; bounded on-demand scan, no game-format decoder |
| Asset operations | Copy/move/rename, Trash/restore, no overwrite, dirty protection | Disk roundtrip, traversal/collision rejection and references across document Undo checked |
| Asset editors | Extension registry, independent graph tabs and WIC image editor; metadata fallback | PNG/JPEG/BMP metadata, bounded decode, RGBA/RGB/single channels, pan/zoom/reload; no game texture decoder; importers documented in AssetImport.md |
| Portable 3D source component | `.cypack` envelope + self-contained GLB, byte-preserving transfer, declarative conversion plans and optional Core worker operations | 22 component tests and two Khronos-validated original fixtures; bounded native .cy3d editing and GLB/CyPack import/export now active; no real game codecs; reference-pose retargeting added in 0.21 |
| Persistence | Versioned projects/assets, validated loads, temporary-file replacement | Roundtrip and invalid load tested; startup shows Sandbox, Ctrl+O loads saved project |
| Editor plugins | Source IEditorPlugin lifecycle and Diagnostics example | Graphical activation/deactivation/panel checks; no ABI-stable DLL loader |
| Search | Commands, panels, objects, properties, nodes and asset fields | Localized GUI labels plus stable IDs; native API retains protocol IDs |
| MCP / AI SDK | CyToolsCore 0.10.1 / official MCP 1.30.0; shared native dispatcher | Real stdio initialize/list/call/poll/debug/shader compile/finish; 173 editor operations, 194 tools including Core controls |
| Jobs / privacy | CyToolsCore runtime owns jobs, sessions and private results | SDK identity isolation and parameter validation checked |
| Configuration | Versioned recipes, load without execution, optional parameter autosave | Roundtrip and SDK replay checked |
| Localization/themes | English/French, Unreal-inspired Dark plus five CyUI themes, stable window IDs | Both languages and all six themes rendered; screenshots under data/reports |
| Local functions | Pure and execution modes, typed signatures, nested calls, isolated locals, cached returns and subgraph tabs | Nested control/data flow, isolation, domain rejection, missing returns, budget failures and official MCP evaluation checked |
| Graph navigation | Search roots/functions, node comments/values/variables, per-view bookmarks, typed reroutes | Root/function rerouting preserves results; bounds, persistence, debug invalidation and undo checked |
| Material instances | Parent chain, typed overrides, live inheritance, lit preview | Inheritance, parent isolation, cycles, type/path rejection and stale override recovery tested; eight parent links, manual disk refresh |
| Build providers | Optional adapter targets/configuration/options; immutable snapshot and official Core ProcessWorker jobs | Sandbox exports document ZIP, checksum and diagnostics; cancellation stops worker; no game compiler |
| Call-stack debugger | F10 over, F11 into, Shift+F11 out, frame arguments/locals/watches and contextual navigation | Nested breakpoints, live breakpoint refresh, semantic invalidation and actual ImGui keyboard path checked; pure calls remain atomic |
| Graph comparison | Saved/live structured changes, external disk-change detection and presentation filter | Read-only revision/Undo checks, function context, file corruption and Unicode previews tested; bounded details, no visual merge or arbitrary revisions |
| Blueprint depth | Typed graphs, functions, containers, classes/methods, inline macros, foreach, logical delay/events and runtime object debugger | Bounded interpreter; no complete Unreal parity, engine objects, replication or game compilation |
| Platforms | Windows x64 graphical host and local MCP | Linux/macOS graphical hosts and redistributable installer remain future work |

Current presentation audit and limits: [InterfaceAudit](InterfaceAudit.md).

## Native 0.20 additions

See [CompletionMilestones](CompletionMilestones.md) and [Native3DAssets](Native3DAssets.md).

| Capability | Delivered behavior | Limits |
|---|---|---|
| Import/reimport | Real image copy and GLB/CyPack conversion, sidecars, hashes and thumbnails | No batch/watcher/game codecs |
| Static mesh | DX11 geometry/depth/lighting/wireframe and editable vertex/normal/UV/weights | Small asset profile; static LODs and primitive collision shapes added in 0.21; no topology editor |
| Skeleton/animation | Hierarchy/TRS, skinning, playback and authored clips/tracks/keys | Reference-pose retargeting added in 0.21; 0.22 adds bounded chain IK; no contact solve or curve tangent handles |
| JSON assets | Native tree, scalars/arrays/objects, add/remove, Undo and guarded saves | 512 KiB, four documents; long strings read-only in GUI |
| Material textures | CPU and compiled DX11 sRGB bilinear repeat image sampling | Eight bindings/64 MiB, literal paths, editor preview |

Current evidence: completion-build.log, completion-native.log,
completion-integration.log, completion-scene3d-gui-tests.log,
completion-interface-tests.log, completion-portable.log, completion-khronos.json,
completion-captures.json and completion-validation.json under data/reports.
Historical reports below retain their original scope.

## Evidence

- data/reports/m15-build.log — final MSVC build.
- data/reports/m15-tests.log — integration suite against CyGameEditor.exe.
- data/reports/integration.json — official MCP roundtrip and discovered tool count.
- data/reports/function-gesture.json — real F10/F11/Shift+F11, local values and nested frame navigation.
- data/reports/graph-gesture.json — actual marquee, group drag and Undo.
- data/reports/gizmo-gesture.json — actual ImGui mouse input, one transaction, Undo.
- data/reports/{level,scripting,materials}-{en,fr}.bmp — workspace captures.
- data/reports/{viewport,logic,material,options}.bmp and theme-*.bmp — standard QA set.
- data/reports/{navigation,navigator,instance,build}-{en,fr}.bmp and instance-theme-*.bmp — new panel captures.
- data/reports/execution-{stack,body,diff}-{en,fr}.bmp and execution-theme-*.bmp — new authoring/debugger captures.
- data/reports/m15-validation.json — version, hashes, operation count and checks.
- data/reports/m15-*.bmp — procedural sphere/plane, instances, compiler success/error and image channels in EN/FR and all five themes.
- data/reports/m15-capture-results.json — actual compiled preview states before capture.
- app/tests/core_tests.cpp — native contracts, rollback, custom node and cycle checks.
- app/tests/test_integration.py — thirty-three scenarios, including eleven graph examples and an image.

~~~powershell
.\app\build.ps1
.\runtime\build\CyGameSDKTests.exe (Get-Location).Path
$env:CYGAME_TEST_GUI='1'
.\runtime\python\Scripts\python.exe app\tests\test_integration.py
.\runtime\python\Scripts\python.exe app\capture.py
~~~

Without CYGAME_TEST_GUI=1, the same EditorCore/EditorIpc run in a native headless
host. The six GUI-only scenarios are skipped. Tests use isolated workspaces under
data/reports and remove them after their owned processes stop.

Milestones.md gives the controls, current limits and next extension layers.

## Native 0.21 additions

Read [AdvancedAuthoring](AdvancedAuthoring.md) and [GameCodecs](GameCodecs.md).

| Capability | Delivered behavior | Limits |
|---|---|---|
| Typed runtime | int/vector3, arrays/maps/sets, local class inheritance and virtual methods | Transient object handles, bounded one-level containers, no engine reflection |
| Macros/loops | Compile-time scoped expansion and typed execution callback foreach | No cyclic wires; scoped budgets and source debugger mappings |
| Latent/events | Nonblocking logical delay, explicit tick, named event queue | One execution lane; no real-time engine timer binding |
| LODs | Real reduced buffers, preview/export, stale-derived-data checks | Static vertex clustering; four levels; no automatic screen-distance switching |
| Collisions | Conservative box/sphere/capsule fit, editable properties and wire preview | Metadata shapes; no engine physics cooking or simulation |
| Retargeting | Bone mapping, rest-world rotation correction, baked clip and skinning | TRS rigs; 0.22 adds separate chain IK/root-motion tools, without contact solving |
| Game codecs | Trusted registry, exact target declarations, Core worker export, hash/loss checks and GUI | Empty default registry; synthetic contract fixture is not a game codec |

Evidence: data/reports/authoring-{tests,integration,scene3d,interface,portable}.log,
authoring-native.log, authoring-captures.json, authoring-khronos.json and
authoring-validation.json. Earlier completion/m15 reports retain historical scope.

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

| Class interfaces (0.24) | Local pure/execution contracts, typed modal, matching class bindings, support query and dynamic dispatch | Atomic validation, inheritance, nominal signatures, clipboard/function context, recursion guards and real EN/FR gestures; no engine ABI or Unreal compatibility |

| Explicit signature migration (0.25) | Preview linked declarations and losses, rename ports, migrate compatible values/connections, hash-guarded apply and whole-owner Undo | Functions/macros, bound methods/interfaces, compatible foreach extra arguments; no mode/type-shape/automatic asset-load migration |

## Native 0.26 type migration

| Capability | Delivered behavior | Validation / limits |
|---|---|---|
| Enum/structure migration (0.26) | Explicit local shape preview, field/member maps, nested defaults/values, node pins and hash-guarded apply with Undo | Loss acknowledgement, stale-plan rejection and bounded reports; removed bound set-fields block; no cross-asset or live-object migration |

Verification: data/reports/type-migrations-validation.json. The active native
catalogue contains 173 editor operations / 194 tools including Core controls.

## Native 0.27 graph extraction

| Capability | Delivered behavior | Validation / limits |
|---|---|---|
| Collapse selection | Pure regions become local typed functions/macros, reviewed ports, preserved node/link IDs and metadata, one Undo | Per-output eager-input guards, logic/material and function contexts, real shader compilation and EN/FR gestures; execution/disconnected regions reject |

Evidence: data/reports/collapse-validation.json. Current catalogue: 173 editor
operations / 194 tools including Core controls.

## Native 0.28 call expansion

| Capability | Delivered behavior | Validation / limits |
|---|---|---|
| Expand call | Pure local functions/macros become editable nodes, retain definitions, typed reroutes, wire metadata and original-call provenance; one Undo | Per-output eager function arguments and nested lazy macros checked; execution calls reject; native EN/FR gestures and actual material shader compile |

Current catalogue: 175 editor operations / 196 tools including Core controls.
Read [GraphExpand](GraphExpand.md). Evidence: data/reports/expand-validation.json.
