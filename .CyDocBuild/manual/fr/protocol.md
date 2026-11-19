---
id: protocol
title: Protocole de transport et événements
description: JSON SDK natif, MCP et enveloppes de compatibilité
section: SDK
order: 53
sourceRevision: 2
parent: automation
---

## Enveloppes natives et de compatibilité {#envelopes}

**CyGameCLI jsonl** et HTTP POST **/v1** authentifié utilisent le JSON du SDK.
Une réponse contient result ou error. MCP utilise son propre JSON-RPC via **mcp**
stdio ou **/mcp** ; une requête JSONL générique n’est pas un paquet MCP.

~~~json
{"method":"invoke","name":"scene_list","arguments":{}}
~~~

**app/tool.py stdio** conserve les enveloppes 1.0 prises en charge :

~~~json
{"protocolVersion":"1.0","id":1,"method":"DescribeTool","params":{}}
~~~

HTTP demande un jeton Bearer privé et utilise une identité authentifiée par endpoint.
Propriétaire et workspace sont attribués par l’hôte ; aucun principal fourni par
l’appelant n’est accepté par MCP/HTTP public. **--root** sélectionne l’éditeur associé.

## Familles de méthodes {#methods}

La découverte native emploie list, describe et invoke. Le canal Python de compatibilité
gère DescribeTool, GetCapabilities, ListOperations, DescribeOperation, ListBackends,
ListModels, GetRuntimeStatus, CreateSession, SubmitJob, GetJob, ListJobs, CancelJob,
ExportJob, FinishTask et CloseSession.

Les anciennes méthodes génériques de modèles/ressources/téléchargements ne sont pas
implémentées par cet éditeur. Elles retournent UnsupportedMethod explicitement.
Le registre métier natif et les schémas des services durables font autorité.

## Changements et récupération {#events}

ChangeFeed et les abonnements MCP compatibles signalent les changements de catalogue
ou ressources. TTL et rafraîchissement explicite restent disponibles. Les snapshots
de scène suivent les révisions de documents ou de sélection ordonnée. Les mutations
ne sont ni mises en cache ni rejouées. Les mesures JSONL privées facultatives excluent
messages, résultats, clés et identifiants sensibles.

Après déconnexion ou perte de notification, relisez les états durables. Une ressource
temporaire expirée ne rejoue pas l’opération ; récupérez les fichiers par ID de job.

## Ordonnancement {#scheduling}

Executor exécute les jobs de l’application ; un mutex hôte sérialise le calcul d’un
même projet. L’annulation est coopérative puis arrête au besoin l’arbre de processus
détenu par le SDK. L’état terminal du bureau suit la fermeture du runtime. États et
résultats survivent à l’arrêt ; un travail interrompu devient Interrupted, sans reprise automatique.

L’hôte n’implémente pas l’ancien ordonnanceur générique de priorités/budgets GPU,
les téléchargements de modèles ou l’orchestration inter-applications. Le temps de
simulation des graphes reste indépendant. [Cycle des jobs](doc:jobs).
