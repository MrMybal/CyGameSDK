---
id: jobs
title: Durable jobs, workers and results
description: Follow native jobs and recover their outputs after restart.
section: SDK
order: 50
revision: 2
parent: automation
---

## Follow the job lifecycle {#lifecycle}

CyMCPSDK supplies native execution/cancellation and owned processes. The application
persists job state and files. Canonical operations return direct results; compatibility
op_<id> aliases submit asynchronous jobs. Poll job_get or cy_get_job until Completed,
Cancelled, Failed or Interrupted. A cancelled call may already have crossed a persistent
commit boundary: inspect current state before deciding to invoke again.

Jobs and artifacts are durable application data. Finish-task does not remove results.
They survive restart and SDK resource TTL. Export deliberately and remove retained data
only when no longer needed. Recovery marks unfinished work Interrupted when its owner
has ended; it never replays a job automatically.

## Implement a provider {#providers}

Use the trusted source factories and bounded JobContext.path outputs. Existing Python
computations run beneath the SDK-owned process tree, with cooperative cancellation and
final cleanup before desktop status reports clientClosed. Real game formats/toolchains
remain adapter work. Never accept executable commands from asset/project JSON.

## Shared clients and transports {#clients}

CLI, chat and MCP use the same registry, native command validation and host policy.
Windows editor calls are serialized on its owning thread. A loopback endpoint represents
one authenticated identity, with an owner-only token. Isolate endpoints/stores for distinct
remote identities. The compatibility Python client retains authenticated job ownership.
Do not parse translated labels for status or errors.

## Recover complete results {#results}

job_list supports state/operation filters and offset/limit pagination; follow nextOffset.
job_get returns durable output locations. job_result_read returns UTF-8-safe JSON chunks.
job_output_read returns bounded base64 file chunks across CLI processes. job_output_open
produces a private SDK resource progressively; follow its manifest and chunk URIs.
An expired resource never causes an export or mutation to run again.

Completed artifacts are outputs. Partial files from failed/interrupted jobs are diagnostics.
Permissions and path confinement are checked for every result/file request.

## Existing installations {#legacy}

The compatibility loader reads old SQLite records without the former runtime package.
It preserves owner IDs, token hashes, unknown metadata, output locations and recipe formats.
The original database/artifact folders remain unchanged. The explicit legacy_jobs.py
utility can copy records into the new store; reassignment to the local endpoint identity
requires its explicit --owner local option.
