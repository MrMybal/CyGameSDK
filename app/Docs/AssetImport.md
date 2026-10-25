# Asset import pipeline — 0.20.0

The editor ships `image.copy` for original PNG/JPEG/BMP and `scene3d.glb` for
self-contained GLB/CyPack to editable `.cy3d`. Source paths must be absolute and
outside `data/assets`; destinations are content-relative and cannot overwrite an
existing asset. Image copy keeps the extension/bytes; portable 3D decoding has the
bounded profile described in Native3DAssets.md. Neither provider is a game codec.

## Desktop workflow

Content Browser > Add > Import source file opens Asset Import. Browse for a source,
choose the destination and optionally select a registered importer. Validate
source checks the plan; Import starts an official CyToolsCore job. The panel shows
progress, errors and cancellation. Imported assets have a badge; PNG/JPEG/BMP tiles
show decoded thumbnails. Search `imported:true` or `imported:false` to filter them.

An imported asset's context menu offers Import settings and Reimport. Check source
compares SHA-256 hashes and reports Up to date, Source changed, Local changes,
Source missing or Asset missing. Reimport reuses the recorded source/provider and
refuses to discard locally edited destination bytes. Preserve those edits before
reimporting. Restore a missing source at its recorded path; source relinking and
filesystem watching are not implemented. Refresh and checks are explicit.

Import settings, texture views, queries and jobs leave document revision/Undo
unchanged. Filesystem imports are persistent user operations, outside document
Undo. Copy/move/Trash/restore transfers the `.cyimport` companion with the asset.
Companions are hidden in the Content Browser and cannot be directly operated on
through the normal asset file API. Image copies preserve the source association. A .cy3d copy instead becomes an
independent document with a fresh UUID and no reimport sidecar.

## Operations and lifetime

- `asset_importers`: registered native provider descriptors.
- `asset_import_prepare`: validates source, destination, provider and current hashes.
- `asset_import` / `asset_reimport`: long SDK/MCP operations with Core job progress.
- `asset_import_commit`: validates and installs an owned worker report.
- `asset_import_info` / `asset_import_check`: metadata and explicit source checks.
- `asset_import_view` / `asset_import_request` / `asset_import_status` /
  `asset_import_cancel`: desktop panel and its Core SDK client.
- `asset_browser_settings`: query, category, folder and tile/list presentation.

`import_service.py` registers the long operations in the existing CyToolsCore
runtime. `ImportProvider` uses the official `ProcessWorker`; the desktop reuses
`BuildClient` / `build_bridge.py` as a persistent SDK client. There is no additional
scheduler or MCP implementation. Core owns identities, job files and cancellation.

The worker copies/hashes chunks, detects size/mtime changes while reading, and
writes a staged file and report through `JobContext.path`. The native owner thread
checks confinement, destination/metadata hashes, staged bytes and provider
inspection before installation. GUI/default SDK job storage is beneath the editor
root. An SDK runtime placed outside that root cannot commit its reports.

Cancellation before commit keeps the destination absent or unchanged. Once a
persistent commit begins, cancellation or an interrupted reply can coexist with a
completed import: inspect the destination before retrying. Imported files and
metadata survive job/task cleanup. Staging and reports follow Core task lifetime.
The Sandbox document ZIP export does not yet package imported files.

## Metadata and rollback

`<asset>.cyimport` is UTF-8 JSON (`format: cygame-import`, `schemaVersion: 1`):
provider ID, absolute source path, source/imported SHA-256 hashes, source byte
count, import UTC time and inspected image information. The source path is local
project metadata, not a portable source dependency. It is returned to authorized
clients of this editor. Companions are bounded to 64 KiB.

Reimport compares both asset and metadata with the prepared snapshot to reject
intervening modifications. Before replacing files the editor creates
`data/import-recovery/<id>/transaction.json`, recording the destination; `a` and
`m` hold previous asset/metadata bytes. Short names avoid the previous nested job
backup path exceeding Windows path limits. Other Windows path limits still apply.
A successful commit or rollback removes its journal. If rollback itself fails,
the error identifies this persistent recovery folder, which survives job cleanup.
Keep its files and restore the recorded destination/companion manually with the
editor closed. Automatic recovery after a process/OS crash is not implemented;
the pair replacement is not a filesystem-wide atomic operation.

## Adding a trusted source provider

1. Register `ImportDefinition` with `EditorCore::registerImporter`: unique `id`,
   human `name`, lowercase `extensions`, `maxBytes` (at most 32 MiB), and an
   `inspect(path)` callback. It is called on both the source and staged result.
   Optional `outputExtensions` permits converted output; otherwise the destination
   retains the source extension. A converter's inspector must understand both.
2. Register a matching Python `ImportProvider(id, command)` using
   `create_runtime(..., import_providers=...)`. The supplied mapping replaces the
   defaults, so include `image.copy` and `scene3d.glb` if both should remain available. Desktop runtime
   construction must receive the same trusted registry when customized.
3. Implement the official one-shot ProcessWorker protocol. Read the owned plan,
   write staged bytes/report beneath the job workspace and return `reportPath`.
   See `import_worker.py` for `cygame-import-result` schema 1: original plan,
   staged path, source/imported hashes, source byte count and import UTC time.
4. Add provider-specific bounds, failure/cancellation checks and meaningful tests.
   Do not accept compiler commands, executable plugins or code from asset JSON.

Providers are trusted application source code, with the local user's permissions.
There is no binary plugin loader or sandbox for arbitrary importer code. The native
contract test registers a custom extension/output extension; it does not ship a
converter for game formats. Native game-mesh/audio/game-texture import belongs to game
adapters. Inspector failures occur before persistent mutation.

## Decode and thumbnail bounds

Image inspection and preview use WIC: at most 32 MiB per file, 8192 pixels per side
and 4,194,304 decoded pixels. Inspection and final decode checks are synchronous on
the owner thread and bounded; copy/hashing runs in the worker. This is not a fully
asynchronous image-decode pipeline.

`ThumbnailPreview` has a separate 64-entry LRU of 96 x 48 DX11 textures. It requests
only visible tiles and decodes at most one new image per frame; failed entries use
an icon until Refresh. CPU downsampling preserves aspect ratio. Retired GPU views
stay alive through the current ImGui frame. Refresh/import completion invalidates
the thumbnail cache. The full image editor retains its separate eight-entry cache.
Material texture sampling is described in MaterialPipeline.md. Batch and
drag/drop imports remain work.

## Portable 3D reimport

The worker decodes source into .cy3d and the native owner validates the entire
staged document before installation. GLB reimport preserves the imported UUID;
CyPack source identity must remain unchanged. Hash checks protect edited disk
assets; live unsaved model changes are checked at prepare and commit. After a
clean reimport, use Reload from disk in an already-open model tab. No watcher or
implicit replacement of live edits is installed.
