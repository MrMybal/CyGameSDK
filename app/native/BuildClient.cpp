#include "BuildClient.h"
namespace cygame {
namespace {bool terminal(const Json& state){const auto s=state.value("state","Idle");return s=="Idle"||s=="Completed"||s=="Failed"||s=="Cancelled"||s=="Interrupted";}}
BuildClient::~BuildClient(){try{if(!terminal(status()))cancel();}catch(...){}if(process_)CloseHandle(process_);}
Json BuildClient::start(const Json& parameters,const std::string& operation){
    if(operation!="build_execute"&&operation!="material_compile"&&operation!="asset_import"&&operation!="asset_reimport"&&operation!="model_export"&&operation!="model_process"&&operation!="game_codec_list"&&operation!="game_codec_export")throw std::runtime_error("Unsupported desktop job operation");
    if(!terminal(status(true)))throw std::runtime_error("A desktop build job is already active");
    wchar_t exe[32768]{};GetModuleFileNameW(nullptr,exe,32768);const auto install=fs::path(exe).parent_path(),python=install/"runtime/python/Scripts/python.exe",bridge=install/"app/build_bridge.py";
    if(!fs::is_regular_file(python)||!fs::is_regular_file(bridge))throw std::runtime_error("Install the private CyToolsCore runtime with app/setup_mcp.ps1");
    if(process_){CloseHandle(process_);process_=nullptr;}const auto id=uniqueId();run_=root_/"data/build-runs"/id;
    writeJson(run_/"request.json",{{"editorRoot",root_.u8string()},{"parameters",parameters},{"operation",operation}});state_={{"runId",id},{"state","Created"}};writeJson(run_/"state.json",state_);
    const auto quoted=[](const fs::path& p){return L"\""+p.wstring()+L"\"";};std::wstring command=quoted(python)+L" "+quoted(bridge)+L" --run "+quoted(run_);
    STARTUPINFOW startup{};startup.cb=sizeof(startup);startup.dwFlags=STARTF_USESHOWWINDOW;startup.wShowWindow=SW_HIDE;PROCESS_INFORMATION process{};
    if(!CreateProcessW(python.c_str(),command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,install.c_str(),&startup,&process)){state_={{"runId",id},{"state","Failed"},{"error","Cannot start the build SDK client"}};writeJson(run_/"state.json",state_);throw std::runtime_error("Cannot start the build SDK client");}
    process_=process.hProcess;CloseHandle(process.hThread);lastPoll_=0;return state_;
}
Json BuildClient::status(bool refresh){
    if(refresh)lastPoll_=0;
    if(run_.empty())return state_;
    if(GetTickCount64()-lastPoll_>100){lastPoll_=GetTickCount64();try{state_=readJson(run_/"state.json");}catch(...){}
        DWORD code=STILL_ACTIVE;if(process_&&GetExitCodeProcess(process_,&code)&&code!=STILL_ACTIVE&&!terminal(state_))state_={{"runId",run_.filename().u8string()},{"state","Failed"},{"error","The build SDK client exited before publishing a terminal result"}};
    }return state_;
}
Json BuildClient::cancel(){if(!terminal(status())&&!run_.empty())writeJson(run_/"cancel.request",{{"requested",true}});return status();}
}
