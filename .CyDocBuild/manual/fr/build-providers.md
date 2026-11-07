---
id: build-providers
title: Cibles de compilation et packaging
description: Cibles de compilation et packaging
section: Intégration
order: 61
sourceRevision: 1
parent: game-adapters
---

## Choisir une cible déclarée {#targets}

Ouvrez Compilation et packaging. `buildTargets()` de l’adaptateur déclare ID de cible/fournisseur, plateforme, configurations et `optionsSchema`. Par défaut, la méthode ne propose rien. Un indicateur de capacité ne découvre pas de compilateur. `sandbox.document` utilise `sandbox.archive`, configuration Development et options compression/includeMaterialValues ; il crée une archive de documents, pas un exécutable de jeu.

`build_describe` expose les cibles. `build_prepare` valide cible/configuration, complète les défauts déclarés, vérifie scène/graphes chargés et résout les instances de matériaux. Le plan immuable contient instantané et signature ; un changement avant lancement demande une nouvelle préparation. Les options sont des données, jamais des commandes de shell fournies par un asset.

## Exécuter et consommer {#jobs}

L’interface réutilise son client SDK persistant et le ProcessWorker officiel. Les fournisseurs Python source enregistrés réalisent découverte, compilation et packaging propres au jeu. Fournissez une table complète pour remplacer les valeurs par défaut. Core gère progression, annulation, chemins privés et nettoyage. Exportez les résultats temporaires avant de terminer ; ceux du bureau suivent la session persistante.

Un ZIP Sandbox réussi contient des documents, pas des contenus de jeu préparés ni les octets des fichiers importés. Un adaptateur réel doit trouver compilateur/SDK plateforme, produire le format attendu et le valider dans le jeu. Des boutons de compilation/lancement ne prouvent pas la compatibilité.

```image
{"id": "build", "src": "assets/screenshots/build.png", "alt": "Cible déclarée d’archive de documents Sandbox", "controls": "minimal"}
```

## Exemples de code de référence {#code-examples}

Exemples originaux des guides source. Adaptez chemins, noms et valeurs explicatives ; ils ne constituent pas un codec fonctionnel pour un jeu arbitraire.

```json
{
  "id": "sandbox.document",
  "name": "Sandbox document archive",
  "providerId": "sandbox.archive",
  "platform": "Any",
  "configurations": ["Development"],
  "gameExecutable": false,
  "optionsSchema": {
    "type": "object",
    "properties": {
      "compression": {"type": "string", "enum": ["Stored", "Deflated"], "default": "Deflated"},
      "includeMaterialValues": {"type": "boolean", "default": true}
    },
    "additionalProperties": false
  }
}
```
