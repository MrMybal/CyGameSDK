---
id: workspace
title: Espace de travail et aide
description: Espace de travail et aide
section: Premiers pas
order: 2
sourceRevision: 1
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
