#pragma once
#include "Editor.h"
namespace cygame {
// Preview setup uses the native source-plugin seam; apply/ack/Undo use own ImGui.
class MigrationSmokeTest {
    EditorCore& core_;EditorApp& app_;fs::path report_;int frame_=0;bool passed_=true,pointerValid_=false;ImVec2 pointer_{};ImGuiID viewport_=0;Json checks_=Json::object(),before_,renamed_;size_t undo_=0;
    struct Target{ImVec2 point;int frame;ImGuiID viewport;};std::map<std::string,Target> targets_;
    void check(const char* name,bool ok){checks_[name]=ok;passed_&=ok;}
    void hover(const std::string& id){const auto found=targets_.find(id);if(found==targets_.end()||frame_-found->second.frame>4){check(("target/"+id).c_str(),false);return;}pointer_=found->second.point;viewport_=found->second.viewport;pointerValid_=true;}
    void seed(){
        core_.execute("graph_function_create",{{"graphId","logic"},{"name","Echo"}});
        const auto call=core_.execute("graph_node_add",{{"graphId","logic"},{"type","function.call"},{"functionRef","Echo"},{"position",{100,180}}}).at("node").at("id");core_.execute("graph_node_value",{{"graphId","logic"},{"nodeId",call},{"key","x"},{"value",9}});
        const auto text=core_.execute("graph_node_add",{{"graphId","logic"},{"type","convert.float_string"},{"position",{400,180}}}).at("node").at("id");core_.execute("graph_link_add",{{"graphId","logic"},{"fromNode",call},{"fromPin","result"},{"toNode",text},{"toPin","value"}});core_.execute("graph_link_add",{{"graphId","logic"},{"fromNode",text},{"fromPin","value"},{"toNode","print"},{"toPin","text"}});
        before_=core_.graphs().at("logic");undo_=core_.execute("edit_history")["undo"].size();const auto plan=app_.previewSignatureMigration({{"graphId","logic"},{"target","function"},{"name","Echo"},{"inputs",{{"data",{{"type","float"},{"default",0.}}}}},{"outputs",{{"answer",{{"type","float"},{"default",0.}}}}},{"renames",{{"inputs",{{"x","data"}}},{"outputs",{{"result","answer"}}}}}});
        check("previewIsReadOnly",core_.graphs().at("logic")==before_);check("renameHasNoLoss",plan.value("valid",false)&&!plan.value("requiresLossAcceptance",true));
    }
public:
    MigrationSmokeTest(EditorCore& core,EditorApp& app,fs::path report):core_(core),app_(app),report_(std::move(report)){}
    bool enabled()const{return !report_.empty();}
    void probe(const std::string& id,ImVec2 a,ImVec2 b){targets_[id]={{(a.x+b.x)*.5f,(a.y+b.y)*.5f},frame_,ImGui::GetWindowViewport()->ID};}
    void input(ImGuiIO& io,int frame){if(!enabled())return;frame_=frame;io.AddFocusEvent(true);if(pointerValid_){io.AddMouseViewportEvent(viewport_);io.AddMousePosEvent(pointer_.x,pointer_.y);}
        if(frame==18)seed();
        const std::map<int,std::string> hoverAt={{50,"migration/apply"},{130,"migration/acceptLoss"},{146,"migration/apply"}};if(hoverAt.count(frame))hover(hoverAt.at(frame));
        if(std::set<int>{52,132,148}.count(frame))io.AddMouseButtonEvent(0,true);if(std::set<int>{54,134,150}.count(frame))io.AddMouseButtonEvent(0,false);
        if(frame==64){renamed_=core_.graphs().at("logic");check("renameAppliedThroughUI",renamed_["functions"]["Echo"]["inputs"].contains("data"));check("renamePreservesExecution",core_.execute("graph_run",{{"graphId","logic"}})["messages"]==Json::array({"9"}));check("oneRenameUndo",core_.execute("edit_history")["undo"].size()==undo_+1);}
        if(frame==70||frame==168){io.AddKeyEvent(ImGuiMod_Ctrl,true);io.AddKeyEvent(ImGuiKey_Z,true);}if(frame==72||frame==170){io.AddKeyEvent(ImGuiKey_Z,false);io.AddKeyEvent(ImGuiMod_Ctrl,false);}
        if(frame==80)check("keyboardUndoRestoresGraph",core_.graphs().at("logic")==before_);
        if(frame==88){io.AddKeyEvent(ImGuiMod_Ctrl,true);io.AddKeyEvent(ImGuiKey_Y,true);}if(frame==90){io.AddKeyEvent(ImGuiKey_Y,false);io.AddKeyEvent(ImGuiMod_Ctrl,false);}
        if(frame==98)check("keyboardRedoRestoresMigration",core_.graphs().at("logic")==renamed_);
        if(frame==110){undo_=core_.execute("edit_history")["undo"].size();const auto plan=app_.previewSignatureMigration({{"graphId","logic"},{"target","function"},{"name","Echo"},{"inputs",Json::object()},{"outputs",{{"answer",{{"type","float"},{"default",0.}}}}}});check("lossPreviewRequiresAcceptance",plan.value("requiresLossAcceptance",false));check("lossPreviewIsReadOnly",core_.graphs().at("logic")==renamed_);}
        if(frame==160){check("acceptedLossAppliedThroughUI",core_.graphs().at("logic")["functions"]["Echo"]["inputs"].empty()&&core_.execute("graph_run",{{"graphId","logic"}})["messages"]==Json::array({"0"}));check("oneLossUndo",core_.execute("edit_history")["undo"].size()==undo_+1);}
        if(frame==180)check("keyboardUndoRestoresLoss",core_.graphs().at("logic")==renamed_);
        if(pointerValid_){io.AddMouseViewportEvent(viewport_);io.AddMousePosEvent(pointer_.x,pointer_.y);}
    }
    int finish(int frame){if(!enabled()||frame!=190)return -1;writeJson(report_,{{"passed",passed_},{"input","native preview setup; own ImGui apply/acceptance clicks and Ctrl+Z/Ctrl+Y"},{"checks",checks_}});return passed_?0:3;}
};
}
