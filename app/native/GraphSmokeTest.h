#pragma once
#include "Editor.h"
#include <algorithm>
namespace cygame {
class GraphSmokeTest {
    EditorCore& core_;fs::path report_;Json boxes_=Json::array(),before_,selection_=Json::array();ImVec2 low_{},high_{},header_{};
public:
    GraphSmokeTest(EditorCore& core,fs::path report):core_(core),report_(std::move(report)),before_(core.graphs().at("logic")){}
    bool enabled()const{return !report_.empty();}
    void probe(const std::string& key,const Json& boxes,const Json& selection){if(key=="logic"){boxes_=boxes;selection_=selection;}}
    void input(ImGuiIO& io,int frame){
        if(!enabled()||frame<25||frame>64)return;
        if(frame==25&&!boxes_.empty()){
            low_={1e9f,1e9f};high_={-1e9f,-1e9f};
            for(const auto& box:boxes_){low_.x=std::min(low_.x,box["a"][0].get<float>()-12);low_.y=std::min(low_.y,box["a"][1].get<float>()-12);high_.x=std::max(high_.x,box["b"][0].get<float>()+12);high_.y=std::max(high_.y,box["b"][1].get<float>()+12);if(box["id"]=="begin")header_={box["a"][0].get<float>()+70,box["a"][1].get<float>()+12};}
        }
        ImVec2 pointer=frame<33?low_:frame<=43?high_:header_;
        if(frame>=49){pointer.x+=std::min(60.f,float(frame-48)*6);pointer.y+=std::min(30.f,float(frame-48)*3);}
        io.AddMousePosEvent(pointer.x,pointer.y);
        if(frame==31||frame==47)io.AddMouseButtonEvent(0,true);
        if(frame==41||frame==59)io.AddMouseButtonEvent(0,false);
    }
    int finish(int frame){
        if(!enabled()||frame!=68)return -1;
        const auto after=core_.graphs().at("logic"),history=core_.execute("edit_history");bool moved=true;
        for(size_t i=0;i<before_.at("nodes").size();++i){const auto& a=before_["nodes"][i];const auto& b=after["nodes"][i];moved&=std::abs(b["position"][0].get<float>()-a["position"][0].get<float>()-60)<.5f&&std::abs(b["position"][1].get<float>()-a["position"][1].get<float>()-30)<.5f;}
        const bool single=history.at("undo").size()==1;if(core_.canUndo())core_.execute("edit_undo");const bool restored=core_.graphs().at("logic")==before_;
        const bool passed=selection_.size()==2&&moved&&single&&restored;
        writeJson(report_,{{"passed",passed},{"selection",selection_},{"before",before_},{"after",after},{"history",history},{"restored",restored}});return passed?0:4;
    }
};
}
