# CyUI integration — native 0.31

CyGameSDK pins CyUI's current modular implementation at
`6ef01cfcdb197f3a89d64618ce5ef8818c704728`. Upstream still declares version
1.2.0; the exact commit in `app/dependencies.lock.json` identifies these newer
editor APIs. CyDoc and CyMCPSDK have independent exact locks. DocumentViews supplies the
shared ChatUI transcript; no local conversation renderer is copied.

This application owns documents, engine/game adapters, validation, resource access,
persistence and Undo. CyUI supplies presentation models and gestures. No sibling
repository is modified. The previous 0.29 source and executable are preserved as
local development checkpoints, not dependencies.

## Build and module selection

Configure `CYUI_MODULES` from the lock before adding the library and link explicit
targets. One host-owned `editor_imgui` target supplies the pinned docking snapshot
to every module, backend and the existing ImGuizmo adapter.

| Module | Application integration |
| --- | --- |
| Core | Palette, density/DPI, fonts, localization, layout, ordinary widgets, filters and notifications |
| Nodes / Canvas | Logic/material/function cards, wires, navigation, selection and comment-group gestures |
| Inspector | Exact scalar values, numeric expressions, vectors, typed records and virtual arrays/Sets/Maps |
| Hierarchy | World Outliner navigation, selection, rename, visibility, locking and folder drop proposals |
| DataView | Adapter-driven scene property table, sorting/filtering, rectangular selection and atomic typed TSV paste |
| FilePicker | Native source-file selection with cached asynchronous host filesystem listings |
| Code / Diff | Generated source/result presentation, syntax colors, search and saved/current comparisons |
| CodeEditor | Editable JSON fields and read-only character selection through the common native text backend |
| Assets | Existing content tiles/list, filtering, thumbnails, selection and standard actions |
| Images / Viewport | Existing texture/model surfaces, pan/zoom and host rendering callbacks |
| ImGuizmo adapter | Existing revision-aware scene transforms and snapping |
| Timeline | Existing bounded animation track, marker and playhead presentation |
| Commands | Palette plus shared menu/toolbar/keyboard metadata and persisted shortcut bindings |
| DocumentViews | Shared CyMCPSDK ChatUI conversation documents |
| Diagnostics | Bounded output logs and navigable graph/shader diagnostics |

Canvas and Images are enabled transitively. Audio, AudioViews, Gallery, demos,
Web/CEF, Media, Office and document-authoring modules are not application
dependencies. No `All` or umbrella target is used.

CodeEditor and CyMCPSDK need C++20. Their private bridges isolate SDK headers and
JSON types from the C++17 domain/host. Read [CyMCPSDK integration](CyMCPSDKIntegration.md).

The optional CodeEditor backend needs C++20. `editor_source_ui` is a small
private C++20 bridge containing only that backend; the editor and domain targets
remain C++17. CodeEditor headers do not enter Core or force the legacy host's
UTF-8 filesystem/JSON contracts into another language mode. The bridge delegates
to CyUI rather than copying TextEditor or its implementation.

```powershell
.\\app\\build.ps1
# Use only a clean checkout matching the exact dependency lock:
.\\app\\build.ps1 -CyUIRoot C:\\src\\CyUI
# Independent configure/output for UI work:
.\\app\\build.ps1 -CyUIRoot C:\\src\\CyUI -BuildDirectory runtime\\build\\cyui-next -OutputDirectory runtime\\qa-cyui-next
```

Alternate outputs contain native binaries and help, not a standalone installer.
Desktop job checks also need app, CyGameCLI and compute Python beside the binary.
The local QA candidate reuses the original installation through private junctions;
no venv is copied or relocated. The default build outputs into the actual project.

For repeated native-only iterations, configure CMake with
`-DCYGAME_BUILD_DOCUMENTATION=OFF`: it copies the existing complete reader.
Fresh/production configurations default to regeneration. Generate the final
manual through `app/build_documentation.ps1`; do not ship an obsolete copy.

## Typed Inspector ownership

`UiPropertyAdapter` converts schema/value snapshots into common property models
and proposals back into JSON. Empty collections retain their element schema;
integer arrays never round-trip through double. Record field metadata and
unknown host metadata remain destination-owned. The validator checks the complete
proposed value against the original application schema before a commit.

Graph collection schemas retain their actual array/object kind instead of being
flattened to an untyped JSON text field. The existing graph budgets remain in
force. Primitive values, vectors, records, arrays, Sets and Maps use the shared
editors; arbitrary JSON uses a bounded explicit JSON popup and CodeEditor.

Authored enums use a custom one-row selector because CyUI's numeric enum captions
are localized. Its typed payload preserves the original JSON member and a
graph-scoped nominal/schema identity. RGB values use a custom three-channel row:
the shared RGBA editor must not introduce and silently discard an alpha channel.
These are domain adapters, not copied Inspector implementations.

Escape, hidden fields, changed source types/values and external revisions cancel
drafts. The existing active-ID/deactivation replay guard remains. JSON popups also
check their captured source value, schema and revision before accepting Apply.
A committed field/collection change is one document transaction.

## Scene hierarchy and table

Hierarchy IDs derive from untranslated entity/folder identities, with collision
checks. Selection maps back to Core selection. Folder rows adapt the existing
organizational folder strings; they do not introduce entity parenting or ordering.
A move into another entity is vetoed. Folder rename/delete requests are not
invented scene operations; supported object actions remain in Core.

The table builds its typed columns from the adapter schema and its rows from the
same live objects. Locked rows and read-only fields remain protected. CyUI validates
every proposed paste cell; `property_set_many` then independently validates the
whole batch, rejects duplicate targets/stale revisions and creates one Undo.
A failed adapter scene validation rolls back every field through EditorCore.

Hierarchy/table mode and key bindings are view preferences. They are not document
Undo. `Focus selection`, keyboard navigation and source object IDs survive locale,
theme and density changes.

## Graph groups and semantic clipboard

Authored comments are common `NodeGroup` objects. Move/resize/rename/removal
proposals return through `graph_layout_apply`, together with node movements and
incident-link removals. One completed group movement, including contained nodes,
is one revision-guarded Undo. Model validation preserves function boundaries and
unknown comment/node metadata. Cancel/stale gestures do not change the document.

CyUI's generic clipboard contains visual cards and pins, not this editor's node
types, values or function/type/interface libraries. Copy requests are translated
through `graph_fragment_copy` and the domain fragment format. Comment groups now
travel with that format and receive fresh IDs on paste. The host's checked paste
and duplicate actions remain authoritative.

The common generic Paste/Duplicate actions stay disabled because their allocator
cannot reconstruct game/graph semantics. Existing toolbar/keyboard actions remain.
Functional reroutes stay typed graph nodes; generic visual link points must not
replace execution or nominal pin semantics. Existing intersection marquee and
click-click wiring are retained alongside common drag wiring.

## Files, commands and source views

The source picker performs no import itself. It consumes cached listings prepared
outside drawing, limits a directory to 10,000 entries and rejects loading/stale/
failed snapshots. The host rechecks selected regular files, then uses existing
provider preparation/import validation. Drive choice, lexical navigation and
filters are presentation; OS access, symlinks and import destination checks stay
in the host.

The command registry owns common IDs, metadata, bindings, enablement and typing
policy. Menus and toolbar tooltips use those bindings. This multi-root docking
shell performs one host-wide key pass through registry matching. The existing
Core dispatcher executes actions; the registry does not become a scheduler.
Bindings persist only with normal view preferences.

Source views use the common code font, syntax palette and per-view zoom. Editable
operation/class/codec JSON retains its explicit size limit and Execute/Apply
workflow. The read-only character-selection mode now uses CodeEditor as well.
Text Undo belongs to a form buffer; document Undo remains in Core. Changing a
form's source externally replaces its text backend instead of replaying old text.

## Remaining adaptations and upstream limits

- Assets still has no custom context-action callback or type/size/badge columns.
  The existing Asset actions button, folder/breadcrumb/query adapters and selected
  metadata retain import/reimport, references, material instances, moves and trash.
- Timeline remains a bounded clip/track viewer. The host's real animation-key,
  interpolation, retargeting and motion operations are not replaced by generic
  clip split/reorder requests. A game animation binding still needs its own adapter.
- Native Code/Diff row selection remains useful for diagnostics. Character
  selection uses the optional read-only CodeEditor mode. No source-language
  compiler, LSP service or paged engine-file editor is implied.
- Nodes semantic pin colors, breakpoints, debugger traces and runtime meanings
  remain host overlays/adapters. No Unreal runtime or Blueprint codec is bundled.
- The selected Code module still builds its upstream lexical catalogue as one
  dependency; there is no JSON/HLSL-only lexer switch at this revision.
- Sandbox scene projection/picking and DX11 previews remain host renderers.
  Common Viewport does not render a game scene.
- The Windows SDK `pascal` macro is scoped around Code-related headers without
  patching the dependency.

## Verification

Native contracts cover atomic property batches, duplicate/stale/invalid rejection,
group-plus-node Undo and fresh comment IDs. `CyGameUIContractsTests` covers exact
int64 values, empty collection templates, nominal clipboard rejection, retained
metadata and atomic typed TSV proposals.

The isolated module fixture exercises scalar commits, Undo/Redo, Escape, stale
SDK drafts, schema replacement, command searches, asset/image gestures, virtual
array clipping and real keyboard hierarchy selection/F2 rename. Existing graph,
migration, animation, worker and UI suites remain the regression baseline.

`CYGAME_EDITOR_EXE` selects a candidate executable in tests/capture scripts.
This permits verification before activation without testing the user's old binary.
Only Windows x64 / MSVC / DX11 is tested by this host; Linux/macOS native hosts
remain unimplemented. Actual run results and captures belong in private reports.

## Notices

Retain the original application MIT license, CyUI and third-party notices,
TextEditor MIT, and JetBrains Mono's SIL OFL 1.1. Bundled code fonts require no
system installation. CyMCPSDK and Python compute package notices remain independent.
