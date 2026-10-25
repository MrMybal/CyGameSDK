# Game SDK integration dossier — template

Copy into the derived game's documentation, for example `Docs/MyGameSDK.md`.
Replace placeholders with evidence. Keep UNKNOWN explicit. Mark each capability
as implemented/tested/planned/unavailable; a requested feature is not verification.
Read [GameAdapterGuide.md](GameAdapterGuide.md) before filling the implementation plan.

## Identity and first deliverable

- Game / edition / exact build: <UNKNOWN>
- Platform / architecture: <UNKNOWN>
- Editor product / stable adapter ID / adapter version: <UNKNOWN>
- Destination project root authorized by the user: <UNKNOWN>
- Foundation version / source archive or revision / dependencies: <UNKNOWN>
- First end-to-end user workflow and acceptance criterion: <UNKNOWN>
- Actual output expected by the game or validator: <UNKNOWN>

## Evidence and fixtures

| Question | Observed answer | Source / fixture / reproduction | Confidence |
|---|---|---|---|
| Level and asset formats / versions | UNKNOWN | UNKNOWN | UNKNOWN |
| Format/compression/endianness | UNKNOWN | UNKNOWN | UNKNOWN |
| Entities / component/property metadata | UNKNOWN | UNKNOWN | UNKNOWN |
| Coordinate axes / handedness / units / rotations | UNKNOWN | UNKNOWN | UNKNOWN |
| Asset IDs, paths, dependencies and archives | UNKNOWN | UNKNOWN | UNKNOWN |
| Game scripting and material semantics | UNKNOWN | UNKNOWN | UNKNOWN |
| Available compiler/exporter/validator | UNKNOWN | UNKNOWN | UNKNOWN |

List the smallest known-good fixture, its hash, expected decoded values and a
repeatable validation of output. Distinguish facts observed in files from inference.
Document redistribution/licensing constraints for game fixtures and dependencies.

## Capability plan

| Feature | Current state | Implementation entry point | Verification / remaining work |
|---|---|---|---|
| Scene load and validation | UNKNOWN | UNKNOWN | UNKNOWN |
| Entity creation / property editing | UNKNOWN | UNKNOWN | UNKNOWN |
| Level/asset serialization and migration | UNKNOWN | UNKNOWN | UNKNOWN |
| Asset browser / specialized editors / dependencies | UNKNOWN | UNKNOWN | UNKNOWN |
| Viewport rendering / picking / coordinate conversion | UNKNOWN | UNKNOWN | UNKNOWN |
| Gizmos / Undo / multi-viewport behavior | UNKNOWN | UNKNOWN | UNKNOWN |
| Game graph/runtime bindings | UNKNOWN | UNKNOWN | UNKNOWN |
| Native material lowering / compilation | UNKNOWN | UNKNOWN | UNKNOWN |
| Build / package / play | UNKNOWN | UNKNOWN | UNKNOWN |
| MCP/SDK operation parity | UNKNOWN | UNKNOWN | UNKNOWN |
| English default / selectable translations | UNKNOWN | UNKNOWN | UNKNOWN |

## Integration decisions

- Internal editor document versus native game data: <UNKNOWN>
- Portable format adoption / glTF profile / UUID conflict policy: <UNKNOWN>
- Rig correspondence / retargeting / root-motion and conversion-loss policy: <UNKNOWN>
- Namespaces for preserved engine-specific metadata: <UNKNOWN>
- Source assets / editable documents / generated output directories: <UNKNOWN>
- Preservation of originals and unknown format fields: <UNKNOWN>
- Entity-ID/type registry and property schema mapping: <UNKNOWN>
- Supported native schema constraints and additional validation: <UNKNOWN>
- Current Sandbox assumptions retained / generalized / intentionally removed: <UNKNOWN>
- Project save path / adapter identity checks / schema migrations: <UNKNOWN>
- Custom graph types, semantics and unsupported expressions: <UNKNOWN>
- Renderer API, device/texture ownership and input/picking bridge: <UNKNOWN>
- Asset dependencies and move/rename policy: <UNKNOWN>
- Trusted provider IDs, fixed toolchain argv and output contracts: <UNKNOWN>
- Atomic mutation / document Undo / persistent file-operation boundaries: <UNKNOWN>
- Resource budgets, input limits, cancellation and recovery policy: <UNKNOWN>

## AI operation catalogue

For every added operation record: stable ID, purpose, exact input/output schema,
read-only or mutation, expectedRevision policy, required permissions, long/short
execution path, worker/provider, native handler, GUI entry point, progress,
cancellation, error codes, output ownership and validation example.

| Operation ID | Native / worker implementation | GUI entry point | Verification |
|---|---|---|---|
| <UNKNOWN> | <UNKNOWN> | <UNKNOWN> | <UNKNOWN> |

## Build and setup

- Exact supported toolchain versions and installation sources: <UNKNOWN>
- Final dependency/runtime paths and setup command: <UNKNOWN>
- Clean build / manifest generation command: <UNKNOWN>
- Native and isolated graphical integration commands: <UNKNOWN>
- MCP client configuration / transport / lifecycle: <UNKNOWN>
- Game output validation command or reproducible manual procedure: <UNKNOWN>
- Reproducible sample import/edit/export workflow: <UNKNOWN>

## Acceptance evidence and handoff

- Fixture decode and native-game roundtrip results: <NOT RUN>
- Invalid/unsupported/corrupt input and path/size bounds: <NOT RUN>
- Coordinate/render/picking/gizmo consistency: <NOT RUN>
- Atomic failure, Undo/Redo and stale-revision rejection: <NOT RUN>
- Actual compiler/exporter output and game/validator result: <NOT RUN>
- Worker cancellation and artifact/task lifetime: <NOT RUN>
- GUI and MCP/SDK reach the same document/provider path: <NOT RUN>
- English default, chosen translations, layout/theme verification: <NOT RUN>
- Build ID, executable hash, evidence paths and test timestamps: <UNKNOWN>
- Known limitations and next milestone: <UNKNOWN>

Do not replace NOT RUN with passed until that verification has actually completed.

## Codec and advanced authoring dossier

- Trusted provider ID, registration source and adapter declaration: <UNKNOWN>
- Exact game/version/platform/extension and bounded options schema: <UNKNOWN>
- Fixture provenance, file hashes, decoder and verified roundtrip: <NOT RUN>
- Target validator/game command and observed output: <NOT RUN>
- Explicit losses, unsupported metadata and accepted-loss policy: <UNKNOWN>
- Rig mapping/rest poses/units/axes, IK/contact/root-motion requirements: <UNKNOWN>
- LOD screen-distance policy, collision cooking and stale-derived-data checks: <UNKNOWN>
- Engine object/event/time bindings versus local graph runtime: <UNKNOWN>
