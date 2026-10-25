#include "Editor.h"
#include "BuildClient.h"
#include <algorithm>
namespace cygame {
void EditorApp::modelMotionUI(const std::string& id,const Json& model){
    auto& v=modelViews_[id];const bool open=ImGui::BeginTabItem(tr("Motion tools"),nullptr,v.requestedTab=="Motion tools"?ImGuiTabItemFlags_SetSelected:0);if(!open)return;if(v.requestedTab=="Motion tools")v.requestedTab.clear();
    if(!v.motionInitialized){v.motionInitialized=true;if(!model["skins"].empty()){const auto& joints=model["skins"][0]["joints"];v.ikRoot=joints.front();v.ikTip=joints.back();v.motionRoot=v.ikRoot;}else if(!model["nodes"].empty()){v.ikTip=int(model["nodes"].size())-1;}}
    if(v.motionSource!=v.animation){v.motionSource=v.animation;v.trimStart=0;v.trimEnd=0;if(v.animation>=0&&!model["animations"][v.animation]["tracks"].empty())v.trimStart=model["animations"][v.animation]["tracks"][0]["times"].front();if(v.animation>=0)for(const auto& t:model["animations"][v.animation]["tracks"]){v.trimEnd=std::max(v.trimEnd,t["times"].back().get<float>());}}
    auto start=[&](const char* action,Json options){invoke("model_process",{{"modelId",id},{"action",action},{"options",options}});};
    auto nodeChoice=[&](const char* title,int& node){node=model["nodes"].empty()?0:std::clamp(node,0,int(model["nodes"].size())-1);cyui::property(tr(title));if(ImGui::BeginCombo((std::string("##")+title).c_str(),model["nodes"].empty()?tr("None"):model["nodes"][node].value("name","").c_str())){for(size_t i=0;i<model["nodes"].size();++i)if(ImGui::Selectable(label(model["nodes"][i].value("name",""),std::to_string(i)).c_str(),node==int(i)))node=int(i);ImGui::EndCombo();}};
    auto begin=[&](const char* key,const char* title){bool active=ImGui::BeginTabItem(tr(title),nullptr,v.motionTool==key?ImGuiTabItemFlags_SetSelected:0);if(active&&v.motionTool==key)v.motionTool.clear();return active;};
    if(ImGui::BeginTabBar("##motionSubtools")){
        if(begin("IK","IK")){
            ImGui::TextWrapped("%s",tr("Bake a fixed-root chain to a world-space target. Unit scale is required; no joint limits or foot contacts."));
            if(cyui::beginProperties("##ikInputs",cyui::dp(100))){nodeChoice("Chain root",v.ikRoot);nodeChoice("Chain tip",v.ikTip);cyui::property(tr("Target position"));ImGui::DragFloat3("##ikTarget",v.ikTarget,.01f);cyui::property(tr("Use pole"));ImGui::Checkbox("##usePole",&v.ikUsePole);if(v.ikUsePole){cyui::property(tr("Pole position"));ImGui::DragFloat3("##ikPole",v.ikPole,.01f);}cyui::property(tr("Bake FPS"));ImGui::SliderFloat("##ikFps",&v.motionFps,1,120,"%.0f");cyui::property(tr("Iterations"));ImGui::SliderInt("##ikIterations",&v.ikIterations,1,128);cyui::property(tr("Tolerance"));ImGui::DragFloat("##ikTolerance",&v.ikTolerance,.0001f,.000001f,.1f,"%.5f",ImGuiSliderFlags_AlwaysClamp);cyui::endProperties();}
            if(cyui::ghostButton(tr("Bake IK clip"))){Json p={{"animation",v.animation},{"root",v.ikRoot},{"tip",v.ikTip},{"target",{v.ikTarget[0],v.ikTarget[1],v.ikTarget[2]}},{"iterations",v.ikIterations},{"tolerance",v.ikTolerance},{"fps",v.motionFps}};if(v.ikUsePole)p["pole"]={v.ikPole[0],v.ikPole[1],v.ikPole[2]};start("ik",p);}probeInterface("motion/bakeIK");
            ImGui::EndTabItem();
        }
        if(begin("RootMotion","Root motion")){
            ImGui::TextWrapped("%s",tr("Extract root motion into a new in-place clip and a rigid displacement curve. The original clip is retained."));
            if(cyui::beginProperties("##rootMotionInputs",cyui::dp(100))){nodeChoice("Motion root",v.motionRoot);cyui::property(tr("Extract axes"));ImGui::Checkbox("X",&v.motionAxes[0]);ImGui::SameLine();ImGui::Checkbox("Y",&v.motionAxes[1]);ImGui::SameLine();ImGui::Checkbox("Z",&v.motionAxes[2]);cyui::property(tr("Extract rotation"));ImGui::Checkbox("##extractRotation",&v.motionRotation);cyui::property(tr("Bake FPS"));ImGui::SliderFloat("##motionFps",&v.motionFps,1,120,"%.0f");cyui::endProperties();}
            {cyui::ScopedDisabled disabled(v.animation<0);if(cyui::ghostButton(tr("Extract motion pair")))start("root_motion",{{"animation",v.animation},{"root",v.motionRoot},{"axes",{v.motionAxes[0],v.motionAxes[1],v.motionAxes[2]}},{"rotation",v.motionRotation},{"fps",v.motionFps}});probeInterface("motion/extract");}
            const auto records=model.value("extensions",Json::object()).value("org.cygame.editor",Json::object()).value("rootMotion",Json::array());
            if(records.empty())cyui::textDim("%s",tr("No motion pairs extracted yet."));
            for(const auto& r:records){const std::string rid=r.at("id");ImGui::PushID(rid.c_str());const int a=r.at("inPlace"),b=r.at("motion");cyui::sectionTitle(model["animations"][a].value("name","").c_str());
                if(cyui::ghostButton(tr("Preview in place"))){v.animation=a;v.time=0;v.playing=false;}ImGui::SameLine();if(cyui::ghostButton(tr("Preview displacement"))){v.animation=b;v.time=0;v.playing=false;}
                if(cyui::ghostButton(tr("Reconstruct motion clip")))start("root_motion",{{"mode","restore"},{"recordId",rid}});ImGui::PopID();
            }cyui::textDim("%s",tr("Changed motion tracks require a new extraction before reconstruction."));ImGui::EndTabItem();
        }
        if(begin("Clip","Clip tools")){
            ImGui::TextWrapped("%s",tr("Create a trimmed and resampled copy. Time starts at zero; the source clip is unchanged."));
            if(cyui::beginProperties("##clipTools",cyui::dp(100))){cyui::property(tr("Start time"));ImGui::InputFloat("##trimStart",&v.trimStart,0,0,"%.3f");cyui::property(tr("End time"));ImGui::InputFloat("##trimEnd",&v.trimEnd,0,0,"%.3f");cyui::property(tr("Bake FPS"));ImGui::SliderFloat("##trimFps",&v.motionFps,1,120,"%.0f");cyui::endProperties();}
            {cyui::ScopedDisabled disabled(v.animation<0);if(cyui::ghostButton(tr("Create trimmed clip")))start("clip_edit",{{"animation",v.animation},{"start",v.trimStart},{"end",v.trimEnd},{"fps",v.motionFps}});probeInterface("motion/trim");}ImGui::EndTabItem();
        }ImGui::EndTabBar();
    }
    if(v.processor){const auto state=v.processor->status();if(state.value("state","")=="Completed"&&state.contains("job")){const auto summary=state["job"].value("outputs",Json::object()).value("summary",Json::object());if(summary.contains("maximumError")){cyui::sectionTitle(tr("IK result"));ImGui::Text("%s %.6f",tr("Maximum target error"),summary["maximumError"].get<double>());ImGui::Text("%s %d",tr("Unreachable samples"),summary.value("unreachableSamples",0));ImGui::Text("%s %d",tr("Unconverged samples"),summary.value("unconvergedSamples",0));}if(summary.contains("distance"))ImGui::Text("%s %.4f",tr("Displacement distance"),summary["distance"].get<double>());}}
    ImGui::EndTabItem();
}
}
