#pragma once
#include "Core.h"
#include <array>
namespace cygame {
void validateModelAuthoring(const Json& model);
Json modelAtLod(const Json& model,int lod);
void validateScene3D(const Json& model);
struct ModelVertex {float position[3],normal[3],color[3];};
struct ModelFrame {std::vector<ModelVertex> triangles;std::vector<std::array<float,3>> nodes;std::array<float,3> minimum{},maximum{};};
ModelFrame sampleScene3D(const Json& model,int animation,float time,int lod=0);
}
