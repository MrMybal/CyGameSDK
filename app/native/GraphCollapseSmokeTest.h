#pragma once
#include "Editor.h"
namespace cygame {
// Exercises selection, modal controls and keyboard gestures only in its owned host.
class GraphCollapseSmokeTest {
    EditorCore& core_;EditorApp& app_;fs::path report_;int frame_=0;bool passed_=true,pointerValid_=false;ImVec2 pointer_{};ImGuiID viewport_=0;Json checks_=Json::object(),before_,collapsed_,selected_=Json::array();size_t undo_=0;std::string add_,multiply_;
    struct Target{ImVec2 point;int frame;ImGuiID viewport;};std::map<std::string,Target> targets_;
    void check(const char* key,bool result){checks_[key]=result;passed_&=result;}
    void hover(const std::string& id){const auto it=targets_.find(id);if(it==targets_.end()||frame_-it->second.frame>4){check(("target/"+id).c_str(),false);return;}pointer_=it->second.point;viewport_=it->second.viewport;pointerValid_=true;}
    void seed(){
        auto add=[&](const char* type,Json position){return core_.execute("graph_node_add",{{"graphId","logic"},{"type",type},{"position",position}}).at("node").at("id").get<std::string>();};
        add_=add("math.add",{80,200});multiply_=add("math.multiply",{340,200});const auto value=add("constant.float",{80,380}),text=add("convert.float_string",{600,200});
        auto set=[&](const std::string& id,const char* pin,double v){core_.execute("graph_node_value",{{"graphId","logic"},{"nodeId",id},{"key",pin},{"value",v}});};set(value,"value",2);set(add_,"b",1);set(multiply_,"b",3);
        auto wire=[&](const std::string& a,const char* ap,const std::string& b,const char* bp){core_.execute("graph_link_add",{{"graphId","logic"},{"fromNode",a},{"fromPin",ap},{"toNode",b},{"toPin",bp}});};wire(value,"value",add_,"a");wire(add_,"value",multiply_,"a");wire(multiply_,"value",text,"value");wire(text,"value","print","text");
        before_=core_.graphs().at("logic");undo_=core_.execute("edit_history")["undo"].size();
    }
public:
    GraphCollapseSmokeTest(EditorCore& core,EditorApp& app,fs::path report):core_(core),app_(app),report_(std::move(report)){}
    bool enabled()const{return !report_.empty();}
    void probe(const std::string& id,ImVec2 a,ImVec2 b){targets_[id]={{(a.x+b.x)*.5f,(a.y+b.y)*.5f},frame_,ImGui::GetWindowViewport()->ID};}
    void graphProbe(const std::string& key,const Json& boxes,const Json& selection){if(key!="logic")return;selected_=selection;for(const auto& box:boxes){const std::string id=box.at("id");const auto& a=box.at("a");const auto& b=box.at("b");targets_["node/"+id]={{(a[0].get<float>()+b[0].get<float>())*.5f,a[1].get<float>()+cyui::dp(45)},frame_,ImGui::GetWindowViewport()->ID};}}
    void input(ImGuiIO& io,int frame){if(!enabled())return;frame_=frame;io.AddFocusEvent(true);
        if(frame==18)seed();const std::map<int,std::string> moves={{30,"node/"+add_},{38,"node/"+multiply_},{50,"collapse/open"},{60,"collapse/preview"},{78,"collapse/apply"},{152,"node/"+add_},{162,"node/"+multiply_},{174,"collapse/open"},{184,"collapse/kind/macro"},{194,"collapse/preview"},{216,"collapse/apply"}};if(moves.count(frame))hover(moves.at(frame));
        if(frame==40||frame==164)io.AddKeyEvent(ImGuiMod_Ctrl,true);
        if(std::set<int>{32,40,52,62,80,154,164,176,186,196,218}.count(frame))io.AddMouseButtonEvent(0,true);
        if(std::set<int>{34,42,54,64,82,156,166,178,188,198,220}.count(frame))io.AddMouseButtonEvent(0,false);if(frame==42||frame==166)io.AddKeyEvent(ImGuiMod_Ctrl,false);
        if(frame==46)check("selectionFromCanvas",selected_.size()==2);
        if(frame==74)check("functionPreviewReadOnly",core_.graphs().at("logic")==before_&&core_.execute("edit_history")["undo"].size()==undo_);
        if(frame==90){collapsed_=core_.graphs().at("logic");const auto functions=collapsed_.value("functions",Json::object());check("functionAppliedThroughUI",functions.contains("CollapsedGraph")&&functions["CollapsedGraph"].value("kind","")=="function");check("functionExecution9",core_.execute("graph_run",{{"graphId","logic"}})["messages"]==Json::array({"9"}));check("functionOneUndo",core_.execute("edit_history")["undo"].size()==undo_+1);bool selectedCall=false;for(const auto& n:collapsed_.at("nodes"))if(n.at("type")=="function.call"&&selected_.size()==1&&selected_[0]==n.at("id"))selectedCall=true;check("callerSelected",selectedCall);}
        if(std::set<int>{100,136,236}.count(frame)){io.AddKeyEvent(ImGuiMod_Ctrl,true);io.AddKeyEvent(ImGuiKey_Z,true);}if(std::set<int>{102,138,238}.count(frame)){io.AddKeyEvent(ImGuiKey_Z,false);io.AddKeyEvent(ImGuiMod_Ctrl,false);}
        if(frame==110)check("keyboardUndoFunction",core_.graphs().at("logic")==before_);if(frame==118){io.AddKeyEvent(ImGuiMod_Ctrl,true);io.AddKeyEvent(ImGuiKey_Y,true);}if(frame==120){io.AddKeyEvent(ImGuiKey_Y,false);io.AddKeyEvent(ImGuiMod_Ctrl,false);}if(frame==128)check("keyboardRedoFunction",core_.graphs().at("logic")==collapsed_);if(frame==146)check("secondUndoRestoresSource",core_.graphs().at("logic")==before_);
        if(frame==208)check("macroPreviewReadOnly",core_.graphs().at("logic")==before_&&core_.execute("edit_history")["undo"].size()==undo_);
        if(frame==228){const auto graph=core_.graphs().at("logic"),functions=graph.value("functions",Json::object());check("macroAppliedThroughUI",functions.contains("CollapsedGraph")&&functions["CollapsedGraph"].value("kind","")=="macro");check("macroExecution9",core_.execute("graph_run",{{"graphId","logic"}})["messages"]==Json::array({"9"}));check("macroOneUndo",core_.execute("edit_history")["undo"].size()==undo_+1);}if(frame==246)check("keyboardUndoMacro",core_.graphs().at("logic")==before_);
        if(pointerValid_){io.AddMouseViewportEvent(viewport_);io.AddMousePosEvent(pointer_.x,pointer_.y);}
    }
    int finish(int frame){if(!enabled()||frame!=260)return -1;writeJson(report_,{{"passed",passed_},{"input","native fixture setup; own ImGui canvas Ctrl-selection, function/macro modal, preview/apply, Ctrl+Z/Ctrl+Y"},{"checks",checks_}});return passed_?0:3;}
};
}
