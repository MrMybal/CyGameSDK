# Example source adapter

`ExampleGameAdapter.h` implements the current source-level `IGameAdapter` interface without
an ImGui dependency. It demonstrates schema-generated properties with an additional
bounded integer `health`, an initial scene, validation and conservative capabilities.
It has no game file parser, renderer, build target or engine runtime binding.
It is not compiled into or selected by the default editor.

Read [AI_GUIDE.md](../../../AI_GUIDE.md) and
[GameAdapterGuide.md](../../Docs/GameAdapterGuide.md) first. In a derived
project, include this header from main.cpp and replace the EditorCore construction
with an injected `std::make_unique<cygame::examples::ExampleGameAdapter>()`. Add
Health to the translation catalogue when exposing the example UI. Register an
actual viewport renderer for a non-Sandbox adapter; the default box view is gated
on the Sandbox ID. Do not announce preview/build/play support from this starter.

The documentation contract verification links against the active core and checks:
factory/schema validation, the custom property and Undo/Redo, duplicate-ID/range
rejection, entity creation, project roundtrip and absence of build targets. Results
are recorded in `data/reports/ai-game-sdk-docs-validation.json` at the project root.
These checks establish an adapter example, not compatibility with a real game.
