---
id: automation
title: Automatisation MCP et SDK
description: Automatisation MCP et SDK
section: Premiers pas
order: 3
sourceRevision: 1
---

## Utiliser ce guide {#getting-started}

Démarrez l’éditeur. Configurez votre client MCP avec app/mcp.example.json et les chemins absolus de votre installation. Exécutez app/tool.py avec l’environnement Python privé. CyToolsCore fournit serveur et jobs.

```powershell
.\runtime\python\Scripts\python.exe app\tool.py describe
.\runtime\python\Scripts\python.exe app\tool.py mcp
```

- Découvrez les opérations et leurs schémas avant de les appeler.

- Interface, IPC et SDK partagent validation d’EditorCore et transactions de documents.

- Suivez les jobs jusqu’à un état terminal. Après un timeout de modification, inspectez l’état réel avant de réessayer : la modification peut déjà être appliquée.

## Pour aller plus loin {#next}

- [Première séance](doc:quickstart)
- [Parcourir les assets](doc:content-browser)
- [Édition de graphes](doc:graphs)
- [Matériaux](doc:materials)
- [Assets 3D](doc:assets-3d)
- [Référence des opérations SDK](doc:operations)
