---
id: content-browser
title: Files, assets and references
description: Files, assets and references
section: Assets
order: 20
revision: 1
---

## Browse and filter {#browse}

Content Browser reflects the real `data/assets` filesystem. Navigate folders with the tree/breadcrumbs, switch grid/list, filter by type or fields and keep favorites. The asset editor registry chooses a view by extension; a recognized extension is not evidence that a game format is decoded. Common CyUI rendering does not expose every custom context action: use **Asset actions** for extended workflows, and selected-item metadata for type and size.

Search `imported:true` or `imported:false` for import associations. Thumbnail decoding is bounded. Hidden `.cyimport` companions cannot be operated on directly through the ordinary asset API.

## File lifecycle {#files}

Create folders, copy, rename/move, Trash and restore through the editor. Paths are content-relative and confined to the content root. Overwrite, traversal and dirty-document protections apply. References can prevent a move or deletion; inspect consumers and save modified references first. Filesystem imports and moves are persistent file operations, distinct from document Undo.

Image copies preserve the import association and companions move with assets. A copied .cy3d becomes a separate document with a fresh UUID and no reimport sidecar. Never copy a local runtime or settings tree as though it were an asset.

## Reference inspection {#references}

Reference Viewer combines saved and live declarations and shows direct/transitive dependencies and consumers, missing targets and cycles. The on-demand scan is bounded and reports incomplete results. A missing reference is not silently repaired. It indexes declared .cyasset relationships; it does not infer dependencies in unknown binary game formats.

[Import and reimport](doc:asset-import) · [Portable identity](doc:portable-assets)
