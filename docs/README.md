# CyGameSDK static showcase and manual

This directory contains the bilingual static showcase and the complete CyDoc reader.
It needs no remote fonts, analytics, server framework or CDN.

- **index.html / site-data.js / site.js / styles.css**: English/French product presentation.
- **guide.html**: compatibility redirect into the actual CyDoc reader, preserving language.
- **manual/Documentation.html**, **manual/DocAI.md** and **manual/DocData/**: the official
  offline export, copied by **app/build_documentation.ps1**. Do not replace it with Markdown links.
- **assets/screenshots**: real Sandbox views from 0.29. The current assistant image is
  also supplied in **app/Docs/images** for the README. Preserve image provenance when replacing it.

Preview locally with **python -m http.server 8080 --bind 127.0.0.1 --directory docs**,
or open index.html directly. **?lang=fr** selects French; English is the default.
The manual also opens by file URL with no server or Internet.

After changing scripts/styles, run **python docs/tools/stamp_assets.py** and commit
the updated HTML with the assets. Resource query strings use content hashes.

GitHub Pages is configured from **main /docs**. Deployment is automatic through
GitHub's Pages workflow when Actions is enabled. When publication must use no
workflows, leave Actions disabled and do not request a Pages build. The hosted
site stays on its previous deployment; updated sources and the complete current
offline manual remain available from GitHub/release assets.

Public links:
- Showcase: https://mrmybal.github.io/CyGameSDK/
- English manual: https://mrmybal.github.io/CyGameSDK/manual/Documentation.html#/en/overview
- French manual: https://mrmybal.github.io/CyGameSDK/manual/Documentation.html#/fr/overview

Version 0.31.0 is a development preview with a portable Windows x64 folder, sources
and an offline documentation ZIP. Real game support requires verified adapters.
Linux/macOS native hosts and full Unreal/Blueprint compatibility are not supplied.
See **app/Docs/Releasing.md** for the local packaging/publication procedure.
