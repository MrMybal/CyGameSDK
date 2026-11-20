---
id: setup
title: Build and launch on Windows.
description: Build and launch on Windows
section: Getting started
order: 1
revision: 5
---

The 0.32.0 Windows preview includes the Free access profiles. Extract the folder,
launch the editor and copy its local MCP address to connect without a token in Free.

## Windows preview ZIP {#prebuilt}

The 0.32.0 Windows ZIP contains CyGameEditor.exe, CyGameCLI.exe, application
sources/examples, notices and this complete offline reader. Extract it to a new writable
folder and launch Lancer.cmd. The Microsoft Visual C++ 2015–2022 x64 runtime is required.

Native editing, assistant and CLI/MCP need no compiler, dependency checkout, Node.js or
Python. Import, shader, packaging and 3D compute jobs require Python 3.11+ followed by
app/setup_mcp.ps1. No venv, provider credentials or model weights are bundled.
Use your own provider/account. Read the release checksums before replacing an installation.

A standalone documentation ZIP contains the same reader. Open Documentation.html with
its complete DocData folder; no server or connection is required. The hosted Pages
manual follows its last deployment.

## Using this guide {#getting-started}

Install Visual Studio 2022 with Desktop development with C++, the Windows SDK, Git, CMake 3.24+ and Node.js 22+ with npm (manual generation only). Clone or extract the source in a new directory. Run these commands from the project root.

```powershell
.\app\build.ps1
.\app\setup_mcp.ps1
.\runtime\python\Scripts\python.exe app\setup_examples.py
.\Lancer.cmd
```

- The native build generates local help with the locked CyDoc revision and builds the locked CyMCPSDK source and fetches the exact CyUI revision from app/dependencies.lock.json. If upstream is private, authenticate Git first; never put credentials in source or CMake arguments.

- For offline builds, pre-cache the locked CyUI, CyMCPSDK and CyDoc checkouts and npm toolchains. Pass -CyUIRoot and -CyMCPSDKRoot for clean pinned checkouts. A clean checkout alone does not supply the complete npm cache. Reading the exported manual needs none of these tools or network access.

- setup_mcp.ps1 installs only the bounded Python compute-worker dependencies. CyMCPSDK is compiled from C++ sources; no previous runtime wheel is required.

- The host needs the Microsoft C++ runtime. Native CLI/MCP and chat do not require Python. Import, shader, build and 3D compute jobs use the configured Python workers.

## Continue reading {#next}

- [First editing session](doc:quickstart)
- [Browse assets](doc:content-browser)
- [Graph authoring](doc:graphs)
- [Materials](doc:materials)
- [3D assets](doc:assets-3d)
- [SDK operation reference](doc:operations)
