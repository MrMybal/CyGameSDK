# CyGameSDK 0.32.0 — local MCP access profiles

Windows x64 development preview with Free local MCP access by default.
Download the Windows folder, sources or complete offline manual from the v0.32.0 release.

CyMCPSDK 0.7.1 is compiled from locally sourced revision
**34d4bcae126bc83d1a2925b5047dac36e3d09d92**. A clean private clone of that local
commit prevents concurrent sibling edits from changing the build. CyUI remains pinned
to **6ef01cfcdb197f3a89d64618ce5ef8818c704728** and the same explicit modules.

- Free is the default for new settings or settings without an explicit profile.
- Free uses LocalAccessProfile::free, Policy::allow_all() and unauthenticated HTTP
  on 127.0.0.1. All registered operations remain available without a token or extra grant.
- Local controlled, Token only and Protected remain available. Protected still requires
  a valid token and applies the existing application operation policy.
- Existing explicit profiles and unrelated settings survive migration. Invalid or
  conflicting profiles fail explicitly. Argument/business validation and Undo remain.
- Native CLI supports --access-profile and address-only connections to tokenless endpoints.
  Existing Python CLI remote calls accept these endpoints too.
- The assistant shows its active profile, a next-start profile selector, local MCP
  enable/disable and a copyable URL. Profile selection applies after editor restart.
- Chat streaming, tools, cancellation/history and durable jobs keep their existing paths.

Read [integration](CyMCPSDKIntegration.md) for all four profiles and exact connection syntax.
Tests use the real native editor with isolated Sandbox roots and a local API fixture,
not an actual provider account/model. Windows x64/MSVC/DX11 is the implemented host.
No GitHub workflow, game codec or additional engine integration is introduced.

## Distribution

The Windows ZIP includes CyGameEditor.exe, CyGameCLI.exe, application sources/examples,
notices and the complete bilingual offline manual. Native editing/chat/MCP need no
compiler or Python installation. Compute workers optionally require Python 3.11+ and
app/setup_mcp.ps1. Install the Microsoft Visual C++ 2015–2022 x64 runtime if missing.
The documentation ZIP opens directly from disk. SHA256SUMS.txt covers all three ZIPs.
