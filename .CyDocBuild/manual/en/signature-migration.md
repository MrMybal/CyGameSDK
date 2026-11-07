---
id: signature-migration
title: Migrate function and method signatures
description: Migrate function and method signatures
section: Graphs
order: 16
revision: 1
parent: graphs
---

## Preview before applying {#preview}

Open the signature migration action on a function, macro or supported method in one owning graph. Describe the requested inputs/outputs, defaults and renames. Preview reports the closure of affected definitions and call sites, including interface contracts, method overrides and shared bindings. It does not mutate the document.

The candidate is recomputed from the request, validated and identified by an exact `planHash`. Apply must supply that hash and the same request. Revision/source changes or a tampered plan reject the edit. Inspect removed connections and reset fallbacks. If preview reports loss, explicit `acceptLoss` is required; a hash alone is not consent to lose data.

## What remains stable {#preservation}

One whole-owner Undo restores the previous graph. Unknown metadata is retained. Never change kind, pure/execution mode, `self`, or foreach callback item/index/break contracts through this migration. No implicit migration occurs on asset load, and no external asset registry or live engine object is modified.

SDK: `graph_signature_plan` and `graph_signature_apply` are described by the operation catalogue. Use discovery for exact required fields rather than constructing an edited plan client-side. [Operation reference](doc:operations).
