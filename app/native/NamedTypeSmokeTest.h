#pragma once
#include "Editor.h"
namespace cygame {
// Own ImGui context only. QA roots are isolated and do not save UI preferences.
class NamedTypeSmokeTest {
    EditorCore& core_;fs::path report_;int frame_=0;bool passed_=true,havePointer_=false;ImVec2 pointer_{};ImGuiID pointerViewport_=0;size_t initialUndo_=0,fieldUndo_=0;Json checks_=Json::object();
    struct Target{ImVec2 p;int frame;ImGuiID viewport;};std::map<std::string,Target> targets_;
    void check(const char* name,bool ok){checks_[name]=ok;passed_&=ok;}
    void hover(const std::string& id){auto found=targets_.find(id);if(found==targets_.end()||frame_-found->second.frame>4){checks_["target/"+id]=false;passed_=false;return;}pointer_=found->second.p;pointerViewport_=found->second.viewport;havePointer_=true;}
public:
    NamedTypeSmokeTest(EditorCore& core,fs::path report):core_(core),report_(std::move(report)){}
    bool enabled()const{return !report_.empty();}
    void probe(const std::string& id,ImVec2 a,ImVec2 b){targets_[id]={{(a.x+b.x)*.5f,(a.y+b.y)*.5f},frame_,ImGui::GetWindowViewport()->ID};}
    void input(ImGuiIO& io,int frame){if(!enabled())return;frame_=frame;io.AddFocusEvent(true);if(havePointer_){io.AddMouseViewportEvent(pointerViewport_);io.AddMousePosEvent(pointer_.x,pointer_.y);}
        if(frame==18)initialUndo_=core_.execute("edit_history")["undo"].size();
        const std::map<int,std::string> hoverAt={{20,"types/new"},{32,"types/apply"},{48,"types/new"},{60,"types/kind"},{70,"types/kind/struct"},{82,"types/apply"},{110,"types/value/variable/logic#/Record/Value"}};
        if(hoverAt.count(frame))hover(hoverAt.at(frame));
        const std::set<int> downs={22,34,50,62,72,84,112},ups={24,36,52,64,74,86,114};if(frame==112)io.AddKeyEvent(ImGuiMod_Ctrl,true);if(downs.count(frame))io.AddMouseButtonEvent(0,true);if(ups.count(frame))io.AddMouseButtonEvent(0,false);
        if(frame==42){const auto types=core_.graphs().at("logic").value("types",Json::object());check("enumCreatedThroughUI",types.contains("enum.NewType"));check("oneDeclarationUndo",core_.execute("edit_history")["undo"].size()==initialUndo_+1);if(core_.canUndo())core_.execute("edit_undo");check("declarationUndo",!core_.graphs().at("logic").value("types",Json::object()).contains("enum.NewType"));if(core_.canRedo())core_.execute("edit_redo");check("declarationRedo",core_.graphs().at("logic").value("types",Json::object()).contains("enum.NewType"));}
        if(frame==94){const auto types=core_.graphs().at("logic").value("types",Json::object());check("structCreatedThroughUI",types.contains("struct.NewType"));if(types.contains("struct.NewType")){core_.execute("graph_variable_add",{{"graphId","logic"},{"name","Record"},{"type","struct.NewType"},{"default",{{"Value",0.}}}});fieldUndo_=core_.execute("edit_history")["undo"].size();}}
        if(frame==116)io.AddKeyEvent(ImGuiKey_A,true);if(frame==118){io.AddKeyEvent(ImGuiKey_A,false);io.AddKeyEvent(ImGuiMod_Ctrl,false);io.AddInputCharactersUTF8("9");}if(frame==120)io.AddKeyEvent(ImGuiKey_Enter,true);if(frame==122)io.AddKeyEvent(ImGuiKey_Enter,false);
        if(frame==136){const auto vars=core_.graphs().at("logic").value("variables",Json::object());check("recordDefaultChangedThroughUI",vars.contains("Record")&&vars["Record"]["default"]["Value"]==9);check("oneFieldUndo",core_.execute("edit_history")["undo"].size()==fieldUndo_+1);if(core_.canUndo())core_.execute("edit_undo");const auto undone=core_.graphs().at("logic").value("variables",Json::object());check("undoRestoresRecord",undone.contains("Record")&&undone["Record"]["default"]["Value"]==0);if(core_.canRedo())core_.execute("edit_redo");const auto redone=core_.graphs().at("logic").value("variables",Json::object());check("redoRestoresRecord",redone.contains("Record")&&redone["Record"]["default"]["Value"]==9);}
        if(havePointer_){io.AddMouseViewportEvent(pointerViewport_);io.AddMousePosEvent(pointer_.x,pointer_.y);}
    }
    int finish(int frame){if(!enabled()||frame!=144)return -1;writeJson(report_,{{"passed",passed_},{"input","own ImGui mouse/keyboard events; enum/struct declarations and structured property edit"},{"checks",checks_}});return passed_?0:3;}
};
}
