---
id: runtime-objects
title: Runtime time, events and objects
description: Runtime time, events and objects
section: Graphs
order: 15
revision: 1
parent: graphs
---

## Advance a simulation {#time}

A delay suspends a logical execution lane until graph simulation time advances. Use debugger tick/event controls or the matching `graph_debug_tick` and `graph_debug_event` operations. This is not a wall-clock job scheduler. Event delivery, lane count and execution steps remain bounded; local objects share the same runtime state across calls.

Inspect object fields with `graph_debug_status` and edit supported values with `graph_debug_object_set`. Use current handles and schema types; restarting or invalidating a runtime makes old handles unusable. Macro expansion preserves a source mapping for debugger navigation.

## Keep ownership clear {#ownership}

Core jobs run imports, material compilation and build providers outside the UI. Their identities, cancellation, sessions and output lifetimes belong to CyToolsCore. [Worker jobs](doc:jobs) describe that separate lifecycle. Runtime variables and object field edits do not silently rewrite graph declarations or saved source assets.
