---
id: images
title: Image inspection
description: Image inspection
section: Materials
order: 31
revision: 1
parent: materials
---

## Inspect without painting {#view}

Double-click a supported PNG, JPEG or BMP. The WIC-backed view shows metadata, pixels and channels. Use RGBA/RGB or individual channels, **Fit image**, **100%**, wheel zoom and middle-drag pan. RGB ignores alpha; individual channels are grayscale with opaque alpha. Reload explicitly to read changed bytes.

Fit mode follows the available canvas size. Wheel zoom/pan leaves fit mode; zoom remains between 5% and 800%. View settings are presentation, not document Undo entries. There is no pixel-painting editor or native game texture decoder.

The decoder confines paths to the content root and checks 32 MiB/file, 8192 pixels/side and 4,194,304 decoded pixels. The image cache is bounded to eight entries. Importing a texture is an [explicit source operation](doc:asset-import), distinct from viewing it.

```image
{"id": "texture", "src": "assets/screenshots/texture.png", "alt": "Image channels and checkerboard preview", "controls": "minimal"}
```
