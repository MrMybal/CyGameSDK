# Expand pure calls — native 0.28

Select one local function or macro call in a logic/material graph or function body,
then **Expand call → Preview expansion → Apply expansion**. English remains the
source/default/fallback; French and all six CyUI themes use the same controls.
The preview is read-only. Apply selects the resulting editable nodes and adds one
whole-document Undo entry. Ctrl+Z restores the exact call; Ctrl+Y reapplies it.
Use Frame all / Home when the expanded region extends beyond the current camera.

Example: BlueprintExamples/Example_BlueprintExpand.cyasset prints 9 before and
after expansion. The Calculate definition remains in My Graph. This complements
[Collapse selection](GraphCollapse.md); expanding does not remove shared definitions.
No files are saved implicitly, and opening an asset never expands a call.

## Preserved authored data

- Expand one function.call or macro.call with mode pure. The source definition,
  other calls, owner libraries and unrelated bodies stay unchanged. Nested calls
  are copied at one level, retaining their references; no recursive unfolding.
- Clone every interior node, including dormant ones, with deterministic,
  collision-checked new IDs. Retain values, unknown metadata, comments, breakpoints
  and relative positions. Copy internal wires with new IDs and original metadata.
  Original boundary data remains intact in the retained definition.
- One typed reroute per declared input/output replaces the removed boundary.
  External wires keep their IDs/metadata and fanout. Input reroutes retain the
  call's authored argument defaults, including defaults hidden by connected wires.
  Interior destination fallbacks remain on the cloned nodes.
- Unwired pure-function outputs retain the actual return node's authored fallback.
  Existing macro runtime semantics instead use the output declaration default;
  expansion preserves this distinction, even when the two values differ.
- A comment group surrounds the expanded region. Its expandedCall property has
  format cygame-expanded-call-v1, the complete original call JSON, sourceHash and
  function name. This preserves otherwise removed call metadata. Existing comment
  editing preserves these fields; explicitly deleting the group deletes its record.
  Function comment groups are also copied, translated and assigned new IDs.
- Node positions translate from the helper's interior bounding origin to the call's
  location, with input/output reroutes around them. No automatic layout rearranges
  unrelated nodes. Unknown metadata is retained literally, not interpreted as ID
  references or automatically migrated to the cloned IDs.

Builtin scalar/vector/object/container reroutes and nominal enum/record reroutes
use the existing interpreter and descriptors. Runtime objects remain local to this
interpreter; they do not reflect an arbitrary game's memory.

## Evaluation guards and limits

Pure functions eagerly resolve every argument, even when a requested return value
ignores one. Inlining such a call can hide division-by-zero, invalid object reads,
or other failures in unused wired arguments. Therefore **each connected call output
must depend on every wired call input**. The dependency walk distinguishes UV
transform U/V outputs and expands nested macros for analysis, since macros are lazy.
Unsafe functions reject with a diagnostic; use the original call. Unwired argument
values already pass schema validation and need no forced evaluation. Dormant calls
with no connected output may be expanded without evaluating them.

Macros already inline lazily in the runtime. They do not need the eager-input guard;
unused incoming wires remain on unreachable input reroutes. Passthrough, fanout,
unwired outputs, named types and material sampling context retain their behavior.

Limits: at most 128 interior nodes per expansion plus up to 32 boundary reroutes;
256 KiB preview including invalid reports. Whole-owner validation and runtime macro
expansion validation precede commit. Existing 1 MiB document, 500 nodes / 2000 links
per body, 2000 aggregate nodes, 128 comment groups, position/group bounds, recursion
and nesting limits remain enforced. Nothing is pruned to fit a limit.

Execution/latent calls, object/interface dispatch calls and unsupported scoped nodes
reject. This is not execution-flow inlining, Unreal Blueprint binary compatibility,
a game compiler or an engine binding. Work/budget cost, debugger call stacks, error
ordering and watch paths can change. Semantic edits mark an active debugger Stale;
restart it. Existing watches are not remapped. There is no cross-asset refactoring.

## SDK / MCP

The catalogue has 175 editor operations / 196 tools including Core controls.
Both new operations share EditorCore::execute with the GUI and native IPC, through
the existing official CyToolsCore authenticate / submit / wait job path.

```json
{"graphId":"asset:BlueprintExamples/Example_BlueprintExpand.cyasset","nodeId":"calculate"}
```

Optional functionName selects the caller body. nodeId is 1..128 bytes in the
operation contract.

1. graph_expand_plan(request) returns valid, diagnostic when invalid, sourceHash,
   planHash, nodeId, sourceFunction, name, kind, nodeMap, addedNodeIds, inputs,
   outputs, clonedNodes and internal/incoming/outgoing link counts. Each port lists
   type, relayNodeId and connected. Successful candidates also identify the
   provenanceCommentId. Large fallback values are not duplicated in the report.
2. graph_expand_apply(same request, planHash) rebuilds and validates the candidate,
   requires the exact hash and commits once. Optional expectedRevision protects
   against a concurrent editor document edit. Stale, invalid or altered requests
   fail atomically without modifying graph data or Undo history.

Hashes cover the complete owner, including library/default/metadata/layout changes,
plus the exact caller context and node ID. Generated IDs are derived from that hash.
Plans contain no executable commands, and no data-driven codec/provider is enabled.

## Verification

- tests/test_graph_expand.py: deterministic previews, metadata/wire/defaults, Undo,
  passthrough/fanout, eager/lazy guards including nested macros and UV branches,
  local contexts, named/container types, actual material shader compilation,
  object reads, budgets, bounded invalid reports, official Core jobs and saves.
- --test-expand: isolated-host ImGui canvas selection, function/macro modal,
  preview/apply and real Ctrl+Z/Ctrl+Y, in EN/FR. No global input is injected.
- capture_expand.py: official Core caller/helper examples, full and partial GUI
  gesture captures and six themes. Partial previews are not full gesture runs.
- data/reports/expand-validation.json: exact binary/source hashes, suite/capture
  evidence and preservation of pre-existing assets/projects/preferences/layout.
