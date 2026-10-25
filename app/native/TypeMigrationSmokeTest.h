#pragma once
#include "Editor.h"
namespace cygame {
// Native setup; only this isolated host's own ImGui is used for gestures.
class TypeMigrationSmokeTest {
    EditorCore& core_;EditorApp& app_;fs::path report_;int frame_=0;bool passed_=true,pointerValid_=false;ImVec2 pointer_{};ImGuiID viewport_=0;Json checks_=Json::object(),before_,renamed_;size_t undo_=0;
    struct Target{ImVec2 point;int frame;ImGuiID viewport;};std::map<std::string,Target> targets_;
    void check(const char* name,bool ok){checks_[name]=ok;passed_&=ok;}
    void hover(const std::string& id){const auto found=targets_.find(id);if(found==targets_.end()||frame_-found->second.frame>4){check(("target/"+id).c_str(),false);return;}pointer_=found->second.point;viewport_=found->second.viewport;pointerValid_=true;}
    void seed(){
        core_.execute("graph_type_define",{{"graphId","logic"},{"typeId","enum.Team"},{"definition",{{"kind","enum"},{"members",{"Friendly","Hostile"}},{"default","Friendly"}}}});
        const Json fields={{"health",{{"type","float"},{"default",1.},{"hint","preserve"}}},{"team",{{"type","enum.Team"},{"default","Friendly"}}}};
        core_.execute("graph_type_define",{{"graphId","logic"},{"typeId","struct.Stats"},{"definition",{{"kind","struct"},{"fields",fields},{"metadata",{{"keep",true}}}}}});
        core_.execute("graph_variable_add",{{"graphId","logic"},{"name","Record"},{"type","struct.Stats"},{"default",{{"health",9.},{"team","Hostile"}}}});
        auto add=[&](const char* type,Json p){p["graphId"]="logic";p["type"]=type;p["position"]={100,180};return core_.execute("graph_node_add",p).at("node").at("id");};
        const auto get=add("typed.get",{{"typeRef","struct.Stats"},{"variableName","Record"}}),split=add("struct.break",{{"typeRef","struct.Stats"}}),text=add("convert.float_string",Json::object());
        auto wire=[&](const Json& a,const char* ap,const Json& b,const char* bp){core_.execute("graph_link_add",{{"graphId","logic"},{"fromNode",a},{"fromPin",ap},{"toNode",b},{"toPin",bp}});};wire(get,"value",split,"value");wire(split,"health",text,"value");wire(text,"value","print","text");
        before_=core_.graphs().at("logic");undo_=core_.execute("edit_history")["undo"].size();auto desired=fields;desired.erase("health");desired["hp"]={{"type","float"},{"default",1.}};desired["armor"]={{"type","float"},{"default",5.}};
        const auto plan=app_.previewTypeMigration({{"graphId","logic"},{"typeId","struct.Stats"},{"definition",{{"kind","struct"},{"fields",desired}}},{"renames",{{"health","hp"}}}});check("previewIsReadOnly",core_.graphs().at("logic")==before_);check("recordRenameHasNoLoss",plan.value("valid",false)&&!plan.value("requiresLossAcceptance",true));
    }
public:
    TypeMigrationSmokeTest(EditorCore& core,EditorApp& app,fs::path report):core_(core),app_(app),report_(std::move(report)){}
    bool enabled()const{return !report_.empty();}
    void probe(const std::string& id,ImVec2 a,ImVec2 b){targets_[id]={{(a.x+b.x)*.5f,(a.y+b.y)*.5f},frame_,ImGui::GetWindowViewport()->ID};}
    void input(ImGuiIO& io,int frame){if(!enabled())return;frame_=frame;io.AddFocusEvent(true);if(pointerValid_){io.AddMouseViewportEvent(viewport_);io.AddMousePosEvent(pointer_.x,pointer_.y);}
        if(frame==18)seed();const std::map<int,std::string> hoverAt={{50,"migration/apply"},{130,"migration/acceptLoss"},{146,"migration/apply"}};if(hoverAt.count(frame))hover(hoverAt.at(frame));
        if(std::set<int>{52,132,148}.count(frame))io.AddMouseButtonEvent(0,true);if(std::set<int>{54,134,150}.count(frame))io.AddMouseButtonEvent(0,false);
        if(frame==64){renamed_=core_.graphs().at("logic");check("recordMigrationAppliedThroughUI",renamed_["variables"]["Record"]["default"].value("hp",0.)==9.&&renamed_["variables"]["Record"]["default"].value("armor",0.)==5.);check("recordMigrationPreservesExecution",core_.execute("graph_run",{{"graphId","logic"}})["messages"]==Json::array({"9"}));check("oneRecordMigrationUndo",core_.execute("edit_history")["undo"].size()==undo_+1);}
        if(frame==70||frame==168){io.AddKeyEvent(ImGuiMod_Ctrl,true);io.AddKeyEvent(ImGuiKey_Z,true);}if(frame==72||frame==170){io.AddKeyEvent(ImGuiKey_Z,false);io.AddKeyEvent(ImGuiMod_Ctrl,false);}
        if(frame==80)check("keyboardUndoRestoresRecord",core_.graphs().at("logic")==before_);
        if(frame==88){io.AddKeyEvent(ImGuiMod_Ctrl,true);io.AddKeyEvent(ImGuiKey_Y,true);}if(frame==90){io.AddKeyEvent(ImGuiKey_Y,false);io.AddKeyEvent(ImGuiMod_Ctrl,false);}if(frame==98)check("keyboardRedoRestoresRecord",core_.graphs().at("logic")==renamed_);
        if(frame==110){undo_=core_.execute("edit_history")["undo"].size();const auto plan=app_.previewTypeMigration({{"graphId","logic"},{"typeId","enum.Team"},{"definition",{{"kind","enum"},{"members",{"Friendly"}},{"default","Friendly"}}}});check("enumLossRequiresAcceptance",plan.value("valid",false)&&plan.value("requiresLossAcceptance",false));check("enumLossPreviewIsReadOnly",core_.graphs().at("logic")==renamed_);}
        if(frame==160){check("enumLossAppliedThroughUI",core_.graphs().at("logic")["variables"]["Record"]["default"]["team"]=="Friendly");check("oneEnumMigrationUndo",core_.execute("edit_history")["undo"].size()==undo_+1);}if(frame==180)check("keyboardUndoRestoresEnum",core_.graphs().at("logic")==renamed_);
        if(pointerValid_){io.AddMouseViewportEvent(viewport_);io.AddMousePosEvent(pointer_.x,pointer_.y);}
    }
    int finish(int frame){if(!enabled()||frame!=190)return -1;writeJson(report_,{{"passed",passed_},{"input","native preview setup; own ImGui record/enum apply, loss acceptance and Ctrl+Z/Ctrl+Y"},{"checks",checks_}});return passed_?0:3;}
};
}
