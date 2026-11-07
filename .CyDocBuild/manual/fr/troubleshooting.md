---
id: troubleshooting
title: Résoudre les problèmes de compilation et de lancement
description: Résoudre les problèmes de compilation et de lancement
section: Premiers pas
order: 6
sourceRevision: 1
---

## Compiler et lancer {#launch}

Utilisez la version source/catalogue de ce projet. Compilez avec app/build.ps1 et configurez Python/Core avec app/setup_mcp.ps1. Vérifiez Visual Studio C++, SDK Windows, accès Git aux dépendances verrouillées et CMake. Les candidats historiques ne remplacent pas l’exécutable courant.

Lancez le CyGameEditor.exe résultant ou Lancer.cmd. Le lanceur protège le chemin par guillemets et choisit le dossier de travail ; les échecs immédiats apparaissent dans sa console. Les journaux natifs sont dans data/logs. Un accès refusé demande de vérifier exécutable exact, droits et journaux Windows/contrôle d’applications. Ne renommez pas un ancien binaire et ne désactivez pas la protection pour remplacer le diagnostic. Préservez projets, assets et préférences.

## Outil ou manuel absent {#missing}

Une vue ne prouve pas la présence d’un codec ou compilateur moteur. Vérifiez descripteur d’adaptateur et registre des fournisseurs exécutables. Pour matériaux/import/build, inspectez état des jobs, rapports et configuration Core. L’absence de Documentation/Documentation.html signifie que le dossier généré complet n’a pas été construit ou copié. Régénérez-le avec le build documenté plutôt que d’ouvrir l’ancien guide web à six sections.
