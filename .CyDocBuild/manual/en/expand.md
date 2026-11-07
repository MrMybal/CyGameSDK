---
id: expand
title: Expand a pure call into editable nodes
description: Expand a pure call into editable nodes
section: Graphs
order: 19
revision: 1
parent: graphs
---

## Preview one call {#preview}

Select one local pure `function.call` or `macro.call`, then **Expand call > Preview expansion > Apply expansion**. The definition remains available in My Graph. The example `Example_BlueprintExpand.cyasset` prints 9 before and after. No expansion runs on load and no file is saved implicitly.

Every interior node is cloned, including dormant nodes, with deterministic collision-checked IDs. Nested calls remain calls; there is no recursive unfolding. Values, comments, breakpoints, metadata and relative positions survive. Typed boundary reroutes preserve external wire IDs, fanout, hidden call defaults and destination fallbacks. A provenance comment retains original call JSON, `sourceHash` and function name. Removing that comment intentionally removes its provenance record.

## Evaluation guards {#guards}

Each connected pure-function output must read every wired argument, because functions eagerly resolve arguments. The analysis includes lazy nested macros and per-output UV dependencies; unsafe expansion is refused rather than hiding division-by-zero or invalid-object failures. Macros already inline lazily and preserve unreachable input reroutes. Unwired function outputs retain return-node fallbacks; macro outputs retain declaration defaults. Dormant calls with no connected output can expand without evaluating them.

## Apply and restore {#apply}

The candidate and runtime macro expansion are recomputed and validated. Supply the exact hash; do not edit the preview on the client. One Undo restores the original call and one Redo restores the expansion. Frame all / Home reveals nodes outside the current camera. Limits include 128 interior nodes plus up to 32 boundary reroutes; whole-owner budgets also apply. [Collapse](doc:collapse) extracts a pure region in the other direction.
