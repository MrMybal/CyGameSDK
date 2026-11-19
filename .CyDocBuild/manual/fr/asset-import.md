---
id: asset-import
title: Importer et réimporter des sources
description: Importer et réimporter des sources
section: Ressources
order: 21
sourceRevision: 3
parent: content-browser
---

## Importer explicitement {#import}

Choisissez **Navigateur de contenu > Ajouter > Importer un fichier source**. Sélectionnez une source absolue hors de `data/assets`, une destination relative nouvelle et un fournisseur enregistré. **Valider la source** prépare le plan ; **Importer** lance un job Core officiel. `image.copy` conserve octets et extension PNG/JPEG/BMP ; `scene3d.glb` convertit un GLB/CyPack autonome vers le profil .cy3d borné. Aucun n’est un codec de jeu.

Inspection, empreintes, octets préparés et confinement sont revérifiés lors de la validation finale sur le thread propriétaire. Taille et date de source sont surveillées pendant la lecture. Le runtime SDK doit garder les rapports applicables sous la racine de l’éditeur. L’import écrit des fichiers persistants hors de l’annulation du document ; les réglages de vue/requête ne changent pas sa révision.

## Réimporter sans perdre les modifications {#reimport}

Ouvrez les paramètres d’import dans Actions sur l’asset. **Vérifier la source** compare les SHA-256 : à jour, source modifiée, changements locaux, source ou asset absent. Le réimport réutilise fournisseur/source et refuse d’effacer les changements de destination. Préservez-les d’abord. Actualisation explicite : ni reliaison de source ni surveillance du disque ne sont implémentées.

`<asset>.cyimport` est un JSON UTF-8 borné à 64 Kio : fournisseur, source absolue locale, empreintes, taille et date d’import. C’est une métadonnée du projet accessible aux clients autorisés, pas une dépendance portable. Les assets survivent au nettoyage des tâches ; préparation et rapports restent des fichiers de jobs durables.

## Annulation et récupération {#recovery}

Avant validation finale, annuler laisse la destination absente/inchangée. Une fois l’écriture commencée, une réponse interrompue ou annulée peut coexister avec un import terminé : inspectez avant de réessayer. Le remplacement de la paire n’est pas une transaction atomique globale du système de fichiers. Si le retour arrière échoue, gardez `data/import-recovery/<id>/transaction.json` et ses sauvegardes `a`/`m`. Fermez l’éditeur avant de restaurer asset et compagnon. La récupération automatique après crash n’est pas implémentée. Les ZIP Sandbox n’incluent pas les fichiers importés.

Limites WIC : source de 32 Mio, 8192 pixels maximum par côté et 4 194 304 pixels décodés. La 3D neutre a [son propre profil](doc:assets-3d#limits).

## Sélecteur de sources local {#file-picker}

**Browse source...** ouvre le sélecteur natif CyUI dans l’éditeur. Choisissez un volume ou un dossier, utilisez les filtres de fichiers pris en charge/images/3D, recherchez dans la liste mise en cache et sélectionnez une source. Le parcours du dossier se déroule hors du rendu ; une liste en cours de chargement, périmée ou en erreur ne peut pas être acceptée. La liste est limitée à 10 000 entrées.

La sélection remplit la source et propose une destination. Elle n’importe rien avant validation et lancement de l’action d’import existante. L’hôte revérifie l’existence de la source ; fournisseur d’import, limites de taille, confinement des destinations et protections de réimport restent la référence.
