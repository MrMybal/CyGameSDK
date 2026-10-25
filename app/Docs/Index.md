# Documentation CyGameSDK et références CyToolsCore

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
