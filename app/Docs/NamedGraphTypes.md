Native 0.26: [TypeMigration](TypeMigration.md) adds explicit, reviewed enum/record
shape migration within one owner. The original scope/limits below describe this
component's earlier release; they do not remove that separate migration workflow.
The current catalogue has 173 editor operations / 194 tools including Core.

> Native 0.24 adds [GraphInterfaces](GraphInterfaces.md). The interface limitation below records the original 0.23 scope.

# Named graph data types — native 0.23.0

The logic editor now supports graph-local named enums and structures. They work
through the same native dispatcher, Undo, serialization and existing CyToolsCore
MCP/SDK. The source/default/fallback UI language remains English; French is selectable.
These types are local declarations inside .cyasset graphs, not standalone Unreal
assets, engine reflection or a claim of complete Blueprint compatibility.

## Try the interface

Open BlueprintExamples/Example_BlueprintTypes.cyasset. Its structure feeds an enum
switch; Run graph prints 88 on the Hostile branch. The other branch outputs are
available for wiring. Stats in My Graph shows a structured default with its enum
selector and numeric field. Source defaults stay separate from runtime values.

My Graph > Data types > New data type opens a native designer:

- Enumeration: add/remove member identifiers and choose a default member.
- Structure: add/remove named fields, choose their type and edit defaults with
  ordinary properties. Fields can use another declared enum or acyclic structure.
- Apply data type validates the whole graph before one Undo transaction. Edit
  preserves unknown declaration/field metadata. Existing types keep their identity;
  there is no silent rename or shape migration.

The type library offers literal/make/break/switch shortcuts. Add node search also
lists contextual named operations, including equality, field replacement, reroutes
and typed object field access. A structure field replacement returns a new value;
changing that value does not mutate an earlier literal or declaration.

Variable and function-signature type selectors include the local types. Values
use recursive field inspectors and enum dropdowns. Execution functions may use
named locals; pure functions receive values through their typed arguments.
Classes may declare named fields; the debugger edits their runtime values using
those same properties without changing asset defaults.

## Serialized contract and nominal identity

```json
{
  "types": {
    "enum.Faction": {
      "kind":"enum",
      "members":["Friendly","Hostile","Neutral"],
      "default":"Neutral"
    },
    "struct.Stats": {
      "kind":"struct",
      "fields": {
        "health":{"type":"float","default":100},
        "faction":{"type":"enum.Faction","default":"Neutral"}
      }
    }
  }
}
```

Enum values serialize as member strings; structure values serialize as JSON
objects. The declared type ID supplies identity. enum.Faction and enum.Other are
incompatible pins even if both contain the same strings. The same applies to two
structures with identical fields but different type IDs. There are no implicit
casts between nominal types or from strings/objects into named values.

IDs begin with enum. or struct.; names/members/fields are identifiers beginning
with a letter or underscore, followed by letters/digits/underscores, at most
64 bytes. Names are case-sensitive. An enum and structure may share the leaf name
because their full IDs differ. Member/field labels are user data, not translated UI.

All structure fields are required and additional fields reject. Enums only accept
declared members. Defaults and values are checked recursively. A library has at
most 64 types, an enum 64 members, a structure 32 fields, expansion depth eight
and 512 expanded entries. A resolved schema is at most 128 KiB and all resolved
library schemas at most 2 MiB, preventing enum/record expansion from amplifying
type discovery beyond the transport budget. Structure values are bounded to 64 KiB; the existing
1 MiB graph / node/link / function / runtime-object budgets remain in force.
Recursive structures and containers of named values are not implemented. Existing
primitive arrays/maps/sets may be fields. Named types are restricted to logic
assets; the material preview compiler retains its existing expression subset.

## Nodes and functions

The global registry contains templates; nodeDefinition resolves their pins using
the owning graph's type library. A node stores typeRef; struct.set also stores
field (authored through fieldRef in graph_node_add).

| Node | Behavior |
|---|---|
| typed.literal | Nominal enum/structure value |
| typed.get / typed.set | Declared variable access; setter has execution pins |
| struct.make / struct.break | Assemble fields / expose individual field outputs |
| struct.set | Return a copy with one selected field replaced |
| enum.equal / enum.to_string | Same-type comparison / member string |
| enum.switch | One execution output per member; execute the selected branch |
| route.typed | Preserve a nominal wire's type |
| object.get.typed / object.set.typed | Access an object field with the exact declared type |

Functions and macros keep references to the owning library. graphView injects a
read-only working context; serialized embedded function bodies must not duplicate
types/functions/classes. Runtime child frames inherit the root library; nested call descriptors resolve
against that library without duplicating it in each serialized body. Inline
macro defaults become scoped named literals. Source/trace mappings and shared
runtime budgets remain intact. A semantic type change makes an existing debug
session stale; restart it to use the new declarations.

GraphClipboard v1 includes the local type library. Paste merges identical
identities, rejects conflicting definitions and validates before commit. Pasting
into a function also transfers required root libraries/functions/classes; locals
remain local. The existing 512 KiB fragment limit applies. Copies transport local
declarations, not links to a global/shared type asset. Graph comparison includes
type-library changes as document properties.

## Edits and migrations

Changing a default does not overwrite valid existing values. Adding an enum member
keeps existing pins/values intact and adds its switch output. Removing a member,
field or type rejects if existing values, nodes, class fields, signatures or wires
would become invalid. Type deletion also checks dependencies between declarations.
Failures preserve graph data, revision and Undo history. Source metadata is retained.
Unapplied UI drafts tolerate a concurrently removed type: reload or choose a valid
type before applying; the source asset is not silently rewritten.

Structural changes to an in-use record can require an explicit migration; this
release does not automatically rename fields, fill new fields, rewrite IDs or
migrate unrelated SDKs. There is no definition-version registry, enum flags/bitmasks,
interface system, engine object/component binding, replication or game compiler.

## SDK / MCP

The catalogue now contains 164 editor operations / 185 tools including Core.

- graph_types_get(graphId): local declarations, value schemas, default values and
  supported type IDs. Existing graph_get supplies resolved nodeDefinitions.
- graph_type_define(graphId, typeId, definition): create/replace one declaration,
  validate all uses and commit atomically. Optional expectedRevision is supported.
- graph_type_remove(graphId, typeId): remove an unused declaration atomically.
- graph_variable_add accepts a built-in or declared nominal type ID.
- graph_node_add accepts typeRef and optional fieldRef; existing functionName,
  variableName and position semantics remain unchanged.

```json
{"graphId":"logic","typeId":"enum.Faction","definition":{"kind":"enum","members":["Friendly","Hostile","Neutral"],"default":"Neutral"}}
{"graphId":"logic","type":"typed.literal","typeRef":"enum.Faction","position":[100,100]}
{"graphId":"logic","type":"struct.set","typeRef":"struct.Stats","fieldRef":"health","position":[300,100]}
```

Use the existing official Core submit/wait path; no second server, authentication
layer, worker scheduler or file-provided code is introduced. Declarations and node
operations are native transactions. Extend the context-aware graphValueSchema,
graphDefaultValue/graphWidget and nodeDefinition seams when adding source features;
never put per-asset types into the process-global node registry.

## Verification

- tests/test_named_types.py verifies nominal wire rejection, enum switching/equality,
  record value semantics, nested validation, functions/macros, class/debug fields,
  depth/dependency guards, Undo, persistence and cross-asset function clipboard.
- --test-types injects only its own isolated ImGui input: enum/structure creation,
  record field edit and Undo/Redo. It never controls global desktop input.
- capture_types.py renders the example, property inspectors, both designers in
  EN/FR and properties in all six themes. Partial designer captures are labeled;
  full property captures include the gesture checks.
- data/reports/types-validation.json, types-tests.log, types-record-properties-*-gestures.json and
  types-captures.json record the actual validation. Other types-* logs cover
  regression suites. Historical release archives remain unchanged.

The game codec registry remains empty. Shared graph types are not a real engine
adapter or game format codec. See GameCodecs.md for target-specific verification.
