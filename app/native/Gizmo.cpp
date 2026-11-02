#include "Editor.h"
#include <ImGuizmo.h>
#include <algorithm>
#include <cmath>
#include <cstring>
namespace cygame {
bool EditorApp::transformGizmo(const std::string& id,const ImVec2& origin,const ImVec2& size,const float* view,const float* projection){
    auto& draft=gizmos_[id];
    if(draft.active&&(draft.primary!=core.selection()||gizmoOperation_<0)){draft.ui.active=false;draft.ui.blocked=true;draft.active=false;draft.transforms=Json::array();}
    if(core.selection().empty()||gizmoOperation_<0)return false;
    auto entity=core.entity(core.selection());auto properties=entity.at("properties");bool locked=false;
    for(const auto& selected:core.selectedIds())locked|=core.entity(selected).value("editorLocked",false);
    float matrix[16]{};
    if(draft.active)std::copy(std::begin(draft.matrix),std::end(draft.matrix),matrix);
    else{
        float position[3],rotation[3],scale[3];for(int i=0;i<3;++i){position[i]=properties["position"][i];rotation[i]=properties["rotation"][i];scale[i]=properties["scale"][i];}
        ImGuizmo::RecomposeMatrixFromComponents(position,rotation,scale,matrix);
    }
    const auto operation=core.selectedIds().size()>1?ImGuizmo::TRANSLATE:gizmoOperation_==0?ImGuizmo::TRANSLATE:gizmoOperation_==1?ImGuizmo::ROTATE:ImGuizmo::SCALE;
    float value=operation==ImGuizmo::TRANSLATE?translationSnap_:operation==ImGuizmo::ROTATE?rotationSnap_:scaleSnap_,snap[3]={value,value,value};
    std::array<float,16> transform{},viewMatrix{},projectionMatrix{};std::copy(matrix,matrix+16,transform.begin());std::copy(view,view+16,viewMatrix.begin());std::copy(projection,projection+16,projectionMatrix.begin());
    cyui::Gizmo3DOptions options;options.operation=operation;options.mode=localTransform_?ImGuizmo::LOCAL:ImGuizmo::WORLD;options.orthographic=cameras_[id].mode!=0;options.enabled=!locked;options.revision=core.revision();options.snap=snapping_?snap:nullptr;
    const auto edit=cyui::gizmo3D(id.c_str(),draft.ui,transform,viewMatrix,projectionMatrix,{origin,{origin.x+size.x,origin.y+size.y}},options);
    if(edit.phase==cyui::EditPhase::cancel){draft.active=false;draft.transforms=Json::array();return true;}
    const bool usingNow=edit.phase==cyui::EditPhase::begin||edit.phase==cyui::EditPhase::update||draft.ui.active,over=ImGuizmo::IsOver();
    if(edit.phase!=cyui::EditPhase::none)std::copy(edit.value.begin(),edit.value.end(),matrix);
    if(usingNow&&!draft.active){
        draft.active=true;draft.revision=core.revision();draft.primary=core.selection();draft.original=Json::array();
        for(const auto& selected:core.selectedIds()){
            const auto& original=core.entity(selected)["properties"];
            draft.original.push_back({{"entityId",selected},{"position",original.at("position")},{"rotation",original.at("rotation")},{"scale",original.at("scale")}});
        }
    }
    if(usingNow){
        std::copy(matrix,matrix+16,draft.matrix);float position[3],rotation[3],scale[3];ImGuizmo::DecomposeMatrixToComponents(matrix,position,rotation,scale);
        draft.transforms=draft.original;
        const Json* primary=nullptr;for(const auto& t:draft.original)if(t["entityId"]==draft.primary)primary=&t;
        if(primary){
            for(auto& t:draft.transforms){
                if(draft.original.size()>1){for(int i=0;i<3;++i)t["position"][i]=t["position"][i].get<float>()+position[i]-primary->at("position")[i].get<float>();}
                else{t["position"]={position[0],position[1],position[2]};t["rotation"]={rotation[0],rotation[1],rotation[2]};t["scale"]={std::max(scale[0],.01f),std::max(scale[1],.01f),std::max(scale[2],.01f)};}
            }
        }
    }else if(draft.active&&edit.committed()){
        auto transforms=draft.transforms;draft.active=false;
        if(!transforms.empty())invoke("scene_transform_batch",{{"transforms",transforms},{"expectedRevision",draft.revision}});
        draft.transforms=Json::array();
    }
    return over||usingNow||draft.ui.blocked;
}
}
