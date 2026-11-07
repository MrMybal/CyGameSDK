---
id: type-migration
title: Migrer les définitions d’énumérations et de structures
description: Migrer les définitions d’énumérations et de structures
section: Graphes
order: 17
sourceRevision: 1
parent: graphs
---

## Modifier un type local {#preview}

Choisissez une énumération ou une structure locale en conservant son identité nominale et sa nature. Prévisualisez renommage, ajout, retrait ou changement de type/valeur par défaut pris en charge. Les valeurs déclarées dépendantes sont normalisées avant de résoudre les descripteurs de nœuds. Les usages dans le graphe, fonctions, macros, classes et interfaces sont vérifiés ensemble.

Un champ retiré encore référencé par `struct.set` bloque le plan au lieu de supprimer un comportement. Les autres valeurs et connexions retirées/réinitialisées figurent dans les pertes, sans réparation silencieuse. L’application recalcule et valide le candidat ; elle exige le `planHash` exact, la requête originale et `acceptLoss` explicite si nécessaire. Changer le propriétaire ou la requête invalide le plan.

## Restaurer et délimiter {#undo}

Une application réussie correspond à une seule annulation du propriétaire entier. Conservez métadonnées inconnues et identité nominale ; ne remplacez pas une énumération par une structure. Aucune migration au chargement, entre assets ou dans les objets d’exécution vivants. Enregistrez explicitement l’asset. Les contrats SDK exposent les mêmes gardes ; découvrez noms et champs exacts dans les [opérations](doc:operations).
