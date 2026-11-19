# CyGameSDK 0.30.0

[English](Release-0.30.0.md) · [Français](Release-0.30.0.fr.md) · [README](../../README.fr.md)

## Statut et disponibilité

**Préversion de développement — mise à jour locale des sources.** Cette version
adopte les nouvelles API d’édition de CyUI. Elle ne constitue ni un éditeur Unreal
complet ni une prise en charge automatique des fichiers d’un jeu.

Le téléchargement GitHub existant reste
[l’archive de sources v0.29.0](https://github.com/MrMybal/CyGameSDK/releases/tag/v0.29.0).
Elle ne contient pas ces changements. Cette mise à jour locale ne crée ni version
0.30.0 publiée, ni installeur, ni publication sur GitHub.

## Changements d’interface

- Inspector typé commun pour scalaires, vecteurs, structures, tableaux, Sets et
  Maps, avec expressions numériques et lignes de collections virtualisées.
- Hiérarchie commune du World Outliner et table de propriétés issue de l’adaptateur,
  avec tri, filtres, TSV typé et modifications atomiques de plusieurs cellules.
- Sélecteur de sources natif, listes de dossiers asynchrones en cache et validation
  d’import existante.
- CodeEditor commun pour JSON d’opérations/classes/codecs et sélection de caractères
  en lecture seule ; police, coloration, recherche et zoom par vue partagés.
- Métadonnées de commandes communes aux menus, à la barre d’outils et au clavier,
  avec raccourcis configurables.
- Déplacement/redimensionnement/renommage/suppression des groupes de commentaires,
  avec leurs nœuds, un Undo, Échap et annulation en cas de révision périmée.
- Presse-papiers sémantique conservé : types, valeurs, déclarations locales et
  commentaires passent par le contrat de fragments de l’éditeur.

CyUI s’identifie toujours comme 1.2.0 en amont ; la révision exacte est
`1bdba05fac030bb5231797dcbe59e143c0f1d213`. L’instance ImGui unique et les
cibles de modules explicites sont conservées. Le moteur de texte C++20 est isolé
derrière une petite passerelle ; l’hôte et le domaine existants restent en C++17.

Le catalogue ajoute `property_set_many` et `graph_layout_apply`, pour un total
de 177 opérations d’éditeur / 198 outils avec les contrôles Core. Ces commandes
valident tout le lot et produisent un Undo. CyToolsCore conserve MCP, SDK et jobs.

## Compilation et aide

Suivez les [étapes du README](../../README.fr.md#sources-compilation-et-bibliothèques-nécessaires).
L’hôte natif reste Windows x64 / Direct3D 11 / MSVC. Le manuel CyDoc bilingue
complet accompagne l’exécutable et s’ouvre localement par Help/F1.

Conservez les notices TextEditor MIT et JetBrains Mono SIL OFL avec les notices
existantes. Aucune installation de police système n’est nécessaire.

## Limites et problèmes connus

L’adaptateur de jeu fournit toujours formats réels, rendu moteur et chaîne d’outils.
Aucun codec réel de jeu ni compatibilité Blueprint complète n’est fourni. Les
limites existantes des modèles, animations et graphes restent la référence.

L’allocateur générique Coller/Dupliquer de CyUI n’est pas activé pour les graphes
d’exécution ; utilisez le chemin sémantique de la barre d’outils ou du clavier.
Les reroutes d’exécution restent des nœuds applicatifs. L’édition des clés
d’animation et les sémantiques propres aux jeux restent des adaptations de l’hôte.
Audio, navigateur/CEF, Office et édition de documents ne sont pas inclus simplement
parce que la bibliothèque les propose.

Consultez [l’intégration CyUI](CyUIMigration.md) pour les modules et écarts restants,
et le [suivi des problèmes](https://github.com/MrMybal/CyGameSDK/issues) pour les
bugs signalés. Les hôtes natifs Linux/macOS restent indisponibles.
