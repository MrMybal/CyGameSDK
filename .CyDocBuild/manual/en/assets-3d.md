---
id: assets-3d
title: Mesh, skeleton and animation editing
description: Mesh, skeleton and animation editing
section: Assets
order: 22
revision: 1
---

## Open and edit a neutral document {#edit}

Import a self-contained GLB/CyPack to a new `.cy3d`, then double-click it. Each loaded model gets a dockable editor. Tools > 3D Assets lists loaded documents. Closing a tab hides it; **Close document** releases a saved document and refuses unsaved changes. Right-drag orbits, the wheel zooms, Frame resets, and Wireframe/Skeleton change the real triangle preview.

Mesh selects mesh/primitive/vertex and edits position, normal, UV, joint slots and weights. Normal and weight sums normalize on edit; joint indices remain integers. Skeleton edits node name, parent and TRS. Cycles and zero quaternions fail atomically. Matrix nodes require decomposition before TRS editing; inverse-bind matrices are not automatically rebuilt after rig changes.

Animation selects a clip, plays/pauses/loops/scrubs, creates clips/TRS tracks and inserts/removes keys. Times increase strictly and tracks keep at least one key. Empty clips need a track before GLB export. LINEAR, STEP and CUBICSPLINE are evaluated; cubic tangents survive export but have no tangent-handle UI. All authoring uses document Undo. Save asset is explicit; export GLB/CyPack runs a Core job.

## LODs, collisions and retargeting {#authoring}

Generate static LODs through the bounded worker simplifier, then inspect the result. Geometry edits mark generated LODs stale and collision shapes for review. Exporters must honor those markers. Collision authoring uses primitive shapes, not a full game physics cooker. Reference-pose retargeting maps explicit bones/poses; it is not automatic humanoid inference. Keep source skin/bind data and inspect loss reports. [Motion tools](doc:animation-tools) add offline IK, root-motion pairs and clip resampling.

## Document and profile limits {#limits}

The JSON envelope is `format: cygame-scene3d`, `schemaVersion: 1`, with canonical `assetId` UUID, nodes, meshes, skins, materials, animations, extensions and source. Space is right-handed +Y up, metres, XYZW quaternions and seconds. Stable node IDs are `node:<source index>`; reparenting does not change array identity. `source.glbBase64` retains the original GLB; CyPack provenance and namespaced metadata are kept. Rebuilt streams append to original records, without promising that unknown glTF extensions remain valid after editing.

Import source: 32 MiB. Editable JSON: 2 MiB including source. Geometry and animation: 20,000 vertices/triangles and 20,000 keys, 512 nodes, 256 joints/skin, 128 clips. Up to eight models / 4 MiB compact loaded data; project serialization is bounded to 8 MiB. Required extensions, compressed/morph geometry and non-triangle primitive modes are rejected. This is a small asset profile, not a full DCC or game renderer.

```image
{"id": "mesh", "src": "assets/screenshots/mesh.png", "alt": "Neutral mesh editor and editable vertex data", "controls": "minimal"}
```

```image
{"id": "skeleton", "src": "assets/screenshots/skeleton.png", "alt": "Skeleton hierarchy and TRS editing", "controls": "minimal"}
```
