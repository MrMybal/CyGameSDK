#pragma once
#include "Editor.h"
namespace cygame {
class BlueprintSmokeTest {
    EditorCore& core_;EditorApp& app_;fs::path report_;int frame_=0;bool passed_=true,hasPointer_=false;ImVec2 pointer_{};Json checks_=Json::object();size_t initialUndo_=0;
    struct Target{ImVec2 a,b;int frame=0;};std::map<std::string,Target> targets_;
    void check(const char* name,bool value){checks_[name]=value;passed_&=value;}
    void hover(const std::string& name){auto found=targets_.find(name);if(found==targets_.end()||frame_-found->second.frame>4){check("target/"+name,false);return;}const auto& t=found->second;pointer_={(t.a.x+t.b.x)*.5f,(t.a.y+t.b.y)*.5f};hasPointer_=true;}
    void check(const std::string& name,bool value){checks_[name]=value;passed_&=value;}
public:
    BlueprintSmokeTest(EditorCore& core,EditorApp& app,fs::path report):core_(core),app_(app),report_(std::move(report)){}
    bool enabled()const{return !report_.empty();}
    void probe(const std::string& id,ImVec2 a,ImVec2 b){targets_[id]={a,b,frame_};}
    void input(ImGuiIO& io,int frame){if(!enabled())return;frame_=frame;if(hasPointer_)io.AddMousePosEvent(pointer_.x,pointer_.y);
        if(frame==18)initialUndo_=core_.execute("edit_history")["undo"].size();
        const std::map<int,std::string> hoverAt={{22,"bp/newVariable"},{34,"bp/type"},{46,"bp/type/int"},{58,"bp/default"},{78,"bp/type"},{88,"bp/type/array.float"},{100,"bp/createVariable"},{118,"bp/newFunction"},{130,"bp/macro"},{138,"bp/createFunction"}};
        if(hoverAt.count(frame))hover(hoverAt.at(frame));
        const std::set<int> downs={24,36,48,60,80,90,102,120,132,140},ups={26,38,50,62,82,92,104,122,134,142};if(downs.count(frame))io.AddMouseButtonEvent(0,true);if(ups.count(frame))io.AddMouseButtonEvent(0,false);
        if(frame==60)io.AddKeyEvent(ImGuiMod_Ctrl,true);if(frame==64)io.AddKeyEvent(ImGuiKey_A,true);if(frame==66){io.AddKeyEvent(ImGuiKey_A,false);io.AddKeyEvent(ImGuiMod_Ctrl,false);io.AddInputCharactersUTF8("9");}if(frame==68)io.AddKeyEvent(ImGuiKey_Enter,true);if(frame==70)io.AddKeyEvent(ImGuiKey_Enter,false);
        if(frame==110){const auto vars=core_.graphs().at("logic").value("variables",Json::object());check("typedArrayCreated",vars.contains("NewVariable")&&vars["NewVariable"]["type"]=="array.float");check("defaultResetAfterTypeSwitch",vars.contains("NewVariable")&&vars["NewVariable"]["default"]==Json::array());check("oneUndoTransaction",core_.execute("edit_history")["undo"].size()==initialUndo_+1);if(core_.canUndo())core_.execute("edit_undo");check("undoRemovesDeclaration",!core_.graphs().at("logic").value("variables",Json::object()).contains("NewVariable"));if(core_.canRedo())core_.execute("edit_redo");check("redoRestoresDeclaration",core_.graphs().at("logic").at("variables").contains("NewVariable"));}
        if(frame==154){const auto functions=core_.graphs().at("logic").value("functions",Json::object());check("macroCreatedThroughUI",functions.contains("NewFunction")&&functions["NewFunction"].value("kind","")=="macro");bool opened=false;for(const auto& panel:core_.panels)opened|=panel.id=="fn:logic#NewFunction"&&panel.open;check("macroTabOpened",opened);}
        if(hasPointer_)io.AddMousePosEvent(pointer_.x,pointer_.y);
    }
    int finish(int frame){if(!enabled()||frame!=160)return -1;writeJson(report_,{{"passed",passed_},{"input","own ImGui mouse/keyboard events"},{"checks",checks_}});return passed_?0:3;}
};
}
