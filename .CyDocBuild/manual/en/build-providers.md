---
id: build-providers
title: Build targets and packaging
description: Build targets and packaging
section: Integration
order: 61
revision: 1
parent: game-adapters
---

## Choose a declared target {#targets}

Open Build and Package. The adapter's `buildTargets()` declares target ID, provider ID, platform, configurations and an `optionsSchema`. The default method returns no targets. A capability flag does not discover a compiler. The supplied `sandbox.document` target uses `sandbox.archive`, Development configuration, compression and includeMaterialValues options; it creates a document archive, not a game executable.

`build_describe` exposes targets. `build_prepare` validates target/configuration, fills declared option defaults, checks scene/loaded graph diagnostics and resolves material instances. The immutable plan includes a snapshot and signature; changes before launch require preparing again. Build options are data, never asset-supplied shell commands.

## Run and consume {#jobs}

The GUI uses its persistent SDK client and the existing official ProcessWorker. Registered Python source providers perform real discovery/compilation/packaging for a game. Pass a complete provider map when overriding defaults. Core owns progress, cancellation, private job paths and cleanup. Export temporary-task results before finishing; desktop results follow persistent session lifetime.

A successful Sandbox ZIP contains document data, not cooked content or imported file payloads. A real adapter must identify the actual compiler/platform SDK, produce the real format and validate it with the target game. Build/play buttons alone are not proof of support.

```image
{"id": "build", "src": "assets/screenshots/build.png", "alt": "Declared Sandbox document archive target", "controls": "minimal"}
```

## Reference code examples {#code-examples}

Original source-guide examples. Paths, sample names and explanatory placeholders must be adapted; they are not a working codec for an arbitrary game.

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
