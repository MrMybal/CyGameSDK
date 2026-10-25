# Build and launch troubleshooting

Current source/catalogue version: 0.28.0. See the root [README](../../README.md)
for setup and [CompletionMilestones](CompletionMilestones.md) for historical
feature milestones. Historical local archives are not executable replacements
for the current source/catalogue.

## A fresh source checkout

The repository does not include CyGameEditor.exe or the private runtime. Build
with app/build.ps1 and prepare MCP/worker dependencies with app/setup_mcp.ps1.
The native executable is written to the repository root. Verify dependency paths,
the Visual Studio C++ workload and the Windows SDK when compilation fails.

## Launching

Open CyGameEditor.exe or Lancer.cmd from the built project. The launcher quotes
its absolute path and sets the working directory. An immediate launch failure
is reported in its console. Logs from the native host are local under data/logs.

If Windows reports an access error, confirm that the expected executable exists,
inspect its file permissions and the relevant Windows/security application logs.
Treat application-control decisions separately from malware scan results. Do not
rename an older binary or disable protections to work around a launch failure.
Keep source, catalogue and executable versions matched and rebuild from reviewed
source when replacement is necessary. Preserve project, asset and settings data.

English is the default/fallback UI language. A language explicitly saved in Options
is retained on the next normal launch. Capture and automated test hosts use isolated
roots and do not overwrite the user's preferences or layout.
