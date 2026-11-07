---
id: runtime-objects
title: Temps, événements et objets du runtime
description: Temps, événements et objets du runtime
section: Graphes
order: 15
sourceRevision: 1
parent: graphs
---

## Faire avancer une simulation {#time}

Un délai suspend une branche logique jusqu’à l’avancement du temps de simulation du graphe. Utilisez les commandes de temps/événements du débogueur ou `graph_debug_tick` et `graph_debug_event`. Ce n’est pas un ordonnanceur lié à l’horloge réelle. Événements, nombre de branches et étapes restent bornés ; les objets locaux partagent leur état entre appels.

Inspectez les champs avec `graph_debug_status` et modifiez les valeurs prises en charge avec `graph_debug_object_set`. Employez les identifiants courants et les types du schéma ; relancer ou invalider le runtime rend les anciens identifiants inutilisables. Le développement des macros conserve les correspondances de navigation du débogueur.

## Clarifier les responsabilités {#ownership}

Les jobs Core exécutent import, compilation des matériaux et fournisseurs de build hors de l’interface. Identités, annulation, sessions et durée de vie des sorties appartiennent à CyToolsCore. Les [jobs workers](doc:jobs) décrivent ce cycle séparé. Variables et champs d’objets à l’exécution ne réécrivent pas silencieusement les déclarations ni les assets source enregistrés.
