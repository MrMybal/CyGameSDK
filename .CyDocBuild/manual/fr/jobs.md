---
id: jobs
title: Jobs, workers et fichiers de tâches
description: Jobs, workers et fichiers de tâches
section: SDK
order: 50
sourceRevision: 1
parent: automation
---

## Suivre le cycle d’un job {#lifecycle}

CyToolsCore possède identités, sessions authentifiées, ordonnanceur séquentiel, progression, annulation et ProcessWorker. Réutilisez SDK/MCP, pas un second ordonnanceur. Soumettez une opération, interrogez `cy_get_job` et attendez Completed, Cancelled ou Failed. L’annulation est une demande, pas la preuve qu’aucune écriture persistante n’a commencé.

Les sessions temporaires regroupent producteurs et consommateurs. Lisez/importez les sorties tant que la tâche reste ouverte. `cy_export_job` copie un bundle vers une destination unique hors runtime ; exportez avant `cy_finish_task` pour le conserver. Terminer supprime copies importées et espaces de jobs, pas sources, poids de modèles ou exports explicites. Un worker actif retourne Busy ; nettoyage et annulation attendent son arrêt réel. Un chemin local désigne l’hôte du Tool, pas un autre ordinateur.

Les tâches inactives expirent par défaut après 3600 secondes ; le maintien en vie renouvelle ce délai. La rétention explicite va de 1 à 86400 secondes. Les sessions graphiques sont persistantes ; la politique temporaire IA ne supprime pas automatiquement leurs sorties. L’historique peut rester avec contenus marqués purgés. Les anciennes sessions sans indicateur temporaire sont conservées.

## Implémenter un fournisseur {#providers}

Allouez sorties, intermédiaires et diagnostics avec `JobContext.path`. Configurez racines et véritables champs de fichiers d’entrée avant démarrage ; ne déclarez pas poids, exécutables ou destinations d’export comme artefacts jetables. Le worker officiel lit une requête possédée, écrit sorties/rapports possédés et retourne du JSON structuré. Préservez erreurs, sortie, annulation et contrôle d’identité. N’acceptez jamais de commandes exécutables provenant d’un asset.

## Clients partagés et transports {#clients}

Le point d’accès natif Windows est un IPC local. Le pont utilise CLI/JSONL/MCP Core et le service HTTP authentifié existant. Pour partager un même runtime de jobs, utilisez ce service et son pont MCP plutôt que des runtimes concurrents. Enveloppes, validation, erreurs et frontières d’identité relèvent de CyToolsCore. Les codes d’erreurs sont destinés aux machines ; ne parsez pas les libellés localisés de l’interface.
