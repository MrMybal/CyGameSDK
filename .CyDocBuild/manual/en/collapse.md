---
id: collapse
title: Collapse a pure selection into a helper
description: Collapse a pure selection into a helper
section: Graphs
order: 18
revision: 1
parent: graphs
---

## Select a supported region {#selection}

Select a connected pure dependency region and choose **Collapse selection**, then preview a function or macro. Variables, boundaries and execution nodes stay outside. The helper must use every selected node and incoming wire for every exported output. Dependencies are checked per output pin, including independent UV branches. The operation never prunes a selected node or evaluates an unused external input merely to construct a helper.

The preview explains unsupported regions and loss/validity decisions. A candidate that could change eager/lazy evaluation or hide a failure is rejected. A generic-looking selection is not proof that it is collapsible.

## Preserve authored data {#apply}

Original node/link IDs, fallback values and unknown metadata are retained in the helper. Generated IDs are deterministic and checked for collisions. Apply requires the exact request/source hashes and validates the whole owner; source changes require a new preview. One Undo restores the original region. Save explicitly and use the existing runtime to check behavior. No game compilation or engine codec follows from extracting a function.

Use [expand](doc:expand) for the complementary operation on one pure call; it has different preservation and evaluation guards.
