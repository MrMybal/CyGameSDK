# CyGameSDK 0.16 â€” jalons livrÃ©s

Les quinze jalons prolongent le socle natif ImGui/CyUI. Les nouveautÃ©s 0.16 sont dÃ©crites
dans la partie Â« Jalons 13 Ã  15 Â» ci-dessous. Ils utilisent tous le mÃªme
EditorCore, accessible depuis l'interface et le SDK/MCP officiel de CyToolsCore.
La comparaison avec Unreal guide les usages et l'organisation de l'Ã©diteur.
L'adaptateur livrÃ© reste Sandbox ; aucun jeu externe n'est encore intÃ©grÃ©.

## Jalon 1 â€” espaces de travail et contenu

- Espaces Niveau, Logique et MatÃ©riaux ; onglets ancrables et fenÃªtres dÃ©tachables.
- Enregistrement et chargement d'agencements nommÃ©s depuis Espace de travail.
  Les noms acceptent lettres, chiffres, tirets et caractÃ¨res de soulignement.
- Tiroir de contenu avec Ctrl+Espace, fermeture Ã  la perte de focus.
- Explorateur en vignettes ou liste, dossiers, fil d'Ariane, favoris persistants.
- Filtres combinables : type:Graph domain:material name:Surface, ext:png,
  path:Examples et termes nÃ©gatifs comme -name:Test. Les termes sont combinÃ©s par ET ;
  les guillemets regroupent les expressions. Les valeurs de protocole restent en anglais.
- CrÃ©ation de dossiers et graphes ; duplication, dÃ©placement et renommage de fichiers.
  Glisser un asset vers un dossier le dÃ©place. Les collisions sont refusÃ©es.
- Corbeille rÃ©cupÃ©rable, accessible dans l'explorateur et le menu FenÃªtres.
  Les fichiers modifiÃ©s doivent Ãªtre enregistrÃ©s ou rechargÃ©s avant dÃ©placement/suppression.
- Documents de graphes indÃ©pendants, indicateur de modification dans les onglets.
  Ouvrir un asset dÃ©jÃ  chargÃ© conserve son travail en mÃ©moire.

Les opÃ©rations de fichiers sont persistantes et distinctes de l'historique du document.
Les rÃ©fÃ©rences dans les anciens instantanÃ©s suivent les dÃ©placements et la corbeille.
Les copies synchrones sont limitÃ©es Ã  64 Mio ; un fournisseur asynchrone prendra
le relais pour les gros imports. La dÃ©couverte est bornÃ©e Ã  50 000 entrÃ©es et 5 000 rÃ©sultats.
Les vignettes sont des icÃ´nes de type ; les miniatures de jeux restent Ã  fournir.

## Jalon 2 â€” Ã©dition de scÃ¨ne

- Gizmos ImGuizmo : Q sÃ©lectionner, W dÃ©placer, E tourner, R redimensionner.
- RepÃ¨res local/monde et aimantation avec pas rÃ©glable selon l'outil.
- Une manipulation complÃ¨te produit une seule transaction annulable.
  Ã‰chap l'annule. Une modification concurrente invalide le brouillon de manipulation.
- Ctrl+clic pour la sÃ©lection multiple, Ctrl+D pour dupliquer, Suppr pour supprimer.
  Ces raccourcis sont limitÃ©s au viewport et Ã  la hiÃ©rarchie.
- DÃ©placement groupÃ© ; rotation et Ã©chelle du gizmo concernent un objet Ã  la fois.
- Dossiers virtuels dans la hiÃ©rarchie, glisser-dÃ©poser des objets, visibilitÃ©,
  verrouillage et historique des annulations/rÃ©tablissements.
- Les propriÃ©tÃ©s affichent explicitement l'objet principal d'une sÃ©lection multiple.
- Affichage plein/fil de fer, grille, vues perspective/dessus/face, orbite, dÃ©placement,
  zoom et cadrage avec F ; plusieurs viewports et camÃ©ras indÃ©pendantes.
- RÃ©vision monotone du document, transactions de transformation groupÃ©es et paramÃ¨tre
  expectedRevision pour Ã©viter d'Ã©craser un changement concurrent depuis le MCP.

Les dossiers organisent l'outliner ; ils ne crÃ©ent pas de parentage de transformations.
Le viewport Sandbox projette des primitives. Le contrat viewportRenderer accueille
le rendu propre d'un moteur. Les verrous empÃªchent les modifications et suppressions.

## Jalon 3 â€” graphes, dÃ©bogage et matÃ©riaux

- Mon graphe : dÃ©claration de variables float, bool, string et color, valeurs par
  dÃ©faut, renommage avec mise Ã  jour des liaisons, nÅ“uds de lecture et d'affectation.
- SÃ©quence, branche, comparaisons, opÃ©rations arithmÃ©tiques, conversions, couleurs.
- Duplication de nÅ“uds et de leurs connexions internes, commentaires par nÅ“ud.
  Tout cadrer / touche DÃ©but ajuste la vue Ã  lâ€™ensemble des nÅ“uds.
- Diagnostic navigable : sortie de matÃ©riau manquante, Ã©vÃ©nement absent, nÅ“uds
  dÃ©connectÃ©s et variables inutilisÃ©es. Les connexions invalides sont refusÃ©es.
- Points d'arrÃªt persistants F9, dÃ©marrage du dÃ©bogage, pas Ã  pas F10,
  continuer F5, arrÃªt, trace d'exÃ©cution et inspection des variables/valeurs observÃ©es.
- L'exÃ©cution emploie un Ã©tat de variables sÃ©parÃ© des valeurs par dÃ©faut du document.
  Modifier le programme pendant une pause exige un redÃ©marrage ; dÃ©placer un nÅ“ud,
  modifier son commentaire ou ses points d'arrÃªt conserve la session.
- Les erreurs d'exÃ©cution sont visibles et l'interprÃ©teur est bornÃ© :
  4 096 Ã©tapes, 10 000 Ã©valuations, 256 lignes de trace et 128 messages conservÃ©s.
- AperÃ§u DX11 d'une sphÃ¨re Ã©clairÃ©e, pilotÃ© automatiquement par couleur, rugositÃ©
  et mÃ©tal du graphe. Chaque document dispose de sa texture d'aperÃ§u.
- Les opÃ©rations de dÃ©bogage et de variables sont aussi exposÃ©es par CyToolsCore.

L'aperÃ§u compile un shader HLSL fixe de prÃ©sentation. Il ne gÃ©nÃ¨re pas le shader du
jeu depuis un graphe arbitraire. Les fonctions pures et sous-graphes sont livrÃ©s au jalon 4. Les macros, classes,
conteneurs, opÃ©rations latentes, pile d'appels, profils, textures/UV et compilation
vers un moteur restent des prochains jalons. Il n'y a pas de compatibilitÃ© Blueprint.
Le jalon 5 complÃ¨te le canvas avec la sÃ©lection multiple et le copier-coller.

## Exemples

Ouvrir Examples dans l'explorateur de contenu :

- Example_DebugCounter : DÃ©boguer, puis Continuer atteint le point d'arrÃªt avant
  l'affectation. Pas Ã  pas passe Counter de 1 Ã  7 ; Continuer affiche 7.
- Example_ParameterMaterial : modifier Tint, Roughness et Metallic dans Mon graphe
  actualise la sphÃ¨re sans modifier la dÃ©finition des types de nÅ“uds.
- Example_PlayerLogic et Example_Surface : les exemples simples du socle initial.

app/setup_examples.py recopie les nouveaux exemples sans Ã©craser les assets existants.

## VÃ©rifications et points de reprise

Le build MSVC, les contrats C++ et les tests Python utilisent les vrais composants.
La suite graphique couvre les commandes de l'Ã©diteur, le cycle d'assets, les agencements,
les verrous, les rÃ©visions, les variables, le dÃ©bogage et un geste rÃ©el dans le contexte
ImGui : dÃ©placement du gizmo, une seule transaction, puis restauration par Annuler.
Le test MCP dÃ©marre le serveur officiel, dÃ©couvre le catalogue, soumet des jobs,
puis exÃ©cute Ã©galement le dÃ©bogage par ce transport.

Les rÃ©sultats locaux sont sous data/reports : milestone3-build.log,
milestone3-gui-tests.log, integration.json, gizmo-gesture.json et captures des espaces
en anglais/franÃ§ais. Les contrÃ´les de thÃ¨mes couvrent les cinq palettes CyUI.
Les points de reprise source et les binaires intermÃ©diaires sont dans runtime/milestones ;
le point de reprise m3 conserve la version 0.4 corrigÃ©e. Le point m6 plus bas
conserve la version 0.7 ; m9 conserve la version 0.10 ; m12 conserve la version 0.13 actuelle.

RÃ©fÃ©rences d'ergonomie :
[interface Unreal](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-editor-interface),
[dÃ©bogueur Blueprint](https://dev.epicgames.com/documentation/unreal-engine/blueprint-debugger-in-unreal-engine?lang=en-US).

## Jalons 4 Ã  6 â€” version 0.7

Ces trois jalons ajoutent des fonctions rÃ©utilisables aux graphes, des outils de
sÃ©lection nodale et un visualiseur de rÃ©fÃ©rences. Le socle Windows/CyUI et le
runtime SDK/MCP de CyToolsCore restent communs.

### Jalon 4 â€” fonctions pures et sous-graphes typÃ©s

- Mon graphe contient une bibliothÃ¨que de fonctions locale Ã  chaque asset.
- CrÃ©er une fonction ouvre un onglet avec ses frontiÃ¨res EntrÃ©es et Sorties.
  La fonction initiale est une identitÃ© : x entre et result ressort.
- La signature est Ã©ditable : entrÃ©es/sorties nommÃ©es, types float, bool, string,
  color et valeurs par dÃ©faut. Appliquer la signature est une transaction annulable.
- Depuis le graphe principal, Appeler la fonction crÃ©e un nÅ“ud dont les broches
  suivent cette signature. Ouvrir la fonction dans ses propriÃ©tÃ©s retrouve son onglet.
- Les appels sont utilisables dans les graphes logiques et matÃ©riaux ; les fonctions
  peuvent appeler d'autres fonctions du mÃªme asset et retourner plusieurs valeurs.
- Renommer une fonction met Ã  jour ses appels. Supprimer une dÃ©finition encore
  utilisÃ©e est refusÃ©. Les changements incompatibles avec les fils sont refusÃ©s :
  dÃ©connecter les broches concernÃ©es avant de modifier leur type ou de les retirer.
- Les valeurs d'appel existantes compatibles sont conservÃ©es lors d'un changement
  de signature ; celles devenues incompatibles prennent le nouveau dÃ©faut.
- La fonction peut Ãªtre Ã©valuÃ©e directement avec ses dÃ©fauts depuis son onglet,
  ou avec des arguments explicites par graph_function_evaluate dans le SDK/MCP.
- DÃ©placer un nÅ“ud ou commenter une fonction conserve la session de dÃ©bogage du
  graphe appelant. Modifier le programme de la fonction exige son redÃ©marrage.

Les fonctions sont pures : pas de broches d'exÃ©cution, d'affectations de variables
ou d'effets de bord. Passer les variables par leurs entrÃ©es. La bibliothÃ¨que est
embarquÃ©e dans l'asset ; ce jalon ne crÃ©e pas de liaison de code vers un fichier
externe. Le copier-coller d'un appel transporte ses dÃ©finitions pour les rÃ©utiliser
ailleurs. Limites : 32 fonctions, 16 ports par direction, 16 niveaux d'appel,
500 nÅ“uds par graphe et 2 000 au total avec ses fonctions. Les appels rÃ©cursifs sont
refusÃ©s ; tous les appels partagent le budget de 10 000 Ã©valuations.
Le dÃ©bogueur observe les rÃ©sultats d'appels purs, sans pas Ã  pas Ã  l'intÃ©rieur
ni pile d'appels de fonctions dans cette version.

### Jalon 5 â€” Ã©dition nodale par sÃ©lection

- Ctrl+clic ajoute ou retire un nÅ“ud de la sÃ©lection ; glisser dans une zone vide
  dessine un rectangle de sÃ©lection. Ctrl+A sÃ©lectionne tous les nÅ“uds.
- DÃ©placer l'en-tÃªte d'un nÅ“ud sÃ©lectionnÃ© dÃ©place le groupe. Le relÃ¢chement crÃ©e
  une seule entrÃ©e d'historique. Ã‰chap annule ; une rÃ©vision concurrente annule
  Ã©galement le brouillon de dÃ©placement.
- Ctrl+C / Ctrl+V copient et collent les nÅ“uds avec leurs fils internes, variables
  utilisÃ©es et bibliothÃ¨que de fonctions. Les identifiants sont rÃ©gÃ©nÃ©rÃ©s.
- Les dÃ©clarations de mÃªme nom sont rÃ©utilisÃ©es si elles sont identiques ; une
  dÃ©finition diffÃ©rente provoque un refus atomique, sans Ã©crasement.
- Ctrl+D duplique la sÃ©lection ; Suppr la supprime avec ses connexions.
- Aligner Ã  gauche s'applique au groupe en une seule opÃ©ration annulable.
- Groupe de commentaires crÃ©e un cadre nommÃ© autour de la sÃ©lection. Ces cadres
  sont enregistrÃ©s avec le graphe et peuvent Ãªtre retirÃ©s depuis ses propriÃ©tÃ©s.
- Rechercher dans Mon graphe filtre les graphes, fonctions et variables. Les nouveaux
  onglets d'assets et de fonctions s'ouvrent dans la zone centrale de travail.

Le presse-papiers utilise un fragment JSON versionnÃ©, limitÃ© Ã  512 Kio au collage.
Les frontiÃ¨res d'une fonction peuvent Ãªtre dÃ©placÃ©es mais ne peuvent pas Ãªtre
copiÃ©es ou supprimÃ©es. Les cadres de commentaires sont actuellement statiques ;
ils ne parentent pas les nÅ“uds et n'ont pas encore de poignÃ©es de redimensionnement.

### Jalon 6 â€” rÃ©fÃ©rences et dÃ©pendances d'assets

- Clic droit sur un asset > RÃ©fÃ©rences ouvre un visualiseur central : assets qui
  l'utilisent Ã  gauche, asset sÃ©lectionnÃ© au centre, dÃ©pendances Ã  droite.
- Cliquer un Ã©lÃ©ment navigue dans le rÃ©seau. Les listes sous le diagramme montrent
  toutes les relations directes ; le dessin en montre jusqu'Ã  quatre par cÃ´tÃ©.
- Les chemins manquants sont signalÃ©s. Le SDK fournit aussi les dÃ©pendances indirectes
  avec parcours bornÃ© et protection contre les cycles.
- Un graphe chargÃ© possÃ¨de une liste de rÃ©fÃ©rences explicites, Ã©ditable depuis le
  visualiseur puis enregistrÃ©e avec Enregistrer l'asset. Une rÃ©fÃ©rence manquante est
  permise pour travailler sur un projet incomplet ; les chemins sortant du contenu
  sont refusÃ©s.
- L'index rÃ©unit les rÃ©fÃ©rences enregistrÃ©es et celles des documents en mÃ©moire.
  Retirer une rÃ©fÃ©rence sans sauvegarder son consommateur ne masque donc pas les
  dÃ©pendances encore prÃ©sentes sur disque.
- DÃ©placer, renommer ou mettre Ã  la corbeille un fichier rÃ©fÃ©rencÃ© est refusÃ©.
  Modifier les consommateurs puis les enregistrer permet l'opÃ©ration. Les rÃ©fÃ©rences
  conservÃ©es dans lâ€™historique suivent ensuite le renommage, pour Ã©viter quâ€™un ancien
  Annuler rÃ©introduise le chemin prÃ©cÃ©dent.
- Une analyse incomplÃ¨te signale ses erreurs et bloque ces changements de fichiers.
  Les sauvegardes, corrections de rÃ©fÃ©rences et restaurations restent disponibles.
- Les mÃªmes opÃ©rations de consultation, modification et ouverture du visualiseur
  sont exposÃ©es par CyToolsCore.

L'index couvre les rÃ©fÃ©rences dÃ©clarÃ©es dans les .cyasset. Il ne dÃ©code pas les
rÃ©fÃ©rences de formats de jeux inconnus et n'effectue pas de rÃ©Ã©criture automatique
de plusieurs fichiers. Il est actualisÃ© Ã  la demande, sans observateur de fichiers.
Bornes actuelles : 1 024 assets, 10 000 entrÃ©es parcourues, 16 Mio lus au total,
1 Mio par asset et 256 rÃ©fÃ©rences par document. Les grands projets nÃ©cessiteront
un fournisseur d'index asynchrone. Le parcours indirect est bornÃ© Ã  4 096 Ã©lÃ©ments.
Les deux nouveaux exemples rÃ©fÃ©rencent les exemples initiaux pour tester le rÃ©seau.

### Exemples et vÃ©rifications de la version 0.7

- Example_FunctionLogic : la fonction Double est appelÃ©e par le graphe et affiche 6.
  Ouvrir son sous-graphe pour modifier le calcul ou sa signature.
- Example_FunctionMaterial : TintScale multiplie la couleur par Gain avant l'aperÃ§u
  Ã©clairÃ© ; la fonction et ses paramÃ¨tres sont Ã©ditables sÃ©parÃ©ment.
- Les exemples initiaux restent disponibles. setup_examples.py n'Ã©crase aucun fichier.

Les preuves sont dans data/reports/m6-build.log, m6-tests.log, integration.json,
graph-gesture.json, gizmo-gesture.json, m6-renderer-qa.log et m6-validation.json.
Le test nodal pilote le vrai contexte ImGui : rectangle, sÃ©lection de deux nÅ“uds,
dÃ©placement de 60 Ã— 30, une transaction puis Annuler. Les tests SDK vÃ©rifient aussi
les fonctions imbriquÃ©es, signatures, sorties multiples, refus de rÃ©cursion,
fragments, conflits de dÃ©clarations, rÃ©fÃ©rences cycliques/manquantes et protections
des fichiers. Le transport MCP officiel crÃ©e et Ã©value une fonction dans un job.
Les captures couvrent les espaces EN/FR et les cinq palettes CyUI.

Le point de reprise complet de ces trois jalons est runtime/milestones/m6-source.zip,
avec le binaire CyGameEditor-0.7.0.exe. Les archives prÃ©cÃ©dentes sont conservÃ©es.
Les prochains axes restent les fonctions avec exÃ©cution et pile d'appels, les macros,
les types composÃ©s/classes, les textures et la compilation par adaptateur de moteur.


## Jalons 7 Ã  9 â€” version 0.10

### Jalon 7 â€” navigation des graphes

- Ctrl+F ou la loupe de la barre nodale ouvre Navigation des graphes.
- La recherche examine les types et noms de nÅ“uds, valeurs, noms de variables et
  commentaires des nÅ“uds. Elle inclut les fonctions locales. Activer Tous les graphes
  chargÃ©s Ã©tend la recherche aux autres documents ouverts.
- Cliquer un rÃ©sultat ouvre le bon onglet, sÃ©lectionne le nÅ“ud et cadre sa position.
  Les rÃ©sultats suivent les changements du document et du graphe actif.
- Lâ€™Ã©toile mÃ©morise la position et le zoom courants sous un nom. Vues mÃ©morisÃ©es
  rappelle une vue ou permet de la retirer. Les vues sont conservÃ©es avec lâ€™asset,
  y compris pour les fonctions ; elles participent Ã  Annuler sans changer le programme.
- Les propriÃ©tÃ©s des connexions proposent InsÃ©rer un reroutage : un petit nÅ“ud
  conserve le type du fil (exÃ©cution, nombre, boolÃ©en, texte ou couleur).
- Les groupes de commentaires ont maintenant des champs Texte, Position et Taille.
  Les cadres restent indÃ©pendants des nÅ“uds et sans poignÃ©es de redimensionnement.
- Example_Navigation illustre un reroutage dâ€™exÃ©cution, un cadre et une vue mÃ©morisÃ©e.

La recherche couvre les documents chargÃ©s, sans indexer le contenu de tous les
fichiers du disque ; lâ€™API signale une troncature au-delÃ  de 2 000 rÃ©sultats.
Chaque graphe ou fonction accepte au plus 64 vues mÃ©morisÃ©es.

### Jalon 8 â€” instances de matÃ©riaux

- Clic droit sur un matÃ©riau dans le contenu > CrÃ©er une instance de matÃ©riau.
- Lâ€™onglet dâ€™instance prÃ©sente le parent, les paramÃ¨tres exposÃ©s par ses variables,
  la chaÃ®ne de parents et une sphÃ¨re dâ€™aperÃ§u.
- Cocher un paramÃ¨tre active une surcharge ; dÃ©cocher rÃ©tablit lâ€™hÃ©ritage. La valeur
  hÃ©ritÃ©e reste visible. Les Ã©diteurs de nombres, couleurs, boolÃ©ens et textes sont
  gÃ©nÃ©rÃ©s selon le type dÃ©clarÃ©.
- Une instance peut hÃ©riter dâ€™une autre instance. Les modifications des parents
  chargÃ©s sont prises en compte sans sauvegarde prÃ©alable.
- Ouvrir le parent permet dâ€™Ã©diter sa topologie. Lâ€™instance nâ€™a pas de graphe propre.
  Enregistrer lâ€™asset conserve ses surcharges ; Annuler fonctionne sur les modifications.
- Les cycles, parents absents, chemins extÃ©rieurs et surcharges de type incorrect sont
  refusÃ©s. AprÃ¨s renommage dâ€™un paramÃ¨tre parent, lâ€™erreur propose de retirer la
  surcharge devenue obsolÃ¨te.
- Les liens parents alimentent le visualiseur de rÃ©fÃ©rences et les protections
  contre le dÃ©placement ou la suppression de fichiers encore utilisÃ©s.
- Example_MaterialInstance hÃ©rite de Example_FunctionMaterial et surcharge Tint/Gain.

Bornes : huit liens parents, 256 surcharges. Actualiser le matÃ©riau relit les parents
externes non chargÃ©s ; il nâ€™y a pas encore dâ€™observateur de fichiers. Les textures,
UV, permutations de shaders et compilation HLSL depuis le graphe restent Ã  ajouter.

### Jalon 9 â€” fournisseurs de construction et premier export

- Construire / Exporter ouvre un panneau central ancrable.
- Cibles, configurations et options proviennent du descripteur de lâ€™adaptateur.
  Valider la demande examine la scÃ¨ne, les diagnostics des graphes et les matÃ©riaux.
- Exporter lâ€™archive lance un vrai job build_execute CyToolsCore, avec progression,
  rÃ©sultat et annulation. Son ProcessWorker est possÃ©dÃ© et arrÃªtÃ© par CyToolsCore.
- Sandbox fournit une archive ZIP du document actuellement chargÃ©, ses mÃ©tadonnÃ©es
  et, en option, les valeurs Ã©valuÃ©es des matÃ©riaux. Compression activable.
- Une copie du document et sa rÃ©vision sont figÃ©es avant de lancer le worker :
  modifier lâ€™Ã©diteur ensuite ne change pas le contenu de cet export.
- Le rÃ©sultat expose le chemin de lâ€™archive, sa taille, SHA-256, son rapport et la
  rÃ©vision exportÃ©e. Les diagnostics et entrÃ©es restent dans lâ€™espace privÃ© du job.
- Les commandes de description, validation, exÃ©cution et suivi sont exposÃ©es par
  le mÃªme SDK/MCP. Un fournisseur source peut remplacer ou ajouter le traitement
  dâ€™un jeu ; voir [BuildProviders.md](BuildProviders.md).

Ce premier export est une archive de document chargÃ©. Il ne rassemble pas les
fichiers externes rÃ©fÃ©rencÃ©s et ne constitue pas un paquet de jeu autonome.
Aucun compilateur de jeu, cooking de formats natifs ou exÃ©cutable nâ€™est livrÃ©.
Les exports de lâ€™interface restent dans des sessions Core persistantes ; la fermeture
sollicite lâ€™annulation dâ€™un job actif. Lâ€™historique GUI et la reprise aprÃ¨s redÃ©marrage
restent Ã  implÃ©menter. Les fichiers de transport ne remplacent pas le suivi Core.

### VÃ©rifications et point de reprise 0.10

- Build MSVC et contrats C++ passÃ©s, dont lâ€™injection dâ€™un autre adaptateur de build.
- 23 scÃ©narios dâ€™intÃ©gration passÃ©s sur le vÃ©ritable host graphique, avec MCP officiel
  (94 opÃ©rations dâ€™Ã©diteur, 115 outils avec les contrÃ´les Core).
- Export SDK et MCP, contenu et SHA-256 du ZIP, options, isolation des identitÃ©s,
  diagnostics dâ€™Ã©chec, arrÃªt du worker Ã  lâ€™annulation et client graphique vÃ©rifiÃ©s.
- Navigation, reroutage de fonctions, hÃ©ritage, cycles, restauration et huit exemples
  contrÃ´lÃ©s ; captures EN/FR et cinq palettes CyUI.

Preuves : data/reports/m9-build.log, m9-tests.log, m9-renderer-qa.log,
renderer-qa.log, integration.json et m9-validation.json.
Point de reprise : runtime/milestones/m9-source.zip et CyGameEditor-0.10.0.exe.
Les archives antÃ©rieures sont conservÃ©es.

Pour poursuivre vers un Ã©diteur du niveau de Blueprint : fonctions avec exÃ©cution,
pile dâ€™appels, macros, types composÃ©s/classes, Ã©vÃ©nements latents, textures/UV,
importeurs et compilation propre Ã  chaque jeu restent des jalons distincts.


## Jalons 10 Ã  12 â€” version 0.13

### Jalon 10 â€” fonctions avec exÃ©cution

- Dans Mon graphe > Nouvelle fonction, cocher Fonction avec exÃ©cution pour un asset
  logique. Le mode pur reste le dÃ©faut et les anciens assets conservent leur comportement.
- Lâ€™entrÃ©e de fonction et son retour portent un fil dâ€™exÃ©cution ; lâ€™appel expose
  ExÃ©cution/Suite en plus de ses arguments et rÃ©sultats typÃ©s.
- Les variables dÃ©clarÃ©es dans une fonction avec exÃ©cution sont locales. Chaque
  invocation les initialise Ã  leurs dÃ©fauts et ne modifie pas celles de lâ€™appelant.
- Les appels peuvent sâ€™imbriquer et appeler des fonctions pures. Les matÃ©riaux et
  fonctions pures refusent les fonctions avec effets.
- Le retour fige les rÃ©sultats de cet appel avant de poursuivre son appelant.
  Lire un rÃ©sultat avant lâ€™exÃ©cution ou terminer sans retour produit une erreur.
- Les signatures, dÃ©placements, commentaires, copier-coller et Annuler utilisent
  le mÃªme socle. Le mode est choisi Ã  la crÃ©ation ; sa conversion reste Ã  ajouter.
- Ã‰valuer la fonction lâ€™exÃ©cute aussi isolÃ©ment avec ses arguments par dÃ©faut ; le
  SDK accepte des arguments explicites et renvoie valeurs, messages, trace et variables.
- Example_ExecutionFunctions appelle Compute, qui utilise Counter local puis Report.
  Le graphe affiche Â« Inside Report Â» puis Â« 6 Â» ; Counter du graphe reste Ã  900.

Les appels sont locaux Ã  lâ€™asset, sans rÃ©cursion, limitÃ©s Ã  16 niveaux. Le budget de
4 096 Ã©tapes et 10 000 Ã©valuations est partagÃ© par les appels imbriquÃ©s. Les fonctions
nâ€™ont pas encore dâ€™accÃ¨s aux objets du jeu, Ã©vÃ©nements latents ni opÃ©rations asynchrones.

### Jalon 11 â€” pile dâ€™appels et pas Ã  pas

- F10 passe un appel, F11 entre dans une fonction avec exÃ©cution et Maj+F11 ressort
  vers lâ€™appelant. Les boutons correspondants sont dans le canvas et le dÃ©bogueur.
- Les actions de pas Ã  pas ouvrent le bon onglet et sÃ©lectionnent le prochain nÅ“ud.
- Pile dâ€™appels affiche lâ€™appel actif et ses appelants. Cliquer une ligne permet de
  consulter ses arguments, variables locales et valeurs observÃ©es. Suivre lâ€™appel
  actif rÃ©tablit le suivi automatique de lâ€™inspection.
- La trace associe chaque Ã©tape Ã  sa fonction. Cliquer une Ã©tape navigue dans le
  sous-graphe concernÃ©, mÃªme si dâ€™autres fonctions utilisent le mÃªme identifiant de nÅ“ud.
- Les points dâ€™arrÃªt sont pris en compte dans les fonctions appelÃ©es, y compris
  pendant F10 ou Maj+F11. Continuer reprend le point dâ€™arrÃªt courant une seule fois.
- Un point dâ€™arrÃªt peut Ãªtre modifiÃ© pendant la pause. Modifier le programme rend
  la session obsolÃ¨te ; dÃ©placer un nÅ“ud ou modifier un commentaire conserve la pause.
- Les erreurs gardent la pile et les variables accessibles pour le diagnostic.

Les fonctions pures restent Ã©valuÃ©es comme expressions ; F11 ne sâ€™arrÃªte pas dans
leur calcul. Au niveau racine, Maj+F11 poursuit jusquâ€™Ã  la fin ou un point dâ€™arrÃªt.
Le dÃ©bogueur interprÃ¨te le Sandbox ; il ne dÃ©bogue pas un exÃ©cutable de jeu.

### Jalon 12 â€” comparer aux donnÃ©es enregistrÃ©es

- Comparer Ã  la sauvegarde, depuis le canvas, une instance de matÃ©riau ou le menu
  contextuel dâ€™un asset, ouvre Modifications du graphe dans la zone centrale.
- Le panneau liste les nÅ“uds, connexions, variables, fonctions, cadres, vues et
  propriÃ©tÃ©s ajoutÃ©s, retirÃ©s ou modifiÃ©s. Les fonctions conservent leur contexte.
- SÃ©lectionner une ligne montre les valeurs enregistrÃ©es et actuelles. Localiser
  le nÅ“ud actuel ouvre son onglet et cadre sa position.
- Ignorer lâ€™agencement et les commentaires permet dâ€™examiner les changements du
  programme sans les dÃ©placements, cadres, points dâ€™arrÃªt et vues mÃ©morisÃ©es.
- La rÃ©fÃ©rence provient du fichier sur disque. Pour les graphes intÃ©grÃ©s sans fichier,
  le panneau utilise la rÃ©fÃ©rence de session et lâ€™indique explicitement.
- Une modification externe du fichier est signalÃ©e ; Actualiser la comparaison
  relit le disque. Un fichier absent/corrompu produit une erreur visible.
- La comparaison nâ€™applique aucun changement et ne touche pas Ã  Annuler. Enregistrer
  lâ€™asset met explicitement Ã  jour le fichier et la comparaison.

Le rÃ©sumÃ© compte toutes les diffÃ©rences. Le dÃ©tail est limitÃ© Ã  1 024 lignes/2 Mio,
avec indicateur de troncature ; les aperÃ§us de valeurs conservent un UTF-8 valide.
Cette Ã©tape fournit une comparaison structurÃ©e entre sauvegarde et document vivant.
Les vues graphiques cÃ´te Ã  cÃ´te, anciennes rÃ©visions, fusion et intÃ©gration Git restent Ã  ajouter.

### VÃ©rification et point de reprise 0.13

Build MSVC et contrats natifs vÃ©rifiÃ©s. La suite de 28 scÃ©narios couvre les appels,
les erreurs de domaine/retour, les budgets partagÃ©s, la pile, les arrÃªts imbriquÃ©s,
la comparaison en lecture seule, les modifications externes et les aperÃ§us Unicode.
Le test dÃ©diÃ© pilote les touches F10/F11/Maj+F11 du vrai contexte ImGui et contrÃ´le
lâ€™ouverture des sous-graphes et les variables locales. Le MCP officiel crÃ©e et Ã©value
une fonction avec exÃ©cution et consulte les diffÃ©rences, via ses jobs habituels.
Le catalogue comprend 98 opÃ©rations dâ€™Ã©diteur, soit 119 outils avec les contrÃ´les Core.

Preuves : data/reports/m12-build.log, m12-native-tests.log, m12-tests.log,
function-gesture.json, integration.json, m12-renderer-qa.log et m12-validation.json.
Exemples contrÃ´lÃ©s en anglais/franÃ§ais et avec les cinq palettes CyUI.
Point de reprise : runtime/milestones/m12-source.zip et CyGameEditor-0.13.0.exe.

Les prochains axes sont les macros, types composÃ©s/classes et rÃ©fÃ©rences dâ€™objets,
les textures/UV et le pipeline matÃ©riau, les importeurs de formats rÃ©els et les
fournisseurs de compilation adaptÃ©s Ã  un premier jeu.


## Jalons 13 Ã  15 â€” version 0.16

### Jalon 13 â€” matÃ©riaux procÃ©duraux et coordonnÃ©es UV

- CoordonnÃ©es de texture expose U/V ; Transformation UV rÃ©pÃ¨te/dÃ©cale chaque axe.
- Damier et DÃ©gradÃ© UV calculent une couleur Ã  partir des coordonnÃ©es et de deux teintes.
- Composer une couleur, Sinus, Partie fractionnaire et Limiter entre 0 et 1 complÃ¨tent
  les expressions. Ils utilisent les mÃªmes broches, valeurs, transactions et Undo.
- material_sample Ã©value un point UV explicite avec lâ€™interprÃ©teur commun. Le contexte
  UV traverse les fonctions pures ; les valeurs hors du domaine du contrat sont refusÃ©es.
- Example_UVMaterial relie UV, Tile, TintA/TintB et Roughness Ã  sa sortie.
  Example_UVInstance hÃ©rite du parent avec un damier plus fin et une teinte verte.

Les UV sont deux nombres scalaires. Il nâ€™y a pas encore de type vectoriel UV, de
coordonnÃ©es monde ni de nÅ“ud Ã©chantillonnant un fichier texture.

### Jalon 14 â€” compiler lâ€™aperÃ§u et modifier ses paramÃ¨tres en direct

- Compiler lâ€™aperÃ§u lance material_compile via un ProcessWorker officiel de CyToolsCore.
  La compilation HLSL ps_5_0 utilise D3DCompiler ; le rÃ©sultat est du vrai DXBC.
- Lâ€™aperÃ§u GPU utilise le graphe sur une sphÃ¨re ou un plan. Les paramÃ¨tres dÃ©clarÃ©s et
  les surcharges dâ€™instance deviennent des uniforms, modifiables sans recompiler.
- Les fonctions pures prises en charge sont intÃ©grÃ©es au shader. Les types/nÅ“uds non
  pris en charge renvoient un diagnostic contextualisÃ©, sans succÃ¨s ambigu.
- Modifier une constante, connexion ou dÃ©finition rend le shader obsolÃ¨te. DÃ©placer
  un nÅ“ud ou modifier un paramÃ¨tre ne le fait pas. Recompiler est une action explicite.
- Le panneau Compilateur de matÃ©riaux montre lâ€™Ã©tat, la progression, lâ€™annulation,
  les diagnostics navigables et la source HLSL dâ€™un shader appliquÃ© avec succÃ¨s.
- Les jobs SDK/MCP retournent source, bytecode, rapport, empreintes et correspondances
  des nÅ“uds. material_preview_apply permet dâ€™appliquer explicitement le rÃ©sultat au GUI.
- Lâ€™application refuse les chemins extÃ©rieurs, les fichiers altÃ©rÃ©s et les rapports
  obsolÃ¨tes. Le shader GPU et la copie de la source survivent Ã  la fin du job/tÃ¢che.
- Une division impossible au point UV central reste une erreur de lâ€™Ã©chantillon CPU,
  sans empÃªcher la compilation. Le GPU protÃ¨ge un dÃ©nominateur quasi nul et retourne 0.

Ce compilateur produit un aperÃ§u pour lâ€™Ã©diteur DX11, pas un matÃ©riau natif dâ€™un jeu.
Le GPU calcule en float32 ; lâ€™interprÃ©teur utilise des nombres double. Les plugins
source purement CPU nâ€™obtiennent pas automatiquement un gÃ©nÃ©rateur HLSL. Tous les
fichiers sont produits dans le workspace de job CyToolsCore ; aucune commande ou
source HLSL arbitraire nâ€™est acceptÃ©e dans les assets. Contrat : MaterialPipeline.md.

### Jalon 15 â€” premier Ã©diteur spÃ©cialisÃ© dâ€™images

- Double-cliquer un PNG/JPEG/BMP ouvre son onglet via le registre commun dâ€™Ã©diteurs.
- Transparence sur damier, affichage RGBA/RGB ou dâ€™un canal R/G/B/A, dimensions et format.
- Ajuster lâ€™image, 100 %, boutons de zoom, molette ancrÃ©e au pointeur et dÃ©placement
  au bouton milieu. Recharger lâ€™image relit le fichier explicitement.
- texture_info donne les mÃ©tadonnÃ©es ; texture_open et texture_view_settings ouvrent
  et rÃ¨glent la vue pour une IA sans modifier le document ou son historique.
- Le dÃ©codage WIC et le renderer DX11 sont sÃ©parÃ©s de lâ€™interface. Exemple fourni :
  Example_Image.png, motif de test incluant une zone transparente.

Limites : 32 Mio par fichier, 8 192 pixels par cÃ´tÃ© et 4 194 304 pixels dÃ©codÃ©s ; cache
LRU de huit images. Pas encore dâ€™import/rÃ©import automatique, DDS/formats de jeu,
Ã©dition des pixels, vignettes dÃ©codÃ©es dans lâ€™explorateur ou liaison au matÃ©riau.

### VÃ©rification et point de reprise 0.16

Contrats natifs et 33 scÃ©narios sur le vrai host graphique : shader compilÃ© par le
worker, DXBC chargÃ© par Direct3D, paramÃ¨tres en direct, obsolescence, empreintes,
confidentialitÃ©, annulation, UV dans les fonctions et mÃ©tadonnÃ©es dâ€™images. Le MCP
stdio officiel compile aussi un matÃ©riau. Le catalogue comporte 110 opÃ©rations
Ã©diteur et 131 outils avec les contrÃ´les Core. Captures EN/FR et cinq palettes.

Preuves : data/reports/m15-{build,native-tests,tests,renderer-qa}.log,
m15-capture-results.json, m15-validation.json et integration.json.
Point de reprise : runtime/milestones/m15-source.zip et CyGameEditor-0.16.0.exe.
Les Ã©tapes suivantes restent les textures dans les matÃ©riaux, les importeurs et un
premier adaptateur de jeu, ainsi que macros, types composÃ©s et rÃ©fÃ©rences dâ€™objets.

## Ã‰volution commune â€” assets portables, prototype 0.1

- Contrat .cypack v1 : manifeste JSON, identitÃ© UUID, empreinte, mÃ©tadonnÃ©es
  namespacÃ©es et donnÃ©es glTF/GLB conservÃ©es.
- Transfert vÃ©rifiÃ© dâ€™un mesh statique et dâ€™un mesh skinnÃ© avec squelette/animation.
- Profils de cibles : fonctions absentes bloquantes, extensions optionnelles Ã 
  examiner explicitement, fournisseur rÃ©el requis pour les formats de jeux.
- BibliothÃ¨que Python, CLI et handlers optionnels utilisant ProcessWorker/Core ;
  rÃ©sultats privÃ©s au job et export explicite avant nettoyage.
- 22 tests rÃ©ussis ; deux fixtures validÃ©es par Khronos sans erreur/avertissement.

Ce composant source indÃ©pendant ne change pas le binaire ni le catalogue actif
0.16. Les documents natifs Ã©ditables, les Ã©diteurs visuels de mesh/animation, le
retargeting et les codecs de jeux restent les couches suivantes. Lâ€™inspection du
paquet nâ€™est pas une validation sÃ©mantique complÃ¨te glTF. Voir CommonAssetFormat.md.

## Jalons de complÃ©tion â€” 0.20.0

Les sections prÃ©cÃ©dentes dÃ©crivent leurs versions historiques. La version 0.20
ajoute les imports/rÃ©imports et vignettes, documents .cy3d Ã©ditables, aperÃ§u DX11,
squelette/skinning, clips et clÃ©s dâ€™animation, arbre JSON et textures de matÃ©riaux.
Voir [CompletionMilestones](CompletionMilestones.md) pour les preuves et les
fonctionnalitÃ©s encore absentes. [Native3DAssets](Native3DAssets.md) dÃ©crit les API,
les workflows et le profil bornÃ©. Le candidat binaire 0.19 nâ€™est pas activÃ©.

## 0.21 â€” advanced shared authoring

Typed containers/classes/virtual methods, scoped inline macros, typed foreach,
logical delay/events and runtime object inspection. Static LOD generation with
real reduced preview/export, editable conservative primitive collisions and
reference-pose retargeting with explicit bone mapping. Official Core worker commits
are hash-guarded and undoable. Source edits invalidate derived LOD/collision data.
Game codec registry/panel is ready for trusted target-specific providers; none
is bundled. EN/FR and all six themes are included. Details, fixtures and limits:
AdvancedAuthoring.md, GameCodecs.md and data/reports/authoring-validation.json.

## 0.22 â€” offline animation tools

Fixed-root single-chain FABRIK baking with a pole, target-error/reachability reports,
root-motion extraction into a new in-place clip and rigid delta clip, checked
sampled reconstruction and trim/resample. New translated Motion tools tab uses
the existing Core jobs. Unknown portable namespaces now survive raw GLB transfer.
Terminal desktop job readiness follows Core handle closure. Details and evidence:
AnimationTools.md and data/reports/motion-validation.json. Production joint limits,
contacts, engine movement/physics and real game codecs remain integration work.

## 0.23 â€” nominal graph enums and structures

Local enum/record declarations, contextual typed nodes, recursive properties, named
variables/signatures/class fields and function/macro/runtime propagation. Clipboard
merges owner libraries across assets/functions, rejecting conflicts. Type edits
validate all uses atomically with Undo and preserve metadata. Native EN/FR designers
and isolated actual-input checks included. No automatic type migration, shared type
asset registry or interface/engine binding. See NamedGraphTypes.md and types-validation.json.

## Native 0.24 â€” class interfaces

Local method contracts, native typed interface designer, class implementation
selection/bindings, support query and pure/execution dispatch. Inherited and
unrelated implementations share the same contract; debugger stepping and latent
calls use the existing shared runtime. Atomic schema/implementation checks,
conservative dynamic recursion detection and clipboard transfers retain document Undo.
Read GraphInterfaces.md and data/reports/interfaces-validation.json for verification.
No shared interface asset registry, engine reflection or real game codec is implied.

## Native 0.25 â€” explicit signature migrations

Read SignatureMigration.md. Functions, macros and linked interface/class method
signatures have a read-only preview and hash-guarded atomic apply. Port rename
maps preserve compatible fallback values, link/node IDs and metadata. Type/removal
losses are listed and require explicit acceptance; unsupported final graphs reject.
The native preview panel, one Undo/Redo and official Core operations share the
same implementation. Only the current owner is migrated, never assets on load.
Validation: data/reports/migrations-validation.json.

## Native 0.26 — enum and structure shape migrations

Read TypeMigration.md. Explicit field/member renames, additions/removals and field
type changes propagate through nested values, parent defaults, variables, class
fields, function/macro/interface signatures and typed node fallbacks. Make/break/
set references and enum switch wires are remapped with stable IDs and metadata.
Hash-guarded preview/apply, explicit loss acknowledgement and whole-owner Undo use
the existing native dispatcher/Core path. Referenced removed set-fields block.
Validation: data/reports/type-migrations-validation.json. No automatic asset-load
migration, cross-asset registry, live object conversion or game codec is supplied.

## Native 0.27 — collapse pure regions into functions/macros

Read GraphCollapse.md. Actual canvas selection and a translated review modal feed
the same hash-guarded planner/apply used by Core. Typed ports preserve fallbacks,
original node/link IDs and metadata; one Undo restores the complete owner. Root,
function, macro and material contexts retain existing budgets and local libraries.
Per-output dependency checks reject unused eager inputs, including UV branches.
Execution/disconnected regions and cross-asset libraries remain separate work.
Verification: data/reports/collapse-validation.json.
