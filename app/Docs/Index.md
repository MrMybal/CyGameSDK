# CyGameSDK documentation sources

The human manual is now the complete local [CyDoc reader](../../Documentation/Documentation.html#/en/overview)
([français](../../Documentation/Documentation.html#/fr/overview)). Open it directly;
no server or Node.js is needed to read it. Help/F1 in the editor uses the same reader.

These Markdown files remain source-level contracts, historical evidence and generic
CyToolsCore references. They are preserved for developers and compatibility; they
are not a replacement HTML help system. Generic model/CUDA/downloader requirements
apply only to integrations that actually use those components.

Authoring/build instructions and content disposition:
[CyDoc integration](CyDocIntegration.md). Start a game adaptation with
[AI_GUIDE.md](../../AI_GUIDE.md).

---

## Preserved source index

## Pour adapter cet éditeur à un jeu

- [Point d’entrée IA CyGameSDK](../../AI_GUIDE.md).
- [Guide d’adaptation par jeu](GameAdapterGuide.md) : API et limites réelles du socle.
- [Dossier de portage à remplir](GamePortingTemplate.md).
- [Format commun portable](CommonAssetFormat.md) : transferts de meshes, rigs et animations.
- [Exemple d’adaptateur source](../examples/adapters/README.md).
- [Audit de l’interface et thème](InterfaceAudit.md).
- [Architecture éditeur](EditorArchitecture.md), [couverture](CapabilityCoverage.md),
  [fournisseurs de build](BuildProviders.md), [matériaux](MaterialPipeline.md).

Les guides suivants sont les références générales CyToolsCore. Leurs exigences
spécifiques aux modèles, CUDA et outils de génération s’appliquent seulement si
le produit créé utilise réellement ces fonctions.

## Pour créer un Tool

- [Guide IA](AgentGuide.md) : contrat de travail, configuration et définition de terminé.
- [Prompt prêt à copier](AgentPrompt.md).
- [Premier CyTool](CreatingYourFirstCyTool.md) : installation et commandes.

## Standard et architecture

- [Vue d'ensemble](Overview.md), [architecture](Architecture.md), [manifest](ToolManifest.md).
- [Opérations](Operations.md), [paramètres](Parameters.md), [sorties](Outputs.md).
- [Protocole](Protocol.md), [versions](Versioning.md), [erreurs](ErrorModel.md).

## Exécution

- [Jobs](Jobs.md), [multi-client](MultiClient.md), [scheduler](Scheduler.md), [événements](Events.md).
- [Ressources](Resources.md), [plateformes](Platforms.md), [modèles](Models.md), [workers](Workers.md).
- [Dépendances](Dependencies.md), [téléchargements](Downloads.md).
- [Transports](Transports.md), [MCP](MCP.md).

## Validation et limites

- [Fonctionnalités livrées](ImplementationStatus.md).
- [Résultats de validation](Validation.md).
- [Licence MIT et redistribution](Licensing.md).

## Mettre à jour un produit existant

- [Migration des logiciels et plugins utilisant les SDK](UpdateExistingProducts.md).
