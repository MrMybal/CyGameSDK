<table width="100%">
  <tr>
    <td width="300" align="left" valign="top">
      <a href="https://mrmybal.github.io/CyGameSDK/">Website</a><br>
      <a href="https://mrmybal.github.io/CyGameSDK/manual/Documentation.html#/en/overview">Manual</a><br>
      <a href="https://github.com/MrMybal/CyGameSDK">GitHub</a><br><br>
      <sub>README</sub><br>
      <a href="README.md">English</a> · <a href="README.fr.md">Français</a>
    </td>
    <td width="400" align="center" valign="middle">
      <a href="https://mrmybal.github.io/CyGameSDK/"><img src="docs/assets/mark.svg" alt="CyGameSDK" width="128"></a>
    </td>
    <td width="300" align="right" valign="top">
      <p><sub>Application license</sub><br><a href="LICENSE"><strong>MIT</strong></a></p>
      <p><sub>Editor / catalogue</sub><br><a href="app/Docs/Release-0.29.0.md"><strong>0.29.0</strong></a><br><strong>DEVELOPMENT PREVIEW</strong></p>
      <p><a href="https://github.com/MrMybal/CyGameSDK/releases/download/v0.29.0/CyGameSDK-0.29.0-source.zip"><strong>Download source</strong></a><br><sub>Source ZIP · build on Windows x64</sub></p>
    </td>
  </tr>
</table>

<h1 align="center">CyGameSDK</h1>

A reusable native editor foundation for developers building a game-specific SDK
or authoring tool. Start with a dockable workspace, visual graphs, asset editors
and a shared MCP/AI SDK, then connect the formats, renderer and toolchain of the
game you want to support.

> **DEVELOPMENT PREVIEW · 0.29.0**
>
> Experimental tooling with a Sandbox adapter. Features follow documented limits;
> game compatibility requires a verified adapter. Read the
> [version notes, download scope and known limitations](app/Docs/Release-0.29.0.md).

[![CyGameSDK Sandbox workspace with viewport, transform gizmo, World Outliner, Details and Content Browser](app/Docs/images/editor-workspace.png)](app/Docs/images/editor-workspace.png)

## About CyGameSDK

Arrange panels, select scene objects, edit schema-driven properties and undo document
changes. Browse assets and open a specialized graph, material, JSON or neutral 3D
editor. The Unreal-inspired workspace uses C++17, Dear ImGui docking, modular CyUI
and Direct3D 11. English is the default and fallback language; French is selectable
in Options and an explicitly saved choice is preserved.

### Visual logic and materials

Create typed logic graphs with local functions, macros, classes and interfaces.
Inspect execution with breakpoints and a call stack. Preview signature/type changes
before applying them, collapse supported pure selections into helpers, or expand
pure calls into editable nodes with document Undo.

[![Logic graph editor with connected execution nodes, navigation and graph diagnostics](app/Docs/images/logic-graph.png)](app/Docs/images/logic-graph.png)

Author material graphs, texture sampling and instances, then compile the DX11
editor preview through a CyToolsCore worker. This is an editor shader workflow;
a game's material format and compiler need their own provider.

[![Material graph with texture sampling nodes and a compiled DX11 color preview](app/Docs/images/material-editor.png)](app/Docs/images/material-editor.png)

### Assets, animation and automation

Work with bounded `.cy3d` mesh, skeleton and animation documents; exchange GLB and
CyPack assets, author static LODs/collision shapes and use reference-pose retargeting,
offline IK, root-motion pairs and clip trim/resample tools. These are documented
editing profiles, not a complete DCC application or automatic support for game files.

[![Animation editor with the AnimatedTriangle example, skeleton overlay, keyframes and timeline](app/Docs/images/animation-editor.png)](app/Docs/images/animation-editor.png)

CyToolsCore supplies MCP, the AI SDK, authentication, jobs and ProcessWorker. The
interface, native IPC and SDK share EditorCore validation and document transactions.
Source adapters supply real game decoding, serialization, rendering and build/play
behavior. The default codec registry is empty; full Unreal Engine/Blueprint
compatibility is not provided.

## Features

- **Workspace:** docked tabs, detached windows, saved layouts, independent viewport
  cameras, command search, themes, text size and DPI support.
- **Assets:** folders, filters, thumbnails, favorites, references, guarded
  import/reimport, JSON tree editing and bounded PNG/JPEG/BMP inspection.
- **Graph authoring:** nominal enum/record types, containers, functions, macros,
  interfaces, debugger, guarded migrations and pure-call collapse/expansion.
- **Materials and 3D:** compiled editor previews, neutral mesh/skeleton/animation
  editing, portable interchange, LODs, collision shapes and motion tools.
- **Automation:** 175 editor operations, 196 tools including Core controls, with
  shared schemas and explicit job progress/cancellation.
- **Local help:** a CyDoc reader with 35 English/French chapters and the operation
  reference; Help and contextual F1 open the relevant page. No server is needed.

---

## Installation

The published **v0.29.0 download contains source**, not an installer or prebuilt
editor. The native graphical host runs on Windows x64 / Direct3D 11. Linux/macOS
native hosts are not implemented.

1. [Download the v0.29.0 source archive](https://github.com/MrMybal/CyGameSDK/releases/download/v0.29.0/CyGameSDK-0.29.0-source.zip)
   and extract it into a new folder, or clone the current source below.
2. Follow the build requirements and commands for the source you selected.
3. Start the built `CyGameEditor.exe` or `Lancer.cmd`. The graphical host requires
   the Microsoft Visual C++ 2015–2022 x64 runtime.

**The current `main` branch includes CyDoc help; the published v0.29.0 archive
predates that integration.** Clone `main` for the complete current manual and F1
integration. The archive's original setup remains in its
[tagged release guide](https://github.com/MrMybal/CyGameSDK/blob/v0.29.0/app/Docs/Release-0.29.0.md).

On first launch, the scene is a Sandbox demonstration. Ctrl+S saves the project;
Ctrl+O reloads it. Projects, working assets and preferences live in this installation's
`data/projects`, `data/assets` and `data/settings`. Keep existing data backed up.
Installing the supplied examples never overwrites existing assets.

The current manual can also be read directly from
[`Documentation/Documentation.html`](Documentation/Documentation.html), with its
complete `DocData/` folder beside it. Reading requires no Node.js or Internet access.

## Source, build and required libraries

<details>
<summary>Build the current source and configure MCP / worker jobs</summary>

### Requirements

- Windows x64; Visual Studio 2022 with **Desktop development with C++** and Windows SDK.
- Git and CMake 3.24+; the Visual Studio CMake tools are detected automatically.
- Node.js 22+ and npm for generating the current CyDoc manual, not for reading it.
- The exact [CyUI and CyDoc revisions](app/dependencies.lock.json). The build obtains
  them from their source repositories. Offline builds need complete cached toolchains
  and dependencies, not just a source folder.
- For MCP, the AI SDK, imports and worker-based shader/build jobs: Python 3.11+ and the
  **CyToolsCore 0.10.1 wheel**, built beforehand from the Core checkout.

### Get the source

```sh
git clone https://github.com/MrMybal/CyGameSDK.git
cd CyGameSDK
```

### Build and run

Run PowerShell from the repository root:

```powershell
.\app\build.ps1
.\Lancer.cmd
```

For MCP and worker jobs, prepare the Core wheel at
`CyToolsCore/dist/cytools_core-0.10.1-py3-none-any.whl`, then configure the private
Python environment. The default Core checkout is a sibling of CyGameSDK:

```powershell
.\app\setup_mcp.ps1
.\runtime\python\Scripts\python.exe app\setup_examples.py
```

Select alternative authorized checkouts with `-CyUIRoot` on `app/build.ps1` or
`-CyToolsCoreRoot` on `app/setup_mcp.ps1`. A supplied CyUI checkout must be clean
and match the dependency lock; it is not modified. `-BuildDirectory` and
`-OutputDirectory` select an independent native build.

CyUI shares one host-owned ImGui instance and compiles the selected modules.
Native Audio, Gallery, Demo, CodeEditor and Web/CEF integrations are disabled.
CyDoc's separate reader uses image, gallery and graph; it does not ship Three.js.

The build writes `CyGameEditor.exe` to the project root and copies the complete
local manual beside it. `app/build_native.cmd` delegates to the same CMake entry.
To regenerate only the manual:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File app/build_documentation.ps1
```

### Connect an MCP client

Start the editor and adapt the installation paths in
[app/mcp.example.json](app/mcp.example.json). Use the private Python to run:

```powershell
.\runtime\python\Scripts\python.exe app\tool.py describe
.\runtime\python\Scripts\python.exe app\tool.py mcp
```

Discover operations with `cy_list_operations` / `cy_describe_operation`, invoke
`op_<id>` and follow `cy_get_job` to Completed, Cancelled or Failed. After a mutation
timeout, inspect live state before retrying. Shared clients use the existing
Core authenticated HTTP service and MCP bridge.

Build/setup details: [troubleshooting](app/Docs/LaunchRecovery.md),
[CyUI integration](app/Docs/CyUIMigration.md),
[CyDoc generation](app/Docs/CyDocIntegration.md).

</details>

---

## License

**CyGameSDK's original code, documentation and examples use the [MIT license](LICENSE).**
Retain its copyright and license notice when redistributing. Third-party libraries
keep their own terms.

### Native editor and SDK

| Library · source | License · terms | Use / scope |
| --- | --- | --- |
| [CyUI 1.2.0](https://github.com/MrMybal/CyUI) | [MIT and component notices](app/LICENCES/CyUI-MIT.txt) · [bundled notices](app/LICENCES/CyUI-THIRD-PARTY-NOTICES.md) | Selected native UI modules; exact revision in the dependency lock. |
| [Dear ImGui, pinned CyUI snapshot](https://github.com/ocornut/imgui) | [MIT](app/LICENCES/Dear-ImGui-MIT.txt) | Docking and native widget rendering; one shared instance. |
| [Bundled stb](https://github.com/nothings/stb) | [MIT option](app/LICENCES/Dear-ImGui-stb-MIT.txt) | Font/text helpers included with ImGui. |
| [ImGuizmo](https://github.com/CedricGuillemet/ImGuizmo) | [MIT](app/LICENCES/ImGuizmo-MIT.txt) | Viewport transform gizmos; pinned source snapshot. |
| [nlohmann/json 3.11.3](https://github.com/nlohmann/json) | [MIT](app/LICENCES/nlohmann-json-MIT.txt) | Native document and operation JSON. |
| [Lexilla, Scintilla and SciTE data](https://github.com/ScintillaOrg/lexilla) | [Permissive notice](app/LICENCES/Lexilla.txt) · [Scintilla](app/LICENCES/Scintilla.txt) · [SciTE](app/LICENCES/SciTE-Properties.txt) | Syntax engines, interface headers and properties through Code/Diff; no Scintilla editor UI. |
| [CyToolsCore 0.10.1](https://github.com/MrMybal/CyToolsCore) | [MIT](app/LICENCES/CyToolsCore-MIT.txt) | Python MCP/AI SDK, authenticated sessions, jobs and workers. |
| [CPython and Python packages](https://github.com/python/cpython) | [Python terms](app/LICENCES/Python.txt) · [package licenses/notices](app/licenses-python.json) | Separately configured MCP/worker environment; packages retain their listed licenses. |
| Microsoft Windows SDK / Direct3D / C++ runtime | [Component scope](app/LICENCES/README.md) · Microsoft terms | Windows host and graphics toolchain; system fonts are not redistributed. |

### Documentation and build tools

| Library · source | License · terms | Use / scope |
| --- | --- | --- |
| [CyDoc 0.1.0](https://github.com/MrMybal/CyDoc) | [MIT](app/LICENCES/CyDoc-MIT.txt) | Exported local reader; image, gallery and graph modules. |
| [Node.js 22+](https://github.com/nodejs/node) | [MIT and component notices](https://github.com/nodejs/node/blob/main/LICENSE) | Documentation generation toolchain; not a reader runtime. |
| [marked 17.0.6](https://github.com/markedjs/marked) | [MIT](https://github.com/markedjs/marked/blob/v17.0.6/LICENSE.md) | Build-time Markdown compilation in the locked application toolchain. |
| [PrismJS 1.30.0](https://github.com/PrismJS/prism) | [MIT](https://github.com/PrismJS/prism/blob/v1.30.0/LICENSE) | Build-time code highlighting. |
| [esbuild 0.25.12](https://github.com/evanw/esbuild) | [MIT](https://github.com/evanw/esbuild/blob/v0.25.12/LICENSE.md) | Bundling the offline reader and selected modules. |
| [TypeScript](https://github.com/microsoft/TypeScript) | [Apache-2.0](https://github.com/microsoft/TypeScript/blob/main/LICENSE.txt) | Building the pinned CyDoc package before export. |

This table is an index, not a replacement for complete notices. Preserve
[app/LICENCES](app/LICENCES/README.md), the Python package notices and the license
files distributed with generated documentation and toolchain packages.

## References and links

- [English manual](https://mrmybal.github.io/CyGameSDK/manual/Documentation.html#/en/overview) · [Manuel en français](https://mrmybal.github.io/CyGameSDK/manual/Documentation.html#/fr/overview).
- [Product website](https://mrmybal.github.io/CyGameSDK/) · [Source repository](https://github.com/MrMybal/CyGameSDK).
- [Published source release](https://github.com/MrMybal/CyGameSDK/releases/tag/v0.29.0) · [Issue tracker](https://github.com/MrMybal/CyGameSDK/issues).
- [Portable asset contract](https://mrmybal.github.io/CyGameSDK/manual/Documentation.html#/en/portable-assets) · [Source adapter example](app/examples/adapters/ExampleGameAdapter.h).

<details>
<summary><strong>AI Usage</strong></summary>

- [Start here: primary AI guide](AI_GUIDE.md) — project overview, important files and the path to a game-specific SDK.
- [Repository instructions](AGENTS.md) · [Native developer guide](app/AGENTS.md) — ownership, versions and implementation constraints.
- [Build this application](app/Docs/LaunchRecovery.md) — setup and launch troubleshooting.
- [Create a game adapter](app/Docs/GameAdapterGuide.md) · [Porting dossier](app/Docs/GamePortingTemplate.md) — source contracts and real game evidence.
- [CyUI integration](app/Docs/CyUIMigration.md) · [CyDoc authoring/build](app/Docs/CyDocIntegration.md) — presentation modules and local help maintenance.

</details>
