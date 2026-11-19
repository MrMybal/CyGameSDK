# CyGameSDK 0.31.0 — automatisation intégrée

Préversion Windows x64. La release v0.31.0 fournit le dossier Windows portable,
les sources et le manuel hors ligne complet.

CyMCPSDK 0.7.0, révision **791d4d7d7680213b282ec8529d248a10f8494d77**, remplace
l’ancien runtime Python. Il est compilé depuis ses sources avec les modules Core,
Platform, Http, MCP, AI et ChatUI. Local et le téléchargement de modèles sont exclus.

- Assistant intégré avec ChatUI : découverte des fournisseurs/modèles, streaming,
  annulation, restauration de l’historique, pièces jointes et fenêtre de contexte API.
- **CyGameCLI.exe** : catalogue réel, schémas, appels JSON, diagnostics, MCP stdio
  et HTTP local authentifié. Les options de lancement de l’éditeur et les scripts
  **app/tool.py** restent disponibles.
- Un registre et les mêmes droits pour le chat, le CLI et le MCP : 177 opérations
  métier, 177 alias de jobs et dix opérations de service, ou cinq outils de découverte compacte.
- Jobs durables privés : résultats conservés après fin de session, redémarrage et
  expiration des ressources SDK ; aucune relance automatique d’un travail interrompu.
- Lecture des clients, empreintes de jetons, sessions, jobs SQLite et recettes existants
  sans l’ancien package, en conservant bases et fichiers d’origine.
- CyUI verrouillé sur **6ef01cfcdb197f3a89d64618ce5ef8818c704728**, avec DocumentViews
  pour ChatUI. Domaine/hôte C++17 conservés, passerelles SDK/CodeEditor C++20 isolées.
- Aide CyDoc hors ligne maintenue en anglais et français.

Les vérifications Windows x64/MSVC/DX11 couvrent les appels réels chat/CLI/MCP, les
droits, l’annulation, l’historique, le contexte, le cache, les gros résultats,
l’interruption/récupération et les parcours métier affectés, dans des projets isolés.

Le chat est vérifié avec une API de test locale, sans authentification ni génération
auprès d’un compte/modèle réel. Les pièces jointes suivent les capacités du fournisseur
et présentent leur nom ; aucun nouveau lecteur de média de jeu n’est ajouté.
Aucun serveur MCP externe n’est activé sans besoin. Aucun codec de jeu, compilateur
moteur ou hôte natif Linux/macOS n’est ajouté.

Voir [intégration et commandes](CyMCPSDKIntegration.md), [UI](CyUIMigration.md)
et [notices](../LICENCES/README.md).

## Distribution

Le ZIP Windows fournit éditeur/CLI, sources/exemples, notices et manuel complet.
Aucune compilation ni checkout de bibliothèque n’est requis pour édition/chat/MCP natifs.
Installez le runtime C++ x64 Microsoft si nécessaire. Python 3.11+ et app/setup_mcp.ps1
sont facultatifs pour les calculs ; aucun environnement Python ni poids n’est embarqué.
Le ZIP de documentation s’ouvre hors ligne. SHA256SUMS.txt couvre les trois archives.

Les sources incluent aussi les outils 0.30 : Inspector typé, table/hiérarchie de scène,
CodeEditor, FilePicker et groupes de nœuds transactionnels.
