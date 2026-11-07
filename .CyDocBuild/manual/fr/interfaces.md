---
id: interfaces
title: Classes, interfaces et objets d’exécution
description: Classes, interfaces et objets d’exécution
section: Graphes
order: 14
sourceRevision: 1
parent: graphs
---

## Lier un contrat {#contracts}

Les interfaces sont des contrats nommés locaux, avec signatures nominales. Définissez le contrat dans Mon graphe puis liez une classe dont les méthodes le respectent. Toute modification de signature doit préserver les liaisons. Les appels d’interface utilisent des objets d’exécution partagés, pas des copies déconnectées de leur état.

Vues de fonctions, macros, presse-papiers et runtimes enfants transportent la bibliothèque du propriétaire. Les corps de fonctions sérialisés ne la dupliquent pas. La détection de récursion considère chaque implémentation possible, y compris la distribution dynamique des appels. Une modification qui casserait une liaison échoue atomiquement.

## Objets locaux {#objects}

Les classes déclarent champs, méthodes et héritage pris en charge. Les nœuds de création, lecture, écriture et appel manipulent des identifiants temporaires dans l’interpréteur borné. Le débogueur inspecte les champs ; les modifications SDK utilisent les mêmes gardes de type et de budget. Ces identifiants ne sont ni des identifiants d’assets persistants, ni des pointeurs moteur, ni des identités multijoueurs.

Les contrats item/index/break du foreach, le temps de simulation logique, les délais et événements appartiennent à cet interpréteur. Ils sont distincts de l’ordonnanceur de jobs CyToolsCore. Réflexion des composants moteur, réplication, liaison physique et compilation JIT ne sont pas fournis.

Pour modifier des méthodes liées, utilisez la [migration de signatures](doc:signature-migration). Conservez `self`, le mode d’appel et les contrats du foreach.
