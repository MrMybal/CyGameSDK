---
id: workspace
title: Espace de travail et aide
description: Espace de travail et aide
section: Premiers pas
order: 2
sourceRevision: 2
---

## Utiliser ce guide {#getting-started}

Le Sandbox de démarrage est une scène de démonstration. Ctrl+O charge un projet enregistré ; Ctrl+S le sauvegarde. L’installeur d’exemples refuse d’écraser les assets.

- Niveau : ancrez les panneaux, sélectionnez les objets, inspectez les propriétés et utilisez les gizmos.

- Graphes : ouvrez un exemple, inspectez les connexions typées et utilisez le débogueur.

- Matériaux : ouvrez Example_TexturedMaterial.cyasset et compilez l’aperçu DX11. C’est un shader d’éditeur, pas un export de matériau de jeu.

- Examples3D : ouvrez StaticTriangle.cy3d ou AnimatedTriangle.cy3d pour maillage, squelette, animation et échanges GLB/CyPack.

- Ctrl+P ouvre la recherche de commandes. Ctrl+Espace ouvre le tiroir de contenu. Le navigateur propose filtres, favoris et références.

## Pour aller plus loin {#next}

- [Première séance](doc:quickstart)
- [Parcourir les assets](doc:content-browser)
- [Édition de graphes](doc:graphs)
- [Matériaux](doc:materials)
- [Assets 3D](doc:assets-3d)
- [Référence des opérations SDK](doc:operations)

```image
{"id": "workspace", "src": "assets/screenshots/workspace.png", "alt": "Espace de travail Sandbox", "controls": "minimal"}
```


## Dispositions, densité et langue {#layouts}

Le menu Fenêtre affiche un panneau ou crée un viewport indépendant. Les dispositions Niveau, Graphes et Matériaux réorganisent les mêmes outils. Enregistrez/chargez explicitement une disposition nommée ; lettres, chiffres, tiret et soulignement sont acceptés. Dispositions et préférences ne relèvent pas de l’annulation du document. Le tiroir Ctrl+Espace peut rejoindre la disposition.

Options choisit thème, densité compacte/confortable, taille du texte et langue. L’anglais est la source, le défaut et le secours indépendamment de Windows. Un choix explicite du français est conservé. Les identifiants de données/fenêtres ne changent pas avec les libellés. Noms d’utilisateur et fichiers ne sont jamais traduits. Échap annule une saisie ; un champ masqué ou périmé ne doit pas rejouer d’anciennes entrées ImGui sur le nouvel état SDK.

## Aide contextuelle {#help}

Aide > Documentation ouvre le manuel local. F1 ouvre le guide du panneau d’édition actif, dans la langue de l’éditeur. Donnez le focus à un outil, appuyez sur F1 puis suivez son lien de section stable. Cela ouvre un fichier local, sans serveur de documentation.

## Propriétés typées {#typed-properties}

Details utilise maintenant l’Inspector commun de CyUI pour les champs scalaires, vecteurs, structures, tableaux, Sets et Maps. Double-cliquez sur un nombre ou choisissez son action d’expression pour saisir un calcul tel que `Value*2`. L’application valide toujours la proposition selon le schéma de l’adaptateur ou du graphe avant de l’appliquer. Le stockage exact des entiers est conservé.

Les collections gardent leur schéma d’élément lorsqu’elles sont vides. Leurs lignes sont virtualisées, avec filtrage et actions de collection communes. Les limites existantes de nombre, de taille et de type du graphe restent en vigueur ; un widget plus complet n’étend pas le format documentaire. Les membres d’énumérations conservent leur texte original et leur identité nominale locale au graphe.

Échap annule un brouillon actif. Un changement externe du document ou un champ masqué annule une édition périmée. Une modification de champ ou de collection validée produit une seule transaction Undo du document.

## Hiérarchie et table de scène {#scene-table}

Le World Outliner alterne entre **Hiérarchie** et **Vue tabulaire**. La hiérarchie permet navigation clavier, sélection Ctrl/Maj, renommage F2, visibilité, verrouillage et déplacement d’objets dans les dossiers. **Centrer sur la sélection** cadre l’objet courant dans le viewport. Les lignes de dossiers représentent l’organisation existante de la scène, pas une parenté d’entités du moteur.

La table affiche les propriétés de l’adaptateur, y compris vecteurs et couleurs typés. Triez ou filtrez les lignes visibles, sélectionnez une plage rectangulaire et utilisez Ctrl+C/Ctrl+V pour le TSV typé. Un collage valide tout le lot avant de modifier une cellule ; un champ en lecture seule, un objet verrouillé ou une valeur invalide rejette le lot. Un collage accepté correspond à un Undo. Ces éditions utilisent l’opération Core `property_set_many`, également accessible au SDK.

## Raccourcis clavier {#shortcuts}

Options > **Raccourcis clavier** permet de réaffecter les commandes communes de sauvegarde/chargement, Undo/Redo, recherche, tiroir de contenu et aide contextuelle. Les conflits dans un même contexte sont refusés. Échap annule la capture et Retour arrière efface une liaison. Les raccourcis sont des préférences d’interface ; menus et infobulles de la barre d’outils affichent la liaison courante du registre. La langue choisie ne change pas l’identité des commandes.

Les éditeurs de texte conservent leur propre annulation pendant la saisie. Les commandes documentaires globales passent par le répartiteur EditorCore existant.
