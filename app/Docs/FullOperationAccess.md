# FullOperationAccess — current application contract

This legacy guide URL is preserved for existing links. The old generic runtime guide
has been superseded by the application-specific [CyMCPSDK integration](CyMCPSDKIntegration.md#ownership-and-entry-points).

Use the shared native Registry for ChatUI, CLI and MCP. The application owns validation,
document Undo, durable jobs/identities and result recovery. SDK Executor/Process own
execution and process trees; no Python scheduler/server or model downloader is required.

Read [architecture](EditorArchitecture.md), [capability coverage](CapabilityCoverage.md)
and the complete offline [manual](../../Documentation/Documentation.html#/en/automation).
Historical releases/checkpoints retain their original contracts and evidence.
