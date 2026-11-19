---
id: jobs
title: Jobs durables, workers et résultats
description: Suivre les jobs natifs et retrouver leurs sorties après redémarrage.
section: SDK
order: 50
revision: 2
sourceRevision: 2
parent: automation
---

## Suivre le cycle de vie {#lifecycle}

CyMCPSDK fournit exécution/annulation natives et processus possédés. L’application
persiste états et fichiers. Les opérations canoniques retournent leur résultat ; les alias
op_<id> soumettent des jobs asynchrones. Interrogez job_get ou cy_get_job jusqu’à Completed,
Cancelled, Failed ou Interrupted. Un appel annulé peut déjà avoir franchi sa frontière
d’écriture persistante : inspectez l’état avant de décider de l’exécuter à nouveau.

Jobs et fichiers sont des données durables. Terminer une tâche ne supprime pas ses
résultats. Ils survivent au redémarrage et au TTL des ressources SDK. Exportez puis
supprimez délibérément les données devenues inutiles. La récupération marque Interrupted
le travail inachevé dont le propriétaire s’est arrêté ; elle ne le relance jamais.

## Implémenter un fournisseur {#providers}

Utilisez les fabriques source de confiance et les sorties bornées JobContext.path.
Les calculs Python existants s’exécutent sous l’arbre de processus possédé par le SDK,
avec annulation coopérative et fermeture effective avant le statut clientClosed du bureau.
Les formats/toolchains réels restent à fournir par l’adaptateur. N’acceptez jamais de
commande exécutable provenant du JSON d’un asset/projet.

## Clients et transports {#clients}

CLI, chat et MCP utilisent le même registre, les validations natives et la politique
hôte. Les appels Windows sont sérialisés sur le thread propriétaire de l’éditeur.
Un point loopback représente une identité authentifiée et un jeton privé. Isolez
points/stockages pour des identités distantes distinctes. Le client Python compatible
conserve la propriété authentifiée des jobs. Ne parsez pas des libellés traduits pour
déterminer états et erreurs.

## Récupérer les résultats complets {#results}

job_list filtre par état/opération et pagine par offset/limit ; suivez nextOffset.
job_get retourne les chemins durables. job_result_read fournit le JSON par morceaux
respectant l’UTF-8. job_output_read fournit les fichiers en blocs base64 bornés entre
plusieurs appels CLI. job_output_open produit progressivement une ressource privée SDK ;
suivez son manifeste puis ses URI. Une expiration ne relance jamais un export ou une
modification.

Les fichiers terminés sont des sorties. Les fichiers partiels d’un échec/interruption
sont des diagnostics. Autorisations et confinement sont vérifiés à chaque lecture.

## Installations existantes {#legacy}

Le client compatible lit les anciennes données SQLite sans l’ancien runtime. Identités,
empreintes de jetons, métadonnées inconnues, chemins des résultats et formats de recettes
sont conservés. Base et fichiers d’origine restent inchangés. legacy_jobs.py permet de
copier les enregistrements ; leur partage avec l’identité locale nécessite explicitement
l’option --owner local.
