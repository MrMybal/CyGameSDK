---
id: content-browser
title: Fichiers, assets et références
description: Fichiers, assets et références
section: Ressources
order: 20
sourceRevision: 1
---

## Parcourir et filtrer {#browse}

Le navigateur reflète le véritable système de fichiers `data/assets`. Utilisez arbre et fil d’Ariane, grille/liste, filtres par type/champ et favoris. Le registre choisit un éditeur selon l’extension ; reconnaître celle-ci ne prouve pas qu’un format de jeu est décodé. Le rendu commun CyUI n’expose pas chaque action contextuelle spécifique : utilisez **Actions sur l’asset** pour les workflows étendus et les métadonnées sélectionnées pour type et taille.

`imported:true` et `imported:false` filtrent les associations d’import. Le décodage des vignettes est borné. Les compagnons `.cyimport` cachés ne sont pas manipulables directement par l’API ordinaire.

## Cycle de vie des fichiers {#files}

Créez des dossiers, copiez, renommez/déplacez, mettez à la corbeille et restaurez depuis l’éditeur. Les chemins restent relatifs et confinés au contenu. Écrasement, traversée et documents modifiés sont protégés. Les références peuvent empêcher déplacement ou suppression ; inspectez les consommateurs et enregistrez leurs changements. Imports et déplacements sont des opérations persistantes sur fichiers, distinctes de l’annulation du document.

Une copie d’image conserve son association d’import et les compagnons suivent les déplacements. Une copie .cy3d devient un document indépendant avec nouvel UUID et sans compagnon de réimport. Ne copiez jamais runtime ou préférences comme s’il s’agissait d’un asset.

## Inspecter les références {#references}

La vue combine déclarations enregistrées et actives, dépendances/consommateurs directs et transitifs, cibles absentes et cycles. Le scan à la demande est borné et signale les résultats incomplets. Une référence absente n’est pas réparée silencieusement. L’index connaît les relations .cyasset déclarées, pas les dépendances des formats binaires inconnus.

[Import et réimport](doc:asset-import) · [Identité portable](doc:portable-assets)
