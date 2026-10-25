#pragma once
#include "Core.h"
namespace cygame {
struct ImageData{Json info;std::vector<unsigned char> rgba;};
ImageData readImageFile(const fs::path& path,bool pixels);
ImageData readImage(const fs::path& root,const std::string& path,bool pixels);
}
