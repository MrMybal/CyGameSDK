---
id: type-migration
title: Migrate enum and structure definitions
description: Migrate enum and structure definitions
section: Graphs
order: 17
revision: 1
parent: graphs
---

## Change one local type {#preview}

Choose one local enum or structure, retaining its nominal ID and kind. Preview an explicit member/field rename, addition, removal or supported field-type/default change. Dependent declaration defaults are normalized before resolving node descriptors. Uses across the owning graph, function/macro bodies, classes and interface signatures are checked as a closure.

A removed field referenced by `struct.set` blocks the plan instead of deleting behavior. Other removed/reset values and connections are loss reports, not silent repairs. Apply recomputes and validates the candidate, requires the exact `planHash`, the original request and explicit `acceptLoss` when needed. A changed owner or request invalidates the plan.

## Restore and scope {#undo}

A successful apply is one whole-owner Undo. Retain unknown metadata and nominal identity; do not replace an enum with a structure. There is no migration on load, across assets or into live runtime objects. Save the edited asset explicitly. SDK contracts expose the same guarded plan/apply workflow; discover exact names and fields in [operations](doc:operations).
