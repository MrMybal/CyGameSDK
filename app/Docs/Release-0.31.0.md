# CyGameSDK 0.31.0 — integrated automation

Development preview for Windows x64. Download the portable Windows folder,
source archive or complete offline manual from the v0.31.0 release.

CyMCPSDK 0.7.0, revision **791d4d7d7680213b282ec8529d248a10f8494d77**, replaces the
former Python automation runtime. The SDK is compiled from sources. Core, Platform,
Http, MCP, AI and ChatUI are enabled; Local/model downloads are excluded.

- AI Assistant uses common ChatUI: provider/model discovery, streaming, cancellation,
  history restore, file attachments and optional API context reduction.
- **CyGameCLI.exe** provides real discovery, schemas, JSON invocation, diagnostics,
  stdio MCP and authenticated loopback HTTP. Existing editor launch options and
  **app/tool.py** scripts remain supported.
- Chat, CLI and MCP share 177 business operations and one permission policy.
  Full discovery includes 177 compatibility job aliases and ten service operations;
  compact discovery uses five gateway tools.
- Private durable jobs retain outputs after finish-task, restart and SDK resource expiry.
  Interrupted work is inspected explicitly and never automatically replayed.
- Legacy SQLite clients/token hashes, sessions, job records and saved recipe formats
  are readable without the old package. Original databases/artifact files are retained.
- CyUI moves to **6ef01cfcdb197f3a89d64618ce5ef8818c704728** for ChatUI compatibility,
  adding DocumentViews to the existing explicit module selection.
- The C++17 domain/host is preserved with isolated C++20 SDK and text-editor bridges.
  Offline CyDoc help remains complete in English and French.

Verification covers native chat/tool edit/refusal, partial streaming/cancellation,
history/context preservation, real CLI/MCP calls, cache invalidation, disk spooling,
complete chunk recovery, process interruption and durable output recovery.
Existing affected integration, portable assets, authoring, animation and UI module
cases run in isolated workspaces on Windows x64/MSVC/DX11.

The chat tests use an explicitly local API fixture. They do not establish successful
login or generation with a real provider account/model. Attachments use SDK transport
capabilities and filename previews; no new game-media attachment renderer is supplied.
No external MCP tools are enabled without an application need. No game codec,
engine compiler or Linux/macOS native host is added.

Read [integration and commands](CyMCPSDKIntegration.md), [UI ownership](CyUIMigration.md)
and [license notices](../LICENCES/README.md).

## Distribution

The Windows ZIP contains Editor/CLI, source/examples, notices and the complete manual.
No compilation or library checkout is required to launch native editing/chat/MCP.
Install the Microsoft C++ x64 runtime if needed. Python 3.11+ and app/setup_mcp.ps1
are optional for compute workers. No Python environment or model weights are bundled.
The documentation ZIP opens directly offline. SHA256SUMS.txt covers all three archives.

The source release includes the previous 0.30 typed Inspector, scene table/hierarchy,
CodeEditor, FilePicker and transactional node-group work as well as this SDK migration.
