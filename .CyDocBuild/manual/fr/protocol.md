---
id: protocol
title: Protocole de transport et événements
description: Protocole de transport et événements
section: SDK
order: 53
sourceRevision: 1
parent: automation
---

## Enveloppes CyTools {#envelopes}
Le protocole Core 1.0 emploie les mêmes enveloppes JSON UTF-8 sur JSONL stdio et HTTP POST `/v1`. HTTP V1 exige Content-Length ; les requêtes chunked ne sont pas prises en charge. Il s’agit du protocole CyTools ; MCP est un adaptateur officiel distinct. Corrélez les IDs des réponses. Une réponse contient exactement result ou error. Les commandes JSONL sont traitées dans l’ordre ; les jobs soumis peuvent fonctionner en parallèle. Aucun batch JSON-RPC implicite n’est pris en charge.

```json
{"protocolVersion":"1.0","id":1,"method":"DescribeTool","params":{}}
```

Les arguments du Runtime Python utilisent snake_case ; les champs réseau et LocalClient.call utilisent camelCase. HTTP s’authentifie avec Authorization: Bearer TOKEN. Un ID client est une identité, pas un secret. Le serveur crée les sessions/workspaces authentifiés ; SubmitJob n’accepte ni propriétaire ni chemin de workspace choisis par l’appelant.

## Familles de méthodes {#methods}
| But | Méthodes |
| --- | --- |
| Découverte | DescribeTool, GetCapabilities, ListOperations, DescribeOperation(operationId), ListBackends, ListModels(backendId?) |
| Runtime | GetRuntimeStatus, GetSystemResources, EstimateResources, CanRun, Diagnose |
| Sessions | CreateSession(temporary?, retentionSeconds?), CloseSession(sessionId) |
| Jobs | SubmitJob(sessionId, operationId, parameters, backendId?, modelId?, priority?, queueTimeout?, executionTimeout?), GetJob(jobId), ListJobs, CancelJob(jobId), ShareJob(jobId, visibility, sharedWith?) |
| Fichiers de tâche | ImportFile(sessionId, source, companions?), ExportJob(jobId, destination), FinishTask(sessionId, cancelRunning?), KeepTaskAlive(sessionId) |
| Événements/ressources | GetEvents(after?), ReleaseResources(level) |

DescribeTool renvoie le manifeste complet. CanRun indique l’admissibilité et waitingForResources/waitingForConcurrency, sans réserver. GetRuntimeStatus renvoie toolId, activeClients, activeSessions, queuedJobs, runningJobs, runningWorkers, unknownWorkers, resources et acceptingJobs ; les compteurs globaux n’exposent pas les données des jobs d’autrui. CloseSession est Busy si des jobs sont actifs. ReleaseResources exige manageResources. Voir [le cycle des tâches](doc:jobs).

## Événements et reprise du curseur {#events}
Les événements portent sequence, type, timestamp, toolId, data et les clientId/sessionId/jobId applicables. GetEvents(after) renvoie les événements autorisés, cursor et gap. Réutilisez le curseur retourné. Sur gap, relisez l’état persistant des jobs. Réinitialisez le curseur s’il recule après redémarrage du runtime. Le journal en mémoire est borné, sans archivage durable implicite.

Les événements comprennent ToolStarted/ToolStopping, SessionCreated/SessionClosed, JobCreated/Queued/WaitingForResources/Preparing/Started/Progress/Cancelling/Cancelled/Completed/Failed, ResourceReservationCreated/Released et Log. `context.log(message, level)` corrèle un log au job ; les niveaux usuels sont Trace, Debug, Info, Warning, Error, Critical. Des événements optionnels de téléchargement/modèles ne prouvent pas leur implémentation dans cet éditeur.

## Ordonnancement {#scheduling}
Les jobs admissibles suivent la priorité Interactive, Critical, High, Normal, Background, Low puis l’arrivée. Les jobs démarrés ne sont pas préemptés. maxWorkers, concurrence des backends, budgets et exclusiveGroups communs s’appliquent. Un groupe est un identifiant logique sensible à la casse, pas un GPU ni une réservation VRAM. Les groupes sont acquis atomiquement et gardés jusqu’à la fin du handler et du nettoyage de ses workers, même après annulation. Un conflit reste Queued ; la pression mémoire utilise WaitingForResources. La priorité ne donne aucun droit. Les groupes appartiennent à un Runtime partagé, pas à tous les processus/Tools ; prévention de famine et ordonnancement inter-Tools ne sont pas implémentés.
