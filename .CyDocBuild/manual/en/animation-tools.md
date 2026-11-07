---
id: animation-tools
title: IK, root motion and clip tools
description: IK, root motion and clip tools
section: Assets
order: 23
revision: 1
parent: assets-3d
---

## Try Motion tools {#start}

Open `Examples3D/ChainMotion.cy3d`, then Motion tools. Select a source clip or reference pose. Results create a new clip and select it, with one document Undo and explicit Save asset. Source clips and bind data stay unchanged. For IK choose Root/End, target (1,1,0), optional pole (0,0,1), then Bake IK clip. Inspect maximum error and unreachable/unconverged sample counts.

## Fixed-root IK {#ik}

The bounded FABRIK chain has 3–32 directly connected nodes; root must precede the tip. Each sample keeps root position and original segment lengths, including animated translations. Side branches follow changed rotations. The solver targets position, not end orientation, with 1–128 iterations and tolerance 0.000001–0.1 m. Unreachable targets do not stretch bones; minimum/maximum reach and convergence are reported. A deterministic bending seed handles straight chains; an optional pole orients internal joints.

Coordinates are asset world space: right-handed +Y up, metres. Only decomposed TRS is accepted. Chain and all ancestors need unit scale, including animated scale; zero-length segments fail. Output is a new sampled LINEAR rotation clip at 1–120 FPS; unrelated tracks remain. Reference pose creates one frame. There are no joint limits, contacts, collision avoidance, foot locks, multi-chain constraints or automatic humanoid inference.

## Root-motion pairs {#root-motion}

Select a top-level TRS root. Choose translation axes and optional rotation extraction: rotation means the complete quaternion, not yaw only. The first sampled transform supplies the lock reference. Extraction creates in-place and displacement clips, preserving source and non-root tracks. The displacement clip is transport data, not a game movement controller.

For source position p/rotation q and locked in-place l/r, `Dq=q*inverse(r)` and `Dp=p-rotate(Dq,l)`. Reconstruction uses `p=Dp+rotate(Dq,l)` and `q=Dq*r`. This retains nonzero first-frame offsets and mixed axes/rotation. `extensions["org.cygame.editor"].rootMotion` stores up to 32 pairs with UUID, clip indices, root, policy, FPS and float32 pose fingerprints. Pose edits or root reparenting reject reconstruction; extract a new pair. Clip reorder/deletion must migrate stored indices atomically. Reconstruction matches baked samples, not original cubic curves between samples.

## Trim and resample {#trim}

Choose an inclusive source range and sample rate. Both endpoints are included and the new clip starts at zero. Respect key/document budgets. Report resampling loss instead of claiming exact preservation of nonlinear interpolation. Outputs remain editable/exportable through the existing jobs; engine movement, physics, replication and motion warping are not supplied.

```image
{"id": "animation", "src": "assets/screenshots/animation.png", "alt": "Clip selection, keyframes and timeline", "controls": "minimal"}
```
