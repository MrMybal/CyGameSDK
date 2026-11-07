# CyDoc integration and maintenance

This is developer/build documentation, outside the human manual and its search.
The human reader is `Documentation/Documentation.html`; the entire `DocData` tree
and `DocAI.md` travel with it. No server, Node runtime, CDN or online font is
required to read the export. The web site mirrors the same reader at `docs/manual/`.

## Reproduce the dependency and export

CyDoc 0.1.0 is pinned in `app/dependencies.lock.json` to
`e038c474577e1f305df3eba421f2b9c03b0f1d8d` from
https://github.com/MrMybal/CyDoc. Existing Git authentication accesses the private
repository. No sibling checkout is modified. Node.js 22+, npm and Git are build
tools; the graphical host still requires Windows x64 / DX11 / MSVC.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File app/build_documentation.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File app/build_documentation.ps1 -Check
powershell -NoProfile -ExecutionPolicy Bypass -File app/build_documentation.ps1 -OutputDirectory runtime/help-package/Documentation
```

The script uses a clean private clone under `runtime/dependencies/cydoc`, checks
its exact revision, builds/packs the official package when absent, verifies the
archive SHA-512 against `.CyDocBuild/package-lock.json`, then installs the locked
application generation dependencies. Upstream's own development toolchain may
include optional demo/test dependencies; these stay private. The application
install omits optional peers, including Three.js.

With dependencies already bootstrapped, `npm --prefix .CyDocBuild run build` or
`npm --prefix .CyDocBuild run check` invokes the same generator. The direct CyDoc
CLI is not the app's regeneration entry: first generate the catalogue API inputs
and apply the app's translation/help/package checks through `build.mjs`.

`CyGameDocumentation` is a CMake ALL target and a dependency of CyGameEditor. It
copies the complete reader next to the configured native output. The CMake
installation rule (`--component Documentation`) installs Documentation only; it is not a complete application
installer. Source distributions include the generated reader plus authoring
inputs; exclude node_modules, private runtime and `.CyDocBuild/internal`.
A built reader may be copied without copying or rebuilding the editor.

## Authoring, API and translations

Read the locked CyDoc `ai/README.md`, `INTEGRATION.md`, `AUTHORING.md` and `API.md`.
Its repository AGENTS applies to changes in that library, not this application.
Edit `.CyDocBuild/manual/en` first, then `fr`; preserve IDs and explicit anchors.
Use revision/sourceRevision for prose. Locale defaults to English. The reader
preserves explicit choices; app Help/F1 supplies the active editor language.

The operation catalogue is the real `app/CyTool.json` (175 operations at 0.29.0).
`build.mjs` converts it into CyDoc's neutral API format, calls the public
`cydoc/api-import` API and writes disposable `manual/*/api-generated` Markdown
inputs. This adds an `operations` parent and revision metadata, which the current
neutral importer does not accept. Signatures, exact JSON schemas, defaults,
required parameters and returns still come through the official importer.
`operation-reviews.json` pins reviewed EN/FR input pairs. A changed description,
schema or operation fails until its translation is reviewed; never auto-accept a
new hash merely to make a build pass.

The only indexed source directory is `app/examples/adapters`. EN/FR @doc blocks
supply its public contract; apiIndex supplies public includes/declarations. No
implementation-source browser, vendor trees, credentials, private paths or runtime
files are exported. Source extraction/API-file pages lack revision fields upstream:
the two resulting untracked entries in CyDoc's translation report are additionally
gated by reviewed compiled EN/FR fingerprints in `translation-review.json`.
No missing or outdated translations are allowed. Hash ledgers are application
review gates, not automatic translation or semantic review.

The content disposition inventory is the private `.CyDocBuild/internal/content-inventory.json`.
Existing `app/Docs` contracts/examples remain available at their old source paths.
AI guides, audits and historical milestones stay outside human-reader navigation.
Generic Core model/CUDA/download/standalone-Python-UI requirements are conditional,
not implemented features of this native editor. The obsolete six-section site
reader/data were removed after replacement; `guide.html` redirects old EN/FR
chapters to the actual CyDoc reader. Stable source-guide aliases are also retained.

## Reader modules, integration and constraints

- image: real screenshots, zoom, pan, annotations and in-window enlargement.
- gallery: a tool tour with image children, keyboard selection and minimal controls.
- graph: a small interactive event/print illustration, explicitly not execution.
- No mesh/video/gif: no genuine local demonstrations of those types are supplied.
  Screenshots of the app's 3D tools do not require the mesh reader or Three.js.

Use `cyui::palette`/DPI for the native host, and the official CyDoc themes for the
reader. Dark/Nord fits the application; saved selection is respected. Stock export
starts with the OS/saved reader theme: it exposes no config for synchronizing the
editor theme or setting a per-app initial theme. This integration retains that
behavior and does not fork the viewer. Source-index/include inference is lexical,
not a C++ compiler or proof of a game codec. CyDoc loads all pages together; the
large API reference is not segmented.

`Documentation.cpp` maps stable focused-panel IDs to real page/anchor pairs and
uses the upstream `cydoc_link.hpp` route helper. The host encodes absolute UTF-8
file paths (including spaces/#/%), then ShellExecute opens the local reader.
Help > Documentation opens overview; Context Help/F1 targets the panel. The Editor
Overview also has a Documentation button. A source host may supply its own opener.
Unknown extension panels fall back to overview; game extensions should add a topic.
File-origin storage/persistence remains subject to browser policy.

Exports retain official classic-script bootstrap, embedded data and local media
providers. Packaging only appends content hashes to HTML script/style URLs for
cache renewal; it does not modify the generated manifest/viewer logic. CyDoc MIT
and dependency notices are copied into DocData; the upstream route-helper notice
and exact source identity are retained under third_party/cydoc.

## Verification

`build.mjs` invokes the official compiler/checker and `.CyDocBuild/verify.mjs`:
EN/FR page/anchor equality, actual operation coverage, native help destinations,
local entry assets, no remote entry dependency, selected/used module scripts, no
Three.js and no developer absolute paths in public content. Build/copy mirrors are
also compared during delivery verification.

For native help changes, `--test-help <report.json>` runs only an isolated host's
ImGui input with an intercepted opener. Supply --root with its own catalogue,
locales and complete Documentation tree. It tests real focused-panel F1 dispatch,
language, URL encoding and fallback without manipulating another app's input.
Do not rebuild a user's running editor merely to regenerate the manual.
Browser/file-origin visual tests and platform results belong in private reports,
not human manual pages; do not claim untested hosts or actual touch hardware.
