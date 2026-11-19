---
id: protocol
title: Transport protocol and events
description: Native SDK JSON, MCP and compatibility envelopes
section: SDK
order: 53
revision: 2
parent: automation
---

## Native and compatibility envelopes {#envelopes}

Native **CyGameCLI jsonl** and authenticated HTTP POST **/v1** use SDK JSON.
Responses contain result or error. MCP uses its own JSON-RPC protocol at **mcp**
stdio or **/mcp**; a generic JSONL request is not an MCP packet.

~~~json
{"method":"invoke","name":"scene_list","arguments":{}}
~~~

Existing **app/tool.py stdio** preserves supported protocol-1.0 envelopes:

~~~json
{"protocolVersion":"1.0","id":1,"method":"DescribeTool","params":{}}
~~~

HTTP requires a private Bearer token and uses one authenticated host identity.
Owner and workspace paths are assigned by the host. No caller-supplied principal
is accepted by public MCP/HTTP. Select a matching editor with **--root**.

## Method families {#methods}

Native discovery uses list, describe and invoke. The Python compatibility channel
supports DescribeTool, GetCapabilities, ListOperations, DescribeOperation,
ListBackends, ListModels, GetRuntimeStatus, CreateSession, SubmitJob, GetJob,
ListJobs, CancelJob, ExportJob, FinishTask and CloseSession.

Old generic model/resource/download methods are not implemented by this editor.
They return an explicit UnsupportedMethod error rather than implying a provider.
The native business registry and durable service schemas are authoritative.

## Changes and recovery {#events}

SDK ChangeFeed and compatible MCP subscriptions report catalogue/resource changes.
TTL and explicit refresh remain available. Scene snapshots refresh after document
or ordered-selection revisions. Mutations are never cached or automatically replayed.
Private optional performance JSONL contains metadata only, excluding messages,
results, keys and sensitive identifiers.

Read durable job state after a disconnect or notification gap. Expired transient
resources do not repeat their producing operation; recover original artifacts by job ID.

## Scheduling {#scheduling}

SDK Executor runs application jobs; a host mutex serializes compute for one project.
Cancellation is cooperative, then terminates the SDK-owned child tree if necessary.
Terminal desktop readiness follows runtime closure. Durable records/results survive
process exit; interrupted work is marked Interrupted and never resumed automatically.

This host does not implement the former generic priority/GPU-budget scheduler,
model downloads or cross-application orchestration. Graph simulation time remains
independent of compute scheduling. [Job lifecycle](doc:jobs).
