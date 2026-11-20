---
id: setup
title: Compiler et lancer sous Windows.
description: Compiler et lancer sous Windows
section: Premiers pas
order: 1
sourceRevision: 5
---

La préversion Windows 0.32.0 inclut les profils Free. Extrayez le dossier, lancez
l’éditeur et copiez son adresse MCP locale pour vous connecter sans jeton en Free.

## ZIP de préversion Windows {#prebuilt}

Le ZIP Windows 0.32.0 contient CyGameEditor.exe, CyGameCLI.exe, les sources/exemples,
les notices et ce lecteur hors ligne complet. Extrayez-le dans un nouveau dossier
accessible en écriture et lancez Lancer.cmd. Le runtime Microsoft Visual C++ 2015–2022 x64 est requis.

Édition native, assistant et CLI/MCP ne nécessitent ni compilateur, ni checkout des
bibliothèques, ni Node.js, ni Python. Imports, shaders, packaging et calculs 3D demandent
Python 3.11+ puis app/setup_mcp.ps1. Aucun venv, identifiant ou poids de modèle n’est
embarqué. Utilisez votre fournisseur/compte. Vérifiez les empreintes de release avant remplacement.

Le ZIP de documentation distinct contient le même lecteur. Ouvrez Documentation.html
avec son dossier DocData complet ; aucun serveur ni connexion n’est requis. Le manuel
hébergé sur Pages suit son dernier déploiement.

## Utiliser ce guide {#getting-started}

Installez Visual Studio 2022 avec Développement Desktop en C++, le SDK Windows, Git, CMake 3.24 ou plus récent et Node.js 22+ avec npm (génération du manuel seulement). Clonez ou extrayez les sources dans un nouveau dossier. Exécutez ces commandes à la racine du projet.

```powershell
.\app\build.ps1
.\app\setup_mcp.ps1
.\runtime\python\Scripts\python.exe app\setup_examples.py
.\Lancer.cmd
```

- Le build génère l’aide locale avec CyDoc verrouillé et compile CyMCPSDK et récupère les révisions exactes de app/dependencies.lock.json. Si le dépôt est privé, authentifiez Git au préalable ; jamais d’identifiants dans les sources ou arguments CMake.

- Pour compiler hors ligne, préparez les checkouts CyUI/CyMCPSDK/CyDoc verrouillés et les caches npm complets. -CyUIRoot et -CyMCPSDKRoot sélectionnent les checkouts propres correspondants. Un checkout propre ne remplace pas les dépendances npm. La lecture du manuel exporté ne nécessite ni ces outils ni le réseau.

- setup_mcp.ps1 installe uniquement les dépendances Python des calculs bornés. CyMCPSDK est compilé depuis ses sources C++ ; aucune ancienne wheel de runtime n’est requise.

- L’hôte nécessite le runtime Microsoft C++. Assistant, CLI et MCP sont natifs. Les jobs d’import, de shaders et de compilation utilisent le Python privé configuré.

## Pour aller plus loin {#next}

- [Première séance](doc:quickstart)
- [Parcourir les assets](doc:content-browser)
- [Édition de graphes](doc:graphs)
- [Matériaux](doc:materials)
- [Assets 3D](doc:assets-3d)
- [Référence des opérations SDK](doc:operations)
