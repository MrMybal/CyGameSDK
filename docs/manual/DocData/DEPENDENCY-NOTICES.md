# Component notices

- CyUI 1.2.0 at `6ef01cfcdb197f3a89d64618ce5ef8818c704728`, MIT. CyUI-MIT.txt.
  Upstream: https://github.com/MrMybal/CyUI
  CyUI-THIRD-PARTY-NOTICES.md is preserved verbatim; its relative paths refer to
  the locked CyUI checkout, not this notices directory.
- Dear ImGui 1.93.0 WIP (`IMGUI_VERSION_NUM=19297`), locked CyUI snapshot, MIT.
  Dear-ImGui-MIT.txt and Dear-ImGui-stb-MIT.txt (MIT option for bundled stb).
  Upstream: https://github.com/ocornut/imgui
- CyDoc 0.1.0 at `e038c474577e1f305df3eba421f2b9c03b0f1d8d`, MIT. CyDoc-MIT.txt.
  Upstream: https://github.com/MrMybal/CyDoc. Reader: image, gallery, graph.
- CyMCPSDK 0.7.1 at 34d4bcae126bc83d1a2925b5047dac36e3d09d92, MIT. CyMCPSDK-MIT.txt.
  CyMCPSDK-THIRD-PARTY-NOTICES.md is retained verbatim.
- The older CyToolsCore-MIT.txt remains a scaffolding provenance notice; that runtime is not distributed or required.
- nlohmann/json 3.11.3: vendored single header, MIT. nlohmann-json-MIT.txt.
  Upstream: https://github.com/nlohmann/json/tree/v3.11.3
- Python and installed dependencies: Python.txt, python/ and ../licenses-python.json.
- Microsoft Windows SDK / Direct3D and C++ runtime: respective Microsoft terms.
- Segoe fonts are loaded from the Windows installation, not redistributed.

CyGameSDK's original code, documentation and examples are licensed under the
[MIT License](../../LICENSE). Third-party components retain their respective
licenses; the project license does not replace their copyright or license notices.

- ImGuizmo: official source snapshot at commit 105e4fde45e4e74aae8973d98750820ef7aca7e2,
  MIT. ImGuizmo-MIT.txt. Source and SHA-256 hashes: ../third_party/imguizmo/SOURCE.json.
  Upstream: https://github.com/CedricGuillemet/ImGuizmo

- Lexilla lexical engines, Scintilla interface headers and SciTE keyword/property
  data: HPND-style permissive license. Lexilla.txt, Scintilla.txt and
  SciTE-Properties.txt. Exact source revisions/hashes: CyUI-code-syntax.lock.json.
  Code is linked through the Code/Diff modules; no Scintilla editor UI is linked.
  The optional CodeEditor/TextEditor backend is now linked through an isolated C++20 bridge.
  Audio, Gallery, CEF, Office and Unreal runtimes are not distributed by this application build.

- Native TextEditor backend: MIT, TextEditor-MIT.txt. Built only for the selected CodeEditor module.
- Bundled JetBrains Mono Regular 2.304: SIL Open Font License 1.1, JetBrains-Mono-OFL.txt. Embedded through CyUI Core; no system font installation is required.

- SDK nlohmann/json 3.12.0, cpp-httplib and PicoSHA2: respective MIT notices in this directory.
- MD4C Markdown parser through CyUI DocumentViews: MIT, MD4C-MIT.txt.
