---
id: quickstart
title: Votre première séance
description: Votre première séance
section: Premiers pas
order: 1
sourceRevision: 1
---

## Ouvrir le Sandbox {#open}

Lancez l’éditeur compilé avec `Lancer.cmd`. La scène initiale est le Sandbox, pas un jeu installé. Choisissez **Niveau** dans le sélecteur d’espace de travail. Ctrl+O recharge un projet explicitement sauvegardé ; le démarrage ne recharge pas automatiquement le précédent.

1. Sélectionnez `SM_Cube` dans la hiérarchie ou cliquez dessus dans le viewport.
2. Dans Détails, modifiez Position. Validez le champ, puis utilisez Ctrl+Z et Ctrl+Y pour annuler et rétablir.
3. Utilisez W/E/R pour Déplacer/Tourner/Échelle quand le viewport reçoit les entrées. Choisissez le repère et le magnétisme. La translation de groupe existe ; rotation et échelle de groupe ne sont pas un workflow terminé.
4. Ctrl+P recherche un panneau, une commande ou un objet. Ctrl+Espace ouvre le tiroir de contenu.
5. Ctrl+S enregistre le projet dans `data/projects` de cette installation. Gardez les fichiers du jeu installé séparés.

## Essayer les exemples {#examples}

Après installation des exemples avec `app/setup_examples.py`, ouvrez `StaticTriangle.cy3d`, `AnimatedTriangle.cy3d` ou `Example_Data.json` dans Examples3D. Les graphes d’exemple se trouvent dans les dossiers Examples et BlueprintExamples. L’installation n’écrase jamais un asset existant ; les noms et dossiers dépendent du lot d’exemples choisi.

Ouvrez `Example_TexturedMaterial.cyasset` et choisissez **Compiler l’aperçu** après configuration de Python/Core. Le résultat est un shader d’éditeur, pas un export pour un jeu. [Workflow des matériaux](doc:materials#compile).

## Corriger une erreur {#recover}

Annulez les modifications de documents et sauvegardez explicitement. Un déplacement de fichier peut être bloqué si un graphe enregistré ou actif le référence. Lisez le diagnostic plutôt que de répéter l’opération. Après un timeout SDK, inspectez l’état réel avant de réessayer.
