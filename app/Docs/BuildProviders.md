# Build providers — CyGameSDK 0.10

The build SPI separates native target validation, trusted source provider code and
CyToolsCore job ownership. The delivered Sandbox provider exports a document ZIP.
Game formats, compiler discovery, platform SDKs and engine builds belong to a game adapter.

## Native target declaration

Override `IGameAdapter::buildTargets()`; its default is an empty array. Each target
uses this structure (the example is the delivered target):

```json
{
  "id": "sandbox.document",
  "name": "Sandbox document archive",
  "providerId": "sandbox.archive",
  "platform": "Any",
  "configurations": ["Development"],
  "gameExecutable": false,
  "optionsSchema": {
    "type": "object",
    "properties": {
      "compression": {"type": "string", "enum": ["Stored", "Deflated"], "default": "Deflated"},
      "includeMaterialValues": {"type": "boolean", "default": true}
    },
    "additionalProperties": false
  }
}
```

`build_describe` exposes targets. `build_prepare` validates target/configuration,
defaults to the target’s first configuration, fills declared option defaults, validates options, checks the scene/loaded graph
diagnostics and resolves material instances. It returns `valid`, `diagnostics`,
`revision`, the normalized request and an immutable `snapshot`. It does not change
the document or start a worker. Snapshots are capped at 2 MiB; use an adapter-specific
asset pipeline for larger builds. `expectedRevision` rejects a stale request before
snapshot creation. Adapter scene validators may throw or return `valid:false`.

Target descriptors are trusted source code. Assets and request JSON cannot supply
executables or shell commands. Add validation support before using schema constructs
outside the native editor's supported subset. The native contract test injects a
second adapter and checks target absence, custom defaults/ranges and scene failures.

## Python source provider

`app/build_service.py` registers the `build_execute` handler with the official runtime.
A provider is a fixed argv command using CyToolsCore's `ProcessWorker`:

```python
from build_service import BuildProvider
import tool

provider = BuildProvider('mygame.export', [compiler_python, trusted_worker_script])
with tool.create_runtime(job_directory, build_providers={provider.id: provider}) as runtime:
    # Authenticate/create a session/submit using the normal Core SDK.
    ...
```

Supply a complete provider map when overriding `build_providers`; omitting it installs
`sandbox.archive`. The native target's `providerId` must match a registered key.
The desktop bridge uses the default map, so add a trusted default registration to
connect a new source provider to that interface. There is no external plugin discovery
or binary ABI here. Do not modify CyToolsCore to add a game provider.

The worker reads one JSON value from stdin, then writes one JSON result to stdout:

```json
{
  "protocolVersion": "1.0",
  "workspace": "<absolute job-owned directory>",
  "parameters": {
    "snapshotPath": "<job>/inputs/build-snapshot.json",
    "targetId": "sandbox.document",
    "configuration": "Development",
    "options": {"compression": "Deflated", "includeMaterialValues": true},
    "archivePath": "<job>/outputs/package.zip",
    "reportPath": "<job>/outputs/package-report.json"
  }
}
```

The handler allocates these paths through `JobContext.path`, writes a diagnostic plan
and snapshot, then calls `ProcessWorker.run(context, request)`. It never holds the
native editor thread while compiling. Validate paths against the supplied workspace;
write compiler logs to stderr, never extra stdout text. A provider may produce other
job-owned files through the same worker workspace contract.

`package_worker.py` is a complete one-shot example. Its result has `archivePath`,
`reportPath`, `snapshotRevision`, `bytes`, `sha256`, `entries`, `targetId`, `adapterId`
and `gameExecutable`. The current Build panel consumes this archive-result shape.
A real compiler with a different output contract needs a corresponding result view.
The current GUI title/action still describes packaging, rather than a generic IDE.

Core owns subprocess creation, cancellation, descendants and final cleanup. The
service reports progress at validation/snapshot/provider/completion boundaries;
it does not yet stream compiler output or granular progress from inside the worker.
Tests use a slow worker fixture to verify cancellation reaches the real process
and leaves no live worker. Parameter/diagnostic failures produce Failed jobs.

## Scope, ownership and editor client

The Sandbox archive contains:

- `document.cyproject.json`: a versioned snapshot of all currently loaded scene/graphs.
- `package.json`: format, revision, configuration and explicit `gameExecutable:false`.
- Optional `material-values.json`: evaluated material outputs at that revision.

External asset files are not copied. Parent/reference paths remain declarations in
the document; this ZIP is not a self-contained migrated project or a cooked game.
It does not compile arbitrary shaders, embed engine binaries or produce a game exe.

`build_request`, `build_status` and `build_cancel` control the desktop SDK client.
BuildClient launches the fixed `build_bridge.py` script with the private Python runtime,
without a shell or visible console. The bridge submits one normal Core job, polls
its state, and maps a cancel request onto `runtime.cancel`. It does not run compilers
itself or implement another scheduler. Only one desktop build is active at a time.
The native Operations-panel binding for build_execute also submits through this
client and opens the Build panel; the SDK handler returns the completed job output.

Transport metadata lives in `data/build-runs/<runId>`. Generated build artifacts live
under that run's `sdk/Workspaces/<clientId>/<jobId>` and are allocated by Core.
The status projection contains no client token. Desktop status is shared with the
editor session; do not treat it as an independently private client job. Standalone
SDK jobs retain normal per-identity access checks. Each desktop run owns a runtime;
use Core's existing authenticated service for a shared multi-client scheduler.

The desktop session uses `temporary=False`, so archives remain after window/runtime
closure. Closing the editor requests cancellation of an active job; the UI does not
claim it stopped before Core completes the cancellation. GUI history/recovery and
artifact cleanup controls remain future work. The private Python/Core runtime must
be installed to submit exports, although the native editor can open without Python.

For MCP, call `op_build_execute`, poll `cy_get_job`, consume or explicitly preserve
outputs with `cy_export_job`, then finish the task with `cy_finish_task`. Temporary
task closure removes unexported artifacts according to Core. See TaskFiles.md.
