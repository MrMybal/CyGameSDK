---
id: extensions
title: Adaptateurs source et extensions de l’éditeur
description: Adaptateurs source et extensions de l’éditeur
section: Intégration
order: 60
sourceRevision: 1
parent: game-adapters
---

## Points d’extension publics {#contracts}

`IGameAdapter` fournit `descriptor()`, `propertySchema(entity)`, `createEntity(id,name)`, `initialScene()`, `validateScene(entities)` et éventuellement `buildTargets()`/les déclarations de codecs. Acceptez une entité vide pour découvrir le schéma générique. Gardez lecture, validation et sérialisation indépendantes d’ImGui. Injectez l’implémentation à la construction d’`EditorCore` dans un projet séparé.

`EditorApp` enregistre éditeurs d’assets et callbacks de viewport/maillage/matériau/texture. `IEditorPlugin` possède un cycle de vie source ; l’activation reçoit `EditorCore&`, pas un objet UI dynamique arbitraire. Diagnostics est un exemple source, pas un chargeur de DLL à ABI stable. Retirez les callbacks avant destruction de leurs données capturées. Les assets ne fournissent pas de plugins exécutables.

## Hypothèses Sandbox à revoir {#assumptions}

Le modèle conserve l’ID initial `hero`, propriétés de transformation, convention StaticMesh, graphes logique/matériau initiaux et limites scène/projet. Le viewport par défaut projette des primitives Sandbox ; le rendu et la sélection du jeu sont à raccorder explicitement. Schémas réels, réflexion des entités/composants, persistance et outils demandent une implémentation vérifiée, pas un changement de libellé.

`ExampleGameAdapter.h` ajoute un champ Health borné et conserve les transformations. C’est un exemple, pas un décodeur de jeu. Son contrat public annoté figure dans l’arbre API ; le code complet reste dans la distribution source, sans navigateur de sources dans ce manuel.

Ajoutez les fichiers aux cibles CMake, choisissez les modules CyUI explicitement et conservez un ImGui compatible unique. Préservez anglais par défaut/de secours, identifiants stables et notices. Consultez guide IA et dossier de portage pour dériver un SDK ; les consignes aux assistants restent séparées du manuel humain.

## Exemples de code de référence {#code-examples}

Exemples originaux des guides source. Adaptez chemins, noms et valeurs explicatives ; ils ne constituent pas un codec fonctionnel pour un jeu arbitraire.

```cpp
#include "../examples/adapters/ExampleGameAdapter.h"
// Replace the existing EditorCore core(root) construction, not the whole host:
EditorCore core(root, std::make_unique<cygame::examples::ExampleGameAdapter>());
EditorApp app(core);
```

```powershell
.\runtime\python\Scripts\python.exe app\build_manifest.py
.\runtime\python\Scripts\python.exe app\tool.py describe
```
