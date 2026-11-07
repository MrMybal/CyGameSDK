# CyGameSDK 0.29.0

[English](Release-0.29.0.md) · [Français](Release-0.29.0.fr.md) · [README](../../README.fr.md)

## Statut et téléchargements

**Préversion de développement.** CyGameSDK est une base d’éditeur expérimentale
avec un adaptateur Sandbox. Les sources sont destinées aux essais et au développement
d’adaptateurs de jeu. Les fonctionnalités suivent les profils d’édition documentés.

| Sources | Version | Disponibilité |
| --- | --- | --- |
| Éditeur/catalogue `main` actuel | 0.29.0 | Sources avec manuel CyDoc et aide contextuelle Help/F1. Compilation locale sous Windows x64. |
| Archive source `v0.29.0` publiée | 0.29.0 | Version CyUI modulaire, publiée avant la migration CyDoc. Aucun éditeur précompilé ni installeur inclus. |

[Télécharger le ZIP de sources publié](https://github.com/MrMybal/CyGameSDK/releases/download/v0.29.0/CyGameSDK-0.29.0-source.zip)
· [Version et notes d’origine](https://github.com/MrMybal/CyGameSDK/releases/tag/v0.29.0)
· [Fichier SHA256SUMS.txt publié](https://github.com/MrMybal/CyGameSDK/releases/download/v0.29.0/SHA256SUMS.txt).

Les sources actuelles et l’archive existante partagent le numéro de version mais
contiennent des révisions différentes. Utilisez `main` pour l’intégration de la
documentation actuelle ; les changements ultérieurs ne sont pas nécessairement
dans le ZIP publié.

## Ajouts aux sources après l’archive publiée

La [migration CyDoc](https://github.com/MrMybal/CyGameSDK/commit/d92f95365d3ba373409941860021e8e3b95c2231)
ajoute un lecteur HTML local complet, 35 chapitres anglais/français et les références
des 175 opérations d’éditeur. Help, F1 contextuel et le bouton de documentation de
la vue d’ensemble ciblent des pages/sections stables dans la langue de l’éditeur.

L’export `Documentation/` complet s’ouvre directement par `file://`, sans serveur
ni Node.js pour la lecture. Le site statique conserve ce lecteur et les anciens
liens du guide d’installation y redirigent. Génération, copie, installation,
notices et contrôles de traduction accompagnent les sources. CyDoc 0.1.0 est
verrouillé à `e038c474577e1f305df3eba421f2b9c03b0f1d8d` avec les lecteurs image,
gallery et graph. Aucun lecteur mesh ni Three.js n’est exporté.

## Changements publiés en 0.29.0

Cette version migre l’éditeur natif vers CyUI modulaire en conservant les contrats
d’adaptateur, de documents, de SDK/MCP et d’Undo/Redo :

- Canevas de graphes, édition de propriétés, navigateur de ressources et navigation d’images communs.
- Viewport et adaptateur de transformation, piste d’animation, palette de commandes,
  logs, diagnostics, vues source colorées et comparaisons enregistré/actuel communs.
- CyUI 1.2.0 verrouillé à `ff023e646c6a3038b3dfbfd53f6d2b4a2f5955e3`,
  cibles de modules explicites et une seule instance Dear ImGui appartenant à l’hôte.
- Une chaîne de build CMake ; intégrations Audio, Gallery, Demo, Web/CEF et
  CodeEditor inutilisées désactivées dans l’application native.
- Interface anglaise par défaut/de repli, français sélectionnable, thèmes, densité et DPI.

Consultez les [détails d’intégration CyUI et limites des modules](CyUIMigration.md).
CyToolsCore reste le SDK MCP/IA et le runtime de jobs existant.

## Compilation et premier lancement

L’hôte natif actuel nécessite Windows x64, Visual Studio 2022 avec C++, le SDK
Windows, CMake 3.24+, Git et le runtime Microsoft Visual C++ 2015–2022 x64. La
branche `main` exige aussi Node.js 22+ et npm pour générer l’aide, ainsi que les
sources CyUI/CyDoc verrouillées. Ces outils sont inutiles pour lire le manuel exporté.

Les opérations MCP/IA, imports et jobs de shaders/compilation nécessitent Python
et la wheel CyToolsCore 0.10.1 préparée séparément. `app/setup_mcp.ps1` installe
l’environnement configuré ; `app/setup_examples.py` installe les exemples sans
écraser les ressources existantes. Voir les [étapes du README](../../README.fr.md#sources-compilation-et-bibliothèques-nécessaires).

Compilez avec `app/build.ps1`, puis lancez `Lancer.cmd`. Conservez les installations
dans des dossiers séparés et sauvegardez les données avant mise à jour. Projets,
ressources de travail et préférences appartiennent au dossier `data` local ; les
runtimes privés ne sont pas inclus dans l’archive source. Les prérequis d’origine
de l’archive restent dans son [guide marqué par le tag](https://github.com/MrMybal/CyGameSDK/blob/v0.29.0/app/Docs/Release-0.29.0.md).

## Limites connues

- Hôtes natifs Linux/macOS et installeur redistribuable non implémentés.
- Aucun codec réel de jeu, moteur de rendu de jeu ni compilateur de jeu fourni.
  Le build Sandbox archive les données documentaires, pas des fichiers de jeu préparés ni un jeu exécutable.
- Le runtime de graphes est borné et ne fournit pas la compatibilité Blueprint Unreal complète.
- Les workflows de maillages, squelettes, animations et images suivent des profils
  documentés, sans logiciel DCC ni peinture de pixels complets. Les extensions source
  ne constituent pas un chargeur de DLL à ABI stable.

Le [manuel actuel](https://mrmybal.github.io/CyGameSDK/manual/Documentation.html#/fr/limits)
décrit les limites des fonctionnalités et les exigences de l’adaptateur de jeu.

## Bugs et signalements

La version publiée ne fournit pas de liste des bugs ticket par ticket. Consultez le véritable
[suivi des problèmes](https://github.com/MrMybal/CyGameSDK/issues) et précisez la
révision source, la plateforme, la ressource concernée et les étapes de reproduction.
