<table width="100%">
  <tr>
    <td width="300" align="left" valign="top">
      <a href="https://mrmybal.github.io/CyGameSDK/?lang=fr">Présentation</a><br>
      <a href="https://mrmybal.github.io/CyGameSDK/manual/Documentation.html#/fr/overview">Manuel</a><br>
      <a href="https://github.com/MrMybal/CyGameSDK">GitHub</a><br><br>
      <sub>README</sub><br>
      <a href="README.md">English</a> · <a href="README.fr.md">Français</a>
    </td>
    <td width="400" align="center" valign="middle">
      <a href="https://mrmybal.github.io/CyGameSDK/?lang=fr"><img src="docs/assets/mark.svg" alt="CyGameSDK" width="128"></a>
    </td>
    <td width="300" align="right" valign="top">
      <p><sub>Licence de l’application</sub><br><a href="LICENSE"><strong>MIT</strong></a></p>
      <p><sub>Éditeur / catalogue</sub><br><a href="app/Docs/Release-0.32.0.fr.md"><strong>0.32.0</strong></a><br><strong>PRÉVERSION DE DÉVELOPPEMENT</strong></p>
      <p><a href="https://github.com/MrMybal/CyGameSDK/releases/download/v0.32.0/CyGameSDK-0.32.0-windows-x64.zip"><strong>Télécharger Windows x64</strong></a><br><a href="https://github.com/MrMybal/CyGameSDK/releases/download/v0.32.0/CyGameSDK-0.32.0-source.zip">Sources ZIP · v0.32.0</a></p>
    </td>
  </tr>
</table>

<h1 align="center">CyGameSDK</h1>

Une base d’éditeur natif réutilisable pour les développeurs qui créent un SDK ou
un outil d’édition propre à un jeu. Partez d’un espace de travail ancrable, de
graphes visuels, d’éditeurs de ressources et d’un SDK MCP/IA commun, puis reliez
les formats, le rendu et la chaîne d’outils du jeu choisi.

> **PRÉVERSION DE DÉVELOPPEMENT · 0.32.0**
>
> CyMCPSDK 0.7.1 · Accès Free par défaut · Préversion Windows x64.
>
> Outils expérimentaux avec un adaptateur Sandbox. Les fonctionnalités suivent des
> limites documentées ; la compatibilité d’un jeu nécessite un adaptateur vérifié.
> Consultez les [notes de version, le contenu du téléchargement et les limites connues](app/Docs/Release-0.32.0.fr.md).

[![Espace Sandbox de CyGameSDK avec viewport, gizmo de transformation, hiérarchie, propriétés et navigateur de ressources](app/Docs/images/editor-workspace.png)](app/Docs/images/editor-workspace.png)

## À propos de CyGameSDK

Organisez les panneaux, sélectionnez les objets de scène, modifiez les propriétés
décrites par leurs schémas et annulez les changements de documents. Parcourez les
ressources et ouvrez leur éditeur spécialisé : graphe, matériau, JSON ou 3D neutre.
L’interface inspirée d’Unreal utilise C++17, l’ancrage Dear ImGui, les modules CyUI
et Direct3D 11. L’anglais est la langue par défaut et de repli ; le français se
sélectionne dans Options et un choix enregistré explicitement est conservé.

### Logique visuelle et matériaux

Créez des graphes logiques typés avec fonctions locales, macros, classes et interfaces.
Inspectez l’exécution avec points d’arrêt et pile d’appels. Prévisualisez les
changements de signatures/types avant application, réduisez des sélections pures
prises en charge en fonctions ou développez leurs appels en nœuds éditables, avec Undo.

[![Éditeur de graphe logique avec connexions d’exécution, navigation et diagnostics](app/Docs/images/logic-graph.png)](app/Docs/images/logic-graph.png)

Créez des graphes de matériaux, de l’échantillonnage de textures et des instances,
puis compilez l’aperçu DX11 par un worker de calcul possédé par le SDK. Ce workflow produit un shader
d’éditeur ; le format et le compilateur de matériaux d’un jeu demandent leur propre fournisseur.

[![Graphe de matériau avec échantillonnage de textures et aperçu coloré DX11 compilé](app/Docs/images/material-editor.png)](app/Docs/images/material-editor.png)

### Ressources, animation et automatisation

Travaillez avec les documents `.cy3d` bornés de maillages, squelettes et animations.
Échangez des ressources GLB et CyPack, créez LOD statiques et formes de collision,
puis utilisez le reciblage par pose de référence, l’IK hors ligne, les paires de
mouvement racine et le découpage/rééchantillonnage des clips. Ces profils d’édition
sont documentés ; ils ne constituent ni un logiciel DCC complet ni une prise en
charge automatique des fichiers d’un jeu.

[![Éditeur d’animation avec l’exemple AnimatedTriangle, squelette superposé, clés et piste temporelle](app/Docs/images/animation-editor.png)](app/Docs/images/animation-editor.png)

CyMCPSDK fournit assistant intégré, registre partagé, CLI et MCP.
L’interface, l’IPC natif et le SDK partagent la validation d’EditorCore et les
transactions de documents. Les adaptateurs source apportent décodage, écriture,
rendu et compilation/lancement propres au jeu. Le registre de codecs est vide par
défaut ; la compatibilité complète Unreal Engine/Blueprint n’est pas fournie.

### Assistant intégré

Choisissez un fournisseur/modèle, suivez les réponses en streaming, joignez des fichiers
et restaurez vos conversations privées. Le profil sélectionné contrôle l’accès aux opérations ; assistant, CLI et MCP partagent validation et annulation des documents.
Les résultats des jobs restent récupérables après redémarrage.

[![Paramètres de connexion de l’assistant dans l’éditeur anglais](app/Docs/images/assistant-settings.png)](app/Docs/images/assistant-settings.png)

## Fonctionnalités

- **Interface de l’éditeur** : collections typées et expressions numériques,
  hiérarchie/table de scène communes, sélecteur de sources natif, raccourcis
  configurables, édition JSON partagée et gestes des groupes avec un Undo.

- **Espace de travail** : onglets ancrés, fenêtres détachées, dispositions enregistrées,
  caméras indépendantes, recherche de commandes, thèmes, taille du texte et gestion du DPI.
- **Ressources** : dossiers, filtres, miniatures, favoris, références, import/réimport
  protégé, édition JSON en arbre et inspection bornée des PNG/JPEG/BMP.
- **Graphes** : énumérations/structures nominales, conteneurs, fonctions, macros,
  interfaces, débogueur, migrations contrôlées et réduction/développement d’appels purs.
- **Matériaux et 3D** : aperçus d’éditeur compilés, édition de maillages/squelettes/animations
  neutres, échanges portables, LOD, formes de collision et outils de mouvement.
- **Automatisation** : 177 opérations métier via assistant, CLI natif et MCP,
  schémas communs et progression/annulation explicites des jobs.
- **Aide locale** : lecteur CyDoc avec 35 chapitres anglais/français et la référence
  des opérations ; Help et F1 contextuel ouvrent la bonne page, sans serveur.

---

## Installation

La version **0.32.0** est une préversion Windows x64 / Direct3D 11.

1. [Téléchargez le ZIP Windows](https://github.com/MrMybal/CyGameSDK/releases/download/v0.32.0/CyGameSDK-0.32.0-windows-x64.zip)
   et extrayez-le dans un nouveau dossier accessible en écriture. Il contient
   l’éditeur, le CLI natif, les sources/exemples, les notices et le manuel bilingue hors ligne.
2. Installez le runtime Microsoft Visual C++ 2015–2022 x64 s’il manque.
3. Lancez **CyGameEditor.exe** ou **Lancer.cmd**. Édition native, assistant et CLI/MCP
   ne nécessitent ni Python, ni Node.js, ni Visual Studio, ni checkout des bibliothèques.
4. Pour les imports, shaders d’aperçu, packaging et calculs 3D, installez Python 3.11+
   puis exécutez **app/setup_mcp.ps1** depuis PowerShell. Les exemples facultatifs
   s’installent avec **runtime/python/Scripts/python.exe app/setup_examples.py**.

Le ZIP est une distribution portable en dossier, sans installateur. Python, poids
de modèles et identifiants ne sont pas inclus. Configurez votre fournisseur/compte
dans Assistant IA. Les hôtes natifs Linux/macOS ne sont pas implémentés.

[Sources ZIP](https://github.com/MrMybal/CyGameSDK/releases/download/v0.32.0/CyGameSDK-0.32.0-source.zip) ·
[Manuel hors ligne ZIP](https://github.com/MrMybal/CyGameSDK/releases/download/v0.32.0/CyGameSDK-0.32.0-documentation.zip) ·
[Empreintes SHA-256](https://github.com/MrMybal/CyGameSDK/releases/download/v0.32.0/SHA256SUMS.txt)

Pour compiler les sources, suivez les prérequis ci-dessous. L’accès Git aux dépendances
privées peut être nécessaire ; les exécutables précompilés n’en ont pas besoin.
Le manuel embarqué correspond à 0.32.0 ; celui de Pages suit son dernier déploiement.

Au premier lancement, la scène est une démonstration Sandbox. Ctrl+S sauvegarde
le projet ; Ctrl+O le recharge. Projets, ressources de travail et préférences se
trouvent dans `data/projects`, `data/assets` et `data/settings` de cette installation.
Conservez des sauvegardes des données existantes. L’installation des exemples
fournis n’écrase jamais les ressources déjà présentes.

Le manuel actuel s’ouvre aussi directement avec
[`Documentation/Documentation.html`](Documentation/Documentation.html#/fr/overview),
accompagné de son dossier `DocData/` complet. La lecture ne nécessite ni Node.js ni Internet.

## Sources, compilation et bibliothèques nécessaires

<details>
<summary>Compiler les sources actuelles et configurer MCP / les jobs des workers</summary>

### Prérequis

- Windows x64 ; Visual Studio 2022 avec **Développement Desktop en C++** et SDK Windows.
- Git et CMake 3.24+ ; les outils CMake de Visual Studio sont détectés automatiquement.
- Node.js 22+ et npm pour générer le manuel CyDoc actuel, pas pour le lire.
- Les [révisions exactes de CyUI, CyMCPSDK et CyDoc](app/dependencies.lock.json), récupérées
  depuis leurs dépôts source. Une compilation hors ligne nécessite les dépendances
  et chaînes d’outils entièrement en cache, pas seulement un dossier de sources.
- CyMCPSDK 0.7.1 est compilé depuis ses sources C++ verrouillées. Python 3.11+
  sert uniquement aux calculs d’import, shaders, packaging et traitement 3D.

### Récupérer les sources

```sh
git clone https://github.com/MrMybal/CyGameSDK.git
cd CyGameSDK
```

### Compiler et lancer

Exécutez PowerShell depuis la racine du dépôt :

```powershell
.\app\build.ps1
.\Lancer.cmd
```

Pour les calculs, configurez le Python privé. Automatisation de scène et chat
utilisent le SDK natif, sans serveur MCP Python :

```powershell
.\app\setup_mcp.ps1
.\runtime\python\Scripts\python.exe app\setup_examples.py
```

Sélectionnez les checkouts propres verrouillés avec `-CyUIRoot` et
`-CyMCPSDKRoot` sur `app/build.ps1`. `-BuildDirectory` et `-OutputDirectory`
choisissent des builds indépendants. Les bibliothèques voisines ne sont pas modifiées.

CyUI partage une seule instance ImGui appartenant à l’hôte et compile les modules
sélectionnés. Le moteur CodeEditor utilise une passerelle C++20 privée ; l’hôte et
le domaine restent en C++17. Audio, Gallery, démos, navigateur/CEF, Media et Office
ne sont pas sélectionnés. Le lecteur CyDoc distinct utilise image, gallery et graph, sans Three.js.

Le build place `CyGameEditor.exe` et `CyGameCLI.exe` à la racine du projet et copie le manuel local
complet à côté. `app/build_native.cmd` utilise la même entrée CMake.
Pour régénérer uniquement le manuel :

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File app/build_documentation.ps1
```

### Assistant, CLI et clients MCP

Ouvrez **Assistant IA**, choisissez un fournisseur configuré et son modèle réel, puis
connectez-vous. Free est le profil par défaut : adresse MCP seule et toutes les opérations enregistrées,
sans autorisation d’écriture supplémentaire. Local contrôlé, Jeton seul et Protégé
restent sélectionnables ; les profils contrôlés conservent l’autorisation des écritures.
Les clés restent en mémoire/environnement ; l’historique privé est restaurable.
Consultez [l’intégration assistant/CLI/MCP](app/Docs/CyMCPSDKIntegration.md).

Démarrez l’éditeur pour la racine choisie et adaptez [app/mcp.example.json](app/mcp.example.json) :

~~~powershell
.\CyGameCLI.exe --help
.\CyGameCLI.exe describe-operation property_set
.\CyGameCLI.exe --root C:\Editors\MyGame invoke scene_list
.\CyGameCLI.exe --root C:\Editors\MyGame mcp
~~~

La découverte compacte expose cinq outils ; --full donne le catalogue complet.
Les commandes app/tool.py et alias op_* restent disponibles. Les résultats survivent
au redémarrage et au TTL des ressources. Ne rejouez pas automatiquement une action
ayant expiré.

Détails : [résolution des problèmes](app/Docs/LaunchRecovery.md),
[intégration CyUI](app/Docs/CyUIMigration.md),
[génération CyDoc](app/Docs/CyDocIntegration.md).

</details>

---

## Licence

**Le code, la documentation et les exemples originaux de CyGameSDK sont sous [licence MIT](LICENSE).**
Conservez ses mentions de copyright et sa licence lors d’une redistribution.
Les bibliothèques tierces gardent leurs propres conditions.

### Éditeur natif et SDK

| Bibliothèque · sources | Licence · texte | Utilisation / périmètre |
| --- | --- | --- |
| [CyUI 1.2.0](https://github.com/MrMybal/CyUI) | [MIT et notices des composants](app/LICENCES/CyUI-MIT.txt) · [notices intégrées](app/LICENCES/CyUI-THIRD-PARTY-NOTICES.md) | Modules natifs sélectionnés ; révision exacte dans le verrou des dépendances. |
| [Dear ImGui, version CyUI verrouillée](https://github.com/ocornut/imgui) | [MIT](app/LICENCES/Dear-ImGui-MIT.txt) | Ancrage et rendu des widgets natifs ; une instance commune. |
| [stb intégré](https://github.com/nothings/stb) | [Option MIT](app/LICENCES/Dear-ImGui-stb-MIT.txt) | Utilitaires de polices/texte fournis avec ImGui. |
| [TextEditor](https://github.com/goossens/ImGuiColorTextEdit) | [MIT](app/LICENCES/TextEditor-MIT.txt) | Moteur de texte natif optionnel via le module CyUI CodeEditor sélectionné. |
| [JetBrains Mono 2.304](https://github.com/JetBrains/JetBrainsMono) | [SIL OFL 1.1](app/LICENCES/JetBrains-Mono-OFL.txt) | Police de code embarquée via CyUI Core ; aucune installation système. |
| [ImGuizmo](https://github.com/CedricGuillemet/ImGuizmo) | [MIT](app/LICENCES/ImGuizmo-MIT.txt) | Gizmos de transformation du viewport ; sources verrouillées. |
| [nlohmann/json 3.11.3](https://github.com/nlohmann/json) | [MIT](app/LICENCES/nlohmann-json-MIT.txt) | JSON des documents et opérations natifs. |
| [Lexilla, Scintilla et données SciTE](https://github.com/ScintillaOrg/lexilla) | [Notice permissive](app/LICENCES/Lexilla.txt) · [Scintilla](app/LICENCES/Scintilla.txt) · [SciTE](app/LICENCES/SciTE-Properties.txt) | Moteurs syntaxiques, en-têtes d’interface et propriétés via Code/Diff ; aucune interface d’éditeur Scintilla. |
| [CyMCPSDK 0.7.1](https://github.com/MrMybal/CyMCPSDK) | [MIT](app/LICENCES/CyMCPSDK-MIT.txt) | Registre natif, CLI/MCP, fournisseurs et ChatUI ; révision exacte verrouillée. |
| [MD4C](https://github.com/mity/md4c) | [MIT](app/LICENCES/MD4C-MIT.txt) | Markdown via CyUI DocumentViews / ChatUI. |
| Dépendances SDK | [Notices](app/LICENCES/CyMCPSDK-THIRD-PARTY-NOTICES.md) · [nlohmann/json 3.12](app/LICENCES/CyMCPSDK-nlohmann-3.12-MIT.txt) · [cpp-httplib](app/LICENCES/cpp-httplib-MIT.txt) · [PicoSHA2](app/LICENCES/PicoSHA2-MIT.txt) | JSON, transports et utilitaires du SDK, distincts du JSON hôte. |
| [CPython et paquets Python](https://github.com/python/cpython) | [Conditions Python](app/LICENCES/Python.txt) · [licences/notices des paquets](app/licenses-python.json) | Environnement de calcul facultatif ; chaque paquet conserve sa licence. |
| Microsoft Windows SDK / Direct3D / runtime C++ | [Périmètre des composants](app/LICENCES/README.md) · conditions Microsoft | Hôte Windows et chaîne graphique ; les polices système ne sont pas redistribuées. |

### Documentation et outils de compilation

| Bibliothèque · sources | Licence · texte | Utilisation / périmètre |
| --- | --- | --- |
| [CyDoc 0.1.0](https://github.com/MrMybal/CyDoc) | [MIT](app/LICENCES/CyDoc-MIT.txt) | Lecteur local exporté ; modules image, gallery et graph. |
| [Node.js 22+](https://github.com/nodejs/node) | [MIT et notices des composants](https://github.com/nodejs/node/blob/main/LICENSE) | Génération de la documentation ; inutile au lecteur. |
| [marked 17.0.6](https://github.com/markedjs/marked) | [MIT](https://github.com/markedjs/marked/blob/v17.0.6/LICENSE.md) | Compilation Markdown dans la chaîne d’outils verrouillée de l’application. |
| [PrismJS 1.30.0](https://github.com/PrismJS/prism) | [MIT](https://github.com/PrismJS/prism/blob/v1.30.0/LICENSE) | Coloration du code lors de la génération. |
| [esbuild 0.25.12](https://github.com/evanw/esbuild) | [MIT](https://github.com/evanw/esbuild/blob/v0.25.12/LICENSE.md) | Assemblage du lecteur hors ligne et des modules sélectionnés. |
| [TypeScript](https://github.com/microsoft/TypeScript) | [Apache-2.0](https://github.com/microsoft/TypeScript/blob/main/LICENSE.txt) | Compilation du paquet CyDoc verrouillé avant export. |

Ce tableau sert d’index et ne remplace pas les notices complètes. Conservez
[app/LICENCES](app/LICENCES/README.md), les notices des paquets Python et les fichiers
de licence livrés avec la documentation générée et les paquets d’outils.

## Références et liens

- [Manuel français](https://mrmybal.github.io/CyGameSDK/manual/Documentation.html#/fr/overview) · [English manual](https://mrmybal.github.io/CyGameSDK/manual/Documentation.html#/en/overview).
- [Présentation](https://mrmybal.github.io/CyGameSDK/?lang=fr) · [Dépôt des sources](https://github.com/MrMybal/CyGameSDK).
- [Release Windows et sources](https://github.com/MrMybal/CyGameSDK/releases/tag/v0.32.0) · [Suivi des problèmes](https://github.com/MrMybal/CyGameSDK/issues).
- [Contrat des ressources portables](https://mrmybal.github.io/CyGameSDK/manual/Documentation.html#/fr/portable-assets) · [Exemple d’adaptateur source](app/examples/adapters/ExampleGameAdapter.h).

<details>
<summary><strong>AI Usage</strong></summary>

- [Commencer ici : guide IA principal](AI_GUIDE.md) — présentation du projet, fichiers importants et création d’un SDK pour un jeu.
- [Instructions du dépôt](AGENTS.md) · [Guide de développement natif](app/AGENTS.md) — responsabilités, versions et contraintes d’implémentation.
- [Compiler cette application](app/Docs/LaunchRecovery.md) — configuration et résolution des problèmes de lancement.
- [Créer un adaptateur de jeu](app/Docs/GameAdapterGuide.md) · [Dossier de portage](app/Docs/GamePortingTemplate.md) — contrats source et éléments de preuve propres au jeu.
- [Intégration CyUI](app/Docs/CyUIMigration.md) · [Rédaction/compilation CyDoc](app/Docs/CyDocIntegration.md) — modules d’interface et maintenance de l’aide locale.

</details>
