---
id: game-adapters
title: Create a game-specific editor
description: Create a game-specific editor
section: Getting started
order: 4
revision: 1
---

## Using this guide {#getting-started}

Start with AI_GUIDE.md and the game porting template. Work in a separate project with real game fixtures and evidence for each supported format.

- Record the exact game/platform/version, units, coordinate system and format evidence.

- Implement IGameAdapter, schemas and real parsing/serialization; register source extensions for rendering, editors and build providers.

- Start with one import/edit/export workflow and verify output against the game or an official validator.

- Keep asset IDs, unknown metadata and explicit loss reporting. A conversion profile is a declaration, not a game codec.

## Continue reading {#next}

- [First editing session](doc:quickstart)
- [Browse assets](doc:content-browser)
- [Graph authoring](doc:graphs)
- [Materials](doc:materials)
- [3D assets](doc:assets-3d)
- [SDK operation reference](doc:operations)
