---
id: sdk-contracts
title: Schémas et erreurs du SDK
description: Schémas et erreurs du SDK
section: SDK
order: 52
sourceRevision: 1
parent: automation
---

## Entrées et sorties {#schemas}
Les entrées et résultats utilisent JSON Schema Draft 2020-12. Les types JSON sont `string`, `boolean`, `integer`, `number`, `object` et `array` ; `file` et `float` n’en sont pas. `x-cy-kind` décrit les contrôles (File, Directory, Files, Image, Enum, Object, Array), sans implémenter un décodeur. Les propriétés help/advanced/hidden/runtimeGenerated/step/visibleWhen de `x-cy-ui` sont des indications d’affichage. La validité relève du schéma et du handler : `multipleOf` valide un pas, `x-cy-ui.step` le suggère seulement.

Les valeurs par défaut sont copiées dans properties/items/allOf et la branche if sélectionnée avant validation. Aucun choix arbitraire n’est fait entre anyOf/oneOf. Embarquez les `$defs` locales ; les références distantes sont refusées. `format` est une annotation, pas une preuve de validité d’une date, URL ou existence de fichier. Un résultat incorrect échoue avec `InvalidOutput` ; les métadonnées `outputs` ne remplacent pas `outputSchema`. Écrivez les artefacts avec `context.path('outputs/result.ext')`, dans le workspace du job. Un chemin serveur ne transfère pas ses octets vers un client distant.

## Erreurs structurées {#errors}
`CyToolError` expose code, message, technicalDetails, recoverable et suggestedAction. Conservez les codes inconnus. Un SubmitJob refusé ne crée pas de job ; un handler en erreur produit un job Failed avec error. La réussite d’une lecture GetJob de ce job ne signifie pas que l’opération a réussi. Une exception inattendue du handler devient InternalError.

Les codes courants comprennent InvalidDescriptor, IncompatibleVersion, InvalidParameters, InvalidOutput, MissingImplementation, UnknownOperation, MissingDependency, MissingRuntime, UnsupportedPlatform, AccessDenied, AuthenticationRequired, Busy, Timeout, Cancelled, Interrupted, WorkerCrashed, ProtocolError, IntegrityFailure et InternalError. Les codes de ressources, backends et modèles concernent les fournisseurs optionnels réellement installés. Les logs et technicalDetails peuvent contenir des données utilisateur : masquez les secrets à la frontière du fournisseur.

## Identité du manifeste {#manifest}
Le fichier livré `app/CyTool.json` décrit les opérations de cette version. schemaVersion, protocolVersion, id, version, operations, platforms et concurrency décrivent le Tool. Un identifiant commence par une lettre ASCII, puis lettres/chiffres/underscore/point/tiret, au plus 128 caractères. Les champs inconnus du manifeste sont conservés ; les paramètres métier peuvent les interdire. Une version majeure incompatible échoue. Le champ executable ne lance pas le fichier. Excluez chemins privés, jetons et comptes des catalogues distribués.

Consultez [les opérations](doc:operations), [les tâches temporaires](doc:jobs) et [le protocole](doc:protocol). L’annulation d’un job est distincte de l’Undo documentaire.
