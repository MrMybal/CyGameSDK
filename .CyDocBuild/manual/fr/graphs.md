---
id: graphs
title: Graphes logiques et canevas
description: Graphes logiques et canevas
section: Graphes
order: 10
sourceRevision: 2
---

## Créer un graphe {#canvas}

Passez à Graphes ou ouvrez un graphe `.cyasset`. **Ajouter un nœud** le crée à la position demandée. Le bouton central déplace la vue, la molette zoome et Cadrer tout / Origine retrouve la zone créée. Reliez des broches compatibles en glissant ou en cliquant sur les deux broches. Sens, type, domaine, entrée occupée et cycles sont validés.

Sélectionnez des nœuds ou une zone, déplacez un groupe, alignez, dupliquez, ajoutez des reroutages et des commentaires. Un geste terminé produit une entrée d’annulation cohérente. Déplacer la vue n’est pas une modification du document. Le presse-papiers conserve les bibliothèques et refuse les identités nominales incompatibles au lieu de convertir silencieusement les valeurs.

## Exécuter et inspecter {#run}

L’interpréteur prend en charge événements, branchements, séquences, calculs, comparaisons, conversions et variables. Types, nœuds et exécution sont bornés. Une modification de variable à l’exécution ne réécrit pas sa valeur déclarée. **Vérifier le graphe** signale les diagnostics structurels ; **Exécuter le graphe** lance la logique prise en charge. Enregistrer un brouillon ne prouve pas qu’il est exécutable.

Division par zéro, objet absent et broche incompatible sont des erreurs. Aucune liaison à la mémoire d’un moteur arbitraire n’est fournie. Consultez [fonctions et exécution](doc:functions), [objets d’exécution](doc:runtime-objects) et le [débogueur](doc:debugger).

## Comprendre une connexion {#connections}

Cet exemple interactif de documentation explique la connexion initiale entre événement et affichage. Vous pouvez déplacer la vue, zoomer et sélectionner les nœuds ; c’est une illustration, pas le runtime de l’éditeur.

```graph
{"id": "event-flow", "nodes": [{"id": "begin", "title": "Début du jeu", "x": 0, "y": 0, "outputs": [{"id": "then", "label": "then", "type": "exec"}], "description": "Événement initial de cet exemple de flux d’exécution."}, {"id": "print", "title": "Afficher une chaîne", "x": 330, "y": 0, "inputs": [{"id": "exec", "label": "exec", "type": "exec"}, {"id": "text", "label": "text", "type": "string"}], "description": "Reçoit l’exécution et une chaîne. Cette vue documentaire n’exécute pas le graphe."}], "links": [{"from": ["begin", "then"], "to": ["print", "exec"]}]}
```

## Réorganiser sans perte implicite {#refactor}

[Migrations de signatures](doc:signature-migration), [migrations de types](doc:type-migration), [réduction](doc:collapse) et [développement](doc:expand) passent par un aperçu et une application explicite sur un seul graphe propriétaire. Aucun asset ne migre au chargement. Conservez empreintes exactes, gardes de révision et annulation unique.

```image
{"id": "logic", "src": "assets/screenshots/logic.png", "alt": "Fonction d’exécution avec connexions typées", "controls": "minimal"}
```

## Gestes des groupes et presse-papiers {#native-groups}

Les groupes de commentaires utilisent les gestes communs de Nodes : sélection, déplacement, redimensionnement, renommage et suppression. Déplacer un groupe entraîne les nœuds qu’il contenait au début du geste. L’hôte valide la proposition complète puis applique groupes et nœuds dans une seule transaction `graph_layout_apply`. Échap ou un changement de révision du document annule le geste.

Un fragment générique de CyUI ne contient pas la sémantique d’exécution de cet éditeur. Les demandes de copie repassent donc par l’exporteur sémantique CyGameSDK, qui conserve types de nœuds, valeurs originales, déclarations locales et commentaires sélectionnés. Le collage utilise toujours le contrat vérifié de fragments, avec nouveaux identifiants de nœuds, liens et commentaires, et un Undo. Le collage par la barre d’outils ou le clavier reste le chemin sémantique pris en charge ; les actions génériques Coller/Dupliquer de CyUI ne sont pas activées.

Les reroutes d’exécution restent des nœuds de graphe typés, pas de simples points visuels sur un lien. Ces outils de présentation n’ajoutent ni compilateur de graphes de jeu ni codec Blueprint Unreal.
