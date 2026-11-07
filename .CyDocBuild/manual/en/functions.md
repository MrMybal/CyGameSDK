---
id: functions
title: Functions and macros
description: Functions and macros
section: Graphs
order: 11
revision: 1
parent: graphs
---

## Local definitions {#definitions}

Functions belong to their graph asset. Use My Graph to define a function, its typed inputs/outputs, defaults and local variables, then open its subgraph tab. Pure mode is the default when a saved definition has no mode. Execution mode exposes exec/then boundaries and explicit return flow. Inspect the resolved descriptor: a dynamic call is not fully described by the static node catalogue.

Inputs, outputs and local values are validated. Nested calls share budgets, use isolated locals and cache execution outputs. A path that fails to return is an error. Recursion, including possible interface dispatch targets, is rejected. The debugger keeps context-qualified node identities and a call stack; a node ID alone is not a global address.

## Pure versus execution {#modes}

Pure functions evaluate arguments eagerly. An unused connected argument can still fail. Macros expand inline and lazily, in scoped names, with source debugger mappings. Execution functions must be reached by control flow. Do not turn an execution call into a pure call merely to allow expansion.

Inline macro bodies and function children inherit the owner's nominal type and interface libraries. Do not serialize duplicate owner libraries into every function body. Clipboard merges check identities and all uses atomically.

## Reuse and change {#changes}

Change signatures with [preview/apply migration](doc:signature-migration). Use [collapse](doc:collapse) only for supported pure dependency regions. Use [expand](doc:expand) for a supported pure call while retaining its definition. These operations do not save files implicitly or compile game code.
