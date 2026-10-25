> Native 0.25 adds explicit [SignatureMigration](SignatureMigration.md), including linked interface implementations. The automatic-migration limitations below describe the original 0.24 scope; migration on load remains unavailable.

# Local class interfaces — native 0.24.0

The logic editor supports graph-local interface declarations. A declaration names
pure or execution method signatures; local classes bind ordinary functions to
those methods. Calls resolve the concrete receiver's implementation. This extends
the existing native graph interpreter, Undo, debugger and CyToolsCore MCP/SDK.
English remains the source/default/fallback UI language; French is selectable.

These are local simulation contracts. They do not provide Unreal Blueprint file
compatibility, engine object reflection, an engine ABI or a real game codec.

## Try the delivered interface

Open BlueprintExamples/Example_BlueprintInterfaces.cyasset and choose Run graph.
It constructs Boss, checks Damageable, dispatches Read to BossRead and prints 210:
health 100 + the contract's bonus 5, multiplied by 2. Boss inherits Damageable
from Crate and overrides its method. Selecting Crate on the construction node
instead prints 45. Interface defaults are used by interface call nodes; the
implementation function's different default bonus does not replace them.

My Graph > Interfaces > New interface opens a native modal:

- Give the interface an identifier. Execute starts as an execution method.
- Add/remove methods, choose pure/execution, and add input/output ports.
- Port types include the graph's named enums and structures. Edit defaults with
  the same recursive property widgets used by variables and functions.
- A pure method requires an output. self is an implicit object input and is reserved.
- Apply interface validates every use and creates one document Undo transaction.
  Editing preserves the definition's unknown metadata and keeps its identity.

In Classes > Define class/Edit, select Implemented interfaces and use Interface
method bindings to choose functions with compatible signatures. A method function
must have self:object plus the declared inputs, exact port names/types, the same
mode and exact outputs. Existing JSON fields/method declarations remain available.
Inherited contracts are shown as inherited; bindings can use the parent or a
compatible override. Missing bindings fail validation before the class is changed.

The interface list offers Check interface and Call interface shortcuts. Add node
search includes contextual calls and stable interface.call/interface.supports IDs.
Execution calls are unavailable in pure functions. Calls in functions/macros share
the owning library. Save asset persists edits; runtime object changes do not change
declaration defaults. The debugger steps into the actual implementation function.

## Persistence contract

```json
{
  "interfaces": {
    "Damageable": {
      "methods": {
        "Read": {
          "mode":"pure",
          "inputs":{"bonus":{"type":"float","default":5}},
          "outputs":{"result":{"type":"float","default":0}}
        }
      },
      "metadata":{"source":"authored"}
    }
  },
  "classes": {
    "Crate": {
      "parent":"",
      "interfaces":["Damageable"],
      "fields":{"health":{"type":"float","default":40}},
      "methods":{"Read":"ReadHealth"}
    }
  }
}
```

ReadHealth is an ordinary local function with inputs self:object, bonus:float
and output result:float. Its body uses the existing function format. Interfaces
contain signatures, not executable bodies or defaults for missing implementations.

Interface and method names are case-sensitive ASCII identifiers of at most 64
bytes. The library permits 32 interfaces, one to 16 methods each, up to 15 explicit
inputs and 16 outputs per method. Expanded port schemas across the library are
bounded to 2 MiB; the existing 1 MiB graph, named-value and runtime budgets remain.
A class implements at most 32 distinct interfaces including inherited declarations.
There is no interface inheritance or standalone shared interface asset registry.

A class inherits its parent's contracts and resolved bindings. An unrelated class
may implement the same interface. Multiple interfaces can share a method name
when a single binding satisfies all of their signatures. Pin identity remains
nominal: two separately declared record/enum types are incompatible even when
their shapes match. Interface signatures reserve self; execution signatures also
reserve exec/then. Unknown metadata is retained by the UI, persistence and clipboard.

The source graph stores the library once at its owner. Function views inject it
for authoring/validation; serialized embedded function bodies must not duplicate
interfaces/functions/classes/types. Runtime children inherit the library and share
the existing object pool, logical clock, evaluator budget and debugger call stack.

GraphClipboard v1 transports interfaces alongside types/classes/functions. Paste
merges identical identities and rejects conflicting declarations atomically.
Pasting into a function updates the owner's libraries and keeps its locals local.
Incompatible declaration changes or deletion of an in-use contract fail without
changing revision, source values or Undo. Automatic signature/asset migration is
not implemented. Function rename updates class bindings; contract identity stays.

## Nodes and dispatch

| Node | Contract |
|---|---|
| interface.supports | interfaceRef selects the contract; self:object input, value:bool output. Invalid/destroyed handles return false. |
| interface.call | interfaceRef/methodRef select a signature. self plus declared arguments become inputs; declared outputs retain their nominal types. Execution methods add exec/then. |

A call requires a live object implementing the interface. An invalid/destroyed
receiver or a missing implementation fails explicitly; no silent message fallback
is supplied. Execution outputs require the call to have executed. The interpreter
uses the concrete class's binding and the interface call's argument values.
Latent execution implementations suspend/resume through graph_debug_tick; they
do not block the UI or introduce another scheduler.

Dynamic recursion checks conservatively include every current implementing class,
not just objects currently constructed by a wire. A possible recursive cycle
rejects even when that receiver is unreachable in a particular run. Existing
16-call depth, expression/step limits and single-lane event semantics remain.
Semantic interface changes invalidate the active debugger program.

## MCP / SDK

The catalogue has 167 editor operations / 188 tools including Core controls.
All operations use the existing official Core authentication/submit/wait path and
native dispatcher; there is no replacement MCP server or job scheduler.

- graph_interfaces_get(graphId): raw local declarations.
- graph_interface_define(graphId, name, definition): create/replace a declaration
  and validate the whole owner atomically; optional expectedRevision.
- graph_interface_remove(graphId, name): delete an unused declaration atomically.
- graph_class_define accepts optional interfaces. Omitting it preserves established
  contracts for existing source clients; an explicit empty list removes direct
  declarations only when the graph stays valid. Inherited contracts remain.
- graph_node_add accepts interfaceRef and methodRef; functionName selects an
  authoring body. Nodes persist interface/method fields, not a fixed function target.

```json
{"graphId":"logic","name":"Damageable","definition":{"methods":{"Read":{"mode":"pure","inputs":{"bonus":{"type":"float","default":5}},"outputs":{"result":{"type":"float","default":0}}}}}}
{"graphId":"logic","type":"interface.call","interfaceRef":"Damageable","methodRef":"Read","position":[260,250]}
{"graphId":"logic","type":"interface.supports","interfaceRef":"Damageable","position":[20,440]}
```

## Verification and remaining scope

- tests/test_graph_interfaces.py checks unrelated/inherited dispatch, contract
  defaults, pure/execution modes, invalid/destroyed receivers, typed declarations,
  atomic failure/Undo, dynamic recursion, nested functions/macros, clipboard into
  functions, persistence/metadata, debugger stepping/invalidation, latent mutation,
  incompatible contracts, rename and official Core discovery.
- --test-interfaces runs real creation/signature/Undo gestures using only its own
  isolated ImGui input. English/French reports contain eight checks each.
- capture_interfaces.py captures the example, full gesture runs, new/edit modals
  in EN/FR and the signature modal in all six themes. Partial frames are identified.
- data/reports/interfaces-validation.json records exact executable/source hashes,
  full regression counts, capture evidence and preservation checks. Historical
  release archives stay unchanged. No ordinary editor is left open by QA.

Shared interface assets, interface inheritance, automatic signature migration,
engine components/reflection/replication, asynchronous execution lanes, game graph
compilation/JIT and Unreal compatibility remain separate work. The real game codec
registry remains empty; choose a real target and verify its outputs before claiming
compatibility. Read GameCodecs.md and the game porting guides for that integration.
