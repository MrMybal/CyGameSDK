#pragma once
#include "Core.h"
namespace cygame {
fs::path importSidecar(const fs::path& asset);
void transferAssetPair(const fs::path& source,const fs::path& target,bool copy);
}
