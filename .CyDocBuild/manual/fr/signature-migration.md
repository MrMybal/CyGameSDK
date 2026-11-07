---
id: signature-migration
title: Migrer les signatures de fonctions et méthodes
description: Migrer les signatures de fonctions et méthodes
section: Graphes
order: 16
sourceRevision: 1
parent: graphs
---

## Prévisualiser avant d’appliquer {#preview}

Ouvrez la migration de signature d’une fonction, macro ou méthode prise en charge dans un seul graphe propriétaire. Décrivez entrées/sorties, valeurs par défaut et renommages souhaités. L’aperçu présente la fermeture des définitions et appels concernés, incluant contrats d’interface, redéfinitions de méthodes et liaisons partagées. Il ne modifie pas le document.

Le candidat est recalculé à partir de la requête, validé et identifié par un `planHash` exact. L’application exige cette empreinte et la même requête. Une source/révision modifiée ou un plan altéré est refusé. Examinez connexions retirées et valeurs de secours réinitialisées. Une perte demande un `acceptLoss` explicite ; une empreinte seule ne vaut pas consentement à perdre des données.

## Ce qui reste stable {#preservation}

Une seule annulation du propriétaire entier restaure le graphe. Les métadonnées inconnues sont conservées. Nature, mode pur/exécution, `self` et contrats item/index/break du foreach ne doivent pas changer par cette migration. Rien ne migre au chargement ; aucun registre externe ni objet moteur vivant n’est modifié.

Les opérations SDK de plan et d’application sont décrites dans le catalogue. Découvrez les champs requis plutôt que de modifier un plan côté client. [Référence des opérations](doc:operations).
