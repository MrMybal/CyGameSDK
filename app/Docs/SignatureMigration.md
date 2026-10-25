# Explicit graph signature migration — native 0.25.0

Native 0.26: [TypeMigration](TypeMigration.md) adds explicit, reviewed enum/record
shape migration within one owner. The original scope/limits below describe this
component's earlier release; they do not remove that separate migration workflow.
The current catalogue has 173 editor operations / 194 tools including Core.

This release adds a read-only preview and explicit apply for signatures in one
loaded graph asset. It updates function/macro boundaries and calls, compatible
fallback values and wires. Linked interface methods, implementations and class
overrides migrate together. English remains the default/source/fallback language;
French is selectable. CyToolsCore remains the MCP/AI SDK and job implementation.

Migration never runs when an asset is opened. It does not migrate enum/structure
shapes, arbitrary game data or files in another SDK. A menu or operation here does
not provide a real engine compiler, renderer, codec or full Unreal compatibility.

## Try the interface

Open BlueprintExamples/Example_BlueprintMigration.cyasset. Run graph prints 9.
Choose Echo in My Graph. In its signature, Port renames lets you rename input x
to data and output result to answer in the draft. Choose Preview migration.

The Signature Migration panel lists affected declarations, values/connections and
expandable details. Apply migration preserves the call's authored 9 and the
connections; Run graph still prints 9. One Edit > Undo restores the complete old
signature, calls and wires. Edit > Redo restores the migration.

Adding a parameter installs its new default on old call nodes. Removing a
connected parameter or changing its type can remove a wire or fallback value.
Those losses appear in the preview, and Apply stays disabled until Accept the
listed losses is selected. Unsupported final graphs remain blocked even when
losses are accepted. Changing the graph after preview requires a fresh preview.

Interface definition modals provide Port renames and Preview method migration
for an existing method. This previews that method and its linked implementations,
not unrelated unapplied edits elsewhere in the modal. A new method or changed
execution mode uses the original strict editing workflow, not signature migration.

Existing Apply signature / Apply interface commands remain available and retain
their previous strict validation behavior. Use the migration path when existing
calls or wires must be renamed. Draft port renames do not edit the live document.
After applying, create a new preview for another change; the applied report is a
historical record, including when an Undo subsequently restores the source.

## Scope and propagation

A migration has target=function (also used for macros) or target=interface plus a
method name. It keeps declaration identities, function kind and execution mode.

The planner follows signature-equivalence relationships:

- An interface method and every resolved implementing function.
- Parent method bindings and derived overrides with that method identity.
- Other interface methods sharing one of those same function bindings.

The complete connected group is shown before applying, including implementations
on unrelated classes. Method names and method-to-function bindings remain unchanged.
A class's implicit receiver must keep self:object. Standalone pure/material
functions and execution functions are supported within their existing domains.

The planner visits the root and every local function. It rewrites function input/
output boundary pins, ordinary calls, macro calls, bound method calls and interface
calls. Foreach callbacks can migrate extra arguments; item/index names and types
and an existing break output's name remain fixed. Other invalid callback changes
are blocked by final validation. Runtime loop control semantics are not changed.

Only the selected owner asset is affected. External assets and files on disk are
not rewritten; Save asset remains explicit. There is no cross-project registry,
automatic signature version history or migration on load. Function views still
inject shared libraries without duplicating them into serialized function bodies.

## Values, metadata and losses

Renames are explicit maps from old names to declared destination names per
input/output direction. Unmapped names are kept if still declared, otherwise
removed. Maps are injective; swaps are supported, collisions and unknown endpoints
reject. No guessed name matching or implicit type conversions are supplied.

- A surviving node fallback keeps its value only when the declared pin type is
  identical and the value still validates. This includes exact named enum/record
  identity, not merely equal JSON shape.
- New node inputs receive the new declaration default.
- Removed input fallbacks are listed. Type changes reset them to the new default
  and list the old value, even when the serialized value looks compatible.
- Wires are renamed first, then retained only when both endpoints still exist
  and have the same type. Removed/incompatible wires are listed as losses.
- The primary declaration receives the requested defaults. Linked declarations
  preserve their own defaults and unknown port metadata when the type survives;
  new or changed types use the requested default/schema. Interface defaults can
  intentionally differ from implementation function defaults.
- Existing call fallbacks remain authored values even when they originally equaled
  the former default. Changing a declaration default does not overwrite them.
- Declaration changes, including default/schema changes, are shown as before/after
  entries. Loss acceptance specifically covers removed/reset node fallbacks and
  removed wires; choosing a new declaration default is part of the explicit request.

Node/link IDs, positions, comments, breakpoints, function bodies, locals, root
metadata and unknown surviving fields are preserved. New port specifications
merge with the surviving old port's metadata. Removing a port necessarily removes
its declaration/metadata and is visible in the declaration comparison.

The planner validates the complete candidate owner before applying. Existing
nominal, domain, class/interface, recursion, node/link/value and graph size checks
remain in force. Semantic changes invalidate an active debugger program. A preview
alone does not change revision, Undo, the debugger or the source graph.

## SDK / MCP

The current catalogue contains 173 editor operations / 194 tools including Core controls.
Use the existing official Core authentication/submit/wait path.

Both operations take graphId, target, name, inputs, outputs and optional renames.
Interface targets also take method and omit implicit self from their input schema.
Each port contains type/default and can retain additional editor metadata.

```json
{
  "graphId":"asset:BlueprintExamples/Example_BlueprintMigration.cyasset",
  "target":"function",
  "name":"Echo",
  "inputs":{"data":{"type":"float","default":2}},
  "outputs":{"answer":{"type":"float","default":0}},
  "renames":{"inputs":{"x":"data"},"outputs":{"result":"answer"}}
}
```

1. graph_signature_plan(request) returns valid, sourceHash, planHash, targets,
   summary, changes, changeCount, truncated and requiresLossAcceptance. A candidate
   that fails final validation returns valid=false with diagnostic; malformed
   selectors/maps/signatures can reject immediately.
2. graph_signature_apply(same request, planHash, acceptLoss=false) recomputes the
   candidate and compares the exact source/request fingerprint. It rejects a stale
   or altered request, an invalid candidate or unaccepted losses, then commits the
   whole owner in one Undo transaction. Optional expectedRevision is also supported.

No candidate graph supplied by a caller is trusted or installed. A forged/old hash
cannot authorize different inputs, outputs or mappings. A source change, including
layout/metadata changes, invalidates the preview. Applying an unchanged no-op does
not create an Undo entry or increment revision.

summary counts declarations, affected nodes, renamed/removed links, preserved/
added/removed/reset node fallback values. It includes every change. Details contain
at most 512 entries / 256 KiB, with truncated=true when limited. Old values larger
than 1 KiB have a UTF-8-safe short preview and original byte count; retrieve the
source graph to inspect full data. The existing 1 MiB owner, 32-function, 2,000-node
aggregate and per-body/per-signature budgets remain. Propagation does not expand
these limits or allow recursive calls.

## Verification

- tests/test_signature_migration.py verifies lossless renames, default additions,
  explicit loss handling, stale/changed request rejection, macro/nested/material
  contexts, interface/override/shared-binding propagation, fixed callback contracts,
  compatible callback extra arguments, swaps, nominal type changes, bounded details,
  persistence/metadata, no-op behavior, Undo and official Core jobs.
- --test-migration uses native API setup for previews and only its own isolated
  ImGui input for Apply, loss acceptance, Ctrl+Z and Ctrl+Y. EN/FR gesture reports
  contain twelve checks each. It does not control the user's global desktop.
- capture_migrations.py captures full gesture runs, lossless/loss previews in
  EN/FR, loss previews in all six themes, and the supplied example after official
  Core plan/apply jobs. Partial frames are identified separately from full runs.
- data/reports/migrations-validation.json records exact executable/source hashes,
  regression counts, native captures and preservation checks. Root AGENTS.md and
  AI_GUIDE.md ship in the source checkpoint. Historical releases remain unchanged.

Cross-asset schema migration, enum/record shape conversion, execution-mode
conversion, method addition/removal and engine-specific bindings remain separate
work. Read the game adapter and codec guides before integrating a real target.
