# CyGameSDK 0.30.0

[English](Release-0.30.0.md) · [Français](Release-0.30.0.fr.md) · [README](../../README.md)

## Status and availability

**Development preview — local source update.** This update adopts the newer CyUI
editor APIs. It is not a complete Unreal editor or automatic support for game files.

The existing GitHub download is still the source-only
[v0.29.0 archive](https://github.com/MrMybal/CyGameSDK/releases/tag/v0.29.0).
That archive does not include these changes. No 0.30.0 release, installer or
GitHub publication is created by this local update.

## Editor changes

- Shared typed Inspector for scalars, vectors, records, arrays, Sets and Maps,
  including numeric expressions and virtual collection rows.
- Common World Outliner hierarchy and adapter-driven property table, with sorting,
  filters, typed TSV and atomic multi-cell changes.
- Native source picker using cached asynchronous directory listings and existing
  import validation.
- Common CodeEditor for operation/class/codec JSON and read-only character selection;
  shared code font, syntax colors, search and per-view zoom.
- Common command metadata for menus, toolbar and keyboard, with editable bindings.
- Native comment-group move/resize/rename/removal, including contained node motion,
  one Undo, Escape and stale-revision cancellation.
- Domain graph clipboard retained: types, values, local declarations and comments
  travel through the editor's semantic fragment contract.

CyUI still identifies as 1.2.0 upstream; this update pins exact revision
`1bdba05fac030bb5231797dcbe59e143c0f1d213`. One ImGui instance and explicit
module targets remain. The C++20 text backend is isolated behind a small bridge;
the existing host and domain code remain C++17.

The catalogue adds `property_set_many` and `graph_layout_apply`, bringing it to
177 editor operations / 198 tools including Core controls. These validate the
complete request and create one Undo. CyToolsCore remains the MCP/SDK/job system.

## Build and help

Follow the current [README build steps](../../README.md#source-build-and-required-libraries).
Windows x64 / Direct3D 11 / MSVC remains the native host. The complete bilingual
CyDoc manual ships beside the executable and opens locally with Help/F1.

Keep the TextEditor MIT and JetBrains Mono SIL OFL notices with the existing
component notices. No system font installation is required.

## Boundaries and known issues

The game adapter still supplies real formats, engine rendering and toolchains.
No real game codec or full Blueprint compatibility is bundled. Existing model,
animation and graph budgets remain authoritative.

The generic CyUI graph Paste/Duplicate allocator is not enabled for runtime graphs;
use the existing semantic toolbar/keyboard path. Typed execution reroutes remain
application nodes. Timeline key authoring and game-specific animation semantics
remain host adaptations. Audio, browser/CEF, Office and document-authoring modules
are not included merely because upstream provides them.

Read [CyUI integration](CyUIMigration.md) for the selected modules and remaining
API gaps, and the [issue tracker](https://github.com/MrMybal/CyGameSDK/issues)
for reported bugs. Native Linux/macOS hosts remain unavailable.
