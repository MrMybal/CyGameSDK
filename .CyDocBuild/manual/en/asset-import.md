---
id: asset-import
title: Import and reimport sources
description: Import and reimport sources
section: Assets
order: 21
revision: 1
parent: content-browser
---

## Import explicitly {#import}

Choose **Content Browser > Add > Import source file**. Select an absolute source outside `data/assets`, a new content-relative destination and a registered provider. **Validate source** prepares the plan; **Import** starts an official Core job. `image.copy` preserves PNG/JPEG/BMP bytes and extension; `scene3d.glb` converts a self-contained GLB/CyPack to the bounded .cy3d profile. Neither is a game codec.

Inspection, current hashes, staged bytes and destination confinement are validated again at commit on the owner thread. The source is checked for size/mtime changes during reading. The configured SDK runtime must keep commit reports beneath the editor root. Import writes assets persistently, outside document Undo; query/import-view settings do not change the document revision.

## Reimport safely {#reimport}

Open Import settings from Asset actions. **Check source** compares SHA-256 hashes: up to date, changed source, local changes, missing source or missing asset. Reimport reuses the recorded provider/source and refuses to discard local destination edits. Preserve those edits first. Refresh is explicit; source relinking and filesystem watchers are not implemented.

`<asset>.cyimport` is bounded UTF-8 JSON (64 KiB) with provider, local absolute source, hashes, byte count and import time. It is local project metadata available to authorized editor clients, not a portable dependency. Import artifacts survive task cleanup; staging/reports follow Core job lifetime.

## Cancellation and recovery {#recovery}

Before commit, cancellation leaves the destination absent/unchanged. Once persistent commit starts, an interrupted or cancelled response can coexist with a completed import: inspect before retrying. Pair replacement is not a filesystem-wide atomic transaction. If rollback fails, keep `data/import-recovery/<id>/transaction.json` and its `a`/`m` backups. Close the editor before restoring the recorded asset and companion. Automatic crash recovery is not implemented. Sandbox document ZIP builds do not package imported files.

WIC limits: 32 MiB source, at most 8192 pixels per side and 4,194,304 decoded pixels. Neutral 3D has [its own profile](doc:assets-3d#limits).
