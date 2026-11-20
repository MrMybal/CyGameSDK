---
id: automation
title: Assistant, CLI et MCP
description: Utiliser l’assistant intégré et le registre natif d’automatisation.
section: Prise en main
order: 3
revision: 4
sourceRevision: 4
---

## Utiliser ce guide {#getting-started}

CyMCPSDK 0.7.1 fournit l’assistant natif, le registre d’opérations et les transports MCP.
L’éditeur conserve ses validations réelles, ses documents persistants et son Undo.
Découvrez une commande et consultez son schéma avant de l’exécuter.

## Assistant intégré {#assistant}

Ouvrez **Assistant IA** depuis les modules ou le menu Fenêtres. Dans **Paramètres de
connexion**, détectez/sélectionnez un fournisseur, chargez son catalogue réel de modèles
et connectez-vous. Les clés API utilisent le champ masqué en mémoire ou la variable
d’environnement indiquée. Elles ne sont pas enregistrées. Les agents CLI installés
conservent la gestion de leurs comptes.

Free est le profil par défaut et autorise toutes les opérations enregistrées sans
confirmation d’écriture supplémentaire. Local contrôlé et Protégé conservent
**Autoriser les modifications dans l’éditeur**. Codex et les API compatibles reçoivent
directement le registre ; les autres agents CLI utilisent leur configuration MCP native.

Les réponses apparaissent progressivement dans le panneau Markdown commun. **Arrêter**
annule le tour. Erreurs et réponses partielles interrompues restent visibles. Le sélecteur
commun ajoute des pièces jointes selon les capacités du fournisseur/modèle ; l’aperçu
actuel présente les noms des fichiers plutôt qu’un nouvel éditeur d’assets.

L’historique est privé. Choisissez une conversation puis **Restaurer la conversation
enregistrée** pour reprendre sans renvoyer de message. **Connecter / Nouvelle conversation**
conserve les historiques précédents.

## Profils d’accès {#access-profiles}

**Paramètres de connexion > MCP local** affiche le profil actif et celui du prochain
démarrage. Redémarrez l’éditeur après avoir choisi un autre profil. La case MCP local démarre/arrête HTTP immédiatement.

| Profil | Jeton | Opérations enregistrées |
| --- | --- | --- |
| Free | Non | Toutes autorisées |
| Local contrôlé | Non | Politique de lecture/écriture hôte |
| Jeton seul | Requis | Toutes autorisées |
| Protégé | Requis | Politique de lecture/écriture hôte |

Les nouvelles configurations et celles sans choix explicite migrent vers **free**.
Profils déjà choisis et réglages sans rapport sont conservés. L’ancien allowChanges est
préservé mais ne constitue pas un choix de profil. La validation reste active : arguments
invalides, objets verrouillés et gardes de révision restent refusés. Le champ est
**access_profile** dans data/settings/automation.json. **--access-profile** ne remplace le profil que pour un processus CLI.

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

Ouvrez Assistant IA et activez le MCP local. Copiez son adresse et configurez le client
avec cette URL seule, par exemple http://127.0.0.1:PORT/mcp. Free utilise un serveur HTTP
loopback sans jeton et un registre autorisant toutes les opérations. Aucun jeton ni confirmation supplémentaire n’est demandé.

Protégé et Jeton seul demandent toujours un jeton Bearer. Le fichier privé
data/ipc/automation.json indique URL, profil et requireToken ; seuls les profils avec
jeton en contiennent un. Désactiver/fermer l’endpoint retire ce fichier.

~~~powershell
.\CyGameCLI.exe --root C:\Editors\MyGame serve --port 8766
.\CyGameCLI.exe --root C:\Editors\MyGame --connect http://127.0.0.1:8766/v1 invoke scene_list
~~~

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
