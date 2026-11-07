/* Content grounded in README, AI_GUIDE and app/Docs. No network is required. */
window.CYGAME_SITE = {
  "shared": {
    "version": "0.29.0",
    "repo": "https://github.com/MrMybal/CyGameSDK",
    "release": "https://github.com/MrMybal/CyGameSDK/releases/tag/v0.29.0",
    "download": "https://github.com/MrMybal/CyGameSDK/releases/download/v0.29.0/CyGameSDK-0.29.0-source.zip"
  },
  "en": {
    "title": "CyGameSDK — A foundation for your game editor.",
    "description": "A native, modular game editor foundation with visual graphs, asset tools and a shared MCP/AI SDK. Built for game-specific tooling.",
    "skip": "Skip to content",
    "features": "Features",
    "documentation": "Documentation",
    "download": "Get the source",
    "github": "GitHub",
    "language": "Language",
    "menu": "Navigation",
    "close": "Close",
    "enlarge": "Enlarge screenshot",
    "previous": "Previous screenshot",
    "next": "Next screenshot",
    "imageOf": "Screenshot",
    "escape": "Esc to close · ← → to browse",
    "chapterLabel": "Page chapters",
    "back": "Back to showcase",
    "technical": "Technical reference (English)",
    "chapters": [
      [
        "workspace",
        "Workspace"
      ],
      [
        "graphs",
        "Graphs"
      ],
      [
        "materials",
        "Materials"
      ],
      [
        "assets",
        "Assets & 3D"
      ],
      [
        "automation",
        "MCP & SDK"
      ],
      [
        "extend",
        "Adapt & build"
      ],
      [
        "download",
        "Get started"
      ]
    ],
    "status": "Development preview · 0.29.0",
    "heroLine": "A foundation for your game editor.",
    "heroBody": "Build game-specific SDKs and authoring tools around a native workspace, visual graphs and an AI-ready core.",
    "heroMeta": "Windows x64 · Source release · MIT",
    "heroCaption": "The Sandbox workspace. Actual application capture, with example data.",
    "heroAlt": "CyGameSDK Sandbox workspace with viewport, transform gizmo, hierarchy, properties and Content Browser",
    "strip": [
      "Native C++17 / DX11",
      "Modular CyUI / Dear ImGui",
      "MCP / CyToolsCore"
    ],
    "introKicker": "FOR THE TOOLS YOUR GAME NEEDS",
    "introTitle": "Start with an editor.\nMake it your own.",
    "introBody": "For developers building a game-specific SDK, modding tools or an asset workflow. Reuse the workspace and authoring systems, then connect your game’s formats, renderer and toolchain through source adapters.",
    "introNotes": [
      [
        "A shared foundation",
        "Documents, validation, Undo/Redo and SDK operations stay connected."
      ],
      [
        "Your game, your integration",
        "The supplied Sandbox demonstrates the editor. A real game needs verified parsers, exporters and runtime bindings."
      ]
    ],
    "workspaceKicker": "01 / THE WORKSPACE",
    "workspaceTitle": "A familiar place\nto build something new.",
    "workspaceBody": "Dock panels, detach windows, keep named layouts and open independent viewports. Search commands, properties and content without leaving the editor.",
    "modes": [
      {
        "label": "Level",
        "image": "workspace",
        "title": "Scene, hierarchy and properties.",
        "body": "Select objects, organize virtual folders and edit schema-driven properties. Move, rotate and scale with snapping, local/world modes and document Undo.",
        "tags": [
          "Dockable panels",
          "Independent cameras",
          "Global search"
        ],
        "alt": "Level workspace showing Sandbox objects and schema-driven properties"
      },
      {
        "label": "Scripting",
        "image": "logic",
        "title": "Build behavior as connections.",
        "body": "Work with typed logic graphs, reusable functions and local variables. Follow the flow with breakpoints, stepping and a call-stack debugger.",
        "tags": [
          "Typed pins",
          "Functions & macros",
          "Debugger"
        ],
        "alt": "Execution function graph with typed input and output pins"
      },
      {
        "label": "Materials",
        "image": "materials",
        "title": "Follow the surface from node to preview.",
        "body": "Connect procedural expressions and textures, then compile the supported graph to a DX11 preview. Explore material instances and parameter overrides.",
        "tags": [
          "Texture sampling",
          "Generated HLSL",
          "Live uniforms"
        ],
        "alt": "Textured material graph and its compiled DX11 plane preview"
      },
      {
        "label": "Animation",
        "image": "animation",
        "title": "Work on the motion and the data.",
        "body": "Inspect a skeleton, play clips and edit tracks and keys on neutral .cy3d assets. Keep the original source and explicit conversion limits.",
        "tags": [
          "Skeleton overlay",
          "Clip playback",
          "Keyframes"
        ],
        "alt": "Animation editor with skeleton overlay, clip controls and timeline"
      }
    ],
    "graphs": {
      "kicker": "02 / LOGIC & AUTHORING",
      "title": "Think in graphs.\nKeep control of the details.",
      "body": "Connect typed nodes, define local functions and macros, and structure data with enums, records, classes and interfaces. Inspect execution instead of guessing what happened.",
      "bullets": [
        "Breakpoints, watches, stepping and contextual call stacks",
        "Search, bookmarks, reroutes and authored comment groups",
        "Explicit signature and type migrations with preview and Undo"
      ],
      "link": "Read about graph authoring",
      "alt": "Reusable execution function in the CyGameSDK graph editor"
    },
    "graphCards": [
      {
        "image": "navigator",
        "title": "Find the part you need.",
        "body": "Navigate graphs and functions, search node content and keep per-view bookmarks.",
        "alt": "Graph Navigator panel with local graphs and function navigation"
      },
      {
        "image": "diff",
        "title": "See what changed.",
        "body": "Compare the live graph with its saved version. Review structured changes and source details without modifying the document.",
        "alt": "Saved and current graph comparison with a split JSON diff"
      }
    ],
    "materials": {
      "kicker": "03 / MATERIALS & IMAGES",
      "title": "From a texture\nto a surface.",
      "body": "Use UV coordinates, procedural expressions and image samplers to build materials. Compile the supported expression subset through CyToolsCore workers, then inspect the real DX11 result.",
      "bullets": [
        "Sphere or plane previews and live parameter uniforms",
        "Material instances with parent inheritance and typed overrides",
        "PNG, JPEG and BMP inspection with channels, pan and zoom"
      ],
      "alt": "Connected texture coordinates and sampler nodes feeding a compiled material preview",
      "note": "Editor previews are not a game’s shader compiler or material exporter."
    },
    "materialCards": [
      {
        "image": "texture",
        "title": "Inspect the source image.",
        "body": "Check channels, dimensions and pixels. Fit, zoom and pan without changing the asset.",
        "alt": "Image inspector with the example color texture and channel controls"
      },
      {
        "image": "code",
        "title": "Look behind the preview.",
        "body": "Read generated HLSL with highlighting and search. Compilation progress and diagnostics use the existing Core job system.",
        "alt": "Generated HLSL source displayed in the shader preview panel"
      }
    ],
    "assets": {
      "kicker": "04 / ASSETS & NEUTRAL 3D",
      "title": "An asset workflow,\nwith room for your formats.",
      "body": "Browse files and folders, filter by asset fields, manage favorites and inspect references. Protected file operations and explicit import/reimport keep the document and its sources in view."
    },
    "assetCards": [
      {
        "image": "mesh",
        "title": "Mesh & geometry",
        "body": "Inspect and edit vertices, normals, UVs and weights in a bounded .cy3d profile. Generate static LODs and review primitive collision shapes.",
        "alt": "Neutral mesh editor with geometry preview and vertex property controls"
      },
      {
        "image": "skeleton",
        "title": "Skeleton & retargeting",
        "body": "Edit hierarchy and transforms, inspect skinning and use reference-pose retargeting. Offline chain IK and root-motion tools have explicit limits.",
        "alt": "Skeleton editor with hierarchy, transform values and bone overlay"
      },
      {
        "image": "animation",
        "title": "Clips & keyframes",
        "body": "Play, author, trim and resample clips. Seek in the timeline and preserve source motion when extracting root motion.",
        "alt": "Animation clip editor and track timeline"
      },
      {
        "image": "json",
        "title": "JSON documents",
        "body": "Edit structured objects, arrays and scalars with Undo and guarded saves. A document editor for metadata and neutral asset data.",
        "alt": "JSON document tree and editable scalar properties"
      }
    ],
    "portableKicker": "A COMMON LANGUAGE FOR ASSETS",
    "portableTitle": "Share the asset.\nKeep its identity.",
    "portableBody": "The portable contract uses .cypack with a self-contained GLB, alongside the native .cy3d editing profile. Preserve IDs and unknown metadata across derived SDKs. Conversion plans report capabilities and losses; game codecs still need to be implemented.",
    "portableLink": "Explore the portable asset contract",
    "automation": {
      "kicker": "05 / MCP & AI SDK",
      "title": "One editor.\nThe same operations for AI.",
      "body": "The interface, native IPC and SDK use the same EditorCore. CyToolsCore supplies MCP, identities, sessions, jobs and ProcessWorker, so AI tooling connects to actual editor operations.",
      "steps": [
        [
          "Discover",
          "List operations and inspect their typed parameters."
        ],
        [
          "Act",
          "Read and edit through the same validated document commands."
        ],
        [
          "Observe",
          "Poll worker jobs, inspect diagnostics and check current state."
        ]
      ],
      "note": "MCP connects configured AI clients. CyGameSDK does not bundle an autonomous game-making agent.",
      "link": "Set up MCP & the SDK",
      "alt": "Native Diagnostics panel inside the Sandbox workspace"
    },
    "extend": {
      "kicker": "06 / YOUR GAME, YOUR TOOLCHAIN",
      "title": "Bring the knowledge\nthat makes it your SDK.",
      "body": "Start a separate game-editor project. Implement IGameAdapter and source extensions for asset editors, graph nodes, rendering, build targets and trusted codecs. Keep the reusable foundation intact.",
      "alt": "Build and Package panel configured for the Sandbox document archive",
      "note": "The bundled Sandbox target exports a document ZIP. It does not cook a game or produce a game executable."
    },
    "integrationSteps": [
      [
        "01",
        "Choose a target",
        "Record the game version, coordinate system, representative files and known format evidence."
      ],
      [
        "02",
        "Connect a small workflow",
        "Implement one real import/edit/export path, with validation and predictable Undo."
      ],
      [
        "03",
        "Verify against the game",
        "Test exported data with the actual game or an official validator before claiming support."
      ]
    ],
    "personalize": {
      "kicker": "MAKE ROOM FOR YOUR WAY OF WORKING",
      "title": "Your layout. Your palette.",
      "body": "Choose a theme, use compact or comfortable density and keep the language you select. English is the default and fallback; French is available.",
      "alt": "CyGameSDK Sandbox workspace using the Light Grey theme"
    },
    "docsKicker": "A GOOD PLACE TO START",
    "docsTitle": "Understand the foundation.\nThen build on it.",
    "docsBody": "Start with the bilingual setup guide, then follow the source documentation for adapters, graphs, assets and MCP. The local HTML guide works without an internet connection.",
    "docsButton": "Open the guide",
    "docsSecondary": "AI developer guide",
    "faqTitle": "Before you start.",
    "faqs": [
      [
        "Is this a complete game engine?",
        "No. CyGameSDK is a development foundation for native game editors. The Sandbox is a demonstration adapter. A game renderer, real formats and runtime bindings belong to the game integration."
      ],
      [
        "What can I download?",
        "Version 0.29.0 is available as source, with SHA-256 checksums. There is no packaged Windows installer in this release. Build the native host on Windows x64."
      ],
      [
        "Is this Unreal Engine Blueprint?",
        "No. The editor includes its own typed graph interpreter and authoring systems. Familiar workflows do not imply full Blueprint compatibility or engine parity."
      ],
      [
        "Does it run on Linux or macOS?",
        "The current graphical host is Windows x64 with Direct3D 11. Linux and macOS graphical hosts are not implemented."
      ],
      [
        "Can I use it for a game that has no editor?",
        "That is the intended use case. You still need authorized access to representative game data, format knowledge, toolchains and a verified adapter. An extension filter alone is not a codec."
      ]
    ],
    "downloadTitle": "Build your next editor.",
    "downloadBody": "Get the 0.29.0 source and start with the Sandbox. Connect your game one verified workflow at a time.",
    "downloadNote": "Development preview · Windows x64 host · Source distribution · MIT",
    "sourceButton": "Download source .zip",
    "releaseButton": "Release notes & checksums",
    "footer": "A native foundation for game-specific tools.",
    "license": "MIT license",
    "top": "Back to top",
    "guideTitle": "CyGameSDK guide",
    "guideIntro": "A practical starting point for 0.29.0. This HTML guide is available in English and French and opens offline. Detailed source references are in English.",
    "guideNav": [
      [
        "start",
        "Start here"
      ],
      [
        "build",
        "Build & launch"
      ],
      [
        "explore",
        "Explore the editor"
      ],
      [
        "mcp",
        "MCP & AI SDK"
      ],
      [
        "adapter",
        "Create a game SDK"
      ],
      [
        "scope",
        "Current limits"
      ]
    ],
    "guideSections": [
      {
        "id": "start",
        "title": "Start with the right expectations.",
        "body": "CyGameSDK is a native editor foundation in early development. It supplies a Sandbox adapter and reusable UI, document and authoring systems. It does not automatically support an arbitrary game. Version 0.29.0 is a source release under MIT.",
        "items": [
          "Current graphical host: Windows x64 / Direct3D 11.",
          "The interface defaults to English and preserves an explicitly selected language.",
          "Projects, working assets and settings live in the installation’s data folder. Keep backups and use a separate destination for a derived SDK."
        ],
        "ref": "CapabilityCoverage.md"
      },
      {
        "id": "build",
        "title": "Build and launch on Windows.",
        "body": "Install Visual Studio 2022 with Desktop development with C++, the Windows SDK, Git and CMake 3.24+. Clone or extract the source in a new directory. Run these commands from the project root.",
        "code": ".\\app\\build.ps1\n.\\app\\setup_mcp.ps1\n.\\runtime\\python\\Scripts\\python.exe app\\setup_examples.py\n.\\Lancer.cmd",
        "items": [
          "The native build fetches the exact CyUI revision from app/dependencies.lock.json. If upstream is private, authenticate Git first; never put credentials in source or CMake arguments.",
          "For offline builds, pass -CyUIRoot to app/build.ps1 with a clean checkout at the locked revision.",
          "setup_mcp.ps1 requires the CyToolsCore 0.10.1 wheel built from an authorized checkout. Select its location with -CyToolsCoreRoot.",
          "The host needs the Microsoft C++ runtime. MCP and worker-based import, shader and build jobs need the configured Python/Core environment."
        ],
        "ref": "CyUIMigration.md"
      },
      {
        "id": "explore",
        "title": "Explore the supplied workflows.",
        "body": "The startup Sandbox is a demonstration scene. Ctrl+O loads a saved project; Ctrl+S saves it. The example installer refuses to overwrite assets.",
        "items": [
          "Level: dock panels, select objects, inspect properties and use transform gizmos.",
          "Scripting: open a graph example, inspect typed connections and use the debugger.",
          "Materials: open Example_TexturedMaterial.cyasset and compile its DX11 preview. This is an editor shader, not a game material export.",
          "Examples3D: open StaticTriangle.cy3d or AnimatedTriangle.cy3d for mesh, skeleton, animation and GLB/CyPack workflows.",
          "Ctrl+P opens command search. Ctrl+Space opens the content drawer. The Content Browser has file filters, favorites and references."
        ],
        "ref": "Native3DAssets.md"
      },
      {
        "id": "mcp",
        "title": "Connect to real editor operations.",
        "body": "Start the editor. Configure your MCP client with app/mcp.example.json and absolute paths for your installation. Run app/tool.py with the private Python environment. CyToolsCore provides the server and jobs.",
        "code": ".\\runtime\\python\\Scripts\\python.exe app\\tool.py describe\n.\\runtime\\python\\Scripts\\python.exe app\\tool.py mcp",
        "items": [
          "Discover operations and inspect their schemas before calling them.",
          "GUI, native IPC and SDK share EditorCore validation and document transactions.",
          "Poll Core jobs to a terminal state. After a mutation timeout, inspect current state before retrying: the edit may already have applied."
        ],
        "ref": "MultiClient.md"
      },
      {
        "id": "adapter",
        "title": "Derive an editor for a specific game.",
        "body": "Start with AI_GUIDE.md and the game porting template. Work in a separate project with real game fixtures and evidence for each supported format.",
        "items": [
          "Record the exact game/platform/version, units, coordinate system and format evidence.",
          "Implement IGameAdapter, schemas and real parsing/serialization; register source extensions for rendering, editors and build providers.",
          "Start with one import/edit/export workflow and verify output against the game or an official validator.",
          "Keep asset IDs, unknown metadata and explicit loss reporting. A conversion profile is a declaration, not a game codec."
        ],
        "ref": "GameAdapterGuide.md"
      },
      {
        "id": "scope",
        "title": "What belongs to the integration.",
        "body": "No real game codec is bundled. The scene viewport demonstrates projected primitives; a game renderer is a source integration. The bounded graph runtime is not full Unreal Blueprint.",
        "items": [
          "Sandbox builds produce document ZIP archives, not cooked games or game executables.",
          "Neutral mesh, animation and image tools follow documented bounded profiles; they are not a complete DCC or pixel-painting application.",
          "Editor plugins are source extensions. No ABI-stable dynamic DLL loader is claimed.",
          "Linux/macOS native hosts and a redistributable installer remain future work."
        ],
        "ref": "CapabilityCoverage.md"
      }
    ]
  },
  "fr": {
    "title": "CyGameSDK — Une base pour votre éditeur de jeu.",
    "description": "Une base d’éditeur native et modulaire avec graphes visuels, outils d’assets et un socle MCP/SDK pour les IA. À adapter à votre jeu.",
    "skip": "Aller au contenu",
    "features": "Fonctionnalités",
    "documentation": "Documentation",
    "download": "Obtenir les sources",
    "github": "GitHub",
    "language": "Langue",
    "menu": "Navigation",
    "close": "Fermer",
    "enlarge": "Agrandir la capture",
    "previous": "Capture précédente",
    "next": "Capture suivante",
    "imageOf": "Capture",
    "escape": "Échap pour fermer · ← → pour parcourir",
    "chapterLabel": "Chapitres de la page",
    "back": "Retour à la vitrine",
    "technical": "Référence technique (en anglais)",
    "chapters": [
      [
        "workspace",
        "Interface"
      ],
      [
        "graphs",
        "Graphes"
      ],
      [
        "materials",
        "Matériaux"
      ],
      [
        "assets",
        "Assets et 3D"
      ],
      [
        "automation",
        "MCP et SDK"
      ],
      [
        "extend",
        "Adapter et compiler"
      ],
      [
        "download",
        "Commencer"
      ]
    ],
    "status": "Préversion de développement · 0.29.0",
    "heroLine": "Une base pour votre éditeur de jeu.",
    "heroBody": "Créez des SDK et des outils adaptés à un jeu, autour d’une interface native, de graphes visuels et d’un socle accessible aux IA.",
    "heroMeta": "Windows x64 · Distribution source · MIT",
    "heroCaption": "L’espace Sandbox. Capture réelle de l’application, avec des données d’exemple.",
    "heroAlt": "Interface Sandbox de CyGameSDK avec viewport, gizmo, hiérarchie, propriétés et explorateur d’assets",
    "strip": [
      "C++17 natif / DX11",
      "CyUI modulaire / Dear ImGui",
      "MCP / CyToolsCore"
    ],
    "introKicker": "LES OUTILS DONT VOTRE JEU A BESOIN",
    "introTitle": "Partez d’un éditeur.\nFaites-en le vôtre.",
    "introBody": "Pour les développeurs qui créent un SDK dédié à un jeu, des outils de modding ou un workflow d’assets. Réutilisez l’interface et les systèmes d’édition, puis raccordez les formats, le rendu et la chaîne de compilation de votre jeu grâce aux adaptateurs source.",
    "introNotes": [
      [
        "Une fondation commune",
        "Documents, validation, annulation/rétablissement et opérations SDK restent reliés."
      ],
      [
        "Votre jeu, votre intégration",
        "Le Sandbox illustre l’éditeur. Un vrai jeu demande des lecteurs, exporteurs et liaisons d’exécution vérifiés."
      ]
    ],
    "workspaceKicker": "01 / L’ESPACE DE TRAVAIL",
    "workspaceTitle": "Un cadre familier\npour créer de nouveaux outils.",
    "workspaceBody": "Ancrez les panneaux, détachez des fenêtres, conservez des dispositions nommées et ouvrez des viewports indépendants. Recherchez commandes, propriétés et contenus sans quitter l’éditeur.",
    "modes": [
      {
        "label": "Niveau",
        "image": "workspace",
        "title": "Scène, hiérarchie et propriétés.",
        "body": "Sélectionnez les objets, organisez des dossiers virtuels et modifiez les propriétés décrites par schéma. Déplacez, tournez et redimensionnez avec magnétisme, repères local/monde et annulation.",
        "tags": [
          "Panneaux ancrables",
          "Caméras indépendantes",
          "Recherche globale"
        ],
        "alt": "Espace de niveau avec objets Sandbox et propriétés décrites par schéma"
      },
      {
        "label": "Graphes",
        "image": "logic",
        "title": "Construisez le comportement par connexions.",
        "body": "Utilisez graphes logiques typés, fonctions réutilisables et variables locales. Suivez le déroulement avec points d’arrêt, pas à pas et pile d’appels.",
        "tags": [
          "Broches typées",
          "Fonctions et macros",
          "Débogueur"
        ],
        "alt": "Graphe de fonction d’exécution avec entrées et sorties typées"
      },
      {
        "label": "Matériaux",
        "image": "materials",
        "title": "Suivez la surface du nœud à l’aperçu.",
        "body": "Reliez expressions procédurales et textures, puis compilez le graphe pris en charge en aperçu DX11. Explorez les instances et leurs paramètres surchargés.",
        "tags": [
          "Échantillonnage de textures",
          "HLSL généré",
          "Paramètres en direct"
        ],
        "alt": "Graphe de matériau texturé et aperçu DX11 compilé sur un plan"
      },
      {
        "label": "Animation",
        "image": "animation",
        "title": "Travaillez le mouvement et ses données.",
        "body": "Inspectez un squelette, lisez des clips et modifiez pistes et clés sur des assets neutres .cy3d. Préservez la source et les limites explicites de conversion.",
        "tags": [
          "Superposition du squelette",
          "Lecture des clips",
          "Images clés"
        ],
        "alt": "Éditeur d’animation avec squelette, commandes du clip et timeline"
      }
    ],
    "graphs": {
      "kicker": "02 / LOGIQUE ET ÉDITION",
      "title": "Pensez en graphes.\nGardez la maîtrise des détails.",
      "body": "Connectez des nœuds typés, définissez fonctions et macros locales, et structurez les données avec énumérations, structures, classes et interfaces. Inspectez l’exécution pour comprendre ce qui se passe.",
      "bullets": [
        "Points d’arrêt, valeurs observées, pas à pas et pile d’appels contextuelle",
        "Recherche, signets, reroutages et groupes de commentaires",
        "Migrations explicites de signatures et de types avec aperçu et annulation"
      ],
      "link": "Comprendre les graphes",
      "alt": "Fonction d’exécution réutilisable dans l’éditeur de graphes CyGameSDK"
    },
    "graphCards": [
      {
        "image": "navigator",
        "title": "Retrouvez ce qui vous intéresse.",
        "body": "Parcourez graphes et fonctions, recherchez le contenu des nœuds et conservez des signets propres à chaque vue.",
        "alt": "Navigateur de graphes affichant les graphes locaux et leurs fonctions"
      },
      {
        "image": "diff",
        "title": "Comprenez les changements.",
        "body": "Comparez le graphe actif à sa version enregistrée. Examinez les changements structurés et les détails du code sans modifier le document.",
        "alt": "Comparaison du graphe enregistré et actif avec différences JSON côte à côte"
      }
    ],
    "materials": {
      "kicker": "03 / MATÉRIAUX ET IMAGES",
      "title": "De la texture\nà la surface.",
      "body": "Combinez coordonnées UV, expressions procédurales et échantillonneurs d’images. Compilez le sous-ensemble d’expressions avec les workers CyToolsCore, puis examinez le résultat DX11 réel.",
      "bullets": [
        "Aperçus sur sphère ou plan et paramètres uniformes en direct",
        "Instances avec héritage du parent et surcharges typées",
        "Inspection PNG, JPEG et BMP : canaux, déplacement et zoom"
      ],
      "alt": "Nœuds de coordonnées et d’échantillonnage alimentant un aperçu de matériau compilé",
      "note": "Les aperçus de l’éditeur ne remplacent pas le compilateur de shaders ou l’export de matériaux d’un jeu."
    },
    "materialCards": [
      {
        "image": "texture",
        "title": "Inspectez l’image source.",
        "body": "Vérifiez canaux, dimensions et pixels. Ajustez, zoomez et déplacez la vue sans modifier l’asset.",
        "alt": "Inspecteur d’image avec texture colorée d’exemple et choix des canaux"
      },
      {
        "image": "code",
        "title": "Regardez derrière l’aperçu.",
        "body": "Lisez le HLSL généré avec coloration et recherche. La progression et les diagnostics utilisent le système de jobs Core existant.",
        "alt": "Code HLSL généré affiché dans le panneau d’aperçu de shader"
      }
    ],
    "assets": {
      "kicker": "04 / ASSETS ET 3D NEUTRE",
      "title": "Un workflow d’assets,\nouvert à vos formats.",
      "body": "Parcourez fichiers et dossiers, filtrez les champs, gérez les favoris et inspectez les références. Les opérations protégées et l’import/réimport explicite gardent le lien entre document et sources."
    },
    "assetCards": [
      {
        "image": "mesh",
        "title": "Maillage et géométrie",
        "body": "Inspectez et modifiez sommets, normales, UV et poids dans un profil .cy3d borné. Générez des LOD statiques et vérifiez les formes de collision primitives.",
        "alt": "Éditeur de maillage neutre avec aperçu géométrique et propriétés des sommets"
      },
      {
        "image": "skeleton",
        "title": "Squelette et retargeting",
        "body": "Modifiez hiérarchie et transformations, inspectez le skinning et utilisez le retargeting de pose de référence. L’IK hors ligne et le mouvement racine ont des limites explicites.",
        "alt": "Éditeur de squelette avec hiérarchie, transformations et superposition des os"
      },
      {
        "image": "animation",
        "title": "Clips et images clés",
        "body": "Lisez, créez, découpez et rééchantillonnez les clips. Naviguez dans la timeline et préservez le mouvement source lors de l’extraction du mouvement racine.",
        "alt": "Éditeur de clip d’animation et timeline des pistes"
      },
      {
        "image": "json",
        "title": "Documents JSON",
        "body": "Modifiez objets, tableaux et valeurs avec annulation et sauvegardes protégées. Un éditeur pour métadonnées et données d’assets neutres.",
        "alt": "Arbre de document JSON et propriétés scalaires modifiables"
      }
    ],
    "portableKicker": "UN LANGAGE COMMUN POUR LES ASSETS",
    "portableTitle": "Partagez l’asset.\nPréservez son identité.",
    "portableBody": "Le contrat portable utilise .cypack avec un GLB autonome, en complément du profil natif .cy3d. Préservez identifiants et métadonnées inconnues entre SDK dérivés. Les plans de conversion déclarent capacités et pertes ; les codecs de jeux restent à implémenter.",
    "portableLink": "Explorer le contrat d’assets portable",
    "automation": {
      "kicker": "05 / MCP ET SDK POUR IA",
      "title": "Un seul éditeur.\nLes mêmes opérations pour l’IA.",
      "body": "Interface, IPC natif et SDK utilisent le même EditorCore. CyToolsCore fournit MCP, identités, sessions, jobs et ProcessWorker : les outils IA se connectent aux véritables opérations de l’éditeur.",
      "steps": [
        [
          "Découvrir",
          "Lister les opérations et examiner leurs paramètres typés."
        ],
        [
          "Agir",
          "Lire et modifier avec les mêmes commandes de document validées."
        ],
        [
          "Observer",
          "Suivre les jobs, inspecter les diagnostics et vérifier l’état courant."
        ]
      ],
      "note": "MCP relie des clients IA configurés. CyGameSDK n’intègre pas d’agent autonome de création de jeux.",
      "link": "Configurer MCP et le SDK",
      "alt": "Panneau de diagnostics natif dans l’espace Sandbox"
    },
    "extend": {
      "kicker": "06 / VOTRE JEU, VOTRE CHAÎNE D’OUTILS",
      "title": "Apportez le savoir\nqui en fera votre SDK.",
      "body": "Créez un projet d’éditeur séparé. Implémentez IGameAdapter et les extensions source pour éditeurs d’assets, nœuds, rendu, cibles de compilation et codecs de confiance. Conservez la fondation réutilisable.",
      "alt": "Panneau de compilation configuré pour l’archive de documents Sandbox",
      "note": "La cible Sandbox fournie exporte un ZIP de documents. Elle ne prépare pas un jeu et ne produit pas d’exécutable de jeu."
    },
    "integrationSteps": [
      [
        "01",
        "Choisir une cible",
        "Documentez version du jeu, repères, fichiers représentatifs et connaissances réelles des formats."
      ],
      [
        "02",
        "Relier un petit workflow",
        "Implémentez un chemin réel d’import, d’édition et d’export, avec validation et annulation prévisible."
      ],
      [
        "03",
        "Vérifier dans le jeu",
        "Testez les données exportées avec le jeu réel ou un validateur officiel avant d’annoncer une compatibilité."
      ]
    ],
    "personalize": {
      "kicker": "FAITES PLACE À VOTRE FAÇON DE TRAVAILLER",
      "title": "Votre disposition. Votre palette.",
      "body": "Choisissez un thème, une densité compacte ou confortable et conservez votre langue. L’anglais est la langue par défaut et de secours ; le français est disponible.",
      "alt": "Espace Sandbox de CyGameSDK avec le thème gris clair"
    },
    "docsKicker": "UN BON POINT DE DÉPART",
    "docsTitle": "Comprenez la fondation.\nPuis construisez dessus.",
    "docsBody": "Commencez par le guide bilingue, puis consultez les références source pour les adaptateurs, graphes, assets et MCP. Le guide HTML local fonctionne sans connexion internet.",
    "docsButton": "Ouvrir le guide",
    "docsSecondary": "Guide pour développeurs IA",
    "faqTitle": "Avant de commencer.",
    "faqs": [
      [
        "Est-ce un moteur de jeu complet ?",
        "Non. CyGameSDK est une fondation en développement pour des éditeurs natifs. Le Sandbox est un adaptateur de démonstration. Rendu d’un jeu, formats réels et liaisons d’exécution relèvent de l’intégration ciblée."
      ],
      [
        "Que puis-je télécharger ?",
        "La version 0.29.0 est disponible en sources, avec sommes SHA-256. Cette release ne contient pas d’installeur Windows. L’hôte natif se compile sur Windows x64."
      ],
      [
        "Est-ce le Blueprint d’Unreal Engine ?",
        "Non. L’éditeur possède son propre interpréteur de graphes typés. Il s’inspire de workflows familiers sans annoncer de compatibilité Blueprint complète ni de parité avec le moteur."
      ],
      [
        "Fonctionne-t-il sous Linux ou macOS ?",
        "L’hôte graphique actuel utilise Windows x64 et Direct3D 11. Les hôtes graphiques Linux et macOS ne sont pas implémentés."
      ],
      [
        "Puis-je l’utiliser pour un jeu sans éditeur ?",
        "C’est le cas d’usage visé. Il faut un accès autorisé à des données représentatives, la connaissance des formats, une chaîne d’outils et un adaptateur vérifié. Un filtre d’extension n’est pas un codec."
      ]
    ],
    "downloadTitle": "Construisez votre prochain éditeur.",
    "downloadBody": "Récupérez les sources 0.29.0 et partez du Sandbox. Raccordez votre jeu un workflow vérifié à la fois.",
    "downloadNote": "Préversion de développement · Hôte Windows x64 · Distribution source · MIT",
    "sourceButton": "Télécharger les sources .zip",
    "releaseButton": "Notes de version et sommes de contrôle",
    "footer": "Une base native pour les outils propres à votre jeu.",
    "license": "Licence MIT",
    "top": "Retour en haut",
    "guideTitle": "Guide CyGameSDK",
    "guideIntro": "Un point de départ pratique pour la fondation 0.29.0. Ce guide HTML est disponible en anglais et en français et s’ouvre hors ligne. Les références source détaillées sont en anglais.",
    "guideNav": [
      [
        "start",
        "Bien commencer"
      ],
      [
        "build",
        "Compiler et lancer"
      ],
      [
        "explore",
        "Explorer l’éditeur"
      ],
      [
        "mcp",
        "MCP et SDK pour IA"
      ],
      [
        "adapter",
        "Créer un SDK de jeu"
      ],
      [
        "scope",
        "Limites actuelles"
      ]
    ],
    "guideSections": [
      {
        "id": "start",
        "title": "Commencer avec les bons repères.",
        "body": "CyGameSDK est une base d’éditeur natif en développement précoce. Il fournit un adaptateur Sandbox et des systèmes réutilisables d’interface, de documents et d’édition. Il ne prend pas automatiquement en charge n’importe quel jeu. La version 0.29.0 est distribuée en sources sous MIT.",
        "items": [
          "Hôte graphique actuel : Windows x64 / Direct3D 11.",
          "L’interface démarre en anglais et conserve un choix explicite de langue.",
          "Projets, assets de travail et préférences se trouvent dans le dossier data. Conservez des sauvegardes et une destination séparée pour un SDK dérivé."
        ],
        "ref": "CapabilityCoverage.md"
      },
      {
        "id": "build",
        "title": "Compiler et lancer sous Windows.",
        "body": "Installez Visual Studio 2022 avec Développement Desktop en C++, le SDK Windows, Git et CMake 3.24 ou plus récent. Clonez ou extrayez les sources dans un nouveau dossier. Exécutez ces commandes à la racine du projet.",
        "code": ".\\app\\build.ps1\n.\\app\\setup_mcp.ps1\n.\\runtime\\python\\Scripts\\python.exe app\\setup_examples.py\n.\\Lancer.cmd",
        "items": [
          "Le build récupère la révision exacte de app/dependencies.lock.json. Si le dépôt est privé, authentifiez Git au préalable ; jamais d’identifiants dans les sources ou arguments CMake.",
          "Hors ligne, passez -CyUIRoot à app/build.ps1 avec un checkout propre de la révision verrouillée.",
          "setup_mcp.ps1 nécessite la wheel CyToolsCore 0.10.1 construite depuis un checkout autorisé. Sélectionnez son emplacement avec -CyToolsCoreRoot.",
          "L’hôte nécessite le runtime Microsoft C++. MCP et les jobs d’import, de shaders et de compilation nécessitent l’environnement Python/Core configuré."
        ],
        "ref": "CyUIMigration.md"
      },
      {
        "id": "explore",
        "title": "Explorer les workflows fournis.",
        "body": "Le Sandbox de démarrage est une scène de démonstration. Ctrl+O charge un projet enregistré ; Ctrl+S le sauvegarde. L’installeur d’exemples refuse d’écraser les assets.",
        "items": [
          "Niveau : ancrez les panneaux, sélectionnez les objets, inspectez les propriétés et utilisez les gizmos.",
          "Graphes : ouvrez un exemple, inspectez les connexions typées et utilisez le débogueur.",
          "Matériaux : ouvrez Example_TexturedMaterial.cyasset et compilez l’aperçu DX11. C’est un shader d’éditeur, pas un export de matériau de jeu.",
          "Examples3D : ouvrez StaticTriangle.cy3d ou AnimatedTriangle.cy3d pour maillage, squelette, animation et échanges GLB/CyPack.",
          "Ctrl+P ouvre la recherche de commandes. Ctrl+Espace ouvre le tiroir de contenu. Le navigateur propose filtres, favoris et références."
        ],
        "ref": "Native3DAssets.md"
      },
      {
        "id": "mcp",
        "title": "Se connecter aux véritables opérations.",
        "body": "Démarrez l’éditeur. Configurez votre client MCP avec app/mcp.example.json et les chemins absolus de votre installation. Exécutez app/tool.py avec l’environnement Python privé. CyToolsCore fournit serveur et jobs.",
        "code": ".\\runtime\\python\\Scripts\\python.exe app\\tool.py describe\n.\\runtime\\python\\Scripts\\python.exe app\\tool.py mcp",
        "items": [
          "Découvrez les opérations et leurs schémas avant de les appeler.",
          "Interface, IPC et SDK partagent validation d’EditorCore et transactions de documents.",
          "Suivez les jobs jusqu’à un état terminal. Après un timeout de modification, inspectez l’état réel avant de réessayer : la modification peut déjà être appliquée."
        ],
        "ref": "MultiClient.md"
      },
      {
        "id": "adapter",
        "title": "Dériver un éditeur pour un jeu précis.",
        "body": "Consultez AI_GUIDE.md et le modèle de portage. Travaillez dans un projet séparé avec des données réelles et des éléments de preuve pour chaque format.",
        "items": [
          "Documentez jeu, plateforme, version exacte, unités, repères et connaissances des formats.",
          "Implémentez IGameAdapter, schémas et lecture/écriture réelle ; enregistrez les extensions source de rendu, d’éditeurs et de compilation.",
          "Commencez par un chemin d’import, d’édition et d’export vérifié dans le jeu ou avec un validateur officiel.",
          "Conservez identifiants, métadonnées inconnues et déclaration des pertes. Un profil de conversion reste une déclaration, pas un codec de jeu."
        ],
        "ref": "GameAdapterGuide.md"
      },
      {
        "id": "scope",
        "title": "Ce qui reste propre à l’intégration.",
        "body": "Aucun codec réel de jeu n’est fourni. Le viewport illustre des primitives projetées ; le rendu d’un jeu demande une intégration source. Le runtime borné n’est pas le Blueprint complet d’Unreal.",
        "items": [
          "Les builds Sandbox produisent des ZIP de documents, pas des jeux préparés ni des exécutables de jeu.",
          "Les outils de maillage neutre, d’animation et d’image suivent des profils bornés ; ils ne remplacent pas un logiciel DCC complet ou un outil de peinture.",
          "Les plugins sont des extensions source. Aucun chargeur de DLL dynamique à ABI stable n’est annoncé.",
          "Hôtes natifs Linux/macOS et installeur redistribuable restent à développer."
        ],
        "ref": "CapabilityCoverage.md"
      }
    ]
  }
};
