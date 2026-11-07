---
id: json
title: JSON documents and metadata
description: JSON documents and metadata
section: Assets
order: 25
revision: 1
parent: content-browser
---

## Open and edit {#edit}

Open a JSON asset in Content Browser. Expand objects/arrays, filter keys and edit supported scalars; add/remove entries through the tree actions. Document mutations validate first and participate in shared Undo/Redo. Save JSON is explicit and guards against external disk changes. Copy JSON is a presentation action, not a save.

The current profile is 512 KiB and four loaded JSON documents. Long strings are read-only in the GUI. Unknown fields in neutral asset contracts must be preserved, not stripped by a generic form. Closing a saved document releases it; keep unsaved edits before closing.

This view is not an arbitrary game configuration parser or schema migration engine. Read [portable metadata](doc:portable-assets) and the exact operation schema before applying automated edits.

```image
{"id": "json", "src": "assets/screenshots/json.png", "alt": "JSON tree and scalar editor", "controls": "minimal"}
```
