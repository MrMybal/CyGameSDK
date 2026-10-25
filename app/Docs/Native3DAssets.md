# Native 3D and JSON assets — 0.22.0

The reusable editor now has real neutral 3D documents and editor previews. These
are deliberately bounded authoring tools, not a game renderer or a complete DCC.
Read CommonAssetFormat.md first for the portable identity/metadata contract.

## Desktop workflow

1. Content Browser > Add > Import source file. Choose a self-contained GLB or
   `.cypack`, destination `Imported/Name.cy3d`, Validate source, then Import.
   The official CyToolsCore worker decodes it; native commit validates the result.
2. Double-click the `.cy3d`. Each document gets a separate dockable editor tab.
   Tools > 3D Assets lists loaded documents. Closing a tab hides the view; Close
   document releases a saved document. Unsaved document changes block release.
3. Right-drag orbits, wheel zooms, Frame resets the camera; Wireframe and Skeleton
   control presentation. The DX11 preview draws real triangles, with depth and
   simple lighting, and the CPU pose evaluator supplies skinned vertices.
4. Mesh: choose mesh/primitive/vertex; change position, normal, UV, joint slots or
   weights. Normal vectors and weight sums are normalized on edit; joint indices
   retain integer types. Property gestures participate in the shared document Undo/Redo.
5. Skeleton: select a node, edit name/parent/TRS. Cycles and zero quaternions fail
   atomically. Matrix nodes require decomposition before TRS editing; the editor
   does not automatically rebuild inverse-bind matrices after rig changes.
6. Animation: select a clip, play/pause/loop/scrub, select tracks and keys, change
   key times/values. New clip, TRS track creation and key insertion/removal work.
   Tracks keep at least one key; times increase strictly. A new empty clip must
   acquire a track before GLB export. LINEAR, STEP and CUBICSPLINE are evaluated;
   cubic tangents survive export, but this view does not provide tangent handles.
7. Save asset persists the neutral document. Export GLB / Export CyPack starts an
   official Core job. SDK clients must export job outputs before finish_task;
   the desktop SDK session is persistent. Reimport refuses local/unsaved changes.

Example assets are under `data/assets/Examples3D`: StaticTriangle.cy3d,
AnimatedTriangle.cy3d, Example_Data.json and Example_TexturedMaterial.cyasset.
These are original small fixtures, not evidence of compatibility with a real game.

## Editable document contract

`.cy3d` is JSON: `format: cygame-scene3d`, `schemaVersion: 1`, canonical UUID
`assetId`, name, fixed `space`, nodes, meshes/primitives, skins, materials,
animations, extensions and source. Coordinates are right-handed, +Y up, metres;
rotations are XYZW quaternions and animation time is seconds. Node IDs are stable
`node:<source index>`; reparenting changes parent indices, not array identity.
Primitive identity records source mesh/primitive indices. Preserve these IDs.

`source.glbBase64` retains the original self-contained GLB. When imported from a
CyPack, source.manifest and namespaced metadata preserve provenance and identity.
Exports append rebuilt edited streams/TRS/animation accessors to the original
container and retain other original records. This is not a promise that arbitrary
unknown glTF extensions remain semantically valid after edits. Required extensions,
compressed/morph geometry and non-triangle primitive modes are rejected by this
profile. Inspect/exported data and validate each game conversion independently.
No file-provided code, shell command or custom shader is executed.

Bounds: 32 MiB import source; editable JSON 2 MiB including embedded source;
20,000 vertices/triangles and 20,000 animation keys; 512 nodes, 256 joints per skin,
128 clips. At most eight models / 4 MiB compact loaded documents. Project JSON
serialization remains bounded to 8 MiB. These limits make this an initial small
asset authoring profile, not a pipeline for unrestricted production meshes.

## Native SDK / MCP operations

All descriptors live in next_contracts.py / CyTool.json; handlers use the existing
EditorCore dispatcher, manifest validation, revision checks and shared Undo.

| Operations | Behavior |
|---|---|
| model_open / model_list / model_get | Load confined content assets; enumerate/inspect neutral documents |
| model_node_set / model_vertex_set | Atomic validated hierarchy/TRS/stream edits |
| model_clip_create / model_track_add | Create/copy clips and initial TRS tracks |
| model_key_set / model_key_add / model_key_remove | Edit authoring keyframes |
| model_sample | Evaluate pose and first 1,024 preview vertices; reports full triangle count and bounds |
| model_save / model_reload / model_close | Explicit disk lifecycle; detect external changes; protect dirty documents |
| model_export_prepare / model_export | Snapshot and official worker GLB/CyPack export |
| model_view_settings | GUI-only clip/time/playback/tab presentation |
| panel_open | Reopen `model:<UUID>` or `json:<canonical content path>` |

A duplicate made with asset_copy receives a new portable UUID; a byte-for-byte
external duplicate retains identity and opening both paths is rejected. Reimport
of a GLB preserves the imported UUID. External disk changes block overwrite.

## JSON asset editor

Double-click a `.json` in content to open a tree editor. Keys can be filtered;
context menus add/remove values; JSON Pointer operations handle escaped keys.
Scalars, arrays, objects and null are supported, with shared Undo/Redo and explicit
Save JSON. Numeric entry uses JSON text to retain integer precision; strings over
4,096 bytes are read-only in this view with full-value copy. SDK editing remains
available within the document limit. External file changes block saving.
Limits: four open documents, 512 KiB each, depth 64, 50,000 values.
`json_open/get/value_set/add/remove/save/close` use the same native dispatcher.

## Extension points and remaining work

Keep real game codecs in trusted import/export providers, outside ImGui. Translate
coordinate/rig conventions explicitly and fill GamePortingTemplate.md. The shared
preview uses simple base color and normals, not the game's shaders. Native 0.21
adds static LOD generation, primitive collisions and reference-pose retargeting;
read [AdvancedAuthoring](AdvancedAuthoring.md) for exact algorithms and bounds.
Topology/UV layout tools, joint limits/contact constraints, curve tangent
handles, production streaming and engine rendering remain work. Blueprint-style
classes/containers/macros/events are implemented in the bounded interpreter;
actual game build/play and full engine runtime bindings remain adapter work.

Evidence: tests/test_scene3d.py, tests/test_interface.py, capture_3d.py;
`data/reports/completion-*`. Exported edited/key-authored GLBs are checked by the
pinned optional Khronos validator. This evidence covers the test fixtures only.

## Native 0.22 animation authoring

[AnimationTools](AnimationTools.md) documents offline fixed-root chain IK, extracted
root-motion pairs with sampled reconstruction and trimmed/resampled clips. The
existing model_process/Core worker/commit path is reused, with real native preview,
Undo and GLB/CyPack export. MotionToolsView.cpp supplies translated property views.
The desktop bridge now publishes terminal readiness after Core closes its handles.
No joint limits/contact solver or engine movement/physics binding is supplied.
Evidence: data/reports/motion-validation.json and motion-captures.json.
