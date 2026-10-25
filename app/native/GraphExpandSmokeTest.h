#pragma once
#include "Editor.h"
namespace cygame {
// All input targets belong to this isolated host; no system input injection.
class GraphExpandSmokeTest {
    EditorCore& core_;EditorApp& app_;fs::path report_;int frame_=0;bool passed_=true,pointerValid_=false;ImVec2 pointer_{};ImGuiID viewport_=0;Json checks_=Json::object(),before_,expanded_,selected_=Json::array(),call_;size_t undo_=0;std::string add_,multiply_,callId_;
    struct Target{ImVec2 point;int frame;ImGuiID viewport;};std::map<std::string,Target> targets_;
    void check(const char* key,bool result){checks_[key]=result;passed_&=result;}
    void hover(const std::string& id){const auto it=targets_.find(id);if(it==targets_.end()||frame_-it->second.frame>4){check(("target/"+id).c_str(),false);return;}pointer_=it->second.point;viewport_=it->second.viewport;pointerValid_=true;}
    void prepare(const char* kind){const Json r={{"graphId","logic"},{"nodeIds",{add_,multiply_}},{"name","Calculate"},{"kind",kind}};auto request=r;request["planHash"]=core_.execute("graph_collapse_plan",r).at("planHash");callId_=core_.execute("graph_collapse_apply",request).at("callNodeId");before_=core_.graphs().at("logic");for(const auto& n:before_.at("nodes"))if(n.at("id")==callId_)call_=n;undo_=core_.execute("edit_history")["undo"].size();}
    void seed(){
        auto add=[&](const char* type,Json position){return core_.execute("graph_node_add",{{"graphId","logic"},{"type",type},{"position",position}}).at("node").at("id").get<std::string>();};
        add_=add("math.add",{80,200});multiply_=add("math.multiply",{340,200});const auto value=add("constant.float",{80,380}),text=add("convert.float_string",{600,200});
        auto set=[&](const std::string& id,const char* pin,double v){core_.execute("graph_node_value",{{"graphId","logic"},{"nodeId",id},{"key",pin},{"value",v}});};set(value,"value",2);set(add_,"b",1);set(multiply_,"b",3);
        auto wire=[&](const std::string& a,const char* ap,const std::string& b,const char* bp){core_.execute("graph_link_add",{{"graphId","logic"},{"fromNode",a},{"fromPin",ap},{"toNode",b},{"toPin",bp}});};wire(value,"value",add_,"a");wire(add_,"value",multiply_,"a");wire(multiply_,"value",text,"value");wire(text,"value","print","text");
        prepare("function");
    }
public:
    GraphExpandSmokeTest(EditorCore& core,EditorApp& app,fs::path report):core_(core),app_(app),report_(std::move(report)){}
    bool enabled()const{return !report_.empty();}
    void probe(const std::string& id,ImVec2 a,ImVec2 b){targets_[id]={{(a.x+b.x)*.5f,(a.y+b.y)*.5f},frame_,ImGui::GetWindowViewport()->ID};}
    void graphProbe(const std::string& key,const Json& boxes,const Json& selection){if(key!="logic")return;selected_=selection;for(const auto& box:boxes){const std::string id=box.at("id");const auto& a=box.at("a");const auto& b=box.at("b");targets_["node/"+id]={{(a[0].get<float>()+b[0].get<float>())*.5f,a[1].get<float>()+cyui::dp(45)},frame_,ImGui::GetWindowViewport()->ID};}}
    void input(ImGuiIO& io,int frame){if(!enabled())return;frame_=frame;io.AddFocusEvent(true);
        if(frame==18)seed();if(frame==148){core_.execute("edit_undo");prepare("macro");}
        const std::map<int,std::string> moves={{30,"node/"+callId_},{50,"expand/open"},{60,"expand/preview"},{78,"expand/apply"},{152,"node/"+callId_},{174,"expand/open"},{194,"expand/preview"},{216,"expand/apply"}};if(moves.count(frame))hover(moves.at(frame));
        if(std::set<int>{32,52,62,80,154,176,196,218}.count(frame))io.AddMouseButtonEvent(0,true);if(std::set<int>{34,54,64,82,156,178,198,220}.count(frame))io.AddMouseButtonEvent(0,false);
        if(frame==46)check("callSelectedFromCanvas",selected_==Json::array({callId_}));
        if(frame==74)check("functionPreviewReadOnly",core_.graphs().at("logic")==before_&&core_.execute("edit_history")["undo"].size()==undo_);
        if(frame==90){expanded_=core_.graphs().at("logic");bool removed=true;for(const auto& n:expanded_.at("nodes"))removed&=n.at("id")!=callId_;check("functionAppliedThroughUI",removed&&expanded_.at("nodes").size()>before_.at("nodes").size());check("definitionRetained",expanded_.at("functions")==before_.at("functions"));check("functionExecution9",core_.execute("graph_run",{{"graphId","logic"}})["messages"]==Json::array({"9"}));check("functionOneUndo",core_.execute("edit_history")["undo"].size()==undo_+1);check("expandedNodesSelected",selected_.size()==4);bool kept=false;for(const auto& c:expanded_.value("comments",Json::array()))if(c.contains("expandedCall"))kept|=c.at("expandedCall").at("node")==call_;check("originalCallProvenance",kept);}
        if(std::set<int>{100,136,236}.count(frame)){io.AddKeyEvent(ImGuiMod_Ctrl,true);io.AddKeyEvent(ImGuiKey_Z,true);}if(std::set<int>{102,138,238}.count(frame)){io.AddKeyEvent(ImGuiKey_Z,false);io.AddKeyEvent(ImGuiMod_Ctrl,false);}
        if(frame==110)check("keyboardUndoFunction",core_.graphs().at("logic")==before_);if(frame==118){io.AddKeyEvent(ImGuiMod_Ctrl,true);io.AddKeyEvent(ImGuiKey_Y,true);}if(frame==120){io.AddKeyEvent(ImGuiKey_Y,false);io.AddKeyEvent(ImGuiMod_Ctrl,false);}if(frame==128)check("keyboardRedoFunction",core_.graphs().at("logic")==expanded_);if(frame==146)check("secondUndoRestoresSource",core_.graphs().at("logic")==before_);
        if(frame==208)check("macroPreviewReadOnly",core_.graphs().at("logic")==before_&&core_.execute("edit_history")["undo"].size()==undo_);
        if(frame==228){bool removed=true;for(const auto& n:core_.graphs().at("logic").at("nodes"))removed&=n.at("id")!=callId_;check("macroAppliedThroughUI",removed&&core_.graphs().at("logic").at("functions")==before_.at("functions"));check("macroExecution9",core_.execute("graph_run",{{"graphId","logic"}})["messages"]==Json::array({"9"}));check("macroOneUndo",core_.execute("edit_history")["undo"].size()==undo_+1);}if(frame==246)check("keyboardUndoMacro",core_.graphs().at("logic")==before_);
        if(pointerValid_){io.AddMouseViewportEvent(viewport_);io.AddMousePosEvent(pointer_.x,pointer_.y);}
    }
    int finish(int frame){if(!enabled()||frame!=260)return -1;writeJson(report_,{{"passed",passed_},{"input","native fixture setup; own ImGui canvas call selection, function/macro expansion modal, preview/apply, Ctrl+Z/Ctrl+Y"},{"checks",checks_}});return passed_?0:3;}
};
}
