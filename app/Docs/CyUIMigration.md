# CyUI modular migration — native 0.29

CyGameSDK uses CyUI 1.2.0 at the exact commit
`ff023e646c6a3038b3dfbfd53f6d2b4a2f5955e3`, from
https://github.com/MrMybal/CyUI. This is a presentation migration; the 175 editor
operations / 196 tools and game adapter boundaries stay in place.
`dependencies.lock.json` records the revision and selected modules.
If the upstream repository is private, Git must already be authenticated with an
authorized account. Alternatively, use `-CyUIRoot` with an authorized offline
checkout at the locked revision. Credentials belong in Git's credential manager,
never in the lock file, source tree or CMake arguments.

## Previous integration and build

The previous native build used a sibling CyImgui_Interface checkout, an umbrella
header/library and a separate manual MSVC source list. It compiled unused Audio,
Gallery and ImGui demo sources. Graph cards/wires/navigation, texture canvas,
command popup, property drafts, logs and diff detail panes had local renderers.
The app's theme, schema/graph semantics, folder operations and DX11 renderers are
application-specific adaptations, rather than shared presentation libraries.

`build.ps1` and `build_native.cmd` now both use CMake. The default configure fetches
only the pinned repository revision into the private `runtime/dependencies`
cache. `-CyUIRoot` accepts an existing clean checkout of that exact revision for
offline builds. Neither path modifies a sibling repository. CMake rejects a
mismatched revision or dirty dependency checkout. No machine-specific include or
library paths are embedded in the source.

`CYUI_MODULES` is populated from the lock before `add_subdirectory`. Targets use
explicit `CyUI::<Module>` links and matching headers. There is no production
`All`, umbrella link/header, Audio, Gallery, Demo, CodeEditor, Web or CEF. The host
provides `editor_imgui` through `CYUI_IMGUI_TARGET`; the app, modules, DX11/Win32
backends and `editor_imguizmo` share that one ImGui. The locked snapshot reports
ImGui 1.93.0 WIP (`IMGUI_VERSION_NUM=19297`), above the 1.92 minimum. The host stays
C++17. No Python UI, Unreal connector or embedded browser is used by this host.

## Selected modules and replacements

| Module | Migrated application view / reason |
|---|---|
| Core | Theme/palette, fonts/icons, layout, notifications, widgets and independent localized menu TextFilters |
| Nodes | Logic/material/function graph cards, pin/wire rendering, hit tests, drag connections, selection, movement and removal requests |
| Canvas (automatic) | Graph/image navigation, grid, scrolling and zoom |
| Inspector | Schema-driven boolean, int64, double and string drafts, validation events and edit phases |
| Assets | Common asset grid/list, selection, quick filter, folder navigation, standard context actions and numeric drag payload |
| Images (automatic) | Image surface, checkerboard, fit, pan/zoom and pixel inspection |
| Viewport | Common DX11 model texture surface; host retains its established camera, skeleton/collision overlays and renderer |
| ImGuizmo adapter | Scene transform gesture lifecycle, snapping, operation/mode and revision-aware cancellation |
| Timeline | Animation track/marker display, playhead, seek, range navigation and scrolling |
| Commands | Global Ctrl+P modal, filtering, keyboard ranking/navigation and command requests |
| Diagnostics | Bounded/filterable output log and graph/shader diagnostic lists with host node navigation |
| Code | Searchable, highlighted HLSL, JSON metadata and operation results; shared code rendering for Diff |
| Diff | Saved/current graph entry detail, split/unified code comparison, search and hunk navigation |

Code brings CyUI's pinned Lexilla engines, Scintilla **interface headers** and
SciTE keyword data. It does not bring a Scintilla editor UI. The upstream module
build selects these dependencies automatically. Notices and their syntax-source
lock are retained under `app/LICENCES`. No CodeEditor/TextEditor or CEF runtime is
linked. The optional standalone portable-assets component remains independent.

Removed local implementations include the hand-painted node/wire canvas and drag
state, image pan/zoom/checkerboard loop, asset tile/table renderer, scalar draft
widgets, global command popup, log text renderer, direct ImGuizmo manipulation and
plain diff/source default views. The manual MSVC compilation list and unused
Audio/Gallery/demo dependencies were removed. No Core/MCP scheduler was replaced.

## Ownership and transaction rules

- EditorCore owns entities, graph types/functions/interfaces, model/key data,
  parsers, validation, runtime/compilation, persistence and document Undo/Redo.
  CyUI models are temporary snapshots of those data, never a second document.
- Stable namespaced numeric IDs are derived from untranslated source identities.
  Explicit collision checks protect node/pin/asset maps. Localized captions and
  density changes do not alter source IDs or persisted graph positions.
- Nodes receives the live revision. Only a committed movement with an actual
  position change invokes the existing atomic `graph_nodes_move` with that
  gesture's expected revision. Begin/update/cancel only affect presentation.
  Toolbar/modal replacements refresh the model before rendering the new selection.
- Inspector maps scalar variants back through the existing schema validator and
  host command. Only a changed commit reaches document Undo. Escape, hidden views
  and changed revisions discard drafts. Replacing a local form's scalar type or
  source value cancels its active draft too, even without a Core document edit. On revision cancellation the host also
  clears the matching ImGui active field and drains its deactivation replay for
  two frames, so an old text buffer cannot overwrite a concurrent SDK edit.
- Gizmo transforms still use the existing selection snapshot and one atomic batch.
  Cancellation and stale document revisions do not commit partial transforms.
- Animation timeline navigation, image/channel/zoom state, asset presentation and
  theme/density choices are view state. Image fit-on-resize and the SDK's 5%..800%
  zoom contract are preserved around the common image navigation state. Key edits retain the existing model
  commands, validation and one document transaction per completed edit.
- Every per-document view keeps its own module state/cache. Diagnostics and command
  requests map back to source identities before navigation/execution. File moves
  remap the browser selection along with the host path and favorites.

English remains the source/default/fallback, independently of Windows locale.
The app registers its EN/FR catalogues with CyUI and reapplies the selected locale
for common controls. User entity names and property captions are registered under
namespaced literal keys when passed through APIs that translate labels. Authored
values, paths, IDs and enum members remain unchanged. Explicit preferences survive;
new density preferences accept `compact` or `comfortable`, with compact fallback.
Palette roles and `dp()`/style dimensions are used; Core property-table widths
are now passed in design units rather than being scaled twice.

## Retained adaptations and precise upstream gaps

These are limits of the pinned API, not claimed game-engine capabilities:

- Assets' standard popup emits open/favorite/rename/remove but exposes no custom
  menu extension callback (`AssetAction::context` is not emitted). The selected
  asset's **Asset actions** button retains import/reimport, references, material
  instance, duplicate/move, reload and protected trash workflows. Folder tree,
  breadcrumbs, recursive/compound queries and folder-move targets remain host
  domain adapters. The module's quick filter is an additional visible-name/type
  filter. Common grid/list has no type/size columns or favorite/import badges;
  selected metadata displays type/size and the actions still expose favorite and
  import state. The source descriptors retain those fields.
- Nodes has uniform cards, with no compact reroute shape, editable group-comment
  object, type-specific pin semantics or breakpoint/debugger model. Reroutes remain
  functional cards. The host overlays semantic pin colors, authored comment group
  borders and debugger markers. Existing click-click connections and intersection
  marquee behavior are bridged alongside common drag connections. No library
  canvas implementation is copied into the app. The common node/link API uses
  categorical color indices, so wire tints follow the active accent rather than
  reproducing the former white execution wires exactly. Semantic pin colors remain
  explicit host overlays.
- Inspector supports scalar properties/choices. Vectors, colors, arbitrary JSON,
  recursive records and nominal enum semantics use the existing schema adapters.
  Raw enum choices stay in the host because CyUI translates choice captions; user
  enum members must not become localized values. Record scalar fields use Inspector.
- Timeline is bound in read-only clip mode: real key-value/interpolation authoring
  uses the existing model widgets/commands. It is not a replacement animation
  engine, curve-tangent editor or game retargeting codec.
- Code/Diff select rows, with copy-line rather than arbitrary character selection.
  **Copy text** and an explicit **Text selection** mode retain the native read-only
  text control for that interaction. Editable operation/configuration JSON fields
  remain bounded native form controls; they are not source-code authoring editors.
- The legacy Core log supplies plain text and is displayed at info level. Typed
  graph/shader diagnostics retain their domain severity in the common list.
- The Sandbox scene viewport projects primitives and implements picking itself.
  Common Viewport is used for actual model render textures; a game renderer still
  enters through the documented adapter callback. It does not imply an Unreal
  runtime, renderer, game parser, material codec or compiler.
- The Windows SDK's `pascal` macro collides with `CodeLanguage::pascal`. A scoped
  push/undef/pop around the Code/Diff headers handles it without patching CyUI.
- Code builds its upstream lexical catalogue as one dependency. The pinned CMake
  API has no JSON/HLSL-only lexer switch; this app does not fork or trim that
  catalogue. The unnecessary editor/audio/browser libraries remain disabled.

## Build and verification

From the repository root, with VS2022 C++/Windows SDK, Git and CMake 3.24+:

```powershell
.\app\build.ps1
# Offline alternative, only with the exact clean locked checkout:
.\app\build.ps1 -CyUIRoot 'C:\src\CyUI'
# A fresh independent configure/output:
.\app\build.ps1 -BuildDirectory runtime\build\clean -OutputDirectory runtime\build\clean\output
.\runtime\python\Scripts\python.exe app\verify_cyui.py --captures
```

The test runner writes `data/reports/cyui-*.log`. Graphical tests own isolated roots
and inject only their own ImGui events. Automated input drains the event queue each
frame to prevent Win32 physical-pointer samples from being interleaved with a
synthetic release; normal interactive input keeps ImGui's default queue behavior.

Validation includes the native contracts, all existing Python SDK/MCP/worker and
GUI suites, plus `test_modules.py` in compact English and comfortable French.
The latter checks scalar commit/Undo/Redo, Escape, concurrent-revision cancellation,
local scalar-type replacement, keyboard command execution by caption and protocol ID, asset double-click, favorite actions and image wheel/fit.
`test_interface.py` additionally seeks the common animation timeline and checks
menus, themes, docking, preferences, real vertex gestures and playback.

`capture_interface.py`, `capture_3d.py` and `capture_cyui.py` render isolated EN/FR,
six-theme, density and simulated DPI layouts. Actual release evidence and counts
are recorded in `data/reports/cyui-validation.json`; source/executable checkpoints
and baseline hashes are local development artifacts, not Git dependencies.

Only Windows x64 / MSVC / DX11 is exercised. The application already has no
Linux/macOS native host; this migration adds no new platform restriction to the
Core/domain seams. Cross-platform CyUI itself is not evidence that this host has
been ported. Existing user asset/project files and saved preferences/layout must
match the preserved baseline hashes before activation. No editor is left running
in the user's root, and this migration is not pushed automatically.
