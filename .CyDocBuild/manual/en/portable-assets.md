---
id: portable-assets
title: Portable assets and conversion profiles
description: Portable assets and conversion profiles
section: Assets
order: 24
revision: 1
parent: assets-3d
---

## Preserve identity {#identity}

`.cypack` v1 contains exactly two regular ZIP entries: `manifest.json` and `asset.glb`. The envelope has `format: cygame-portable-asset`, `schemaVersion: 1`, canonical UUID `assetId`, name/kind, space, payload bytes/SHA-256, inspected summary, provenance and namespaced extensions. GLB is self-contained glTF 2.0. The transport preserves its bytes and unknown dotted-lowercase metadata, not arbitrary game semantics.

Use a UUID once and preserve it across edits/transfers. The destination must explicitly resolve an existing UUID with different data; no global asset database, automatic conflict resolver or per-bone UUID map is provided. Provenance permits sourceGame/sourceFormat/sourceSha256/converterId, not automatically added absolute local paths. Coordinate conversion must transform geometry, winding, normals/tangents, bind matrices and animation channels; changing an axis label is not conversion.

## CLI and library {#cli}

Inspect the supplied original fixture:

```powershell
.\runtime\python\Scripts\python.exe app\portable_asset_cli.py inspect app\examples\portable\AnimatedTriangle.cypack
```

Use `portable_asset_cli.py pack SOURCE.glb NEW.cypack --name "My Asset"`, `export-glb SOURCE.cypack NEW.glb` or `plan SOURCE.cypack --profile PROFILE.json`. Destinations are explicit persistent files and are not overwritten. The exclusive publish step requires same-directory hard-link support. Stdout is UTF-8 JSON. Library functions are `package_glb`, `inspect_package`, `read_package`, `export_glb` and `ConversionRegistry.register/plan_export`.

A successful plan command may still say `blocked`. Unsupported optional extensions give `review-required` and explicit loss review. A declared game profile remains `provider-required`, `executionAvailable:false` until a real codec exists. The executable portable path is byte-preserving GLB export, not retargeting. Optional standalone `portable_pack/inspect/export_glb` handlers can be registered into the existing Core; they are not installed as those IDs in the native catalogue. Native integration uses `model_*` and import operations.

## Validation boundaries {#limits}

The standalone package permits 64 MiB GLB, 4 MiB GLB JSON, 128 KiB envelope, 80 MiB ZIP, 64 JSON nesting levels and at most 100,000 records per inspected declaration array. Only JSON then optional BIN and one embedded buffer are allowed. All `uri` properties, even data URIs, are rejected. Resources must be embedded bufferViews. Duplicate/extra/traversal ZIP entries, symlinks, encryption and unsupported compression fail.

Envelope integrity, bounds, framing and inspected shapes do not validate all accessors, vertices, rig relationships or glTF extensions. Reports say `semanticValidation: not-performed`. Use a full glTF validator and the target game's validator. Standalone transport limits differ from the stricter [native editable profile](doc:assets-3d#limits).

## Reference code examples {#code-examples}

Original source-guide examples. Paths, sample names and explanatory placeholders must be adapted; they are not a working codec for an arbitrary game.

```text
manifest.json
asset.glb
```

```json
{
  "format": "cygame-portable-asset",
  "schemaVersion": 1,
  "assetId": "ef7603e1-84f8-44b2-87da-a677412c462a",
  "name": "AnimatedTriangle",
  "kind": "scene3d",
  "space": {
    "handedness": "right", "upAxis": "+Y", "linearUnit": "meter",
    "rotation": "quaternion-xyzw", "timeUnit": "second"
  },
  "payload": {
    "path": "asset.glb", "mediaType": "model/gltf-binary",
    "bytes": "actual integer byte count", "sha256": "actual 64-character lowercase digest"
  },
  "summary": "actual inspected declaration summary",
  "extensions": {"org.example.game": {"opaqueSourceMetadata": {}}},
  "provenance": {"sourceGame": "Example", "sourceFormat": "native-format", "converterId": "example.import"}
}
```

```powershell
.\runtime\python\Scripts\python.exe app\portable_asset_cli.py inspect app\examples\portable\AnimatedTriangle.cypack
.\runtime\python\Scripts\python.exe app\portable_asset_cli.py pack <SOURCE.glb> <NEW_OUTPUT.cypack> --name "My Asset"
.\runtime\python\Scripts\python.exe app\portable_asset_cli.py export-glb <SOURCE.cypack> <NEW_OUTPUT.glb>
.\runtime\python\Scripts\python.exe app\portable_asset_cli.py plan app\examples\portable\AnimatedTriangle.cypack --profile app\examples\portable\Example_StaticOnlyConverter.json
```

```python
from portable_assets.service import OPERATION_IDS, register_operations
# In the existing create_runtime factory, before the runtime starts:
# 1. Declare the descriptors in the manifest.
# 2. Exclude OPERATION_IDS from the generic native-proxy registration loop.
# 3. Register these handlers instead.
register_operations(runtime, dedicated_shared_asset_input_directory)
```
