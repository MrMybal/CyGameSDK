---
id: graphs
title: Logic graphs and the canvas
description: Logic graphs and the canvas
section: Graphs
order: 10
revision: 1
---

## Author a graph {#canvas}

Switch to Scripting or open a `.cyasset` graph. **Add node** creates a node at the requested canvas location. Middle-drag pans, the wheel zooms, and Frame all / Home returns the authored region to view. Connect compatible output/input pins by dragging, or click the two pins. Direction, type, domain, occupied inputs and cycles are validated.

Select or marquee nodes, move a group, align it, duplicate it, add reroutes or create a comment group. A completed gesture creates a coherent Undo entry. Presentation changes such as panning are not document edits. Clipboard merges preserve libraries and reject incompatible nominal identities instead of silently converting values.

## Execute and inspect {#run}

The logic interpreter supports events, branch/sequence, arithmetic, comparison, conversions and variables. It is bounded by type, node and execution budgets. Runtime variable changes do not overwrite declaration defaults. **Check graph** reports structural diagnostics; **Run graph** executes supported logic. Saving a structural draft does not prove it can run.

A division by zero, missing object or incompatible pin is an error. There is no binding to arbitrary engine memory. Read [functions and execution](doc:functions), [runtime objects](doc:runtime-objects) and the [debugger](doc:debugger).

## Understand a connection {#connections}

This interactive documentation example explains the initial event-to-print connection. You can pan, zoom and select nodes here; it is an illustration, not the editor runtime.

```graph
{"id": "event-flow", "nodes": [{"id": "begin", "title": "Begin play", "x": 0, "y": 0, "outputs": [{"id": "then", "label": "then", "type": "exec"}], "description": "Entry event for this illustrative execution flow."}, {"id": "print", "title": "Print string", "x": 330, "y": 0, "inputs": [{"id": "exec", "label": "exec", "type": "exec"}, {"id": "text", "label": "text", "type": "string"}], "description": "Receives execution and a text value. This documentation view does not execute the graph."}], "links": [{"from": ["begin", "then"], "to": ["print", "exec"]}]}
```

## Refactor safely {#refactor}

[Signature migration](doc:signature-migration), [type migration](doc:type-migration), [collapse](doc:collapse) and [expand](doc:expand) are explicit preview/apply operations on one owning graph. They do not migrate an asset automatically on load. Preserve exact hashes, revision guards and one Undo.

```image
{"id": "logic", "src": "assets/screenshots/logic.png", "alt": "Execution function with typed connections", "controls": "minimal"}
```
