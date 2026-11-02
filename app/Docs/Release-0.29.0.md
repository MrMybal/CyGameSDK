# CyGameSDK 0.29.0

This release migrates the native editor to the modular CyUI architecture while
retaining the game adapter, document, SDK/MCP and Undo/Redo contracts.

## Changes

- Shared graph canvas, property editing, asset browsing and image navigation.
- Shared viewport and transform adapter, animation timeline, command palette,
  logs, diagnostics, highlighted source views and saved/current comparisons.
- CyUI 1.2.0 pinned to `ff023e646c6a3038b3dfbfd53f6d2b4a2f5955e3`,
  explicit module targets and one host-owned Dear ImGui instance.
- One CMake build path with unused audio, gallery, demo and browser/editor
  integrations disabled.
- English default/fallback UI, selectable French, themes, density and DPI support.

See [migration details and module limits](CyUIMigration.md).

## Downloads and first launch

`CyGameSDK-0.29.0-source.zip` contains the source and documentation.
This is a source release. Build the editor with `app/build.ps1`, then run
`Lancer.cmd`. The graphical host requires Windows x64 and the Microsoft Visual
C++ 2015-2022 x64 runtime.

Building requires VS2022 C++, CMake, Git and access to the locked CyUI repository
(or an authorized matching offline checkout). Python, CyToolsCore and its worker
dependencies are installed separately with `app/setup_mcp.ps1`; they are required
for MCP/AI operations, imports and worker-based shader/build jobs.
Run `app/setup_examples.py` with that configured Python to install the examples.
Read the root README for setup and the exact CyToolsCore wheel requirement.

Installations use their own `data` folder for projects, assets and preferences.
Keep existing projects backed up and do not extract over a working installation.
The archives contain generated examples under `app/examples`; their installation
refuses to overwrite existing data.

## Scope

The native host is Windows/DX11; Linux/macOS hosts remain unimplemented.
CyGameSDK is a reusable foundation with a Sandbox adapter. It does not bundle a
real game codec, engine renderer or full Unreal Engine/Blueprint compatibility.
Game-specific formats, toolchains and runtime bindings require a verified adapter.
