#pragma once
#include "Core.h"
#include <windows.h>
namespace cygame {
class EditorIpc {
public:
    explicit EditorIpc(EditorCore& core);
    ~EditorIpc();
    void poll();
    const std::string& name() const { return name_; }
private:
    EditorCore& core_;
    HANDLE pipe_=INVALID_HANDLE_VALUE;
    std::string name_,incoming_,outgoing_;
    size_t sent_=0;
    bool connected_=false;
    ULONGLONG since_=0;
    void disconnect();
};
}
