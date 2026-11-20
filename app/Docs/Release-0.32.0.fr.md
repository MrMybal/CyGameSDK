# CyGameSDK 0.32.0 — profils d’accès MCP local

Préversion Windows x64 avec accès MCP local Free par défaut.
La release v0.32.0 fournit le dossier Windows, les sources et le manuel hors ligne complet.

CyMCPSDK 0.7.1 est compilé depuis la révision locale
**34d4bcae126bc83d1a2925b5047dac36e3d09d92**. Un clone privé figé de ce commit local
empêche les changements simultanés du dépôt voisin de modifier le build. CyUI reste
verrouillé sur **6ef01cfcdb197f3a89d64618ce5ef8818c704728**, avec les mêmes modules.

- Free devient le défaut des nouvelles configurations et de celles sans choix explicite.
- Free utilise LocalAccessProfile::free, Policy::allow_all() et HTTP sans jeton sur
  127.0.0.1. Toutes les opérations enregistrées sont accessibles sans autorisation supplémentaire.
- Local contrôlé, Jeton seul et Protégé restent disponibles. Protégé exige toujours un
  jeton valide et applique la politique d’opérations existante.
- Profils déjà choisis et réglages sans rapport sont conservés. Un profil invalide ou
  contradictoire échoue explicitement. Validation métier/arguments et annulation restent actives.
- Le CLI natif propose --access-profile et les connexions aux endpoints sans jeton.
  Le CLI Python existant accepte aussi ces connexions.
- L’assistant affiche le profil actif, le choix du prochain démarrage, l’activation
  du MCP local et une adresse copiable. Un changement de profil demande un redémarrage.
- Streaming, outils du chat, annulation/historique et jobs durables conservent leurs parcours.

Voir [intégration](CyMCPSDKIntegration.md) pour les quatre profils et la syntaxe exacte.
Tests sur le vrai éditeur natif dans des Sandbox isolées et avec une API de test locale,
sans authentification/génération de compte ou modèle réel. Hôte Windows x64/MSVC/DX11.
Aucun workflow GitHub, codec de jeu ou nouvelle intégration moteur n’est ajouté.

## Distribution

Le ZIP Windows inclut CyGameEditor.exe, CyGameCLI.exe, sources/exemples, notices et
manuel bilingue complet. Édition/chat/MCP natifs ne demandent ni compilateur ni Python.
Les calculs facultatifs demandent Python 3.11+ puis app/setup_mcp.ps1. Installez le runtime
Microsoft Visual C++ 2015–2022 x64 s’il manque. Le ZIP de documentation s’ouvre depuis
le disque. SHA256SUMS.txt couvre les trois ZIP.
