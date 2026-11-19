# Saved operation recipes

**generation_config.py** and the native recipe form preserve the versioned
**cytools-generation-config**, schemaVersion 1 envelope for existing user files.
Its name is a compatibility value, not a dependency on the former runtime.

The application-owned schema is **app/generation-config.schema.json**.
Validate toolId, operationId and parameters against the current **CyTool.json**.
Loading validates/restores a draft; it never executes code or starts a job.
Unknown or stale parameters need an explicit correction, never silent replay.

Manual files live in **data/configurations** under unique names. Optional autosave
records accepted job parameters/ID, not successful completion. Input paths reference
original files and do not copy their contents. Credentials and connection secrets are
excluded. User text remains user data in the recipe.

Replay uses the same shared registry and mutation policy as native CLI/chat/MCP.
Read [automation](CyMCPSDKIntegration.md) for CLI syntax and durable result retrieval.
