---
id: setup
title: Build and launch on Windows.
description: Build and launch on Windows
section: Getting started
order: 1
revision: 1
---

## Using this guide {#getting-started}

Install Visual Studio 2022 with Desktop development with C++, the Windows SDK, Git, CMake 3.24+ and Node.js 22+ with npm (manual generation only). Clone or extract the source in a new directory. Run these commands from the project root.

```powershell
.\app\build.ps1
.\app\setup_mcp.ps1
.\runtime\python\Scripts\python.exe app\setup_examples.py
.\Lancer.cmd
```

- The native build generates local help with the locked CyDoc revision and fetches the exact CyUI revision from app/dependencies.lock.json. If upstream is private, authenticate Git first; never put credentials in source or CMake arguments.

- For offline builds, pre-cache the locked CyUI and CyDoc checkouts and npm toolchains. Pass -CyUIRoot for CyUI. A clean checkout alone does not supply the complete npm cache. Reading the exported manual needs none of these tools or network access.

- setup_mcp.ps1 requires the CyToolsCore 0.10.1 wheel built from an authorized checkout. Select its location with -CyToolsCoreRoot.

- The host needs the Microsoft C++ runtime. MCP and worker-based import, shader and build jobs need the configured Python/Core environment.

## Continue reading {#next}

- [First editing session](doc:quickstart)
- [Browse assets](doc:content-browser)
- [Graph authoring](doc:graphs)
- [Materials](doc:materials)
- [3D assets](doc:assets-3d)
- [SDK operation reference](doc:operations)
