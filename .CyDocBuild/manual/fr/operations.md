---
id: operations
title: Référence des opérations du SDK natif
description: Référence des opérations du SDK natif
section: SDK
order: 51
sourceRevision: 1
parent: automation
---

## Utiliser le catalogue réel {#catalogue}

Les pages ci-dessous proviennent du CyTool.json distribué. Elles conservent les schémas exacts, champs requis/défauts et indicateurs déclarés d’asynchronisme, annulation et historique. Elles décrivent les opérations natives ; les fournisseurs de jeux demandent une implémentation réelle. MCP expose ces IDs via le transport Core existant.

## `editor` {#editor}

- [`editor_status`](doc:op-editor-status-operation)
- [`editor_close`](doc:op-editor-close-operation)
- [`editor_search`](doc:op-editor-search-operation)

## `scene` {#scene}

- [`scene_list`](doc:op-scene-list-operation)
- [`scene_select`](doc:op-scene-select-operation)
- [`scene_create`](doc:op-scene-create-operation)
- [`scene_delete`](doc:op-scene-delete-operation)
- [`scene_validate`](doc:op-scene-validate-operation)
- [`scene_select_many`](doc:op-scene-select-many-operation)
- [`scene_delete_many`](doc:op-scene-delete-many-operation)
- [`scene_duplicate`](doc:op-scene-duplicate-operation)
- [`scene_folder_set`](doc:op-scene-folder-set-operation)
- [`scene_lock`](doc:op-scene-lock-operation)
- [`scene_transform`](doc:op-scene-transform-operation)
- [`scene_transform_batch`](doc:op-scene-transform-batch-operation)

## `viewport` {#viewport}

- [`viewport_new`](doc:op-viewport-new-operation)
- [`viewport_focus`](doc:op-viewport-focus-operation)

## `window` {#window}

- [`window_reset_layout`](doc:op-window-reset-layout-operation)

## `property` {#property}

- [`property_set`](doc:op-property-set-operation)

## `edit` {#edit}

- [`edit_undo`](doc:op-edit-undo-operation)
- [`edit_redo`](doc:op-edit-redo-operation)
- [`edit_history`](doc:op-edit-history-operation)

## `project` {#project}

- [`project_save`](doc:op-project-save-operation)
- [`project_load`](doc:op-project-load-operation)

## `panel` {#panel}

- [`panel_open`](doc:op-panel-open-operation)

## `plugin` {#plugin}

- [`plugin_set_enabled`](doc:op-plugin-set-enabled-operation)

## `assets` {#assets}

- [`assets_list`](doc:op-assets-list-operation)

## `asset` {#asset}

- [`asset_save_graph`](doc:op-asset-save-graph-operation)
- [`asset_new_graph`](doc:op-asset-new-graph-operation)
- [`asset_open`](doc:op-asset-open-operation)
- [`asset_copy`](doc:op-asset-copy-operation)
- [`asset_move`](doc:op-asset-move-operation)
- [`asset_trash`](doc:op-asset-trash-operation)
- [`asset_restore`](doc:op-asset-restore-operation)
- [`asset_trash_list`](doc:op-asset-trash-list-operation)
- [`asset_folder_create`](doc:op-asset-folder-create-operation)
- [`asset_reload`](doc:op-asset-reload-operation)
- [`asset_favorite`](doc:op-asset-favorite-operation)
- [`asset_dependencies`](doc:op-asset-dependencies-operation)
- [`asset_references_set`](doc:op-asset-references-set-operation)
- [`asset_reference_view`](doc:op-asset-reference-view-operation)
- [`asset_new_material_instance`](doc:op-asset-new-material-instance-operation)
- [`asset_importers`](doc:op-asset-importers-operation)
- [`asset_import_prepare`](doc:op-asset-import-prepare-operation)
- [`asset_import_commit`](doc:op-asset-import-commit-operation)
- [`asset_import`](doc:op-asset-import-operation)
- [`asset_reimport`](doc:op-asset-reimport-operation)
- [`asset_import_info`](doc:op-asset-import-info-operation)
- [`asset_import_check`](doc:op-asset-import-check-operation)
- [`asset_import_request`](doc:op-asset-import-request-operation)
- [`asset_import_status`](doc:op-asset-import-status-operation)
- [`asset_import_cancel`](doc:op-asset-import-cancel-operation)
- [`asset_import_view`](doc:op-asset-import-view-operation)
- [`asset_browser_settings`](doc:op-asset-browser-settings-operation)

## `graph` {#graph}

- [`graph_get`](doc:op-graph-get-operation)
- [`graph_validate`](doc:op-graph-validate-operation)
- [`graph_run`](doc:op-graph-run-operation)
- [`graph_node_add`](doc:op-graph-node-add-operation)
- [`graph_node_move`](doc:op-graph-node-move-operation)
- [`graph_node_remove`](doc:op-graph-node-remove-operation)
- [`graph_node_value`](doc:op-graph-node-value-operation)
- [`graph_link_add`](doc:op-graph-link-add-operation)
- [`graph_link_remove`](doc:op-graph-link-remove-operation)
- [`graph_variable_add`](doc:op-graph-variable-add-operation)
- [`graph_variable_default`](doc:op-graph-variable-default-operation)
- [`graph_variable_remove`](doc:op-graph-variable-remove-operation)
- [`graph_variable_rename`](doc:op-graph-variable-rename-operation)
- [`graph_breakpoint_set`](doc:op-graph-breakpoint-set-operation)
- [`graph_node_comment`](doc:op-graph-node-comment-operation)
- [`graph_nodes_duplicate`](doc:op-graph-nodes-duplicate-operation)
- [`graph_diagnose`](doc:op-graph-diagnose-operation)
- [`graph_debug_start`](doc:op-graph-debug-start-operation)
- [`graph_debug_step`](doc:op-graph-debug-step-operation)
- [`graph_debug_continue`](doc:op-graph-debug-continue-operation)
- [`graph_debug_stop`](doc:op-graph-debug-stop-operation)
- [`graph_debug_status`](doc:op-graph-debug-status-operation)
- [`graph_function_create`](doc:op-graph-function-create-operation)
- [`graph_function_signature`](doc:op-graph-function-signature-operation)
- [`graph_function_rename`](doc:op-graph-function-rename-operation)
- [`graph_function_remove`](doc:op-graph-function-remove-operation)
- [`graph_function_evaluate`](doc:op-graph-function-evaluate-operation)
- [`graph_nodes_move`](doc:op-graph-nodes-move-operation)
- [`graph_nodes_remove`](doc:op-graph-nodes-remove-operation)
- [`graph_fragment_copy`](doc:op-graph-fragment-copy-operation)
- [`graph_fragment_paste`](doc:op-graph-fragment-paste-operation)
- [`graph_comment_add`](doc:op-graph-comment-add-operation)
- [`graph_comment_remove`](doc:op-graph-comment-remove-operation)
- [`graph_find`](doc:op-graph-find-operation)
- [`graph_link_reroute`](doc:op-graph-link-reroute-operation)
- [`graph_bookmark_add`](doc:op-graph-bookmark-add-operation)
- [`graph_bookmark_remove`](doc:op-graph-bookmark-remove-operation)
- [`graph_comment_edit`](doc:op-graph-comment-edit-operation)
- [`graph_debug_step_into`](doc:op-graph-debug-step-into-operation)
- [`graph_debug_step_out`](doc:op-graph-debug-step-out-operation)
- [`graph_diff`](doc:op-graph-diff-operation)
- [`graph_diff_view`](doc:op-graph-diff-view-operation)
- [`graph_class_define`](doc:op-graph-class-define-operation)
- [`graph_class_remove`](doc:op-graph-class-remove-operation)
- [`graph_debug_tick`](doc:op-graph-debug-tick-operation)
- [`graph_debug_event`](doc:op-graph-debug-event-operation)
- [`graph_debug_object_set`](doc:op-graph-debug-object-set-operation)
- [`graph_debug_view`](doc:op-graph-debug-view-operation)
- [`graph_types_get`](doc:op-graph-types-get-operation)
- [`graph_type_define`](doc:op-graph-type-define-operation)
- [`graph_type_remove`](doc:op-graph-type-remove-operation)
- [`graph_interfaces_get`](doc:op-graph-interfaces-get-operation)
- [`graph_interface_define`](doc:op-graph-interface-define-operation)
- [`graph_interface_remove`](doc:op-graph-interface-remove-operation)
- [`graph_signature_plan`](doc:op-graph-signature-plan-operation)
- [`graph_signature_apply`](doc:op-graph-signature-apply-operation)
- [`graph_type_migration_plan`](doc:op-graph-type-migration-plan-operation)
- [`graph_type_migration_apply`](doc:op-graph-type-migration-apply-operation)
- [`graph_collapse_plan`](doc:op-graph-collapse-plan-operation)
- [`graph_collapse_apply`](doc:op-graph-collapse-apply-operation)
- [`graph_expand_plan`](doc:op-graph-expand-plan-operation)
- [`graph_expand_apply`](doc:op-graph-expand-apply-operation)

## `workspace` {#workspace}

- [`workspace_layout`](doc:op-workspace-layout-operation)
- [`workspace_save`](doc:op-workspace-save-operation)
- [`workspace_load`](doc:op-workspace-load-operation)

## `content` {#content}

- [`content_drawer_toggle`](doc:op-content-drawer-toggle-operation)

## `material` {#material}

- [`material_get`](doc:op-material-get-operation)
- [`material_override_set`](doc:op-material-override-set-operation)
- [`material_override_remove`](doc:op-material-override-remove-operation)
- [`material_parent_set`](doc:op-material-parent-set-operation)
- [`material_prepare`](doc:op-material-prepare-operation)
- [`material_sample`](doc:op-material-sample-operation)
- [`material_compile`](doc:op-material-compile-operation)
- [`material_compile_request`](doc:op-material-compile-request-operation)
- [`material_compile_status`](doc:op-material-compile-status-operation)
- [`material_compile_cancel`](doc:op-material-compile-cancel-operation)
- [`material_preview_apply`](doc:op-material-preview-apply-operation)
- [`material_compiler_view`](doc:op-material-compiler-view-operation)
- [`material_preview_settings`](doc:op-material-preview-settings-operation)

## `build` {#build}

- [`build_describe`](doc:op-build-describe-operation)
- [`build_prepare`](doc:op-build-prepare-operation)
- [`build_execute`](doc:op-build-execute-operation)
- [`build_request`](doc:op-build-request-operation)
- [`build_status`](doc:op-build-status-operation)
- [`build_cancel`](doc:op-build-cancel-operation)

## `texture` {#texture}

- [`texture_info`](doc:op-texture-info-operation)
- [`texture_open`](doc:op-texture-open-operation)
- [`texture_view_settings`](doc:op-texture-view-settings-operation)

## `model` {#model}

- [`model_open`](doc:op-model-open-operation)
- [`model_get`](doc:op-model-get-operation)
- [`model_export_prepare`](doc:op-model-export-prepare-operation)
- [`model_node_set`](doc:op-model-node-set-operation)
- [`model_vertex_set`](doc:op-model-vertex-set-operation)
- [`model_key_set`](doc:op-model-key-set-operation)
- [`model_sample`](doc:op-model-sample-operation)
- [`model_save`](doc:op-model-save-operation)
- [`model_close`](doc:op-model-close-operation)
- [`model_export`](doc:op-model-export-operation)
- [`model_reload`](doc:op-model-reload-operation)
- [`model_view_settings`](doc:op-model-view-settings-operation)
- [`model_clip_create`](doc:op-model-clip-create-operation)
- [`model_track_add`](doc:op-model-track-add-operation)
- [`model_key_add`](doc:op-model-key-add-operation)
- [`model_key_remove`](doc:op-model-key-remove-operation)
- [`model_list`](doc:op-model-list-operation)
- [`model_process_prepare`](doc:op-model-process-prepare-operation)
- [`model_process`](doc:op-model-process-operation)
- [`model_process_commit`](doc:op-model-process-commit-operation)
- [`model_lod_remove`](doc:op-model-lod-remove-operation)
- [`model_collision_remove`](doc:op-model-collision-remove-operation)
- [`model_collision_set`](doc:op-model-collision-set-operation)

## `json` {#json}

- [`json_open`](doc:op-json-open-operation)
- [`json_get`](doc:op-json-get-operation)
- [`json_value_set`](doc:op-json-value-set-operation)
- [`json_add`](doc:op-json-add-operation)
- [`json_remove`](doc:op-json-remove-operation)
- [`json_save`](doc:op-json-save-operation)
- [`json_close`](doc:op-json-close-operation)

## `game` {#game}

- [`game_codec_describe`](doc:op-game-codec-describe-operation)
- [`game_codec_list`](doc:op-game-codec-list-operation)
- [`game_codec_export`](doc:op-game-codec-export-operation)

## Pont natif direct {#python}

Exécutez cet exemple Python avec l’environnement privé et `app` comme répertoire de travail.

```python
from tool import editor_call
status = editor_call("editor_status", {})
```

L’éditeur doit être démarré. `endpoint`, `cancellation` et le mot-clé `timeout` concernent ce pont Python. Il ne remplace pas le serveur MCP. Après un timeout de modification, inspectez l’état courant.
