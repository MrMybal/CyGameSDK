---
id: materials
title: Materials and compiled previews
description: Materials and compiled previews
section: Materials
order: 30
revision: 1
---

## Build a surface {#nodes}

Open a material graph. Connect supported scalar/vector/color operations, parameters, UV U/V, UV transforms, checker/gradient, compose color, sine/fract/saturate and texture samplers to the material output. Pure functions carry the UV evaluation context. Unused UV branches are not evaluated merely to sample another output. A failed center sample does not by itself prevent compiling a valid GPU program.

Material instances keep typed overrides separate from parent topology. Resolve loaded parents first, preserve parent state and check path confinement, types and cycles. The supported parent chain is bounded to eight links; refresh from disk is manual.

## Compile and inspect {#compile}

Choose **Compile preview** with the configured Python/Core environment. Native plans and Python lowering agree on parameter order and hashes. The official ProcessWorker generates/compiles HLSL to DXBC, then a checked report installs the shader in the owning DX11 view. Sphere/plane previews and live uniforms use that shader. Graph root defaults are uniforms; defaults inside functions are program data.

Progress, diagnostics and cancellation belong to the Core job. A stale signature, mismatched checksum, missing output or unsupported expression rejects installation. GPU objects retain what they need after job files are cleaned. Do not submit asset-provided HLSL, arbitrary includes or executable commands as a compiler request. Presentation edits such as panning do not invalidate a shader.

## Image sampling and boundaries {#sampling}

PNG/JPEG/BMP sampling uses bounded decoded images, sRGB conversion, bilinear filtering and repeat addressing. Alpha stays linear. Up to eight bindings / 64 MiB are supported, with literal confined paths. The result is an editor preview, not a game material codec/exporter or an engine shader compiler.

```image
{"id": "materials", "src": "assets/screenshots/materials.png", "alt": "Texture sampler feeding the compiled DX11 material", "controls": "minimal"}
```
