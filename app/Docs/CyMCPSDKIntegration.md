# CyMCPSDK integration

CyGameSDK 0.32 replaces the previous automation runtime with **CyMCPSDK 0.7.1**,
built from local source revision **34d4bcae126bc83d1a2925b5047dac36e3d09d92**.
The lock also pins CyUI to **6ef01cfcdb197f3a89d64618ce5ef8818c704728**, required
by ChatUI's Markdown contract. The native host remains Windows x64 / DX11.

## Ownership and entry points

| Entry point | Behavior |
| --- | --- |
| Native editor | Existing EditorCore validation, owner-thread commands, persistence and Undo |
| AI Assistant | SDK ChatUI/ChatController and the shared native operation Registry |
| CyGameCLI.exe | Native discovery, schemas, direct JSON invocation, JSONL and MCP |
| MCP stdio | SDK McpServer, compact discovery by default; full catalogue with --full |
| Loopback /mcp and /v1 | Profile-selected MCP/JSON endpoints; one local host identity per endpoint |
| Existing app/tool.py | Compatibility client delegating execution to the compiled companion |
| Python SDK helpers | Thin native-host client, with no Python MCP server or scheduler |
| Compute jobs | Existing trusted computations under SDK Process ownership, with durable host records |

The 177 business operations retain their IDs and native handlers. Ten service operations
cover SDK information and jobs/results. The 177 **op_<id>** compatibility aliases submit
asynchronous native jobs using **{parameters: {...}}**. The old **cy_get_job**,
**cy_list_operations** and **cy_finish_task** are among the ten service operations.
The full registry contains 364 tools; compact clients see five discovery/resource tools.

All three native entry points expose the same 177 declared business operations.
Widget focus/dock coordinates and transient ImGui gestures are not business operations.
Provider credentials and trusted executable/provider registration remain host configuration,
not model-callable tools. Source-specific portable_* handlers are opt-in with their own
catalogue; no missing codec or engine operation is synthesized by the registry.

Handlers reach the same dispatcher through its owner-only Windows pipe. Providers,
HTTP threads and workers never mutate ImGui contexts. Core keeps data validation,
persistent edits and Undo. Long computations retain snapshot/hash-checked commit boundaries.

The SDK schema profile omits JSON Schema pattern. The host retains and validates each
original pattern and describes it in the exposed schema. The optional portable UUID field
is also checked. Graph-run alternatives retain their constraints under an object root.

## Modules and build

Core supplies policies/resources; Platform owns child processes; Http supplies provider
and loopback transports; MCP supplies server/client protocols; AI supplies conversations;
ChatUI supplies the common native panel. Local engine/download modules, SDK examples,
benchmarks and SDK test applications are disabled. Running Ollama/llama.cpp/LM Studio
services remain usable through their actual AI provider catalogues.

Private C++20 targets isolate the SDK and assistant from the existing C++17 editor/domain.
SDK JSON types do not cross that boundary. DocumentViews is the new CyUI module;
every component uses the same host-owned ImGui target.

~~~powershell
.\app\build.ps1
.\app\build.ps1 -CyUIRoot C:\src\CyUI -CyMCPSDKRoot C:\src\CyMCPSDK
.\app\setup_mcp.ps1
~~~

The integrated commit is copied from the local SDK Git objects into a private clean checkout
when the sibling is changing. No sibling files are modified. CMake prefers local sibling
SDK source, checks its exact revision and clean state,
and otherwise fetches the lock. Do not modify a sibling library.
The Python setup installs only compute-worker dependencies. Basic scene operations,
CLI/MCP discovery and conversations do not need the former runtime package.

## Native CLI

~~~powershell
.\CyGameCLI.exe --help
.\CyGameCLI.exe list
.\CyGameCLI.exe describe-operation property_set
.\CyGameCLI.exe --root C:\Editors\MyGame invoke scene_list
.\CyGameCLI.exe --root C:\Editors\MyGame --allow-write invoke property_set --params-file change.json
~~~

UTF-8 change.json:

~~~json
{"entityId":"hero","propertyId":"name","value":"Edited from CLI"}
~~~

Discovery, schemas, diagnostics and durable recovery work without a GUI. Live document
operations require an editor opened with the same --root. No hidden editor is launched.
Selecting another root selects another instance.

Direct invoke writes the business JSON to stdout and diagnostic JSON to stderr.
Exit codes: 0 success; 1 operation/transport failure; 2 invalid input/usage; 3 denied.
Timeout/cancellation does not prove that a committed edit was rolled back. Inspect live
state and existing outputs before deciding to invoke again.

Native jsonl/stdio accept SDK objects such as:

~~~json
{"method":"invoke","name":"scene_list","arguments":{}}
~~~

Each response contains result or error. Existing app/tool.py command names/options remain.
Its invoke preserves job-shaped output; its stdio preserves supported protocol-1.0 envelopes.
CYGAME_CLI_EXE selects a candidate companion. Source-owned provider factories remain
available to the compatibility Python helper. Commands never come from asset JSON.

## Permissions and MCP

**Free is the default** for new configurations and existing settings without an
explicit access profile. The native Registry uses **Policy::allow_all()** and HTTP
uses **LocalHttpServer::UnauthenticatedLoopback** bound to **127.0.0.1**.
Clients connect with the MCP URL only and can call all registered operations without
a token or an additional write confirmation. Argument/schema and business validation,
locked objects, exact migration hashes and document Undo remain authoritative.

| Setting **access_profile** | HTTP token | Operation policy |
| --- | --- | --- |
| free | No | All registered operations |
| local_controlled | No | Existing read/write policy |
| token_only | Yes | All registered operations |
| protected | Yes | Existing read/write policy |

The enum values come from **LocalAccessProfile** and **local_access_settings**;
**protected_access** is also accepted as an ID. Existing explicit selections are retained.
Migration inserts **free** only when no profile exists; **allowChanges** is retained and
does not count as a profile selection. Unknown settings remain intact. Invalid/conflicting
profile settings fail explicitly instead of falling back to Free. The compatibility keys
**accessProfile** and **localAccessProfile** are recognized.

In **AI Assistant > Connection settings > Local MCP**, choose the profile for the next
editor start. The current active profile is displayed separately. Restart the editor to
apply a different profile. **Enable local MCP** starts/stops the HTTP endpoint immediately;
it is enabled by default when the assistant is opened. Chat and CLI use the same registry
policy independently of whether HTTP is enabled.

Controlled profiles retain **Allow editor changes** and local **--allow-write**.
A local grant cannot bypass the policy of a remote endpoint.
Free/Token only do not require or display the additional write checkbox.

~~~powershell
.\CyGameCLI.exe --root C:\Editors\MyGame mcp
.\CyGameCLI.exe --root C:\Editors\MyGame serve --port 8766
.\CyGameCLI.exe --root C:\Editors\MyGame --connect http://127.0.0.1:8766/v1 invoke scene_list
.\CyGameCLI.exe --root C:\Editors\MyGame --access-profile protected serve --port 8766 --token-env CYGAME_MCP_TOKEN
~~~

The CLI override applies only to that process and does not overwrite the saved selection.
Protected/Token only require a private token of at least 32 characters. A native HTTP
client sends a token only when supplied; Protected still rejects a missing/incorrect token.

Use [mcp.example.json](../mcp.example.json) for stdio,
[mcp-http.example.json](../mcp-http.example.json) for address-only HTTP, or
[mcp-http.protected.example.json](../mcp-http.protected.example.json) for token profiles.

The endpoint record **data/ipc/automation.json** contains URL, profile and token requirement.
Free/Local controlled store no token there; token profiles add a random private token.
The record is removed when HTTP stops or the editor closes. The UI copies the MCP URL
without adding credentials. Stdio and the editor pipe retain their OS-local transport;
the pipe remains restricted to the owning Windows account.

SDK stdio and loopback JSON/finite-SSE responses are used. Journal subscriptions require
a compatible modern client/transport. Older negotiated clients retain TTL and explicit
refresh. Capability claims follow the locked source, rather than older generic guides.

## Assistant

Choose Connection settings, detect/select a provider, load its actual model catalogue
and connect. Keys use the environment or masked in-memory field and are never saved.
Native agents retain their account management. Detection does not establish authentication.

Codex and compatible API transports receive the app registry directly. Other CLI agents
use their own MCP configuration. Native-agent permissions remain separate from editor
operation permissions; unrestricted native-agent access is not enabled.

The common Markdown panel streams replies, exposes errors and cancels active turns.
The shared file picker adds explicit attachments subject to provider/model limits.
Preview presentation remains the SDK's default filenames, without a new asset renderer.

Private history lives in **data/automation/conversations**. Select and restore a saved
chat without replaying a turn. New chats retain earlier history files. API context budgets
are separate KiB/byte settings; complete history remains stored. System instructions,
active turns and complete tool exchanges remain together. Native agents retain their
own context management. No automatic summarizer is enabled.

## Durable jobs and legacy data

The application owns **data/automation/jobs**. Compatibility clients may use their existing
--data-dir. Owned worker execution is serialized per project. Cancellation is cooperative,
then stops the SDK-owned tree when necessary. Private job status exposes actual worker
PIDs; performance files do not contain them.

After an unexpected owner exit, recovery marks unfinished work **Interrupted**, checking
process creation identity to avoid PID reuse. Recovery never restarts an action.

| Operation | Purpose |
| --- | --- |
| job_list | Filtered pages; follow nextOffset |
| job_get | Recover state and output locations after restart |
| job_cancel | Cancel an owned job |
| job_result_read | Bounded UTF-8-safe durable JSON |
| job_output_read | Base64 file chunks across CLI processes |
| job_output_open | Progressive file production into a private SDK resource |

Original artifacts outlive SDK resource TTL and task completion. Finish-task no longer
deletes outputs implicitly. Explicit export is supported; archive/delete deliberately
when results are no longer needed. Partial files after failure/interruption are diagnostic
artifacts, not proof of valid export.

The compatibility loader reads old SQLite records without the former package, preserving
token hashes, ownership, outputs and recipe formats. The source database is never rewritten.

~~~powershell
.\runtime\python\Scripts\python.exe app\legacy_jobs.py --source data\jobs --destination data\automation\jobs
~~~

Original owners remain by default. **--owner local** explicitly assigns records to the
single local identity. Preserve original artifact folders until a deliberate export has
copied the complete outputs.

## Independent optimizations

Private **data/settings/automation.json** supports independent options:

| Setting | Default |
| --- | --- |
| compactDiscovery | true; five-tool discovery |
| cacheTtlMs | 30000 |
| notifications | true, when negotiated |
| inlineResultBytes | 16384 |
| resultTtlMs | 300000 |
| resultMemoryBytes | 64 MiB |
| diskSpooling | true |
| spoolThresholdBytes / spoolQuotaBytes | 1 MiB / 1 GiB |
| performance | false; private metadata-only JSONL |
| contextWindow / contextKiB | false / 256 |
| saveHistory | true |

Server/storage options apply when the host reopens; provider options apply on the next
connection. Document/selection revisions refresh the scene resource. Closing the editor
removes it. Notifications invalidate compatible caches before TTL; explicit refresh remains.
Mutations are never cached or replayed.

Native grep searches exposed text resources. No filesystem tree or ripgrep root is exposed.
Existing asset/graph filters and job pagination avoid unnecessary bulk reads.
External application tools are not enabled: current workflows have no required external
MCP dependency. Any future delegation must use McpClient with an explicit host allowlist.

## Notices and verification

Retain SDK MIT, nlohmann/json 3.12, cpp-httplib, PicoSHA2, CyUI/MD4C/font notices.
The previous package is not a production/build dependency. Retained older copyright
notices document scaffolding provenance.

Tests use isolated roots and local API fixtures. They validate integration, not an actual
model, provider account or paid API. Windows/MSVC/DX11 is the tested platform.
Linux/macOS native hosts remain unavailable. Private reports stay outside the human manual.
