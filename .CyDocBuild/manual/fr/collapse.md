---
id: collapse
title: Réduire une sélection pure en fonction ou macro
description: Réduire une sélection pure en fonction ou macro
section: Graphes
order: 18
sourceRevision: 1
parent: graphs
---

## Choisir une zone compatible {#selection}

Sélectionnez une zone connectée de dépendances pures puis **Réduire la sélection** et prévisualisez fonction ou macro. Variables, frontières et nœuds d’exécution restent à l’extérieur. Pour chaque sortie exportée, l’auxiliaire doit utiliser tous les nœuds sélectionnés et fils entrants. Les dépendances sont vérifiées par broche de sortie, y compris les branches UV indépendantes. Aucun nœud n’est élagué et aucune entrée externe inutilisée n’est évaluée seulement pour construire l’auxiliaire.

L’aperçu explique zones incompatibles et décisions de validité/perte. Un candidat qui changerait l’évaluation immédiate/paresseuse ou masquerait une erreur est refusé. Une sélection d’apparence simple n’est pas forcément réductible.

## Préserver les données créées {#apply}

Identifiants originaux de nœuds/liens, valeurs de secours et métadonnées inconnues sont conservés dans l’auxiliaire. Les identifiants générés sont déterministes et sans collision. L’application exige les empreintes exactes de source/requête et valide le propriétaire entier ; une source modifiée demande un nouvel aperçu. Une annulation restaure la zone d’origine. Sauvegardez explicitement et vérifiez le comportement avec le runtime existant. Extraire une fonction n’ajoute ni compilateur de jeu ni codec moteur.

Le [développement](doc:expand) est l’opération complémentaire sur un appel pur ; ses règles de préservation et d’évaluation diffèrent.
