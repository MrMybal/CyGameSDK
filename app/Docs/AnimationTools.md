# Animation tools — native 0.22.0

This release adds offline animation authoring to the existing native document and
Core worker pipeline. It does not add an engine animation runtime, physics solver,
Unreal file compatibility or a real game codec. English stays the source/default
language; French remains selectable, with all existing themes/layouts preserved.

## Try it

Open Examples3D/ChainMotion.cy3d, an original three-joint ribbon fixture, then the
Motion tools tab. The left clip selector chooses the source or reference pose.
New worker results select their output clip automatically. Authoring changes the
loaded document in one Undo transaction. Save asset is explicit; the original
source clip and bind data remain unchanged by these tools.

- IK: select Root and End, target (1, 1, 0), optional pole (0, 0, 1), then Bake IK
  clip. Play/scrub the new clip and inspect the reported maximum target error,
  unreachable sample count and unconverged sample count.
- Root motion: choose Walk_Root and the top-level Root node. Select X/Z for planar
  translation, or X/Y/Z for all translation; Extract rotation extracts the entire
  quaternion, not yaw alone. Extract motion pair creates an in-place clip and a
  displacement clip. The panel previews either and can Reconstruct motion clip.
- Clip tools: choose a source, enter Start time / End time within its range, choose
  the sample rate and Create trimmed clip. The new clip starts at zero.

All output clips can be edited in Animation and exported through the existing
GLB/CyPack jobs. The source fixture is installed exclusively; existing files are
never replaced by setup_examples.py. New clip names may be supplied through SDK.

## Fixed-root chain IK

animation_tools.py implements a bounded single-chain FABRIK solver, followed by
reconstruction of local rotations. A chain contains 3..32 directly connected nodes;
its root must be an ancestor of the tip. The root retains its sampled source
position on each frame. Chain translations/bone lengths remain those of the source
pose at that frame, including any animated translations. Side branches inherit the
changed rotations. The end-effector position is solved; orientation is not targeted.

Target and optional pole coordinates are in the asset's right-handed +Y-up world
space, in metres. The solver uses 1..128 iterations and tolerance 0.000001..0.1 m.
Out-of-reach targets do not stretch bones. Minimum and maximum reachable distance
are checked; residual error and lack of convergence are reported explicitly. A
straight-chain singularity receives a deterministic bending-plane seed. An optional
pole orients internal joints around their neighboring-joint axis.

Only decomposed TRS nodes are supported. Chain nodes and all their ancestors must
have unit scale, including animated scale. Zero-length segments are rejected. The
result is a new sampled LINEAR rotation clip at 1..120 FPS; unrelated tracks stay
intact. Reference pose creates a one-frame clip. No joint-angle limits, collision
avoidance, multi-chain constraints, contacts, foot locking or automatic humanoid
rig inference is implemented. This is not a production IK rig/control graph.

Reference: [Aristidou and Lasenby, FABRIK (2011)](https://www.andreasaristidou.com/publications/papers/FABRIK.pdf).
The original research describes broader cases than this bounded implementation.

## Root motion pair

Extraction requires a top-level TRS node. The first sampled root transform supplies
the lock reference. Selected translation components and optional full rotation are
removed from the new in-place root channels. Scale is retained in that clip. Other
nodes retain their source tracks; the original clip is kept.

For root position p(t), rotation q(t), and locked in-place values l(t), r(t), the
rigid delta is Dq(t) = q(t) * inverse(r(t)), Dp(t) = p(t) - rotate(Dq(t), l(t)).
Reconstruction evaluates p = Dp + rotate(Dq, l), q = Dq * r. This accounts for a
nonzero first-frame offset, mixed extracted axes and root rotation together.

The displacement clip has root translation/rotation channels containing these
rigid deltas. It is transport data, not a final character animation by itself.
A generic GLB player may display it as an ordinary root animation. It does not
move a game character, capsule or entity unless a game adapter deliberately binds
that behavior. The shown distance measures the delta-translation curve's path.

extensions["org.cygame.editor"].rootMotion stores at most 32 pairs: record UUID,
source/inPlace/motion clip indices, root node, axes, rotation policy, FPS and two
pose fingerprints. Fingerprints use supported track data quantized to float32,
matching GLB storage; display names, source-channel provenance and unknown metadata
are excluded. Edits that change either clip's pose data reject reconstruction;
extract a new pair. Reparenting the motion root also rejects reconstruction.
A future clip deletion/reorder operation must migrate these indices atomically.

Reconstruction matches the baked samples. Resampling nonlinear source interpolation
can introduce between-sample error; it does not restore the original cubic curves.
No root-motion engine policy, movement controller, gravity, replication or motion
warping is supplied. [Epic's root-motion overview](https://dev.epicgames.com/documentation/unreal-engine/root-motion-in-unreal-engine?lang=en-US)
describes the additional engine behavior; the editor does not claim that behavior.

## Trim/resample

The source range is inclusive. Sampling includes both endpoints and the result's
first key time is zero. Source LINEAR/STEP/CUBICSPLINE are evaluated; output uses
LINEAR keys and normalized, hemisphere-continuous quaternions. Original clips and
unknown track metadata remain intact. This is a sampled conversion, not a lossless
curve/tangent edit. The existing 128-clip / 20,000 total key budget applies.

## SDK / MCP and ownership

Existing model_process and model_process_prepare now accept three more actions;
there is no new MCP server or scheduler and the catalogue remains 161 editor
operations / 182 tools including Core. Use the normal official Core submit/wait
path, as with lod/collision/retarget:

```json
{"modelId":"<loaded UUID>","action":"ik","options":{"animation":0,"root":1,"tip":3,"target":[1,1,0],"pole":[0,0,1],"fps":30,"iterations":64,"tolerance":0.001}}
{"modelId":"<loaded UUID>","action":"root_motion","options":{"animation":0,"root":1,"axes":[true,false,true],"rotation":true,"fps":30}}
{"modelId":"<loaded UUID>","action":"root_motion","options":{"mode":"restore","recordId":"<pair UUID>"}}
{"modelId":"<loaded UUID>","action":"clip_edit","options":{"animation":0,"start":0.4,"end":1.6,"fps":30}}
```

model_view_settings adds tab="Motion tools", motionTool=IK|RootMotion|Clip,
ikRoot/ikTip/motionRoot and ikTarget/ikPole presentation options. The GUI uses the
same dispatcher and trusted workers. Immutable snapshots, cancellation, source
hash guards, protected provenance, bounded commits and document Undo are reused.
The desktop bridge publishes terminal readiness after the Core runtime has closed
its database/worker handles. Subsequent requests refresh terminal readiness rather
than reusing a cached UI state. Persistent desktop export outputs remain job-owned.

## Metadata interchange

The bulk editor namespace stays in glTF root extras["org.cygame.editor"] with its
checked CyPack manifest reference. Other portable namespaces now also travel in
extras["org.cygame.metadata"], so raw GLB transport retains them. Matching CyPack
copies are accepted; conflicts and missing referenced editor payloads reject.
These are editor transport conventions, not registered glTF engine extensions.
The portable component remains version 0.1 / envelope schema v1.

## Evidence and bounds

- tests/test_animation_tools.py: independent pose/length checks, singular/unreachable
  cases, partial-axis reconstruction, GLB/CyPack transfer, metadata conflict checks,
  native skinning, jobs/privacy, Undo/save and real ImGui button-triggered jobs.
- capture_motion.py: EN/FR, unreachable diagnostics and all six themes, actual
  desktop Core jobs and native rendering in isolated roots.
- data/reports/motion-{tests,authoring,integration,scene3d,interface,portable_assets}.log,
  motion-native.log, motion-gestures.json, motion-captures.json, motion-khronos.json
  and motion-validation.json describe the verified scope.

The existing small-asset profile remains 2 MiB per neutral document, eight open
models / 4 MiB total. Larger production meshes/animations need a storage provider
and migration. No real game codec is included; choose a target and verify actual
fixtures in a separate derived SDK before claiming game support.
