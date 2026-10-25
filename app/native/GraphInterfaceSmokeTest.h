#pragma once
#include "Editor.h"
namespace cygame {
// QA events stay inside this isolated host's ImGui context.
class GraphInterfaceSmokeTest {
    EditorCore& core_;fs::path report_;int frame_=0;bool passed_=true,havePointer_=false;ImVec2 pointer_{};ImGuiID viewport_=0;size_t initialUndo_=0,editUndo_=0;Json checks_=Json::object();
    struct Target{ImVec2 p;int frame;ImGuiID viewport;};std::map<std::string,Target> targets_;
    void check(const char* name,bool ok){checks_[name]=ok;passed_&=ok;}
    void hover(const std::string& id){auto found=targets_.find(id);if(found==targets_.end()||frame_-found->second.frame>4){check(("target/"+id).c_str(),false);return;}pointer_=found->second.p;viewport_=found->second.viewport;havePointer_=true;}
public:
    GraphInterfaceSmokeTest(EditorCore& core,fs::path report):core_(core),report_(std::move(report)){}
    bool enabled()const{return !report_.empty();}
    void probe(const std::string& id,ImVec2 a,ImVec2 b){targets_[id]={{(a.x+b.x)*.5f,(a.y+b.y)*.5f},frame_,ImGui::GetWindowViewport()->ID};}
    void input(ImGuiIO& io,int frame){if(!enabled())return;frame_=frame;io.AddFocusEvent(true);if(havePointer_){io.AddMouseViewportEvent(viewport_);io.AddMousePosEvent(pointer_.x,pointer_.y);}
        if(frame==18)initialUndo_=core_.execute("edit_history")["undo"].size();
        const std::map<int,std::string> hoverAt={{20,"interfaces/new"},{50,"interfaces/apply"},{70,"interfaces/edit/NewInterface"},{90,"interfaces/port/Execute/outputs"},{108,"interfaces/apply"}};
        if(hoverAt.count(frame))hover(hoverAt.at(frame));
        if(std::set<int>{22,52,72,92,110}.count(frame))io.AddMouseButtonEvent(0,true);if(std::set<int>{24,54,74,94,112}.count(frame))io.AddMouseButtonEvent(0,false);
        if(frame==62){const auto library=core_.graphs().at("logic").value("interfaces",Json::object());check("interfaceCreatedThroughUI",library.contains("NewInterface"));check("oneDeclarationUndo",core_.execute("edit_history")["undo"].size()==initialUndo_+1);if(core_.canUndo())core_.execute("edit_undo");check("declarationUndo",!core_.graphs().at("logic").value("interfaces",Json::object()).contains("NewInterface"));if(core_.canRedo())core_.execute("edit_redo");check("declarationRedo",core_.graphs().at("logic").value("interfaces",Json::object()).contains("NewInterface"));editUndo_=core_.execute("edit_history")["undo"].size();}
        if(frame==124){auto outputs=[&]{return core_.graphs().at("logic").value("interfaces",Json::object()).value("NewInterface",Json::object()).value("methods",Json::object()).value("Execute",Json::object()).value("outputs",Json::object());};check("typedPortAddedThroughUI",outputs().contains("NewPort")&&outputs()["NewPort"]==Json{{"type","float"},{"default",0.}});check("oneSignatureUndo",core_.execute("edit_history")["undo"].size()==editUndo_+1);if(core_.canUndo())core_.execute("edit_undo");check("signatureUndo",!outputs().contains("NewPort"));if(core_.canRedo())core_.execute("edit_redo");check("signatureRedo",outputs().contains("NewPort"));}
        if(havePointer_){io.AddMouseViewportEvent(viewport_);io.AddMousePosEvent(pointer_.x,pointer_.y);}
    }
    int finish(int frame){if(!enabled()||frame!=136)return -1;writeJson(report_,{{"passed",passed_},{"input","own ImGui pointer events; interface declaration, typed signature edit and Undo/Redo"},{"checks",checks_}});return passed_?0:3;}
};
}
