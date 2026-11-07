---
id: automation
title: MCP and SDK automation
description: MCP and SDK automation
section: Getting started
order: 3
revision: 1
---

## Using this guide {#getting-started}

Start the editor. Configure your MCP client with app/mcp.example.json and absolute paths for your installation. Run app/tool.py with the private Python environment. CyToolsCore provides the server and jobs.

```powershell
.\runtime\python\Scripts\python.exe app\tool.py describe
.\runtime\python\Scripts\python.exe app\tool.py mcp
```

- Discover operations and inspect their schemas before calling them.

- GUI, native IPC and SDK share EditorCore validation and document transactions.

- Poll Core jobs to a terminal state. After a mutation timeout, inspect current state before retrying: the edit may already have applied.

## Continue reading {#next}

- [First editing session](doc:quickstart)
- [Browse assets](doc:content-browser)
- [Graph authoring](doc:graphs)
- [Materials](doc:materials)
- [3D assets](doc:assets-3d)
- [SDK operation reference](doc:operations)
