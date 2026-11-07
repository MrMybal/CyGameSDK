---
id: portable-assets
title: Assets portables et profils de conversion
description: Assets portables et profils de conversion
section: Ressources
order: 24
sourceRevision: 1
parent: assets-3d
---

## Préserver l’identité {#identity}

Un `.cypack` v1 contient exactement deux entrées ZIP ordinaires : `manifest.json` et `asset.glb`. L’enveloppe porte `format: cygame-portable-asset`, `schemaVersion: 1`, UUID canonique `assetId`, nom/nature, repère, taille/SHA-256, résumé inspecté, provenance et extensions nommées. Le GLB est un glTF 2.0 autonome. Le transport garde ses octets et les métadonnées inconnues à noms pointés minuscules, pas les sémantiques arbitraires d’un jeu.

Créez l’UUID une seule fois et conservez-le entre modifications/transferts. La destination doit résoudre explicitement un UUID existant contenant d’autres données ; aucun registre global, résolveur automatique de conflit ou UUID par os n’est fourni. La provenance autorise sourceGame/sourceFormat/sourceSha256/converterId, sans ajout automatique de chemins locaux absolus. Convertir les coordonnées transforme géométrie, ordre des faces, normales/tangentes, matrices de liaison et canaux d’animation ; changer une étiquette d’axe ne suffit pas.

## CLI et bibliothèque {#cli}

Inspectez l’exemple original fourni :

```powershell
.\runtime\python\Scripts\python.exe app\portable_asset_cli.py inspect app\examples\portable\AnimatedTriangle.cypack
```

Utilisez `portable_asset_cli.py pack SOURCE.glb NEW.cypack --name "My Asset"`, `export-glb SOURCE.cypack NEW.glb` ou `plan SOURCE.cypack --profile PROFILE.json`. Les destinations sont persistantes et jamais écrasées. La publication exclusive demande des liens physiques dans le même dossier. La sortie standard est du JSON UTF-8. Fonctions : `package_glb`, `inspect_package`, `read_package`, `export_glb` et `ConversionRegistry.register/plan_export`.

Une commande de plan réussie peut retourner `blocked`. Les extensions facultatives incompatibles donnent `review-required` avec pertes à examiner. Un profil de jeu déclaré reste `provider-required`, `executionAvailable:false` tant qu’aucun codec n’existe. Le chemin portable exécutable est l’export GLB à l’identique, pas le retargeting. Les gestionnaires facultatifs `portable_pack/inspect/export_glb` peuvent rejoindre le Core existant ; ces IDs ne sont pas installés dans le catalogue natif. Celui-ci utilise `model_*` et les imports.

## Limites de validation {#limits}

Le conteneur autonome accepte 64 Mio de GLB, 4 Mio de JSON GLB, 128 Kio d’enveloppe, 80 Mio de ZIP, 64 niveaux JSON et 100 000 entrées par tableau inspecté. Seulement JSON puis BIN facultatif et un tampon embarqué. Toute propriété `uri`, même de données, est refusée ; les ressources doivent utiliser des bufferViews embarquées. Entrées en double/supplémentaires/traversantes, liens symboliques, chiffrement et compression non prise en charge échouent.

Intégrité, bornes, cadrage et structures inspectées ne valident pas chaque accesseur, sommet, liaison de rig ou extension glTF. Les rapports disent `semanticValidation: not-performed`. Utilisez un validateur glTF complet et celui du jeu. Les limites du transport autonome diffèrent du [profil natif éditable](doc:assets-3d#limits), plus strict.

## Exemples de code de référence {#code-examples}

Exemples originaux des guides source. Adaptez chemins, noms et valeurs explicatives ; ils ne constituent pas un codec fonctionnel pour un jeu arbitraire.

```text
manifest.json
asset.glb
```

```json
{
  "format": "cygame-portable-asset",
  "schemaVersion": 1,
  "assetId": "ef7603e1-84f8-44b2-87da-a677412c462a",
  "name": "AnimatedTriangle",
  "kind": "scene3d",
  "space": {
    "handedness": "right", "upAxis": "+Y", "linearUnit": "meter",
    "rotation": "quaternion-xyzw", "timeUnit": "second"
  },
  "payload": {
    "path": "asset.glb", "mediaType": "model/gltf-binary",
    "bytes": "actual integer byte count", "sha256": "actual 64-character lowercase digest"
  },
  "summary": "actual inspected declaration summary",
  "extensions": {"org.example.game": {"opaqueSourceMetadata": {}}},
  "provenance": {"sourceGame": "Example", "sourceFormat": "native-format", "converterId": "example.import"}
}
```

```powershell
.\runtime\python\Scripts\python.exe app\portable_asset_cli.py inspect app\examples\portable\AnimatedTriangle.cypack
.\runtime\python\Scripts\python.exe app\portable_asset_cli.py pack <SOURCE.glb> <NEW_OUTPUT.cypack> --name "My Asset"
.\runtime\python\Scripts\python.exe app\portable_asset_cli.py export-glb <SOURCE.cypack> <NEW_OUTPUT.glb>
.\runtime\python\Scripts\python.exe app\portable_asset_cli.py plan app\examples\portable\AnimatedTriangle.cypack --profile app\examples\portable\Example_StaticOnlyConverter.json
```

```python
from portable_assets.service import OPERATION_IDS, register_operations
# In the existing create_runtime factory, before the runtime starts:
# 1. Declare the descriptors in the manifest.
# 2. Exclude OPERATION_IDS from the generic native-proxy registration loop.
# 3. Register these handlers instead.
register_operations(runtime, dedicated_shared_asset_input_directory)
```
