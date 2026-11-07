---
id: functions
title: Fonctions et macros
description: Fonctions et macros
section: Graphes
order: 11
sourceRevision: 1
parent: graphs
---

## Définitions locales {#definitions}

Les fonctions appartiennent à leur asset graphe. Dans Mon graphe, définissez fonction, entrées/sorties typées, valeurs par défaut et variables locales, puis ouvrez son sous-graphe. Le mode pur s’applique quand une définition enregistrée n’a pas de mode. Le mode d’exécution expose les frontières exec/then et un retour explicite. Inspectez le descripteur résolu : un appel dynamique n’est pas entièrement décrit par le catalogue statique des nœuds.

Entrées, sorties et valeurs locales sont validées. Les appels imbriqués partagent les budgets, isolent les variables locales et mettent en cache les sorties d’exécution. Un chemin sans retour est une erreur. La récursion est refusée, y compris entre implémentations possibles d’une interface. Le débogueur conserve les identités de nœuds qualifiées par contexte et la pile d’appels ; un identifiant de nœud seul n’est pas une adresse globale.

## Pur ou exécution {#modes}

Les fonctions pures évaluent les arguments immédiatement. Un argument connecté mais inutilisé peut donc échouer. Les macros se développent de manière paresseuse, dans une portée isolée, avec correspondance aux sources du débogueur. Les fonctions d’exécution doivent être atteintes par le flot de contrôle. Ne transformez pas un appel d’exécution en appel pur pour permettre son développement.

Corps de macros et fonctions enfants héritent des bibliothèques nominales et d’interfaces du propriétaire. Ne dupliquez pas ces bibliothèques dans chaque corps sérialisé. Les fusions du presse-papiers vérifient identités et usages atomiquement.

## Réutiliser et modifier {#changes}

Modifiez les signatures avec la [migration aperçu/application](doc:signature-migration). La [réduction](doc:collapse) accepte seulement des dépendances pures prises en charge. Le [développement](doc:expand) remplace un appel pur pris en charge tout en conservant sa définition. Ces actions ne sauvegardent pas implicitement et ne compilent pas de code pour un jeu.
