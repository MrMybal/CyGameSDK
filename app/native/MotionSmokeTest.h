#pragma once
#include "Editor.h"
namespace cygame {
// Confined input injection into an isolated QA host, never global desktop input.
class MotionSmokeTest {
    EditorCore& core_;fs::path report_;int frame_=0,phase_=0,baseFrame_=20;bool clicked_=false,done_=false,passed_=true;std::string model_;Json initial_,checks_=Json::object();size_t undo_=0;ImVec2 pointer_{};bool hasPointer_=false;
    struct Target{ImVec2 p;int frame;};std::map<std::string,Target> targets_;
    bool clientsClosed()const{const auto runs=core_.root()/"data/build-runs";if(!fs::is_directory(runs))return false;for(const auto& run:fs::directory_iterator(runs)){if(!run.is_directory())continue;try{if(!readJson(run.path()/"state.json").value("clientClosed",false))return false;}catch(...){return false;}}return true;}
    void check(const char* name,bool ok){checks_[name]=ok;passed_&=ok;}
public:
    MotionSmokeTest(EditorCore& core,fs::path report):core_(core),report_(std::move(report)){}
    bool enabled()const{return !report_.empty();}
    void probe(const std::string& key,ImVec2 a,ImVec2 b){targets_[key]={{(a.x+b.x)*.5f,(a.y+b.y)*.5f},frame_};}
    void input(ImGuiIO& io,int frame){if(!enabled()||done_)return;frame_=frame;if(hasPointer_)io.AddMousePosEvent(pointer_.x,pointer_.y);
        if(frame==20){if(core_.models().empty()){check("modelLoaded",false);done_=true;return;}model_=core_.models().begin().key();initial_=core_.models().at(model_)["animations"][0];undo_=core_.execute("edit_history")["undo"].size();core_.execute("model_view_settings",{{"modelId",model_},{"tab","Motion tools"},{"motionTool","IK"},{"animation",0},{"ikRoot",1},{"ikTip",3},{"motionRoot",1},{"ikTarget",{1,1,0}}});}
        if(model_.empty())return;const char* probes[]={"motion/bakeIK","motion/extract","motion/trim"};int age=frame-baseFrame_;
        if(age==10){auto found=targets_.find(probes[phase_]);if(found==targets_.end()||frame-found->second.frame>4){check("buttonFound",false);done_=true;return;}pointer_=found->second.p;hasPointer_=true;io.AddMousePosEvent(pointer_.x,pointer_.y);}
        if(age==12){io.AddMouseButtonEvent(0,true);clicked_=true;}if(age==14)io.AddMouseButtonEvent(0,false);
        const size_t expected[]={2,4,5};if(clicked_&&age>18&&clientsClosed()&&core_.models().at(model_)["animations"].size()==expected[phase_]){
            check(phase_==0?"ikCreatedThroughUI":phase_==1?"motionPairCreatedThroughUI":"trimCreatedThroughUI",true);
            if(phase_<2){++phase_;baseFrame_=frame;clicked_=false;core_.execute("model_view_settings",{{"modelId",model_},{"animation",0},{"motionTool",phase_==1?"RootMotion":"Clip"}});}
            else{check("sourceClipUnchanged",core_.models().at(model_)["animations"][0]==initial_);check("threeUndoTransactions",core_.execute("edit_history")["undo"].size()==undo_+3);core_.execute("edit_undo");check("undoRemovesOnlyTrim",core_.models().at(model_)["animations"].size()==4);core_.execute("edit_redo");check("redoRestoresTrim",core_.models().at(model_)["animations"].size()==5);done_=true;}
        }
        if(frame>1000){check("jobsCompletedBeforeTimeout",false);done_=true;}
    }
    int finish(){if(!enabled()||!done_)return -1;writeJson(report_,{{"passed",passed_},{"input","own ImGui mouse events; real desktop Core worker jobs"},{"checks",checks_}});return passed_?0:3;}
};
}
