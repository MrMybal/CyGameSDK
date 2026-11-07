---
id: jobs
title: Jobs, workers and task files
description: Jobs, workers and task files
section: SDK
order: 50
revision: 1
parent: automation
---

## Follow the job lifecycle {#lifecycle}

CyToolsCore owns identities, authenticated sessions, sequential scheduling, progress, cancellation and ProcessWorker. Use the existing SDK/MCP server, not an extra scheduler. Submit an operation, poll `cy_get_job`, and wait for Completed, Cancelled or Failed. Cancellation is a request, not proof that persistent commit never began.

Temporary sessions group producers and consumers. Read or import outputs while the task stays open. `cy_export_job` copies a result bundle outside the runtime into a unique destination; export before `cy_finish_task` if it must survive. Finish deletes task-owned imported copies and entire job workspaces, not original files, model weights or explicit exports. Active workers cause Busy; cancellation/cleanup wait for actual worker shutdown. A local path refers to the tool's host, not another computer.

Idle tasks expire by default after 3600 seconds; keep-alive renews them. Explicit task retention is 1–86400 seconds. Desktop GUI sessions are persistent; do not assume the temporary AI session policy also deletes desktop outputs. Task metadata/history can remain while deleted artifact payloads are marked purged. Old sessions without a temporary flag are preserved.

## Implement a provider {#providers}

Allocate generated/intermediate/diagnostic outputs with `JobContext.path`. Configure accepted input roots and real input-file fields before runtime startup; do not declare model weights, executable paths or export destinations as disposable input artifacts. The official one-shot worker reads an owned request, writes owned outputs/reports and returns structured JSON. Preserve exit/error/cancellation semantics and ownership checks. Never accept executable commands from an asset file.

## Shared clients and transports {#clients}

The native Windows editor endpoint is local IPC. The bridge supports the existing Core CLI/JSONL/MCP and authenticated HTTP service. For clients sharing a single job runtime, use that service and its MCP bridge instead of separate competing runtimes. Protocol envelopes, parameter validation, error codes and identity boundaries belong to CyToolsCore. Core error codes are machine-readable; do not parse localized UI strings.
