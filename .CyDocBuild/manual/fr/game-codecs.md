---
id: game-codecs
title: Fournisseurs de codecs de jeu de confiance
description: Fournisseurs de codecs de jeu de confiance
section: Intégration
order: 62
sourceRevision: 1
parent: game-adapters
---

## Un registre vide par défaut {#registry}

Aucun codec réel n’est fourni. `IGameAdapter::gameCodecs()` déclare les cibles ; les `GameCodecProvider` exécutables sont du code source de confiance transmis à `create_runtime(..., game_codecs=...)`. Le panneau Codecs de jeu les liste. Déclarer une cible sans fournisseur exécutable n’implémente pas de conversion.

Un descripteur donne id/gameId, versions prises en charge explicites non vides, plateformes, extension simple, maxBytes (64 Mio maximum) et schéma JSON des options. Choisissez exactement cible/version/plateforme. Le framework vérifie ces valeurs avant le worker officiel. Il ne charge ni code ni commande worker depuis le JSON d’un modèle.

## Sorties vérifiées et pertes {#export}

`game_codec_export` prend un instantané immuable du modèle. Le fournisseur le convertit, valide la sortie réelle et écrit aux chemins possédés. Le rapport `cygame-codec-result` schéma 1 contient codec/jeu/version/plateforme, assetId inchangé, taille/SHA-256 réels, pertes et éléments de validation. L’hôte vérifie propriété, taille, identité, cible et octets. Ces contrôles ne prouvent pas indépendamment la sémantique du jeu.

Acceptez explicitement les pertes et les marqueurs de LOD périmé/collisions à revoir. `requireVerified` vaut true par défaut ; une sortie non vérifiée est refusée sauf choix délibéré de false. Ni sources ni fichiers du jeu installé ne sont écrasés. Exportez les résultats Core temporaires avant de finir la tâche. Pour annoncer une compatibilité, gardez des données réelles, vérifiez repères/rig/animation/matériaux/LOD/collisions et prouvez aller-retour puis chargement dans le jeu ou validation officielle.

## Exemples de code de référence {#code-examples}

Exemples originaux des guides source. Adaptez chemins, noms et valeurs explicatives ; ils ne constituent pas un codec fonctionnel pour un jeu arbitraire.

```json
{
  "format":"cygame-codec-result", "schemaVersion":1,
  "codecId":"your.codec", "gameId":"actual-game",
  "gameVersion":"exact-version", "platform":"Win64",
  "assetId":"same-canonical-UUID-as-snapshot",
  "sha256":"actual-output-digest", "bytes":123,
  "losses":[],
  "validation":{"status":"verified", "method":"actual validator/roundtrip/game-load evidence"}
}
```
