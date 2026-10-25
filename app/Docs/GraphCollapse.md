# Collapse selection into a function or macro — native 0.27

The native graph editor can extract a region of pure calculations into a new local
function or inline macro. It derives typed boundary ports, preserves authored data,
rewires the caller and commits one Undo transaction. The existing interpreter,
material compiler and CyToolsCore dispatcher remain the execution paths.

## Try the editor

Open BlueprintExamples/Example_BlueprintCollapse.cyasset in Scripting. Run graph
prints 9. Ctrl-click Add and Multiply, choose Collapse selection, give the helper
an unused identifier and choose Function or Macro. Preview collapse shows the
selected count, incoming/outgoing connections, typed ports and boundary details.
Apply collapse replaces the selection with its call and selects that call.

Open function in the inspector also works for macro calls. Use Frame all / Home
when opening a helper with nodes outside the current camera. Node positions remain
unchanged in the new body; the generated input/output boundaries sit to either side.
The caller's spatial comment groups remain at the caller. Per-node comments,
breakpoint flags and unknown metadata move with their nodes.

The preview does not edit the owner, revision, debugger or Undo. Any source edit
makes it stale; preview again before applying. Editing the draft name/kind clears
the report. After applying, Ctrl+Z/Ctrl+Y restores the complete old/new owner.
Save asset remains explicit. The example source is not rewritten by QA.
English is source/default/fallback; French remains selectable.

## Supported regions and semantics

- The owner is a logic or material graph. The source body may be the root, a pure
  function, an execution function or a macro. The extracted helper is always pure.
  Material owners support functions; macros remain limited to logic owners.
- Select 1..128 distinct existing nodes. Keep function boundaries, material output
  nodes, variable reads/writes and all nodes with execution pins outside. Connect
  their data through the selection's boundary wires instead.
- There must be 1..16 distinct output endpoints connected to nodes outside the
  selection, and at most 16 incoming data wires. Every exported output must use
  every selected node and every incoming wire. Dead islands or independent outputs
  that fail this rule reject. This protects the existing eager function arguments
  from evaluating data that was previously unused.
- Dependencies are checked per output pin. In particular, UV transform U and V
  have independent inputs; an unused V input cannot become an eager helper argument
  merely because U is being extracted. This includes unused inputs that would fail
  if evaluated. Candidate validation also rejects cycles created by crossing an
  outside path back into the new call.
- Pure function/macro calls, records, enums, containers, object field reads and
  material calculations can participate within these rules. Nominal types and owner
  libraries are inherited by the new helper; no duplicate libraries are serialized
  inside function bodies. These remain local interpreter objects, not game memory.

Each incoming wire gets its own generated in_N port, even if multiple wires share
one external source. Its default is the original destination pin's authored fallback.
This retains different fallback values when one shared source feeds several pins.
Existing selected-node fallback values remain intact inside the helper.

Outgoing wires sharing the same selected node/output pin share one out_N port.
That declaration gets the type's normal default; its return pin is wired to the
original calculation. Original outside destination fallbacks remain unchanged.
Use the existing signature migration tools to rename ports after extraction.

All original node/link IDs and unknown metadata survive exactly once in the owner:
selected nodes and internal links move into the helper; crossing links stay in the
caller, preserving their IDs/metadata while endpoints change to the call. Generated
boundary links and nodes receive deterministic collision-checked IDs. Other bodies,
libraries and owner metadata remain unchanged. Function/macro identities, display
ports and generated IDs are not translated.

The whole candidate passes the existing domain, nominal type, call-graph, recursion,
node/link and document-size validation before commit. Semantic changes mark the
current debugger Stale; restart it. Existing watches are not remapped to the moved
nodes' new context. Interpreter budgets and stack limits remain unchanged; extraction
can add boundary/call work and does not promise identical budget cost, error ordering
or debugger stepping. No execution-flow, latent-node or live-object migration occurs.

## SDK / MCP

The current catalogue has 173 editor operations / 194 tools including Core controls.
Use the official Core authenticate, submit and wait path. Both operations share the
native implementation used by the UI.

```json
{
  "graphId":"asset:BlueprintExamples/Example_BlueprintCollapse.cyasset",
  "nodeIds":["add","multiply"],
  "name":"Calculate",
  "kind":"function"
}
```

Optional functionName selects the source body; omit it for the root. kind defaults
to function and may be macro for a logic owner. name is a new identifier (up to 64
characters); existing names are never replaced. nodeIds are unique, bounded to
128 items / 128 bytes per ID in the operation contract.

1. graph_collapse_plan(request) returns valid, diagnostic when invalid, sourceHash,
   planHash, name, kind, sourceFunction, movedNodeIds, callNodeId when generated,
   inputs, outputs and internal/incoming/outgoing link counts. Incoming port details
   identify the old wire/endpoint and defaultBytes; they do not duplicate full
   authored fallback values. Output details identify the old endpoint and fanout.
2. graph_collapse_apply(same request, planHash) recomputes the candidate, requires
   a valid matching source/request hash and commits it atomically. expectedRevision
   may additionally guard apply. The result includes applied and the callNodeId.

Schema/source validation failures may reject immediately before a report is available.
Node selection order is canonicalized; reversing the same node set does not alter
its plan. Changed source metadata/layout invalidates a preview. No caller-supplied
candidate graph is trusted. No nodes or wires are silently discarded, so there is
no acceptLoss bypass. Invalid regions must be changed or narrowed first.

Reports are limited to 256 KiB. Expanded caller/helper defaults must fit existing
budgets: one owner up to 1 MiB, 32 functions, 500 nodes / 2,000 links per body,
2,000 aggregate nodes and call depth 16. Existing named-schema/value caps also apply.
The tool does not raise limits, prune unused calculations or implicitly cast types.

## Verification

- tests/test_graph_collapse.py checks deterministic read-only previews, exact moved
  nodes/metadata/positions, retained link IDs, authored fallbacks, fanout, nominal
  types, nested pure/execution source bodies, shared runtime objects, shader
  compilation, macro expansion, persistence, stale/tampered/revision rejection,
  bounded functions/ports, lazy UV dependencies, debugger Stale and whole-owner Undo.
- --test-collapse seeds its fixture via native commands, then uses only its own
  isolated ImGui input for actual canvas Ctrl-selection, function/macro controls,
  previews, Apply and Ctrl+Z/Ctrl+Y. EN/FR reports contain fourteen checks each.
- capture_collapse.py produces caller/helper examples after official Core jobs,
  full gesture runs, function/macro previews in EN/FR and previews in six themes.
  Partial frames are distinguished from full runs. Capture entries record native
  executable and source fixture hashes.
- data/reports/collapse-validation.json records the native/source checkpoint,
  regression results, captures and preservation checks. Root AGENTS.md and
  AI_GUIDE.md ship with the source distribution. Historical archives are preserved.

Execution-region extraction, arbitrary disconnected/lazy regions, cross-asset
function libraries, cross-project schema migration, engine compilation/ABI and
real game codecs remain separate work. This feature does not imply Unreal Blueprint
binary compatibility or a complete Unreal-equivalent editor.
