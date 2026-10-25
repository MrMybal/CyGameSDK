> Current native release: 0.28.0. See [GraphExpand](GraphExpand.md) and [AdvancedAuthoring](AdvancedAuthoring.md).
> This document retains its historical milestone/recovery evidence.

# Completion milestones — 0.20.0

This release extends the verified 0.16.1 interface baseline. The charcoal Unreal-
inspired palette, EN default/FR catalogue, docking, workspaces and existing graph
workflows are retained. It does not claim parity with Unreal Engine.

| Milestone | Delivered | Evidence |
|---|---|---|
| Import and browser | Image copy, hashes, protected reimport, sidecars, real decoded thumbnails; restored and rebuilt from preserved 0.19 source | Native contracts and 39 integration scenarios |
| Neutral 3D | GLB/CyPack to editable .cy3d and back; UUID/provenance; guarded saves | Codec/native/official worker tests and Khronos export checks |
| Mesh tools | Real DX11 triangles/depth/lighting/wireframe; vertex/normal/UV/skin properties | Actual ImGui vertex gesture and Undo/Redo |
| Skeleton/animation | Hierarchy/TRS, CPU skinning, quaternion interpolation, clips/tracks/key creation and playback timeline | Native sampling/export tests and real playback gesture |
| JSON assets | Tree, add/remove/edit, precision-preserving numbers, guarded saves | Escaped pointers, Undo, external edit and persistence tests |
| Material textures | Bounded PNG/JPEG/BMP samplers, CPU and real compiled DX11 preview | sRGB/bilinear/repeat tests; GPU resources survive Core task cleanup |
| Release integrity | Source/catalogue/executable versions, root AI guides, source checkpoint, unchanged prior user files | completion-validation.json |

Read Native3DAssets.md, AssetImport.md and MaterialPipeline.md for exact bounds.
Editor Overview reports availability in the application; unavailable work remains
visible rather than being presented as implemented support.

## Remaining product milestones

- Full Blueprint-level language: composite/container types, classes/object references,
  macros, latent execution and engine compilation/runtime bindings.
- Production mesh/rig tools: topology/UV authoring, LOD/collision, retarget/IK,
  curve tangent tools, scalable data streaming and game material rendering.
- Texture pixel authoring and game texture codecs.
- Engine-specific map/render/play/build pipelines, validated against a chosen game.
- Git/Perforce UI, engine profiler, landscape and cinematic authoring.

Those items need further implementations; the current Sandbox cannot validate
arbitrary engine output. A game SDK must be derived separately using AI_GUIDE.md
and GameAdapterGuide.md, with real fixtures and a filled game porting dossier.

Historical 0.16.1 source/binary recovery and 0.19 candidate archives are preserved.
The new binary is rebuilt as 0.20.0; the old candidate executable is not activated.

## Local verification commands

Run from the repository root after regenerating the manifest and building:

```powershell
.\runtime\build\CyGameSDKTests.exe (Get-Location).Path
$env:CYGAME_TEST_GUI='1'
.\runtime\python\Scripts\python.exe app\tests\test_integration.py
.\runtime\python\Scripts\python.exe app\tests\test_scene3d.py
.\runtime\python\Scripts\python.exe app\tests\test_interface.py
.\runtime\python\Scripts\python.exe app\tests\test_portable_assets.py
.\runtime\python\Scripts\python.exe app\capture_interface.py
.\runtime\python\Scripts\python.exe app\capture_3d.py
node app\tests\validate_scene3d_exports.cjs
```

The native contracts and 77 Python scenarios pass (39 integration, 13 neutral
asset/texture, three interface, 22 portable). Renderer QA produces 56 captures
(36 workspace/panel and 20 3D/JSON/import/texture-material), across EN/FR and six
palettes. Two edited/key-authored GLB exports have zero Khronos errors/warnings.
These tests use isolated roots; no editor is left running in the user's workspace.
The prior 12 asset files, language/theme preferences and layout are hash-checked.
See completion-validation.json for the exact binary/source checkpoint hashes.
