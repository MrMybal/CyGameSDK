---
id: interfaces
title: Classes, interfaces and runtime objects
description: Classes, interfaces and runtime objects
section: Graphs
order: 14
revision: 1
parent: graphs
---

## Bind a contract {#contracts}

Interfaces are local named contracts with nominal method signatures. Define the contract in My Graph, then bind a class implementation whose methods satisfy it. Signature changes must keep all bindings valid. Interface calls dispatch through shared runtime objects, not copies of disconnected object state.

Function views, inline macros, clipboard merges and runtime children carry the owner interface library. Serialized function bodies do not duplicate it. Recursion checks consider every possible implementation, including dynamic interface dispatch. An operation that would break a binding fails atomically.

## Local objects {#objects}

Classes declare fields, methods and supported inheritance. Object creation/get/set/call nodes operate on transient handles in the bounded graph interpreter. Inspect live object fields with the runtime debugger; SDK edits use the same type and budget guards. Handles are not persistent asset IDs, engine pointers or multiplayer identities.

Foreach item/index/break contracts, logical simulation time, delay and event dispatch belong to this interpreter. They are distinct from CyToolsCore job scheduling. There is no engine component reflection, replication, game physics binding or JIT compiler.

For linked method changes, use [signature migration](doc:signature-migration). Keep `self`, call mode and foreach contracts unchanged.
