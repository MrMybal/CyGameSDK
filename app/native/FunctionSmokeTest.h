#pragma once
#include "Editor.h"
namespace cygame {
// Exercises the real focused graph windows and keyboard shortcuts in an owned host.
class FunctionSmokeTest {
    EditorCore& core_;EditorApp& app_;fs::path report_;std::string graph_;bool entered_=false,locals_=false,nested_=false,returned_=false,inspector_=false;
    Json state(){return core_.execute("graph_debug_status",{{"graphId",graph_}});}
public:
    FunctionSmokeTest(EditorCore& core,EditorApp& app,fs::path report,std::string graph):core_(core),app_(app),report_(std::move(report)),graph_(std::move(graph)){
        if(enabled())app_.debuggerProbe=[this](const std::string& key,const Json& state){if(key==graph_&&state.value("activeFunction","")=="Compute"&&state.value("callStack",Json::array()).size()==2)inspector_=true;};
    }
    bool enabled()const{return !report_.empty();}
    void input(ImGuiIO& io,int frame){
        if(!enabled())return;
        if(frame==25){app_.invoke("graph_debug_start",{{"graphId",graph_}});app_.open("debugger");app_.open(graph_);}
        if(frame==30||frame==46||frame==54)io.AddKeyEvent(ImGuiKey_F10,true);
        if(frame==31||frame==47||frame==55)io.AddKeyEvent(ImGuiKey_F10,false);
        if(frame==38||frame==62||frame==70){if(frame==70)io.AddKeyEvent(ImGuiMod_Shift,true);io.AddKeyEvent(ImGuiKey_F11,true);}
        if(frame==39||frame==63||frame==71){io.AddKeyEvent(ImGuiKey_F11,false);if(frame==71)io.AddKeyEvent(ImGuiMod_Shift,false);}
        if(frame==42)entered_=state().value("activeFunction","")=="Compute";
        if(frame==58){const auto s=state();locals_=s.value("activeFunction","")=="Compute"&&s.at("variables").value("Counter",0)==5;}
        if(frame==66){const auto s=state();nested_=s.value("activeFunction","")=="Report"&&s.at("callStack").size()==3;}
        if(frame==80)io.AddKeyEvent(ImGuiKey_Home,true);if(frame==81)io.AddKeyEvent(ImGuiKey_Home,false);
        if(frame==84)app_.open("debugger");
        if(frame==76){const auto s=state();returned_=s.value("activeFunction","")=="Compute"&&s.value("nextNode","")=="return";}
    }
    int finish(int frame){
        if(!enabled()||frame!=90)return -1;const bool passed=entered_&&locals_&&nested_&&returned_&&inspector_;
        writeJson(report_,{{"passed",passed},{"input","own ImGui F10/F11/Shift+F11 keyboard events"},{"enteredFunction",entered_},{"isolatedLocals",locals_},{"nestedStack",nested_},{"stepOut",returned_},{"inspectorMatchesSession",inspector_},{"state",state()}});return passed?0:5;
    }
};
}
