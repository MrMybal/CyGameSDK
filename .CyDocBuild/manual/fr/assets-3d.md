---
id: assets-3d
title: Édition de maillages, squelettes et animations
description: Édition de maillages, squelettes et animations
section: Ressources
order: 22
sourceRevision: 1
---

## Ouvrir et modifier un document neutre {#edit}

Importez un GLB/CyPack autonome vers un nouveau `.cy3d`, puis double-cliquez-le. Chaque modèle a son onglet ancrable. Outils > Assets 3D liste les documents chargés. Fermer l’onglet masque la vue ; **Fermer le document** libère un document enregistré et refuse les changements non sauvegardés. Glisser à droite orbite, la molette zoome, Cadrer réinitialise et Filaire/Squelette changent l’aperçu de triangles réel.

Maillage sélectionne maillage/primitive/sommet et modifie position, normale, UV, articulations et poids. Normales et sommes de poids sont normalisées ; les indices d’articulations restent entiers. Squelette modifie nom, parent et TRS. Cycles et quaternions nuls échouent atomiquement. Un nœud matriciel demande une décomposition avant TRS ; les matrices de pose inverse ne sont pas reconstruites automatiquement après modification du rig.

Animation choisit le clip, lit/met en pause/boucle/parcourt, crée clips/pistes TRS et insère/retire des clés. Les temps sont strictement croissants et une piste garde au moins une clé. Un clip vide doit recevoir une piste avant export GLB. LINEAR, STEP et CUBICSPLINE sont évalués ; les tangentes cubiques survivent à l’export sans poignées d’édition. L’édition utilise l’annulation du document. Sauvegarde explicite ; export GLB/CyPack par job Core.

## LOD, collisions et retargeting {#authoring}

Générez les LOD statiques avec le simplificateur borné puis inspectez le résultat. Modifier la géométrie périme les LOD générés et marque les collisions à revoir. Les exporteurs doivent respecter ces marqueurs. Les collisions utilisent des formes primitives, pas un préparateur physique de jeu complet. Le retargeting de pose de référence associe explicitement os et poses ; il ne déduit pas automatiquement un humanoïde. Préservez source et données de liaison, et lisez les pertes. Les [outils de mouvement](doc:animation-tools) ajoutent IK hors ligne, paires de mouvement racine et rééchantillonnage.

## Document et limites du profil {#limits}

L’enveloppe JSON contient `format: cygame-scene3d`, `schemaVersion: 1`, UUID canonique `assetId`, nœuds, maillages, skins, matériaux, animations, extensions et source. Repère droit +Y haut, mètres, quaternions XYZW et secondes. Les IDs stables sont `node:<source index>` ; changer le parent ne change pas l’identité du tableau. `source.glbBase64` conserve le GLB d’origine ; provenance CyPack et métadonnées nommées sont préservées. Les flux reconstruits s’ajoutent aux enregistrements originaux sans garantir la validité sémantique d’extensions glTF inconnues après édition.

Source : 32 Mio. JSON éditable : 2 Mio avec source. Géométrie/animation : 20 000 sommets/triangles et 20 000 clés, 512 nœuds, 256 articulations/skin, 128 clips. Huit modèles / 4 Mio compacts chargés au maximum ; projet limité à 8 Mio. Extensions obligatoires, géométrie compressée/morph et primitives non triangulaires sont refusées. Ce profil sert aux petits assets, pas à remplacer un DCC ou moteur de rendu complet.

```image
{"id": "mesh", "src": "assets/screenshots/mesh.png", "alt": "Éditeur de maillage neutre et propriétés des sommets", "controls": "minimal"}
```

```image
{"id": "skeleton", "src": "assets/screenshots/skeleton.png", "alt": "Hiérarchie du squelette et édition TRS", "controls": "minimal"}
```
