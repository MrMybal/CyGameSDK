# Trusted game codec providers — 0.21.0

A game codec decodes/encodes an actual game format and verifies its output against
that game's exact version/platform. This foundation ships no such game codec.
The executable source registry defaults to empty. The test-only CYSDKTEST worker
under tests is a synthetic ownership/roundtrip fixture, not a supported game.

## Source integration

Derive a separate SDK at the destination authorized by the user. Follow
GameAdapterGuide.md and complete GamePortingTemplate.md with real files, hashes,
format/toolchain knowledge and observed target validation. Do not modify installed
game files or infer codecs from extension/profile declarations.

- Import uses the existing native ImportDefinition and matching trusted
  ImportProvider/ProcessWorker. Register the real source extensions, bounded decode
  and .cy3d inspector/commit path; see AssetImport.md. Codec commands are source
  configuration, never executable instructions loaded from an asset/project JSON.
- Export uses game_codecs.GameCodecProvider(descriptor, trusted_worker_command).
  Supply the mapping to tool.create_runtime(game_codecs=...) or implement the
  source default_codecs factory for the derived desktop build. Native adapters may
  declare assetCodecs() for discovery; declarations alone do not enable a provider.
- The Game Codecs panel refreshes the executable registry through the existing
  desktop SDK client, lets the user choose a loaded model/provider/version/platform,
  accepts typed options JSON and explicit conversion-loss acknowledgements, and
  starts/cancels an official Core export job. No DLL loader or second MCP server
  is introduced. Only registered source workers execute.

A descriptor contains id, gameId, versions (exact nonempty list), platforms,
extension (simple suffix), maxBytes (at most 64 MiB) and optionsSchema (Draft 2020-12).
The framework validates requested target and options before launching the worker.

## Worker protocol

Use the existing Core one-shot ProcessWorker protocol. Parameters contain owned
snapshot/output/report paths, codecId/gameId/gameVersion/platform and options.
Read the neutral snapshot, perform the real conversion and real validation, write
only through those job paths and return outputPath/reportPath. A report contains:

```json
{
  "format":"cygame-codec-result", "schemaVersion":1,
  "codecId":"your.codec", "gameId":"actual-game",
  "gameVersion":"exact-version", "platform":"Win64",
  "assetId":"same-canonical-UUID-as-snapshot",
  "sha256":"actual-output-digest", "bytes":123,
  "losses":[],
  "validation":{"status":"verified", "method":"actual validator/roundtrip/game-load evidence"}
}
```

The values above are explanatory placeholders. The host checks ownership, size,
identity, target and SHA-256 against the actual bytes. It requires bounded explicit
losses and rejects unacknowledged ones. Stale generated LODs or collision review
markers also require explicit review. requireVerified defaults to true; unverified
output is refused unless the caller explicitly selects false. Generic host checks
do not establish semantic game compatibility: the trusted source provider must
perform and document real validation. A provider's boolean claim is not independent
proof, and native capability flags must not be used as substitute evidence.

## SDK / MCP

- game_codec_describe: native adapter declarations; no execution guarantee.
- game_codec_list: executable source registry through the official Core runtime.
- game_codec_export(modelId, codecId, gameVersion, platform, options,
  acceptedLosses, requireVerified): immutable model snapshot to private Core job.

Core owns identities, private paths, progress, cancellation, workers and cleanup.
Export job outputs using export_job before finish_task if they must survive.
This operation never overwrites source assets or installed game files. Desktop
sessions are persistent; their displayed output paths remain beneath data/build-runs.
The existing build-provider seam can consume a verified game codec's output when
the game's real toolchain is implemented.

## Verification before claiming a game supported

Record representative real input/output fixtures and exact versions. Check geometry,
coordinates, bones/bind poses, animations, materials/LODs/collisions and all unknown
metadata relevant to that format. Report unsupported features/losses, prove a real
roundtrip and load the output in the game or its official validator. Add regression
fixtures and tests at the derived destination. The reusable foundation's synthetic
worker/privacy/cancellation tests do not prove any real game codec.
