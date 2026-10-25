#pragma once
#include "Editor.h"
#include <algorithm>
// Deterministic renderer smoke test: input enters our own ImGui context only.
// The Python runner always supplies an isolated content root.
namespace cygame {
class GizmoSmokeTest {
    EditorCore& core_;fs::path report_;ImVec2 projected_{},anchor_{};Json before_;bool havePoint_=false;
public:
    GizmoSmokeTest(EditorCore& core,const fs::path& report):core_(core),report_(report),before_(core.entity("hero").at("properties").at("position")){}
    bool enabled()const{return !report_.empty();}
    void projected(const std::string& id,ImVec2 point){if(id=="viewport"){projected_=point;havePoint_=true;}}
    void input(ImGuiIO& io,int frame){
        if(!enabled()||frame<25||frame>56)return;
        if(frame==25)anchor_=projected_;
        const float offset=frame<=35?0.f:frame>=47?80.f:float(frame-35)*80.f/12.f;
        io.AddMousePosEvent(anchor_.x+offset,anchor_.y);
        if(frame==34)io.AddMouseButtonEvent(0,true);
        if(frame==49)io.AddMouseButtonEvent(0,false);
    }
    int finish(int frame){
        if(!enabled()||frame!=58)return -1;
        const auto after=core_.entity("hero").at("properties").at("position");
        const auto history=core_.execute("edit_history");bool moved=false;
        for(int i=0;i<3;++i)moved|=std::abs(before_[i].get<double>()-after[i].get<double>())>.01;
        bool oneUndo=history.at("undo").size()==1;
        if(core_.canUndo())core_.execute("edit_undo");
        const bool restored=core_.entity("hero").at("properties").at("position")==before_;
        const bool passed=havePoint_&&moved&&oneUndo&&restored;
        writeJson(report_,{{"passed",passed},{"input","own ImGui mouse events"},{"point",{anchor_.x,anchor_.y}},{"before",before_},{"after",after},{"history",history},{"undoRestoresPosition",restored}});
        return passed?0:3;
    }
};
}
