> Current native release: 0.21.0. See [AdvancedAuthoring](AdvancedAuthoring.md).
> This document retains its historical milestone/recovery evidence.

> Historical 0.16.1 audit. Current release 0.20.0 retains this palette/workspace
> baseline and adds the tools in [CompletionMilestones](CompletionMilestones.md).

# Interface audit — CyGameSDK 0.16.1

Audited 2026-10-24. Active native source/catalogue/executable: 0.16.1, based on
0.16.0. This patch changes editor presentation/navigation; it does not activate the
preserved 0.19 import pipeline or add game codecs. The optional portable asset
source component remains 0.1.

## Visual reference and theme contract

Reference: [Epic's Unreal Editor interface](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-editor-interface).
The application uses a new **Unreal-inspired Dark** CyUI palette (`unreal-dark`):
neutral charcoal surfaces, dark fields, restrained blue focus/selection, compact
controls and nearly square tabs. This is an independently authored approximation,
not an Epic theme export or an assertion of pixel-identical styling.

| Role | Colour |
|---|---|
| Canvas / menu / status | `#181818` |
| Panel | `#262626` |
| Header / card | `#303030` |
| Field | `#1c1c1c` |
| Hover / pressed | `#3c3c3c` / `#505050` |
| Primary / secondary / hint text | `#e0e0e0` / `#b5b5b5` / `#929292` |
| Focus / selection accent | `#0070e0` |

`native/EditorTheme.h` owns the palette through `cyui::setPalette`; stock ImGui
controls and custom canvases use the same semantic roles. No sibling CyUI source
was modified. Six theme choices remain available in Options. Unknown preference
keys fall back to the new palette. Explicit theme choices remain persistent;
this workspace's previously saved Navy choice was changed to the requested new
theme. Language, text size, favorites, documents and saved layouts are retained.
English remains the first-run/default/fallback language; French remains selectable.

## Navigation and restoration corrections

- Every top-level menu has an independent filter, matching English and localized labels.
- Window entries now open and close their panels and keep stable IDs.
- Operations has a filter for labels and protocol IDs.
- Help > Editor Overview shows the inventory below with search and working panel links.
- Reset layout restores only the current workspace preset rather than opening every tool.
- Fresh startup uses the Level preset; restoring Scripting/Materials no longer opens an extra Viewport.
- Compiler panel preferences restore after all panels have registered. Disabled plugins stay closed.
- Additional viewports dock beside the existing central view by default.
- Content Drawer offers Dock in Layout; the status bar exposes Output Log.
- Narrow content panels wrap their search/filter toolbar; the Details folder field fits its inspector.
- Adapter identity comes from its descriptor in the menu/project panel. Diagnostics labels are localized.
- Inactive content tiles use neutral icons, keeping blue reserved for active controls and selection.

## Shipped feature inventory

The same data lives in `app/interface_features.json` and Help > Editor Overview.
**Available** means a usable generic editor workflow; **Foundation** means a
bounded implementation with the listed limits. Source components and adapter
seams do not imply a shipped native editor or a working game integration.

| Area | Feature | State | Actual scope / limits |
|---|---|---|---|
| Workspace | Docking and detached windows | Available | Dock tabs, detach windows and save named layouts. |
| Workspace | Multiple viewports | Available | Up to 16 independent cameras; Sandbox preview rendering. |
| Workspace | Search and navigation | Available | Ctrl+P, menu filters, properties, graph navigation and asset queries. |
| Workspace | Language and themes | Available | English by default, selectable French and six palettes. |
| Scene | Level / map editor | Foundation | Scene documents, selection, folders, locks and Undo; no game map codec. |
| Scene | Transform tools | Foundation | Move, rotate, scale, local/world and snapping; group translation only. |
| Scene | Automatic properties | Available | Adapter schemas generate fields; unknown game memory is not reflected. |
| Scene | Engine viewport rendering | Adapter required | The game adapter must provide rendering, picking and engine integration. |
| Assets | Content Browser | Available | Folders, queries, type filters, favorites, tiles/list and file actions. |
| Assets | Asset references and Trash | Available | Declared graph dependencies, consumers and safe file restore. |
| Assets | Graph asset editors | Available | Independent document tabs and local function subgraphs. |
| Assets | Image / texture viewer | Foundation | PNG/JPEG/BMP channels, zoom and pan; no pixel painting or game texture codec. |
| Assets | JSON documents | Foundation | Project/graph/configuration JSON and metadata inspection; no general JSON tree editor. |
| Assets | Import / reimport and thumbnails | Not available | Preserved 0.19 candidate only; not installed in this release. |
| Assets | Static mesh editor | Not available | No native mesh document editor, LOD or collision authoring yet. |
| Assets | Skeleton and animation editors | Not available | No visual rig, skinning, animation timeline or retargeting editor yet. |
| Assets | Portable cross-SDK assets | Source component | Independent .cypack / GLB library and CLI; not exposed in this native UI. |
| Graphs | Logic node authoring | Foundation | Typed wires, variables, local functions, clipboard and comment groups. |
| Graphs | Graph debugger | Available | Breakpoints, call stack, watches, step over/into/out and diagnostics. |
| Graphs | Material authoring | Foundation | Procedural nodes, instances and DX11 sphere/plane preview shaders. |
| Graphs | Complete Blueprint feature set | Not available | Macros, classes, containers, latent execution and engine compilation remain work. |
| Graphs | Material texture sampling | Not available | Image viewing does not supply material samplers or engine shader export. |
| Integration | Editor plugins | Source component | Trusted source plugin lifecycle and editor registry; no dynamic DLL loader. |
| Integration | Build and package | Adapter required | Sandbox exports a document ZIP. A real game compiler requires a provider. |
| Integration | Play in editor | Not available | No game runtime bridge is installed. |
| Integration | Native AI access | Available | Existing CyToolsCore MCP/SDK, shared dispatcher and official worker jobs. |
| Integration | Source control and profiling | Not available | No Git/Perforce UI, engine profiler, landscape or cinematic editor. |

## Verification and reproducible evidence

- Native model contracts: `data/reports/ui-audit-native-tests.log`.
- Graphical integration suite: `data/reports/ui-audit-integration-tests.log`.
- Own-ImGui mouse/keyboard navigation (themes, independent menu filters, close/reopen,
  drawer docking, default workspace and new viewport docking): `data/reports/ui-audit-gestures.json`.
- Persisted French/theme/Materials workspace and all core panel callbacks:
  `data/reports/ui-audit-interface-tests.log`, `ui-audit-panel-registry.json`.
- Isolated EN/FR workspace, tool, function, texture and all-six-theme renderer captures:
  `data/reports/ui-audit-captures.json` and `ui-audit-*.bmp`.
- Release, catalogue, translation inventory, user-data hashes and checkpoint:
  `data/reports/ui-audit-validation.json`.

These checks cover shipped generic workflows and native rendering. They do not
verify external game parsers, game compilation, a full Blueprint implementation,
automatic reflection or the missing specialized editors. Not every combination of
DPI, window size and detached multi-monitor layout has been exercised.

```powershell
.\app\build.ps1
.\runtime\build\CyGameSDKTests.exe (Get-Location).Path
$env:CYGAME_TEST_GUI='1'
.\runtime\python\Scripts\python.exe app\tests\test_integration.py
.\runtime\python\Scripts\python.exe app\tests\test_interface.py
.\runtime\python\Scripts\python.exe app\capture_interface.py
```

## Next interface milestones

1. Restore and independently validate a bounded import/reimport pipeline and decoded thumbnails.
2. Connect neutral native mesh/skeleton/animation documents to dedicated editors, using
   [CommonAssetFormat](CommonAssetFormat.md), with preview, LOD/collision and a timeline.
3. Add material samplers and game-specific rendering/shader conversion providers.
4. Expand graph semantics (containers, macros, latent events) with explicit runtime contracts.
5. Add game-backed level formats, build/play and specialized tools only when an adapter verifies them.

No inactive button or extension filter substitutes for those workflows.
