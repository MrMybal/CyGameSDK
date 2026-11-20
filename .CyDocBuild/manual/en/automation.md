---
id: automation
title: Assistant, CLI and MCP
description: Use the integrated assistant and the native automation registry.
section: Getting started
order: 3
revision: 4
---

## Using this guide {#getting-started}

CyMCPSDK 0.7.1 supplies the native assistant, operation registry and MCP transports.
The editor keeps its actual validation, persistent documents and Undo. Discover a
command and inspect its schema before invoking it.

## Integrated assistant {#assistant}

Open **AI Assistant** from the module sidebar or Window menu. In **Connection settings**,
detect/select a provider, load its real model catalogue and connect. API keys can be
entered in the masked memory-only field or supplied through the named environment
variable. They are not saved. Installed CLI agents retain their account management.

Free is the default and authorizes all registered operations without an additional
write confirmation. Local controlled and Protected retain **Allow editor changes**.
Codex and compatible API transports receive tools directly; other CLI agents use their native MCP configuration.

Replies appear progressively in the common Markdown panel. **Stop** cancels a turn.
Errors and interrupted partial replies stay visible. The shared file picker adds
attachments subject to the provider/model's capabilities; the current preview shows
filenames rather than a separate asset renderer.

History is private. Choose a saved conversation and **Restore saved chat** to resume
without replaying a message. **Connect / New chat** preserves earlier saved histories.

## Access profiles {#access-profiles}

**Connection settings > Local MCP** displays the active profile and the profile for the next
start. Restart the editor after choosing another profile. The local MCP checkbox starts/stops HTTP immediately.

| Profile | Token | Registered operations |
| --- | --- | --- |
| Free | No | All authorized |
| Local controlled | No | Host read/write policy |
| Token only | Required | All authorized |
| Protected | Required | Host read/write policy |

New settings and configurations with no explicit profile migrate to **free**.
Existing profile choices and unrelated settings are retained. Old allowChanges is preserved
but is not an explicit profile choice. Validation remains enabled: invalid arguments,
locked objects and revision guards still fail. The setting is **access_profile** in
data/settings/automation.json. A CLI **--access-profile** override applies to one process.

## Native terminal commands {#cli}

~~~powershell
.\CyGameCLI.exe --help
.\CyGameCLI.exe list
.\CyGameCLI.exe describe-operation property_set
.\CyGameCLI.exe --root C:\Editors\MyGame invoke scene_list
.\CyGameCLI.exe --root C:\Editors\MyGame --allow-write invoke property_set --params-file change.json
~~~

UTF-8 change.json contains real editor arguments, for example:

~~~json
{"entityId":"hero","propertyId":"name","value":"Edited from CLI"}
~~~

Discovery, schema inspection, diagnostics and durable result recovery need no GUI.
Scene operations require an editor opened with the same **--root**. No hidden editor
starts automatically. Stdout contains JSON; diagnostics go to stderr. Exit codes:
0 success, 1 operation/transport failure, 2 invalid input, 3 denied operation.

Existing app/tool.py commands remain available. Its invoke retains job-shaped output;
the native CLI returns direct business output. Native jsonl/stdio use SDK requests,
while app/tool.py stdio preserves supported older protocol envelopes.

## Connect an external AI {#mcp}

Open AI Assistant and enable local MCP. Copy the MCP address and configure your client
with that URL only, for example http://127.0.0.1:PORT/mcp. Free uses an unauthenticated
loopback HTTP server and an allow-all registry. No token or extra operation confirmation is requested.

Protected and Token only still require a Bearer token. The private endpoint file
data/ipc/automation.json contains URLs, profile and requireToken; only token profiles
include a token. Disabling/closing the endpoint removes this file.

~~~powershell
.\CyGameCLI.exe --root C:\Editors\MyGame serve --port 8766
.\CyGameCLI.exe --root C:\Editors\MyGame --connect http://127.0.0.1:8766/v1 invoke scene_list
~~~

## Context, resources and privacy {#optimizations}

The compact catalogue, read TTL, change notifications, result retention/disk quota,
metadata-only performance files and API context window are independent settings.
Server/storage options apply when reopened; provider options apply on the next connection.

Long API conversations keep their complete saved history. Context projection preserves
system instructions, the active turn and complete tool exchanges. Native agents keep
their own context management. No automatic summary model is called.

Large results use private manifests/chunk URIs; follow firstUri and nextUri to recover
the complete content. Expiration never replays an action. Durable job artifacts remain
available through the [job tools](doc:jobs). Native grep searches exposed text snapshots;
no filesystem tree or ripgrep root is exposed. Other applications' tools are not attached
because the current editor has no required external MCP dependency.

## Source and JSON fields {#source-fields}

Operation parameters, class declarations, codec options and assistant provider JSON use
the shared code editor: syntax colors, text selection/search, local text Undo and zoom.
Field-size limits and explicit Execute/Apply/Connect actions remain. Editing text does
not execute a tool or write a game file. Generated HLSL and results stay read-only.

## Continue reading {#next}

- [Durable jobs and recovery](doc:jobs)
- [First editing session](doc:quickstart)
- [Operation reference](doc:operations)
- [Build and launch](doc:setup)
