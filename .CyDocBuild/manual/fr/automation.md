---
id: automation
title: Assistant, CLI et MCP
description: Utiliser l’assistant intégré et le registre natif d’automatisation.
section: Prise en main
order: 3
revision: 3
sourceRevision: 3
---

## Utiliser ce guide {#getting-started}

CyMCPSDK 0.7.0 fournit l’assistant natif, le registre d’opérations et les transports MCP.
L’éditeur conserve ses validations réelles, ses documents persistants et son Undo.
Découvrez une commande et consultez son schéma avant de l’exécuter.

## Assistant intégré {#assistant}

Ouvrez **Assistant IA** depuis les modules ou le menu Fenêtres. Dans **Paramètres de
connexion**, détectez/sélectionnez un fournisseur, chargez son catalogue réel de modèles
et connectez-vous. Les clés API utilisent le champ masqué en mémoire ou la variable
d’environnement indiquée. Elles ne sont pas enregistrées. Les agents CLI installés
conservent la gestion de leurs comptes.

Activez **Autoriser les modifications dans l’éditeur** pour les écritures. Ce réglage
s’applique à l’assistant et au point MCP de cette application. Codex et les transports
API compatibles reçoivent directement les outils de l’éditeur. Les autres agents CLI
utilisent leur configuration MCP native.

Les réponses apparaissent progressivement dans le panneau Markdown commun. **Arrêter**
annule le tour. Erreurs et réponses partielles interrompues restent visibles. Le sélecteur
commun ajoute des pièces jointes selon les capacités du fournisseur/modèle ; l’aperçu
actuel présente les noms des fichiers plutôt qu’un nouvel éditeur d’assets.

L’historique est privé. Choisissez une conversation puis **Restaurer la conversation
enregistrée** pour reprendre sans renvoyer de message. **Connecter / Nouvelle conversation**
conserve les historiques précédents.

## Commandes natives du terminal {#cli}

~~~powershell
.\CyGameCLI.exe --help
.\CyGameCLI.exe list
.\CyGameCLI.exe describe-operation property_set
.\CyGameCLI.exe --root C:\Editors\MyGame invoke scene_list
.\CyGameCLI.exe --root C:\Editors\MyGame --allow-write invoke property_set --params-file change.json
~~~

Le fichier UTF-8 change.json contient les arguments réels, par exemple :

~~~json
{"entityId":"hero","propertyId":"name","value":"Edited from CLI"}
~~~

Découverte, schémas, diagnostics et récupération durable ne nécessitent pas d’interface.
Les opérations de scène nécessitent un éditeur ouvert avec le même **--root**. Aucune
instance cachée ne démarre automatiquement. Stdout contient le JSON ; stderr les diagnostics.
Codes : 0 réussite, 1 erreur métier/transport, 2 entrée invalide, 3 opération refusée.

Les commandes app/tool.py restent disponibles. Son invoke conserve une sortie de type
job ; le CLI natif retourne directement le résultat métier. Les modes jsonl/stdio natifs
utilisent le SDK ; app/tool.py stdio conserve les anciennes enveloppes prises en charge.

## Connecter une IA externe {#mcp}

Adaptez app/mcp.example.json avec CyGameCLI.exe et la racine du projet. Par défaut, le
serveur stdio expose cinq outils de découverte/ressources. Recherchez, chargez un schéma,
puis appelez l’opération d’origine. **mcp --full** expose le catalogue complet. Les alias
de jobs historiques **op_<id>** restent découvrables.

L’ouverture de l’assistant fournit aussi les accès loopback authentifiés **/mcp** et
**/v1**. Adresse et jeton aléatoires sont conservés dans data/ipc/automation.json, limité
au propriétaire. Gardez le jeton privé. Un point représente une identité authentifiée ;
des identités distantes distinctes nécessitent des points/stockages isolés. Stdio natif
et le pipe Windows utilisent l’identité du processus propriétaire.

Un service séparé **serve --port PORT** nécessite la variable privée CYGAME_MCP_TOKEN.
**--connect URL --token-env NAME** permet d’adresser son interface /v1 depuis le CLI.

## Contexte, ressources et confidentialité {#optimizations}

Catalogue compact, TTL de lecture, notifications, rétention/quota disque des résultats,
mesures contenant uniquement des métadonnées et fenêtre de contexte API restent des
réglages indépendants. Les options serveur/stockage s’appliquent à la réouverture ;
celles du fournisseur à la prochaine connexion.

Les longues conversations API conservent leur historique complet. La projection garde
instructions système, tour actif et échanges d’outils complets. Les agents natifs gèrent
leur propre contexte. Aucun modèle de résumé n’est appelé automatiquement.

Les gros résultats utilisent des manifestes/URI privés ; suivez firstUri puis nextUri
pour récupérer le contenu complet. Une expiration ne relance jamais l’action. Les fichiers
durables restent disponibles par les [outils de jobs](doc:jobs). Le grep natif recherche
les textes exposés ; aucun arbre de fichiers ni dossier ripgrep n’est partagé. Les outils
d’autres applications ne sont pas attachés : l’éditeur n’en dépend actuellement pas.

## Champs source et JSON {#source-fields}

Paramètres d’opérations, déclarations de classes, options de codecs et JSON du fournisseur
utilisent l’éditeur de code commun : couleurs syntaxiques, sélection/recherche, Undo texte
et zoom. Limites de taille et actions explicites Exécuter/Appliquer/Connecter restent en
place. Saisir du texte ne lance pas d’outil et n’écrit pas de fichier du jeu. Le HLSL
généré et les résultats restent en lecture seule.

## Continuer {#next}

- [Jobs durables et récupération](doc:jobs)
- [Première session](doc:quickstart)
- [Référence des opérations](doc:operations)
- [Compiler et lancer](doc:setup)
