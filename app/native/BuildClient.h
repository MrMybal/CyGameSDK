#pragma once
#include "Core.h"
#include <windows.h>
namespace cygame {
// A client of the official SDK, not a scheduler. One sidecar owns one Core job.
class BuildClient {
public:
    explicit BuildClient(fs::path root):root_(std::move(root)){}
    ~BuildClient();
    Json start(const Json& parameters,const std::string& operation="build_execute");
    Json status(bool refresh=false);
    Json cancel();
private:
    fs::path root_,run_;HANDLE process_=nullptr;Json state_={{"state","Idle"}};ULONGLONG lastPoll_=0;
};
}
