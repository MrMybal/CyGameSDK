# CyGameSDK

[Présentation](https://mrmybal.github.io/CyGameSDK/?lang=fr) ·
[Documentation en français](https://mrmybal.github.io/CyGameSDK/manual/Documentation.html#/fr/overview) ·
[Versions et sources à télécharger](https://github.com/MrMybal/CyGameSDK/releases)

The main documentation is maintained in English in [README.md](README.md).
See it for build/setup instructions, examples, MCP integration and feature limits.
The editor defaults to English; French remains selectable in Options.

## Documentation locale

Ouvrez [le manuel CyDoc](Documentation/Documentation.html#/fr/overview) directement
avec une URL `file://`, sans serveur ni connexion. Le menu Help > Documentation et
F1 ouvrent le manuel dans la langue de l’éditeur ; F1 cible le panneau actif.
Le lecteur et ses ressources sont livrés dans `Documentation/`.

Régénération (Node.js 22+ et npm nécessaires uniquement pour cette étape) :

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File app/build_documentation.ps1
```

La révision privée de CyDoc est verrouillée. Git utilise votre authentification
existante. Le build CMake génère et copie le lecteur ; il ne publie rien.
