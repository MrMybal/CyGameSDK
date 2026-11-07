---
id: setup
title: Compiler et lancer sous Windows.
description: Compiler et lancer sous Windows
section: Premiers pas
order: 1
sourceRevision: 1
---

## Utiliser ce guide {#getting-started}

Installez Visual Studio 2022 avec Développement Desktop en C++, le SDK Windows, Git, CMake 3.24 ou plus récent et Node.js 22+ avec npm (génération du manuel seulement). Clonez ou extrayez les sources dans un nouveau dossier. Exécutez ces commandes à la racine du projet.

```powershell
.\app\build.ps1
.\app\setup_mcp.ps1
.\runtime\python\Scripts\python.exe app\setup_examples.py
.\Lancer.cmd
```

- Le build génère l’aide locale avec CyDoc verrouillé et récupère la révision exacte de app/dependencies.lock.json. Si le dépôt est privé, authentifiez Git au préalable ; jamais d’identifiants dans les sources ou arguments CMake.

- Pour compiler hors ligne, préparez les checkouts CyUI/CyDoc verrouillés et les caches npm complets. -CyUIRoot sélectionne CyUI. Un checkout propre ne remplace pas les dépendances npm. La lecture du manuel exporté ne nécessite ni ces outils ni le réseau.

- setup_mcp.ps1 nécessite la wheel CyToolsCore 0.10.1 construite depuis un checkout autorisé. Sélectionnez son emplacement avec -CyToolsCoreRoot.

- L’hôte nécessite le runtime Microsoft C++. MCP et les jobs d’import, de shaders et de compilation nécessitent l’environnement Python/Core configuré.

## Pour aller plus loin {#next}

- [Première séance](doc:quickstart)
- [Parcourir les assets](doc:content-browser)
- [Édition de graphes](doc:graphs)
- [Matériaux](doc:materials)
- [Assets 3D](doc:assets-3d)
- [Référence des opérations SDK](doc:operations)
