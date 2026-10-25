> Native 0.23 adds [NamedGraphTypes](NamedGraphTypes.md). Its local enums and
> structures supersede the corresponding 0.21 limitations recorded below.

> Native 0.22 adds [AnimationTools](AnimationTools.md). This document describes
> the retained 0.21 authoring layer and its original limits.

# Advanced authoring — native 0.21.0

This release extends the 0.20 foundation with working graph/runtime and neutral
asset tools. English remains the source/default language; French is selectable.
It does not provide Unreal file compatibility or a complete Unreal Engine runtime.

## Try the delivered workflows

Run CyGameEditor.exe / Lancer.cmd. setup_examples.py installs fixtures exclusively,
without replacing existing content. New source fixtures are original test assets.

| Content folder | Asset | Useful workflow |
|---|---|---|
| BlueprintExamples | Example_BlueprintObjects.cyasset | Construct a derived class, edit its typed field and call an inherited method |
| BlueprintExamples | Example_BlueprintContainers.cyasset | Typed array, inline macro, execution-function loop and loop count |
| BlueprintExamples | Example_BlueprintLatent.cyasset | Debug, Continue, then Advance time by 1 second; emit Interact and Continue |
| Examples3D | Grid_LOD.cy3d | LODs > Generate LOD, select preview LOD, export selected geometry |
| Examples3D | Grid_LOD.cy3d | Collision > fit a box/sphere/capsule; change properties or remove the shape |
| Examples3D | AnimatedTriangle.cy3d and TallTarget.cy3d | Open both; target > Retarget, source/clip/bone mapping, then Bake retargeted clip |

New assets require an explicit Save asset after authoring. Worker commit changes
the loaded document and creates a normal Undo transaction; it does not overwrite
the source file. Save guards, UUIDs, source payload and unknown metadata remain intact.

## Blueprint-style runtime

The existing typed canvas, functions, compiler diagnostics and debugger are reused.
These are concrete additions to that runtime, not empty compatibility flags.

- int32 and vector3 values, object references, typed arrays/maps/sets; literals,
  variable access, container operations, vector add/dot and integer conversion.
  Type IDs appear in graph_get/nodeDefinitions. Containers contain at most 256
  entries / 64 KiB, and supported container elements are one level deep.
- Classes are local to a logic asset. A class declares parent, typed fields and
  optional method-to-function bindings. Defaults inherit; changing an inherited
  field type is rejected. Overrides keep the method signature/mode. A method
  requires a typed `self: object` argument. Calls dispatch to the instance class.
- object.new/get/set/destroy/valid operate on shared references within the current
  runtime. References use unique transient IDs; they are not engine-memory pointers.
  Classes have at most 64 fields/methods, inheritance depth 16; the runtime retains
  at most 256 instances / 256 KiB. Restart invalidates old handles.
- Functions still have isolated locals. Macros are declared with kind=macro and
  use macro.call. The compiler expands them into ordinary nodes/links with scoped
  locals and bound defaults before interpretation. There is no runtime macro call
  frame. The source asset is preserved; trace/navigation maps expanded nodes back
  to the originating call. Macro-body stepping is bounded by the same debugger.
- flow.foreach invokes a typed execution function for each supported array item.
  Its callback declares item with the array element type and index:int. Extra
  arguments use ordinary typed inputs. Optional return `break:bool` stops early;
  count reports completed iterations. No cyclic graph wires are introduced.
- flow.delay suspends without sleeping or blocking the UI. graph_debug_tick adds
  explicit logical time; the debugger offers Advance time. event.custom and
  flow.emit support named events; graph_debug_event queues an external event.
  Instances/variables survive completed events within the same debug session.

This simulation is a single execution lane. Waiting functions suspend that lane;
custom events queue behind its work. There is no engine timer/event/concurrency
bridge. The graph clock is separate from CyToolsCore job scheduling; Core remains
the sole MCP/SDK/worker scheduler. Each advance shares 4,096 execution steps and
10,000 expression operations across nested calls; queues are capped at 4,096,
call depth at 16 and graph/function total at 2,000 source nodes. Expanded bodies
retain the 500-node / 2,000-link limit. Graph assets are capped at 1 MiB.

Use My Graph > New variable/function, the Inline macro option, typed signatures
and Classes. Class field/method declarations use editable JSON. The debugger
shows shared runtime objects with automatically generated typed property fields.
These edits affect runtime instances, not declaration defaults or document Undo.
Clipboard transfers include functions, classes and inherited declarations; name
conflicts reject the paste instead of silently changing the destination.

SDK: existing graph/function/variable operations, graph_class_define/remove,
graph_debug_start/status/continue/step/stop, graph_debug_tick/event/object_set and
graph_debug_view. The view operation requires a graphical host. New types are
logic runtime features; the preview material compiler retains its supported subset.

Remaining Blueprint layers include named struct/enum/interface assets, nested
container types, full engine object/component reflection, replication, asynchronous
execution lanes, engine compilation/JIT and compatibility with Unreal Blueprint.
The current implementation must not be described as full Blueprint parity.

## LOD generation and export

LODs > Generate LOD invokes model_process through an official Core ProcessWorker.
The bounded static simplifier clusters vertices, preserves source boundary/seam
vertices, removes collapsed/duplicate faces, rejects flips/non-manifold candidates
and reports actual triangle count and maximum vertex displacement. This is an
approximation, not QEM or a production topology optimizer. Some meshes cannot be
reduced safely under these rules; failure leaves the document unchanged.

The requested triangle ratio is a target, not a guaranteed exact count. At most
four generated LODs are stored per mesh. Skinned meshes, extra vertex attributes,
morph targets and compressed geometry require an appropriate trusted provider.
Preview LOD uses real geometry buffers. Export preview LOD / model_export(lod=N)
exports the selected geometry; meshes without that level retain their base mesh.
The exported selected-LOD asset omits the other generated levels. Base export
retains authoring metadata; this is not automatic distance-based engine rendering.

A base-vertex edit marks generated levels stale. The UI returns to base preview;
stale levels require Regenerate LOD or removal and cannot be sampled/exported as
current geometry. Undo restores geometry and the derived-state markers together.

## Collision authoring

Automatic fit produces conservative box, sphere or Y-axis capsule shapes for
static reference geometry. Center/orientation/extents/radius/height/enabled are
editable and undoable; the viewport projects actual wire geometry. This is an
asset-reference-space editor contract, not a physics simulation/cooked game format.
64 shapes are supported. Automatic fit currently requires decomposed TRS geometry.

Geometry or reference-transform edits mark collision fits for review. They appear
with warning colors until Confirm collision fit or a new fit. Source providers
must preserve and honor stale/review markers when changing geometry themselves.
Convex decomposition, triangle-mesh cooking, ragdolls, constraints and engine
collision-channel policy remain integration work.

## Retargeting

The target editor selects a loaded source model, source clip and source-node to
target-node mapping. Source/target indices refer to stable node order; ambiguous
name matches are not guessed by the worker. Duplicate target mappings are rejected.
The UI provides a searchable mapping list. SDK options use mapping={"1":1,"2":2}.

The worker samples source animation, applies reference-pose orientation correction
in world rotation space and reconstructs target local rotations in hierarchy order.
Bind mode preserves target translations/bone lengths. Root scaled / All scaled
transfer source translation deltas with an explicit scale. Scale animation is not
transferred. The result is a new target clip; mesh/rig/source UUID are preserved.
Samples bake to LINEAR keys (1..120 FPS), within the existing 20,000-key profile.
Matrix nodes require decomposition. No IK, chain solver, foot contacts, automatic
humanoid rig inference or guaranteed root-motion policy is implemented.

## Authoring jobs and concurrency

model_process(action=lod|collision|retarget, options=...) prepares an immutable
snapshot (combined source/target at most 3 MiB), then executes the trusted worker.
model_process_commit validates model structure, protected provenance, target/source
hashes and the loaded-document budget before one Undo transaction. Changes made
while a job runs reject its stale result. Cancellation belongs to Core and stops
the worker; no second process/job scheduler is added. Results and reports are
private to the Core identity/session. finish_task removes unexported task files;
explicit document commits remain user operations.

Short operations: model_lod_remove, model_collision_set/remove and existing model
lifecycle operations. model_view_settings selects tab/LOD/wireframe/collision display
and retarget source/mapping without changing the document. Capture scripts use the
same native dispatcher and real renderer in isolated roots.

## Interchange and metadata

Authoring data lives in extensions["org.cygame.editor"] in .cy3d. Bulk LOD geometry,
collision shapes and provenance are embedded in glTF extras for GLB transport.
A CyPack envelope references this payload with storage=glTF.extras, schemaVersion=1
and a SHA-256 of canonical sorted compact UTF-8 JSON. The decoder checks it and
restores full metadata. This keeps bulk geometry outside the 128 KiB manifest cap.
Unknown sibling namespaces and source data are retained. Native authoring semantics
require a supporting SDK; a generic glTF consumer may ignore these extras.

.cy3d/CyPack preserve portable UUID identity. A new raw GLB import creates a fresh
UUID; reimport preserves its existing destination identity. Generated LOD/collision
metadata is not a claim that any game understands it. The small 2 MiB neutral /
eight-model / 4 MiB loaded profile remains; larger production assets need a storage
provider and validated migration, not merely larger extension filters.

Read GameCodecs.md before adding a game format. No real game codec ships in this
release because no game/version/files/destination has been selected.
