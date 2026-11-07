---
id: debugger
title: Debugger and graph comparison
description: Debugger and graph comparison
section: Graphs
order: 12
revision: 1
parent: graphs
---

## Follow a call {#stepping}

Start graph debugging, set breakpoints and inspect the trace and variables. F10 steps over, F11 enters and Shift+F11 leaves an execution function. Continue resumes to a boundary or breakpoint; Stop ends the session. Pure calls remain atomic. Watch values and frame arguments/locals in the selected call-stack context.

Edits that change semantics invalidate the paused program; the debugger becomes stale. Live breakpoint refresh is supported. Do not continue an old compiled/interpreted snapshot as if it represented the edited document. Inline macros preserve source mappings; object handles refer only to the current local runtime.

## Search and diagnostics {#navigation}

The Graph Navigator searches roots, functions, comments, values and variables. Per-view bookmarks and typed reroutes help navigation. Graph/shader diagnostics retain severity and can navigate to nodes. Legacy plain-text Core log entries are shown at info severity; their text is not a typed error code.

## Compare with saved content {#compare}

Use **Compare saved** to compare the active graph to its saved asset or session baseline. Filter semantic changes, optionally ignoring positions/comments; inspect split or unified details. Comparison is read-only and does not apply a merge, mutate Undo or silently reload an external edit. External disk changes are detected. Detail rendering is bounded while summary counts are retained; arbitrary Git revisions and visual merges are not implemented.

```image
{"id": "diff", "src": "assets/screenshots/diff.png", "alt": "Read-only comparison of live and saved graph entries", "controls": "minimal"}
```
