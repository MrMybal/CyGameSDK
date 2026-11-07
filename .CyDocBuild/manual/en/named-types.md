---
id: named-types
title: Enums, records and nominal types
description: Enums, records and nominal types
section: Graphs
order: 13
revision: 1
parent: graphs
---

## Declare local types {#declare}

In My Graph, use **New data type** for an enum or structure. The library belongs to this graph asset; it is not a cross-project registry. Enum members and structure fields have stable nominal identity. Two enums with the same labels are not interchangeable merely because their text matches.

Defaults and nested record values are validated against the owning type context. Pin schemas are resolved with that context in root graphs, functions, macros, classes and runtime children. Literal, split/make/get/set and switch nodes use the named type. An in-use edit validates all uses atomically; invalid changes fail without silently resetting values.

## Containers and inheritance {#containers}

The interpreter includes its supported typed arrays/maps, classes and methods. Do not assume arbitrary nested container types or engine reflection. Runtime children inherit local libraries. Clipboard merges carry type definitions and reject conflicting identities.

Use the [explicit type migration](doc:type-migration) to rename enum members or record fields, add fields or accept losses. No migration runs on load. A type keeps its nominal ID and kind; replacing an enum with a structure is not a migration.
