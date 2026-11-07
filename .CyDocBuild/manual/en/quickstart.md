---
id: quickstart
title: Your first editing session
description: Your first editing session
section: Getting started
order: 1
revision: 1
---

## Open the Sandbox {#open}

Launch the built editor with `Lancer.cmd`. The startup scene is the Sandbox, not an installed game. Select **Level** in the workspace selector. Ctrl+O reloads an explicitly saved project; startup does not automatically load the previous project.

1. Select `SM_Cube` in World Outliner or click it in the viewport.
2. In Details, edit Position. Commit the field, then use Ctrl+Z and Ctrl+Y to undo and redo.
3. Use W/E/R for Move/Rotate/Scale while the viewport owns input. Choose local/world mode and snapping. Group translation is supported; group rotation/scale is not a completed workflow.
4. Press Ctrl+P and search a panel, command or object. Ctrl+Space opens the content drawer.
5. Ctrl+S saves the project in this installation's `data/projects`. Keep installed game files separate.

## Try the examples {#examples}

After `app/setup_examples.py` has installed the examples, use Content Browser to open `StaticTriangle.cy3d`, `AnimatedTriangle.cy3d` or `Example_Data.json` under Examples3D. Graph fixtures are supplied under the Examples and BlueprintExamples folders. Installation never overwrites an existing asset; names and directories depend on the selected example set.

Open `Example_TexturedMaterial.cyasset` and choose **Compile preview** after configuring the Python/Core environment. This produces an editor shader, not a game shader export. [Material workflow](doc:materials#compile).

## Recover a mistake {#recover}

Use Undo for document edits. Save explicitly. A file move can be blocked when a saved or live graph references it. Read the displayed diagnostic instead of repeating the mutation. If an SDK mutation times out, inspect the live state before retrying.
