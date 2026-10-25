# Developer entry points

This is the native CyGameSDK editor foundation. See ../README.md for launch/build
instructions and Docs/EditorArchitecture.md for the extension API.

- native/Core.h: engine-independent state, adapter contract, commands and plugins.
- native/Editor.h: panel host, asset editor registry and viewport renderer callback.
- native/Graph.h: typed graph registry, serialization, validation and sandbox evaluation.
- tool.py: CyToolsCore factory and OS-local editor transport client.
- build_manifest.py: operation schemas shared with the native dispatcher.

The editor currently requires Windows. MCP/SDK uses CyToolsCore 0.10.1 with its
official MCP 1.30.0 transport. Native editing commands are short transactions;
heavy builds/imports/exports execute through the official Core ProcessWorker.

Use generation_config.py to validate/replay a saved operation configuration against
the same runtime. No project, asset or recipe loads executable code.

Version 0.7 adds Functions.cpp (typed pure function library), GraphBatch.cpp
(atomic authoring/clipboard commands), AssetReferences.cpp (bounded declared-reference
index), FunctionsView.cpp and ReferenceView.cpp. Controls and limits: Docs/Milestones.md.

Current release: 0.28.0. Read Docs/CompletionMilestones.md and Docs/Native3DAssets.md
for neutral model/animation editing, import/export, JSON assets and material image
sampling. Scene3D/ModelCommands and JsonDocuments keep authoring independent of
ImGui; MeshPreview/ModelView/JsonView supply the native tools. The default UI is
English, with French selectable. Real game codecs/runtime bindings remain adapter work.

Advanced graphs/LODs/collisions/retargeting: Docs/AdvancedAuthoring.md. Trusted game
codec providers: Docs/GameCodecs.md (no real game codec bundled). Source entries:
Blueprint.cpp, Macros.cpp, ModelAuthoring.cpp/ModelAuthoringView.cpp,
model_authoring.py, model_process_service.py and game_codecs.py.
Run tests/test_authoring.py with CYGAME_TEST_GUI=1 for all twelve focused scenarios.

0.22 animation tools: Docs/AnimationTools.md, animation_tools.py, native/MotionToolsView.cpp
and tests/test_animation_tools.py. These extend model_process; no new scheduler.

0.23 named graph types: Docs/NamedGraphTypes.md, native/TypeLibrary.cpp,
native/TypesView.cpp and tests/test_named_types.py. Existing Core runtime reused.

0.24 class interfaces: Docs/GraphInterfaces.md, native/Interfaces.cpp / InterfacesView.cpp,
tests/test_graph_interfaces.py and capture_interfaces.py.

0.25 signature migration: Docs/SignatureMigration.md, native/SignatureMigration.cpp /
SignatureMigrationView.cpp, tests/test_signature_migration.py and capture_migrations.py.

0.26 enum/structure migration: Docs/TypeMigration.md, native/TypeMigration.cpp,
tests/test_type_migration.py and capture_type_migrations.py. The native type designer
opens the shared review panel; explicit apply preserves Undo and uses existing Core.

0.27 Collapse selection: Docs/GraphCollapse.md, native/GraphCollapse.cpp /
GraphCollapseView.cpp, tests/test_graph_collapse.py and capture_collapse.py.
Extract supported pure regions into typed local functions/macros with preview/Undo.

0.28 Expand call: Docs/GraphExpand.md, native/GraphExpand.cpp / GraphExpandView.cpp,
tests/test_graph_expand.py and capture_expand.py. Pure local calls expand into
editable nodes with typed reroutes, retained definitions/defaults/provenance,
preview and one Undo. Execution calls reject.
