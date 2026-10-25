# Material and image pipeline — 0.20.0

The Sandbox shader backend generates a DX11 **editor preview**. It is independent
of game build providers and does not export a material in an external engine format.
CyToolsCore supplies all scheduling, sessions, identity checks and worker lifetime.

## Desktop workflow

1. Open Examples/Example_UVMaterial.cyasset in the Materials workspace.
2. Select Compile preview. The code icon opens Material Compiler, with job state,
   progress/cancel, contextual diagnostics and generated HLSL after successful apply.
3. Change Tile, TintA/TintB or Roughness in My Graph. Declared root variables are
   uniforms: these values update the compiled preview without another compilation.
4. Switch Surface between Sphere and Plane. The sphere uses analytical UVs from
   its normal; the plane uses the preview rectangle's UV coordinates.
5. Change a node constant or a wire: the current shader is stale and a center CPU
   sample is used until the next explicit compile. Layout/comments do not stale it.
6. Open Example_UVInstance to edit overrides on the same parent program.

Refresh material explicitly reloads external parent changes. Loaded parents remain
the authority while open. No filesystem watcher, shader persistence across restarts,
compiler history recovery or automatic compile on every edit is provided.

## AI SDK / MCP contracts

All operations are declared in CyTool.json and use the shared native dispatcher.
The official SDK and op_<operation> MCP tools wrap them in Core jobs as usual.

| Operation | Purpose |
|---|---|
| material_get | Effective parent graph and typed instance parameters |
| material_prepare | Native immutable shader plan, parameter names/uniforms, signature, center sample and optional sampleError |
| material_sample | CPU evaluation at u/v (default .5, bounded to ±10000) |
| material_compile | Official worker job producing HLSL, DXBC and report |
| material_compile_request/status/cancel | Desktop SDK-client request and state; native graphical host required |
| material_preview_apply | Explicitly verify/install a compiler report in the graphical host |
| material_compiler_view | Open the compiler panel for a graph |
| material_preview_settings | Select Sphere or Plane without changing the document |
| texture_info | WIC header metadata inside the content root |
| texture_open | Open an image editor tab on the graphical host |
| texture_view_settings | Channel RGBA/RGB/R/G/B/A, fit and zoom .05–8 |

material_compile accepts graphId and optional expectedRevision. Native preparation
rejects stale revisions before the job snapshots the document. An SDK compilation
returns reportPath, sourcePath, shaderPath, profile, parameterNames, signature,
sourceSha256, bytecodeSha256, bytes, diagnostics and nodeMap. The files belong to
JobContext.path. Apply the report **before** closing a temporary task/session.
An independent SDK job does not install its result into the GUI automatically.
Desktop requests auto-apply only a matching result. Failed jobs use
error.technicalDetails for their report and contextual diagnostics.

A successful apply verifies format/schema/graph ID, current signature and parameter
ordering, file ownership beneath the report directory, source hash and bytecode hash.
The report must lie inside the editor workspace. It creates an ID3D11PixelShader and
retains source text in memory, so deleting job files afterwards does not invalidate
the installed preview. The operation is non-mutating: no document revision/Undo entry.
It is intended for trusted local Core compiler outputs, not arbitrary shader ingestion.

## Code generation and limits

MaterialProgram.cpp resolves a material/instance's loaded or saved parent chain.
The SHA-256 program signature removes root variable defaults and presentation data,
but keeps variable names/types, bindings, constants, links and local function programs.
Parameters are sorted by name, up to the graph's existing 256 declarations, each using
one float4 slot. Booleans use x, colors xyz and scalar values x. String values have no
GPU representation; unsupported reachable inputs fail with node/function context.

shader_codegen.py lowers the trusted expression subset: float/bool/color constants
and gets, typed reroutes, arithmetic/comparisons, UV coordinates/transforms, checker,
gradient, color compose/mix/multiply, sine/fract/saturate and local pure functions.
Only reachable expressions are lowered; pure function inputs/outputs are evaluated
as a complete call signature. There is no HLSL emitter registration for custom CPU
plugin nodes yet. The backend rejects unsupported reachable nodes and execution calls.

Generated source is limited to 1 MiB, 10,000 lowering steps and 2,000 expanded calls.
Native validation retains graph/function/depth limits. D3DCompiler compiles ps_5_0
inside a ProcessWorker, with no includes or asset-supplied commands/source. Core owns
execution timeout/cancellation. The desktop caps active preview requests at two;
exclusive shader jobs are serialized within each official runtime. Private SDK job
results remain isolated per identity; GUI job state is shared editor-session state.

Preview render targets are capped at 512 pixels per side.
CPU evaluation uses double and GPU HLSL uses float32, so exact numeric equality is
not promised. Colors are clamped; roughness is constrained by the lighting model.
GPU division by an absolute denominator below 1e-12 returns zero. The CPU interpreter
reports division by zero; a center sample failure is returned as sampleError and
uses a fallback color while still permitting compilation or sampling another UV point.
Custom node providers, game texture sampling, world inputs, engine render passes,
engine shader output and game-specific parameter packing remain extension work.

## Image editor backend

Image.cpp checks canonical ownership beneath data/assets before opening WIC. Only
PNG/JPEG/BMP are decoded into RGBA8. Metadata inspection is distinct from a complete
pixel decode: a damaged pixel stream can fail later with a visible viewer error.
Limits: 32 MiB encoded, 8192 per side, 4,194,304 pixels (16 MiB RGBA8). Only the first
frame is viewed. ICC/HDR authoring and multi-frame workflows are not provided.

EditorApp registers the image extensions through registerAssetEditor. TextureView.cpp
keeps per-tab zoom/pan/channel state, a transparency checker and manual reload. Wheel
zoom anchors to the pointer; middle drag pans. RGB ignores alpha; individual channels
show grayscale with opaque alpha. Fit and 100% are presentation settings.

TexturePreview.cpp has an eight-image LRU cache, with one decoded CPU image and one
current GPU channel view per entry. Peak retained full-size pixels are approximately
128 MiB CPU plus 128 MiB GPU, excluding temporary conversion buffers and driver costs.
Decode/upload runs synchronously within those bounds. The renderer callback can be
replaced for another platform. AssetImport.md describes import/reimport and
browser thumbnails. Pixel editing and DDS/game-format decoding remain work.

Renderer checks: run app/capture_materials.py with the private Python runtime. It
creates isolated workspaces, waits for actual job completion/current shader state,
and captures both languages and five palettes without modifying user assets/settings.

## Image sampling in material graphs (0.20)

`material.texture` accepts a content-relative literal `path`, u/v inputs and
returns color/alpha. The default image is Examples/Example_Image.png. Connect
material.uv to u/v and color to surface.baseColor; the supplied
Examples3D/Example_TexturedMaterial.cyasset demonstrates this path. Compile preview
uses the existing official Core shader worker and actual DX11 Texture2D bindings.

CPU and GPU use bilinear repeat addressing and sRGB-to-linear RGB conversion;
alpha stays linear. PNG/JPEG/BMP decode bounds still apply. The program supports
at most eight distinct textures and 64 MiB of decoded texture pixels. GPU path
inputs must be literal, with no dynamic path links. Asset paths are canonicalized
under the content root; texture references participate in saved/live dependency
protection before move/Trash.

material_prepare snapshots texture paths and hashes. The worker copies verified
images into its owned output directory alongside HLSL/DXBC/report; texture hashes
participate in program signatures. Installation verifies ownership, hashes and the
current plan before creating immutable sRGB GPU resources. GPU objects retain
pixels independently of job files, so finish_task cannot invalidate the installed
preview. External texture changes require explicit refresh/recompile. No arbitrary
shader source, includes, asset commands or second job scheduler is introduced.

This is preview material support. Game shaders, normal maps, full PBR rendering,
HDR/ICC authoring and engine-specific packing still require implementations.
Evidence: test_scene3d.py CPU samples, real worker compile, GUI install and task
cleanup checks; capture_3d.py captures both languages after cleanup.
