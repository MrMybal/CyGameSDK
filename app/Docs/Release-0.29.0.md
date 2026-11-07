# CyGameSDK 0.29.0

[English](Release-0.29.0.md) · [Français](Release-0.29.0.fr.md) · [README](../../README.md)

## Status and downloads

**Development preview.** CyGameSDK is an experimental editor foundation with a
Sandbox adapter. The source is intended for testing and game-adapter development. Features remain
bounded by the documented editing profiles.

| Source | Version | Availability |
| --- | --- | --- |
| Current `main` editor/catalogue | 0.29.0 | Source includes the CyDoc manual and contextual Help/F1 integration. Build locally on Windows x64. |
| Published `v0.29.0` source archive | 0.29.0 | Modular CyUI release, published before the CyDoc migration. No prebuilt editor or installer is included. |

[Download the published source ZIP](https://github.com/MrMybal/CyGameSDK/releases/download/v0.29.0/CyGameSDK-0.29.0-source.zip)
· [Release and original notes](https://github.com/MrMybal/CyGameSDK/releases/tag/v0.29.0)
· [Published SHA256SUMS.txt](https://github.com/MrMybal/CyGameSDK/releases/download/v0.29.0/SHA256SUMS.txt).

The current source and the existing archive share a version number but contain
different revisions. Use `main` for the current documentation integration; do not
assume that later source changes are present in the published ZIP.

## Current source additions after the published archive

The [CyDoc migration](https://github.com/MrMybal/CyGameSDK/commit/d92f95365d3ba373409941860021e8e3b95c2231)
adds a complete local HTML reader, 35 English/French chapters and references for
175 editor operations. Help, contextual F1 and the Editor Overview documentation
button target stable pages/sections in the selected editor language.

The complete `Documentation/` export opens directly through `file://`; it needs
no server or Node.js at reading time. The static site mirrors that reader, while
old setup-guide links redirect to it. Generation/copy/install rules, dependency
notices and translation checks accompany the source. CyDoc 0.1.0 is pinned to
`e038c474577e1f305df3eba421f2b9c03b0f1d8d` and includes image, gallery and graph
readers. No mesh reader or Three.js is exported.

## Published 0.29.0 changes

This release migrates the native editor to modular CyUI while retaining the game
adapter, document, SDK/MCP and Undo/Redo contracts:

- Shared graph canvas, property editing, asset browsing and image navigation.
- Shared viewport and transform adapter, animation timeline, command palette,
  logs, diagnostics, highlighted source views and saved/current comparisons.
- CyUI 1.2.0 pinned to `ff023e646c6a3038b3dfbfd53f6d2b4a2f5955e3`,
  explicit module targets and one host-owned Dear ImGui instance.
- One CMake build path with unused Audio, Gallery, Demo, Web/CEF and CodeEditor
  integrations disabled in the native application.
- English default/fallback UI, selectable French, themes, density and DPI support.

Read the [CyUI integration details and remaining module limits](CyUIMigration.md).
CyToolsCore remains the existing MCP/AI SDK and job runtime.

## Build and first launch

The current native host requires Windows x64, Visual Studio 2022 with C++, Windows
SDK, CMake 3.24+, Git and the Microsoft Visual C++ 2015–2022 x64 runtime. Current
`main` also requires Node.js 22+ and npm to generate help, plus the locked CyUI/CyDoc
source dependencies. Reading the exported manual does not require these build tools.

MCP/AI operations, imports and worker-based shader/build jobs require Python and
the separately prepared CyToolsCore 0.10.1 wheel. `app/setup_mcp.ps1` installs the
configured environment; `app/setup_examples.py` installs supplied examples without
overwriting existing assets. See the [README build steps](../../README.md#source-build-and-required-libraries).

Build with `app/build.ps1`, then start `Lancer.cmd`. Keep installations in separate
folders and back up user data before updating. Projects, working assets and settings
belong to the local installation's `data` folder; private runtimes are not included
in the source archive. The archive's original requirements remain in the
[tagged guide](https://github.com/MrMybal/CyGameSDK/blob/v0.29.0/app/Docs/Release-0.29.0.md).

## Known limitations

- Native Linux/macOS hosts and a redistributable application installer are not implemented.
- No real game codec, engine renderer or game compiler is bundled. The default
  Sandbox build packages document data, not cooked game files or a runnable game.
- The graph runtime is bounded and does not provide complete Unreal Blueprint compatibility.
- Mesh, skeleton, animation and image workflows use documented profiles, not a
  complete DCC or pixel-painting tool. Source extensions are not an ABI-stable DLL loader.

The [current manual](https://mrmybal.github.io/CyGameSDK/manual/Documentation.html#/en/limits)
explains feature limits and target-game adapter requirements.

## Bugs and issue reports

The published release does not provide a bug-by-bug issue list. Consult the actual
[issue tracker](https://github.com/MrMybal/CyGameSDK/issues) and include the source
revision, platform, relevant asset and reproduction steps when reporting a problem.
