#include "Editor.h"
#include <ImGuizmo.h>
#include <algorithm>
#include <cmath>
#include <cstring>
namespace cygame {
bool EditorApp::transformGizmo(const std::string& id,const ImVec2& origin,const ImVec2& size,const float* view,const float* projection){
    auto& draft=gizmos_[id];const auto& p=cyui::palette();
    ImGuizmo::PushID(id.c_str());ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());ImGuizmo::SetRect(origin.x,origin.y,size.x,size.y);ImGuizmo::SetOrthographic(cameras_[id].mode!=0);
    auto& style=ImGuizmo::GetStyle();for(auto& c:style.Colors)c=cyui::vec(p.textDim);
    const ImU32 axes[]={p.warning,p.success,p.info};
    for(int i=0;i<3;++i){style.Colors[ImGuizmo::DIRECTION_X+i]=cyui::vec(axes[i]);style.Colors[ImGuizmo::PLANE_X+i]=cyui::vec(cyui::alpha(axes[i],0.25f));}
    style.Colors[ImGuizmo::SELECTION]=cyui::vec(p.accent);style.Colors[ImGuizmo::INACTIVE]=cyui::vec(p.textFaint);
    style.Colors[ImGuizmo::TEXT]=cyui::vec(p.text);style.Colors[ImGuizmo::TEXT_SHADOW]=cyui::vec(p.bg0);
    style.Colors[ImGuizmo::ROTATION_USING_FILL]=cyui::vec(cyui::alpha(p.accent,0.15f));
    style.TranslationLineThickness=cyui::dp(2.5f);style.RotationLineThickness=cyui::dp(2.5f);style.ScaleLineThickness=cyui::dp(2.5f);
    style.TranslationLineArrowSize=cyui::dp(6);style.ScaleLineCircleSize=cyui::dp(5);style.CenterCircleSize=cyui::dp(6);
    bool blocked=draft.active;
    if(draft.active&&(draft.revision!=core.revision()||draft.primary!=core.selection()||ImGui::IsKeyPressed(ImGuiKey_Escape))){
        ImGuizmo::Enable(false);draft.active=false;draft.transforms=Json::array();
        if(draft.revision!=core.revision())cyui::notify(tr("Transform cancelled because the document changed."),cyui::Tone::warning);
    }
    if(core.selection().empty()||gizmoOperation_<0){ImGuizmo::PopID();return blocked;}
    auto entity=core.entity(core.selection());auto properties=entity.at("properties");bool locked=false;
    for(const auto& selected:core.selectedIds())locked|=core.entity(selected).value("editorLocked",false);
    ImGuizmo::Enable(!locked);
    float matrix[16]{};
    if(draft.active)std::copy(std::begin(draft.matrix),std::end(draft.matrix),matrix);
    else{
        float position[3],rotation[3],scale[3];for(int i=0;i<3;++i){position[i]=properties["position"][i];rotation[i]=properties["rotation"][i];scale[i]=properties["scale"][i];}
        ImGuizmo::RecomposeMatrixFromComponents(position,rotation,scale,matrix);
    }
    const auto operation=core.selectedIds().size()>1?ImGuizmo::TRANSLATE:gizmoOperation_==0?ImGuizmo::TRANSLATE:gizmoOperation_==1?ImGuizmo::ROTATE:ImGuizmo::SCALE;
    float value=operation==ImGuizmo::TRANSLATE?translationSnap_:operation==ImGuizmo::ROTATE?rotationSnap_:scaleSnap_,snap[3]={value,value,value};
    ImGuizmo::Manipulate(view,projection,operation,localTransform_?ImGuizmo::LOCAL:ImGuizmo::WORLD,matrix,nullptr,snapping_?snap:nullptr);
    const bool usingNow=ImGuizmo::IsUsing(),over=ImGuizmo::IsOver();
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
    }else if(draft.active){
        auto transforms=draft.transforms;draft.active=false;
        if(!transforms.empty())invoke("scene_transform_batch",{{"transforms",transforms},{"expectedRevision",draft.revision}});
        draft.transforms=Json::array();
    }
    ImGuizmo::PopID();return over||usingNow||blocked;
}
}
