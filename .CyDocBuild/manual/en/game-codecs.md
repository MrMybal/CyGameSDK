---
id: game-codecs
title: Trusted game codec providers
description: Trusted game codec providers
section: Integration
order: 62
revision: 1
parent: game-adapters
---

## An empty default registry {#registry}

No real game codec is bundled. `IGameAdapter::gameCodecs()` declares supported targets; executable `GameCodecProvider` registrations are trusted application source passed to `create_runtime(..., game_codecs=...)`. The Game Codecs panel lists them. Declaring a target without an executable provider does not implement conversion.

A descriptor supplies id/gameId, explicit nonempty supported versions, platforms, a simple extension, maxBytes (at most 64 MiB) and JSON options schema. Select the exact target/version/platform. The framework validates these before starting the official one-shot worker. It never loads executable code or a worker command from model JSON.

## Verified outputs and losses {#export}

`game_codec_export` snapshots the model immutably. The source provider converts it, validates real output and writes to owned output/report paths. A `cygame-codec-result` schema 1 report includes codec/game/version/platform, unchanged assetId, actual bytes/SHA-256, losses and validation evidence. Host checks enforce ownership, size, identity, target and bytes. They do not independently prove game semantics.

Explicitly acknowledge conversion losses and stale generated LOD/collision-review markers. `requireVerified` defaults to true; unverified output is rejected unless a caller deliberately selects false. This never overwrites source assets or installed game files. Export temporary Core results before finishing their task. To claim a supported game, keep real fixtures, check coordinates/rig/animation/material/LOD/collision data and prove a roundtrip plus actual game load or official validation.

## Reference code examples {#code-examples}

Original source-guide examples. Paths, sample names and explanatory placeholders must be adapted; they are not a working codec for an arbitrary game.

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
