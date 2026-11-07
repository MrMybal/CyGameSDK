---
id: recipes
title: Save and replay operation configurations
description: Save and replay operation configurations
section: SDK
order: 52
revision: 1
parent: automation
---

## Save values, not execution {#save}

The Operations/Options views save and load versioned recipes. A recipe captures the chosen operation, typed parameters, product/version and configuration metadata. Loading restores values without executing the operation. Review them before submitting. Optional parameter autosave does not grant permission to run a job or overwrite a source asset.

Use the current manifest/schema to validate replays. Unsupported operation IDs, invalid paths or obsolete parameter types are errors, not silent migration. Explicit recipe saves are persistent files; generated worker outputs instead use Core task storage. An SDK timeout can follow a committed mutation; inspect state before replaying it.
