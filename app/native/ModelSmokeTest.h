#pragma once
#include "Editor.h"
namespace cygame {
class ModelSmokeTest {
    EditorCore& core_;EditorApp& app_;fs::path report_;std::string id_;Json initial_,checks_=Json::object();size_t undo_=0;int frame_=0;bool passed_=true,havePointer_=false;ImVec2 pointer_{},anchor_{};
    struct Target{ImVec2 a,b;int frame=0;};std::map<std::string,Target> targets_;
    void check(const char* key,bool value){checks_[key]=value;passed_&=value;}
    void hover(const std::string& key,bool firstComponent=false){const auto found=targets_.find(key);if(found==targets_.end()||frame_-found->second.frame>4){check("targetAvailable",false);return;}const auto& t=found->second;pointer_={t.a.x+(t.b.x-t.a.x)*(firstComponent?1.f/6.f:.5f),(t.a.y+t.b.y)*.5f};havePointer_=true;}
public:
    ModelSmokeTest(EditorCore& core,EditorApp& app,fs::path report):core_(core),app_(app),report_(std::move(report)){if(enabled()&&!core.models().empty()){id_=core.models().begin().key();initial_=core.models().at(id_)["meshes"][0]["primitives"][0]["positions"][0];}}
    bool enabled()const{return !report_.empty();}
    void probe(const std::string& key,ImVec2 a,ImVec2 b){targets_[key]={a,b,frame_};}
    void input(ImGuiIO& io,int frame){if(!enabled())return;frame_=frame;if(id_.empty())return;if(havePointer_)io.AddMousePosEvent(pointer_.x,pointer_.y);
        if(frame==18)undo_=core_.execute("edit_history")["undo"].size();
        if(frame==22)hover("model/tab/Mesh");if(frame==24)io.AddMouseButtonEvent(0,true);if(frame==26)io.AddMouseButtonEvent(0,false);
        if(frame==34){hover("model/vertex",true);anchor_=pointer_;}
        if(frame==38)io.AddMouseButtonEvent(0,true);if(frame>=39&&frame<=49)pointer_.x=anchor_.x+float(frame-38)*4;
        if(frame==51)io.AddMouseButtonEvent(0,false);
        if(frame==57){const auto after=core_.models().at(id_)["meshes"][0]["primitives"][0]["positions"][0];check("actualVertexDrag",after!=initial_);check("singleUndoTransaction",core_.execute("edit_history")["undo"].size()==undo_+1);if(core_.canUndo())core_.execute("edit_undo");check("undoRestoresVertex",core_.models().at(id_)["meshes"][0]["primitives"][0]["positions"][0]==initial_);if(core_.canRedo())core_.execute("edit_redo");check("redoRestoresEdit",core_.models().at(id_)["meshes"][0]["primitives"][0]["positions"][0]==after);if(core_.canUndo())core_.execute("edit_undo");}
        if(frame==66)hover("model/tab/Animation");if(frame==68)io.AddMouseButtonEvent(0,true);if(frame==70)io.AddMouseButtonEvent(0,false);
        if(frame==80)hover("model/play");if(frame==82)io.AddMouseButtonEvent(0,true);if(frame==84)io.AddMouseButtonEvent(0,false);
        if(frame==95){auto state=core_.execute("model_view_settings",{{"modelId",id_}});check("actualClipPlayback",state.at("playing").get<bool>()&&state.at("time").get<double>()>.03);core_.execute("model_view_settings",{{"modelId",id_},{"playing",false},{"time",.5}});}
        if(havePointer_)io.AddMousePosEvent(pointer_.x,pointer_.y);
    }
    int finish(int frame){if(!enabled()||frame!=110)return -1;if(id_.empty())check("loadedModel",false);writeJson(report_,{{"passed",passed_},{"input","own ImGui mouse events"},{"checks",checks_}});return passed_?0:3;}
};
}
