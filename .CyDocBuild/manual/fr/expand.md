---
id: expand
title: Développer un appel pur en nœuds modifiables
description: Développer un appel pur en nœuds modifiables
section: Graphes
order: 19
sourceRevision: 1
parent: graphs
---

## Prévisualiser un appel {#preview}

Sélectionnez un `function.call` ou `macro.call` local pur, puis **Développer l’appel > Aperçu > Appliquer**. Sa définition reste dans Mon graphe. L’exemple `Example_BlueprintExpand.cyasset` affiche 9 avant et après. Rien ne se développe au chargement et aucun fichier n’est sauvegardé implicitement.

Tous les nœuds internes sont copiés, même inactifs, avec identifiants déterministes sans collision. Les appels imbriqués restent des appels, sans développement récursif. Valeurs, commentaires, arrêts, métadonnées et positions relatives sont préservés. Des reroutages typés conservent identifiants des fils externes, embranchements, arguments cachés et valeurs de secours. Un commentaire de provenance garde le JSON de l’appel original, `sourceHash` et le nom de fonction. Supprimer ce commentaire retire volontairement cette provenance.

## Gardes d’évaluation {#guards}

Chaque sortie connectée d’une fonction pure doit lire tous les arguments câblés, car une fonction les évalue immédiatement. L’analyse inclut macros paresseuses imbriquées et dépendances par sortie UV ; elle refuse une expansion qui masquerait division par zéro ou accès d’objet invalide. Les macros s’intègrent déjà paresseusement et gardent les reroutages entrants inaccessibles. Une sortie de fonction non câblée garde le secours du nœud de retour ; celle d’une macro garde le défaut déclaré. Un appel inactif sans sortie connectée peut se développer sans évaluer ses entrées.

## Appliquer et restaurer {#apply}

Le candidat et le développement runtime des macros sont recalculés et validés. Fournissez l’empreinte exacte ; ne modifiez pas l’aperçu côté client. Une annulation restaure l’appel, un rétablissement restaure les nœuds. Cadrer tout / Origine retrouve les nœuds hors champ. La limite comprend 128 nœuds internes et jusqu’à 32 reroutages de frontière ; les budgets du propriétaire s’appliquent aussi. La [réduction](doc:collapse) extrait une zone pure dans l’autre sens.
