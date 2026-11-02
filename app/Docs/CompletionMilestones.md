> Current native release: 0.29.0. See [CyUIMigration](CyUIMigration.md), [GraphExpand](GraphExpand.md) and [AdvancedAuthoring](AdvancedAuthoring.md).
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

## Native 0.29 modular presentation

CyUI 1.2.0 is pinned at `ff023e646c6a3038b3dfbfd53f6d2b4a2f5955e3`.
Common Nodes/Canvas, Inspector, Assets/Images, Viewport/ImGuizmo, Timeline,
Commands, Diagnostics, Code and Diff replace the matching local presentation.
The host retains document data, validation, runtime/compiler providers and Undo.

CMake consumes the explicit module lock, supplies one ImGui target and excludes
Audio, Gallery, Demo, CodeEditor and Web/CEF. Both build scripts use the same
configuration. Clean default-fetch and exact-checkout builds are verified on
Windows x64 / MSVC / DX11. The native Linux/macOS host remains unimplemented.

Verification covers native contracts and 182 Python cases, including actual
EN/FR interactions, revision/local-schema draft cancellation, one-Undo edits,
keyboard command captions/IDs, asset actions, image navigation and timeline seek.
80 isolated captures cover six themes, compact/comfortable densities, simulated
100/150/200 percent DPI, generated HLSL and graph diffs. Existing 28 asset/project
files, saved preferences and layout retain their baseline hashes.

Read [CyUIMigration](CyUIMigration.md) for the exact API gaps and retained adapters.
Evidence: `data/reports/cyui-validation.json`. Catalogue remains 175 editor
operations / 196 tools including Core. No game codec or engine binding is added.
