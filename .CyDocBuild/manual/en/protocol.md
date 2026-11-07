---
id: protocol
title: Transport protocol and events
description: Transport protocol and events
section: SDK
order: 53
revision: 1
parent: automation
---

## CyTools envelopes {#envelopes}
Core protocol 1.0 uses the same UTF-8 JSON envelopes on JSONL stdio and HTTP POST `/v1`. HTTP V1 requires Content-Length; chunked request bodies are unsupported. This is the CyTools protocol; MCP is a separate official adapter. Correlate response IDs. Responses contain exactly result or error. JSONL commands are handled in order, while submitted jobs may run concurrently. No implicit JSON-RPC batch is supported.

```json
{"protocolVersion":"1.0","id":1,"method":"DescribeTool","params":{}}
```

Python Runtime arguments use snake_case; network fields and LocalClient.call use camelCase. Authenticate HTTP with Authorization: Bearer TOKEN. Client IDs are identities, not credentials. The server creates authenticated sessions/workspaces; SubmitJob does not accept an owner identity or workspace path.

## Method families {#methods}
| Purpose | Methods |
| --- | --- |
| Discover | DescribeTool, GetCapabilities, ListOperations, DescribeOperation(operationId), ListBackends, ListModels(backendId?) |
| Runtime | GetRuntimeStatus, GetSystemResources, EstimateResources, CanRun, Diagnose |
| Sessions | CreateSession(temporary?, retentionSeconds?), CloseSession(sessionId) |
| Jobs | SubmitJob(sessionId, operationId, parameters, backendId?, modelId?, priority?, queueTimeout?, executionTimeout?), GetJob(jobId), ListJobs, CancelJob(jobId), ShareJob(jobId, visibility, sharedWith?) |
| Task files | ImportFile(sessionId, source, companions?), ExportJob(jobId, destination), FinishTask(sessionId, cancelRunning?), KeepTaskAlive(sessionId) |
| Events/resources | GetEvents(after?), ReleaseResources(level) |

DescribeTool returns the complete manifest. CanRun reports admissibility and waitingForResources/waitingForConcurrency, not a reservation. GetRuntimeStatus returns toolId, activeClients, activeSessions, queuedJobs, runningJobs, runningWorkers, unknownWorkers, resources and acceptingJobs; global counts do not expose another client's job payloads. CloseSession is Busy with active jobs. ReleaseResources requires manageResources permission. See [task lifecycle](doc:jobs).

## Events and cursor recovery {#events}
Events carry sequence, type, timestamp, toolId, data and applicable clientId/sessionId/jobId. GetEvents(after) returns authorized events, cursor and gap. Pass the returned cursor next time. On gap, re-read persistent job state. Reset your cursor if it goes backwards after runtime restart. In-memory events/logs are bounded, not a durable archive.

Events include ToolStarted/ToolStopping, SessionCreated/SessionClosed, JobCreated/Queued/WaitingForResources/Preparing/Started/Progress/Cancelling/Cancelled/Completed/Failed, ResourceReservationCreated/Released and Log. `context.log(message, level)` correlates a log with its job; usual levels are Trace, Debug, Info, Warning, Error, Critical. Optional download/model events do not prove that this editor includes such providers.

## Scheduling {#scheduling}
Admissible jobs are ordered by Interactive, Critical, High, Normal, Background, Low priority, then arrival. Running jobs are not preempted. maxWorkers, backend concurrency, resource budgets and shared exclusiveGroups still apply. A group is a case-sensitive logical identifier, not a GPU device or VRAM reservation. Groups are acquired atomically and kept until the handler and its worker cleanup finish, including cancellation. A conflict remains Queued; memory pressure uses WaitingForResources. Priority does not grant permissions. Groups belong to one shared Runtime, not all processes or Tools; starvation prevention and cross-Tool scheduling are not implemented.
