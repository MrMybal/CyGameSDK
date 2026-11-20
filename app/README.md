# Developer entry points

Current local development version: **0.32.0**. See the [root README](../README.md)
for launch/build and the [AI guide](../AI_GUIDE.md) for adaptation to a game.

- **native/Core.h**: domain state, adapter contract, commands and Undo.
- **native/Editor.h**: native CyUI host, panels, views and renderer callback.
- **native/automation**: source-built CyMCPSDK shared registry, native CLI, ChatUI and jobs.
- **automation_client.py / legacy_cli.py / tool.py**: compatibility clients and local editor IPC.
- **job_runner.py / job_contract.py**: trusted one-shot compute workers under SDK process ownership.
- **build_manifest.py**: the shared 177-operation business catalogue.
- **dependencies.lock.json**: exact CyUI, CyDoc and CyMCPSDK revisions.

Read [automation integration](Docs/CyMCPSDKIntegration.md),
[modular UI](Docs/CyUIMigration.md) and [architecture](Docs/EditorArchitecture.md).
The host/domain remain C++17; CodeEditor and SDK use isolated private C++20 bridges.
Windows x64/MSVC/DX11 is the implemented native platform.

Game codecs, real engine rendering and game build/play require verified adapters.
Portable 3D, bounded authoring, animation and graph contracts remain documented under
[Docs/Index.md](Docs/Index.md). User projects/assets and unknown metadata are preserved.
Do not copy an installed venv or modify sibling libraries.
