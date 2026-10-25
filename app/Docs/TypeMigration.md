# Enum and structure migration — native 0.26

This component extends the local nominal contracts in [NamedGraphTypes](NamedGraphTypes.md).
It provides explicit, reviewable shape changes inside one logic owner. It preserves
the type ID and kind. It does not migrate external assets, live runtime objects,
engine reflection data or proprietary game files.

## Native workflow

1. In the Scripting workspace, open My Blueprint > Data types and edit an existing
   enum or structure. Editing a new type still uses the strict Apply data type flow.
2. Rename enum members in their fields. For a structure, select Rename structure
   field, choose the field/new identifier, then Rename draft field. These controls
   record explicit old-to-new mappings. Draft edits do not change the live owner.
3. Choose Preview type migration. The docked Type Migration panel lists affected
   declarations, node/value counts and connection changes, with expandable details.
4. Review the report. Removed/reset values or removed connections require Accept
   the listed losses. Invalid candidates cannot apply. A graph edit after preview
   requires Refresh migration preview.
5. Apply migration commits the entire owner as one Undo step. Ctrl+Z/Ctrl+Y restores
   the complete old/new graph. Save asset remains a separate explicit operation.

Apply data type retains its original strict validation behavior. It is useful for
changes that keep all current uses valid; it does not perform this migration.
After applying, create a fresh preview for another change. The applied report is a
historical record, including after Undo. English is default; French is selectable.

The example BlueprintExamples/Example_BlueprintTypeMigration.cyasset starts with
Record={health:9, team:Hostile}. Run prints 9. Rename health to hp, add armor with
float default 5, then rename enum.Team's Hostile member to Enemy. Migrated values
are {hp:9, team:Enemy, armor:5}; compatible branch/data wires retain their IDs and
Run still prints 9. Source fixtures are not rewritten by QA.

## Conversion rules

The request supplies a full member/field definition and an optional flat rename map.
Mappings are injective, including implicit same-name survivors. Swaps are supported;
unknown endpoints and collisions reject. No guessed correspondence is supplied.

- Surviving structure fields retain their authored values and recursively migrate
  nested records/enums. Added fields receive the requested default.
- Changing a field's nominal or primitive type resets its value to the requested
  default and reports a loss. There are no implicit casts, even if two enum types
  contain the same JSON string or two records have identical fields.
- Removed fields/values are reported. Removing an enum member resets its occurrences
  to the new enum default unless an explicit rename preserves them.
- Changing only a declaration default does not replace existing authored values.
- Unknown type metadata and surviving field metadata are merged with the requested
  definition. Requested values take precedence. Removed declarations necessarily
  remove their metadata, visible in the declaration comparison.
- Make structure input pins/fallback keys, Break structure output pins and Switch
  on enum execution branches follow the rename map. Set structure field bindings
  follow field renames. Removing a field still referenced by a Set structure field
  node blocks the candidate; remove/rebind that node first.
- Compatible wires retain IDs and unknown metadata. Missing or incompatible pin
  endpoints remove the wire and report a loss. Node IDs, positions, comments,
  breakpoints and unrelated document metadata remain unchanged.
- Untyped strings/JSON are not interpreted as records or enum members.

The planner visits dependent structure defaults, root/function variable defaults,
class field defaults, interface/function/macro port defaults and typed node fallbacks,
including function/macro/interface calls. All declaration defaults are normalized
before resolving new node descriptors. An internal neutral schema checks shape,
cycles and expansion first; authored defaults are never replaced with that neutral
schema. Function views inherit libraries without serializing duplicate libraries.

A successful semantic edit marks the current debugger program Stale. Restart it
before execution; existing transient runtime objects are not converted in place.
Preview alone changes neither debugger, graph, revision nor Undo. Bound method and
interface signatures keep their nominal types; their nested defaults migrate.

## SDK / MCP contract

The active catalogue has 173 editor operations / 194 tools including Core controls.
Use the existing CyToolsCore authentication, submit and wait path. Both operations
use the same native implementation as the UI:

```json
{
  "graphId":"asset:BlueprintExamples/Example_BlueprintTypeMigration.cyasset",
  "typeId":"struct.Stats",
  "definition":{
    "kind":"struct",
    "fields":{
      "hp":{"type":"float","default":1},
      "team":{"type":"enum.Team","default":"Friendly"},
      "armor":{"type":"float","default":5}
    }
  },
  "renames":{"health":"hp"}
}
```

1. graph_type_migration_plan(request) returns valid, sourceHash, planHash, targets,
   summary, changes, changeCount, truncated and requiresLossAcceptance. Malformed
   types/maps may reject immediately. A candidate that fails final conversion or
   validation returns valid=false and diagnostic.
2. graph_type_migration_apply(same request, planHash, acceptLoss=false) recomputes
   the candidate, verifies the exact source/request fingerprint and accepts only
   valid, explicitly acknowledged changes. Optional expectedRevision also applies.
   It commits one whole-owner Undo transaction; an unchanged no-op creates none.

The caller cannot install a supplied candidate graph. A changed source (including
layout/metadata) or altered request invalidates the hash. There is no automatic
migration on load, type ID/kind conversion, version registry or multi-owner commit.
Type removal remains a strict separate operation and requires no remaining use.

summary counts declarations, affected nodes, renamed/removed links and preserved,
added, removed, reset and migrated values. Counts describe visited occurrences;
a nested converted record can count alongside its converted children. All changes
contribute to the summary, even when detail output is truncated. Details contain
at most 512 entries / 256 KiB. Old values larger than 1 KiB use a UTF-8-safe preview
and original byte count; retrieve the source owner for complete values.

Existing limits remain: 64 local types, 1..64 enum members, 1..32 structure fields,
acyclic nesting up to eight levels / 512 expanded entries, 128 KiB per expanded
schema / 2 MiB for library schemas, 64 KiB per record value and 1 MiB per owner.
Graph/node/function and interpreter budgets are unchanged. An oversized propagated
parent default or value invalidates the candidate before commit.

## Verification and boundaries

- tests/test_type_migration.py covers nested/default propagation, enum swaps,
  nominal type resets, make/break/set references, switch branches, interface/class/
  function/macro contexts, loss acceptance, cycles/maps, stale requests, metadata,
  bounded expansion/details, no-op/optional sections, persistence, debugger Stale,
  official Core jobs and whole-owner Undo/Redo.
- --test-type-migration seeds previews through native APIs, then uses only its own
  isolated ImGui input for Apply, loss acceptance and Ctrl+Z/Ctrl+Y. EN/FR reports
  each contain twelve checks. It does not control the global desktop or automate
  every draft field control.
- capture_type_migrations.py captures two full gesture runs, record/enum previews
  in EN/FR, enum previews in all six themes and the example after official Core
  plan/apply jobs. Partial preview frames are identified separately from full runs.
- data/reports/type-migrations-validation.json records the exact native/source
  hashes, regression results, captures and original data/preferences preservation.
  Root AGENTS.md and AI_GUIDE.md are included in the new source checkpoint.

These are editor interpreter contracts. Cross-asset registries, live-object
migration, engine compilation/reflection, full Unreal Blueprint parity and real
game codecs require separate work and verification against an actual target.
