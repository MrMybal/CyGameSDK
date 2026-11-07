---
id: named-types
title: Énumérations, structures et types nominaux
description: Énumérations, structures et types nominaux
section: Graphes
order: 13
sourceRevision: 1
parent: graphs
---

## Déclarer des types locaux {#declare}

Dans Mon graphe, utilisez **Nouveau type de données** pour une énumération ou une structure. La bibliothèque appartient à cet asset, pas à un registre entre projets. Membres et champs conservent une identité nominale stable. Deux énumérations ayant les mêmes libellés ne deviennent pas interchangeables pour autant.

Valeurs par défaut et structures imbriquées sont validées dans le contexte du propriétaire. Les schémas de broches utilisent ce contexte dans graphes racines, fonctions, macros, classes et runtimes enfants. Les nœuds de valeur, composition/décomposition, lecture/écriture et branchement utilisent le type nommé. Modifier un type utilisé vérifie tous ses usages atomiquement ; une incompatibilité échoue sans remise à zéro silencieuse.

## Conteneurs et héritage {#containers}

L’interpréteur propose ses tableaux/dictionnaires typés, classes et méthodes pris en charge. Cela n’implique ni conteneurs imbriqués arbitraires ni réflexion d’un moteur. Les enfants héritent des bibliothèques locales. Le presse-papiers transporte les définitions et refuse les identités en conflit.

Utilisez la [migration explicite](doc:type-migration) pour renommer membres ou champs, ajouter des champs ou accepter des pertes. Rien ne migre au chargement. L’identifiant nominal et la nature du type restent fixes ; convertir une énumération en structure n’est pas une migration.
