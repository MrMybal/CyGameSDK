#pragma once
#include "Editor.h"
#include "EditorTheme.h"
#include <imgui_internal.h>
#include <map>
namespace cygame {
// Input is injected into this isolated test host's own ImGui context only.
class InterfaceSmokeTest {
    EditorCore& core_;EditorApp& app_;fs::path report_;
    struct Target {ImVec2 point{};int frame=0;};
    std::map<std::string,Target> targets_;Json checks_=Json::object();int frame_=0;bool passed_=true,havePointer_=false;ImVec2 pointer_{};
    void check(const std::string& name,bool value){checks_[name]=value;passed_&=value;}
    bool opened(const std::string& id)const{for(const auto& p:core_.panels)if(p.id==id)return p.open;return false;}
    void click(ImGuiIO& io,const std::string& id){
        const auto found=targets_.find(id);if(found==targets_.end()||frame_-found->second.frame>4){check("target/"+id,false);return;}
        pointer_=found->second.point;havePointer_=true;io.AddMousePosEvent(pointer_.x,pointer_.y);io.AddMouseButtonEvent(0,true);
    }
public:
    InterfaceSmokeTest(EditorCore& core,EditorApp& app,const fs::path& report):core_(core),app_(app),report_(report){}
    bool enabled()const{return !report_.empty();}
    void probe(const std::string& id,ImVec2 a,ImVec2 b){targets_[id]={{(a.x+b.x)*.5f,(a.y+b.y)*.5f},frame_};}
    void input(ImGuiIO& io,int frame){
        if(!enabled())return;frame_=frame;
        // Release each click two frames later; ImGui receives real down/up transitions.
        for(int up:{22,30,42,50,62,70,88,96,104,116,124,138,168})if(frame==up)io.AddMouseButtonEvent(0,false);
        switch(frame){
        case 8:check("freshEnglish",std::string(app_.tr("Options"))=="Options");check("freshPalette",cyui::palette().bg1==cyui::rgb(0x262626));check("freshLevelPanels",opened("viewport")&&!opened("logic")&&!opened("material")&&!opened("diagnostics"));break;
        case 10:app_.open("options");break;
        case 20:case 40:click(io,"theme/combo");break;
        case 28:click(io,"theme/light-grey");break;
        case 36:for(const auto& theme:cyui::builtInThemes())if(theme.key=="light-grey")check("switchLightPalette",cyui::palette().bg1==theme.palette.bg1);break;
        case 48:click(io,"theme/unreal-dark");break;
        case 56:check("restoreCharcoalPalette",cyui::palette().bg1==cyui::rgb(0x262626));break;
        case 60:case 94:click(io,"menu/Window");break;
        case 66:pointer_=targets_["filter/Window"].point;havePointer_=true;break;
        case 68:click(io,"filter/Window");break;
        case 74:io.AddInputCharactersUTF8("Outliner");break;
        case 82:check("localizedMenuFilter",targets_["panel/outliner"].frame>=80&&targets_["panel/material"].frame<74);break;
        case 86:case 102:click(io,"panel/outliner");break;
        case 94+16:check("windowMenuReopens",opened("outliner"));break;
        case 114:check("windowMenuClosedEarlier",checks_.value("windowMenuCloses",false));click(io,"menu/File");break;
        case 120:pointer_=targets_["filter/File"].point;havePointer_=true;break;
        case 122:check("fileFilterIndependent",targets_["action/Exit"].frame>=120);click(io,"filter/File");break;
        case 128:io.AddInputCharactersUTF8("zzzzunmatched");break;
        case 136:check("fileFilterIncludesExit",targets_["action/Exit"].frame<128);click(io,"menu/Edit");break;
        case 146:check("editFilterIndependent",targets_["action/Undo"].frame>=140);break;
        case 148:io.AddKeyEvent(ImGuiKey_Escape,true);break;
        case 150:io.AddKeyEvent(ImGuiKey_Escape,false);break;
        case 156:io.AddKeyEvent(ImGuiMod_Ctrl,true);io.AddKeyEvent(ImGuiKey_Space,true);break;
        case 158:io.AddKeyEvent(ImGuiKey_Space,false);io.AddKeyEvent(ImGuiMod_Ctrl,false);break;
        case 164:check("drawerKeyboard",targets_["drawer/dock"].frame>=160);break;
        case 166:click(io,"drawer/dock");break;
        case 174:{auto* drawer=ImGui::FindWindowByName("###contentDrawer");check("drawerDockAction",opened("content")&&(!drawer||!drawer->WasActive));break;}
        case 178:core_.execute("viewport_new");break;
        case 188:{auto* a=ImGui::FindWindowByName("###viewport");auto* b=ImGui::FindWindowByName("###viewport.2");check("newViewportDocked",a&&b&&a->DockId&&a->DockId==b->DockId);break;}
        case 194:core_.execute("window_reset_layout");break;
        case 202:check("resetKeepsWorkspace",opened("viewport")&&opened("outliner")&&!opened("logic")&&!opened("material")&&!opened("compiler")&&!opened("interface")&&!opened("viewport.2"));break;
        }
        if(frame==94)check("windowMenuCloses",!opened("outliner"));
        if(havePointer_)io.AddMousePosEvent(pointer_.x,pointer_.y);
    }
    int finish(int frame){
        if(!enabled()||frame!=204)return -1;
        writeJson(report_,{{"passed",passed_},{"input","own ImGui mouse and keyboard events"},{"checks",checks_}});return passed_?0:3;
    }
};
}
