# CyGameSDK static showcase

A buildless, bilingual showcase for the 0.29.0 native editor. This directory is a
complete static GitHub Pages source: HTML, CSS, JavaScript, SVG and PNG assets.
It does not require Node, npm, a CDN, remote fonts, analytics or a backend.

## Local preview

From the repository root, use any static server, for example:

```powershell
python -m http.server 8080 --bind 127.0.0.1 --directory docs
```

Open the printed local address. Append `?lang=fr` for French or `?lang=en` for
English. With no saved choice or URL override, English is the default regardless
of browser/Windows locale. The language selection is saved locally when possible.
The site and `guide.html` also open directly from disk without network access;
external GitHub/download/reference links naturally require a connection.

## Files and maintenance

- `index.html`: showcase entry point.
- `guide.html`: bilingual HTML setup/adapter guide. No CyDoc reader was present in
  this repository; no existing documentation reader has been replaced.
- `site-data.js`: complete EN/FR copy and verified product/release links.
- `site.js`: navigation, mode tabs, localization and native-dialog image viewer.
- `styles.css`: graphite/blue responsive presentation and reduced-motion support.
- `assets/screenshots`: fourteen real 0.29.0 application captures with generated
  Sandbox fixtures, exported as PNG without text/EXIF metadata.
- `assets/mark.svg`: simple vector mark for this showcase, under the project MIT license.

After changing CSS or scripts, refresh their content-based query strings:

```powershell
python docs/tools/stamp_assets.py
```

Commit the updated HTML together with the changed assets. A different hash causes
browsers/CDNs to request the new resource; no service worker caches old pages.

Source screenshots were reviewed for local paths and personal content. Do not add
captures of real projects, credentials or private paths. Keep screenshot alt text
and all interaction labels translated. Screenshots show the English application;
this is stated in the page content and is independent of the site language.

## GitHub Pages preparation

When publication is explicitly authorized, this directory can be served from the
`/docs` folder on the selected branch. Relative paths support the `/CyGameSDK/`
project prefix and direct-file use. `.nojekyll` disables Jekyll processing. There
is no deployment workflow, automatic publication or hosting configuration change.

## Product boundaries

The website describes a development foundation, the Windows x64/DX11 host and the
0.29.0 source release. It does not offer an installer, automatic game support, a
full game engine or Unreal Blueprint compatibility. Detailed technical source
references are in English and are labelled accordingly in the bilingual guide.
