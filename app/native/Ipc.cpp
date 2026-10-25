#include "Ipc.h"
#include <sddl.h>
#include <cstring>
namespace cygame {
EditorIpc::EditorIpc(EditorCore& core):core_(core) {
    name_="\\\\.\\pipe\\CyGameSDK-"+std::to_string(GetCurrentProcessId());
    HANDLE token{}; if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token)) throw std::runtime_error("Cannot query Windows identity");
    DWORD bytes=0;GetTokenInformation(token,TokenUser,nullptr,0,&bytes);std::vector<char> data(bytes);
    if(!GetTokenInformation(token,TokenUser,data.data(),bytes,&bytes)){CloseHandle(token);throw std::runtime_error("Cannot read Windows identity");}
    LPWSTR sid{};ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER*>(data.data())->User.Sid,&sid);CloseHandle(token);
    if(!sid) throw std::runtime_error("Cannot resolve Windows SID");
    std::wstring sddl=L"D:P(A;;GA;;;"+std::wstring(sid)+L")";LocalFree(sid);
    PSECURITY_DESCRIPTOR descriptor{};
    if(!ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl.c_str(),SDDL_REVISION_1,&descriptor,nullptr)) throw std::runtime_error("Cannot create pipe ACL");
    SECURITY_ATTRIBUTES sa{sizeof(sa),descriptor,FALSE};
    std::wstring wide(name_.begin(),name_.end());
    pipe_=CreateNamedPipeW(wide.c_str(),PIPE_ACCESS_DUPLEX|FILE_FLAG_FIRST_PIPE_INSTANCE,
        PIPE_TYPE_BYTE|PIPE_READMODE_BYTE|PIPE_NOWAIT|PIPE_REJECT_REMOTE_CLIENTS,1,1024*1024,1024*1024,0,&sa);
    LocalFree(descriptor);
    if(pipe_==INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot create editor pipe");
    writeJson(core_.root()/"data/ipc/editor.json",{{"pipe",name_},{"pid",GetCurrentProcessId()},{"protocolVersion",1}});
}
EditorIpc::~EditorIpc() {
    if(pipe_!=INVALID_HANDLE_VALUE)CloseHandle(pipe_);
    std::error_code ec;fs::remove(core_.root()/"data/ipc/editor.json",ec);
}
void EditorIpc::disconnect() {
    DisconnectNamedPipe(pipe_);connected_=false;incoming_.clear();outgoing_.clear();sent_=0;
}
void EditorIpc::poll() {
    if(!connected_) {
        const bool result=ConnectNamedPipe(pipe_,nullptr)!=FALSE;const auto error=GetLastError();
        if(!result&&error!=ERROR_PIPE_CONNECTED){if(error==ERROR_NO_DATA)disconnect();return;}
        connected_=true;since_=GetTickCount64();
    }
    if(GetTickCount64()-since_>10000){disconnect();return;}
    if(outgoing_.empty()) {
        char buffer[65536];DWORD read{};
        if(ReadFile(pipe_,buffer,sizeof(buffer),&read,nullptr)&&read)incoming_.append(buffer,read);
        else if(GetLastError()!=ERROR_NO_DATA&&GetLastError()!=ERROR_PIPE_LISTENING){disconnect();return;}
        if(incoming_.size()<4)return;
        uint32_t length{};std::memcpy(&length,incoming_.data(),4);
        if(length>1024*1024||!length){disconnect();return;}
        if(incoming_.size()<length+4)return;
        Json response;
        try {
            const auto request=Json::parse(incoming_.substr(4,length));
            if(request.value("protocolVersion",0)!=1)throw std::runtime_error("Unsupported editor IPC version");
            response={{"result",core_.execute(request.at("operation"),request.value("parameters",Json::object()))}};
        } catch(const std::exception& e) {response={{"error",{{"code","EditorCommandFailed"},{"message",e.what()}}}};}
        const auto text=response.dump(-1,' ',false,Json::error_handler_t::replace);uint32_t size=static_cast<uint32_t>(text.size());
        if(size>4*1024*1024){disconnect();return;}
        outgoing_.assign(reinterpret_cast<const char*>(&size),4);outgoing_+=text;
    }
    DWORD written{};
    if(!WriteFile(pipe_,outgoing_.data()+sent_,static_cast<DWORD>(outgoing_.size()-sent_),&written,nullptr)){
        disconnect();return;
    }
    sent_+=written;
    // A new connection is accepted only after the client has read and closed.
    if(sent_==outgoing_.size()) {
        DWORD available{};
        if(!PeekNamedPipe(pipe_,nullptr,0,nullptr,&available,nullptr))disconnect();
    }
}
}
