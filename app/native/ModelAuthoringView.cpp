#include "Editor.h"
#include "BuildClient.h"
#include <DirectXMath.h>
#include <cmath>
namespace cygame {
void EditorApp::modelAuthoringUI(const std::string& id,const Json& model){
    auto& view=modelViews_[id];const auto meta=model.value("extensions",Json::object()).value("org.cygame.editor",Json::object());
    auto start=[&](const char* action,Json options,const std::string& source=""){Json p={{"modelId",id},{"action",action},{"options",options}};if(!source.empty())p["sourceModelId"]=source;invoke("model_process",p);};
    auto tab=[&](const char* name){const bool open=ImGui::BeginTabItem(tr(name),nullptr,view.requestedTab==name?ImGuiTabItemFlags_SetSelected:0);if(open&&view.requestedTab==name)view.requestedTab.clear();return open;};
    if(tab("LODs")){
        cyui::textDim("%s",tr("Static vertex clustering; boundaries and seams are protected."));
        const auto levels=meta.value("lods",Json::object());int maximum=0;for(const auto& entry:levels)maximum=std::max(maximum,int(entry.size()));
        if(cyui::beginProperties("##lodProperties",cyui::dp(110))){cyui::property(tr("Preview LOD"));ImGui::SliderInt("##previewLod",&view.lod,0,maximum);cyui::property(tr("Triangle ratio"));ImGui::SliderFloat("##lodRatio",&view.lodRatio,.05f,.95f,"%.2f");cyui::endProperties();}
        if(cyui::ghostButton(tr("Generate LOD")))start("lod",{{"mesh",view.mesh},{"ratio",view.lodRatio}});
        if(levels.contains(std::to_string(view.mesh)))for(size_t i=0;i<levels[std::to_string(view.mesh)].size();++i){const auto& level=levels[std::to_string(view.mesh)][i];size_t triangles=0;for(const auto& primitive:level["primitives"])triangles+=primitive["indices"].size()/3;ImGui::PushID(int(i));ImGui::Text("LOD %zu / %zu %s",i+1,triangles,tr("triangles"));cyui::textDim("%s %.5f",tr("Maximum vertex displacement"),level.value("maximumVertexDisplacement",0.));if(level.value("stale",false)){cyui::badge(tr("Stale"),cyui::Tone::warning);if(cyui::ghostButton(tr("Regenerate LOD")))start("lod",{{"mesh",view.mesh},{"ratio",level.value("requestedRatio",.5)},{"replace",i+1}});}if(cyui::ghostButton(tr("Remove LOD")))invoke("model_lod_remove",{{"modelId",id},{"mesh",view.mesh},{"lod",i+1}});ImGui::PopID();}
        if(cyui::ghostButton(tr("Export preview LOD")))invoke("model_export",{{"modelId",id},{"format","glb"},{"lod",view.lod}});
        ImGui::EndTabItem();
    }
    if(tab("Collision")){
        ImGui::Checkbox(tr("Show collision"),&view.collisions);const char* shapes[]={tr("Box"),tr("Sphere"),tr("Capsule")};ImGui::Combo("##collisionKind",&view.collisionKind,shapes,3);const char* ids[]={"box","sphere","capsule"};
        if(cyui::ghostButton(tr("Fit collision shape")))start("collision",{{"shape",ids[view.collisionKind]}});
        cyui::textDim("%s",tr("Asset reference space; engine physics cooking requires a provider."));
        for(const auto& shape:meta.value("collisions",Json::array())){
            const std::string collisionId=shape.at("id");ImGui::PushID(collisionId.c_str());if(cyui::beginSection(shape.value("name","Collision").c_str())){
                if(shape.value("needsReview",false)){cyui::callout(cyui::Tone::warning,tr("Review this collision fit after geometry changes."));if(cyui::ghostButton(tr("Confirm collision fit")))invoke("model_collision_set",{{"modelId",id},{"collisionId",collisionId},{"needsReview",false}});}
                Json renamed;if(editValue(id+"/"+collisionId+"/name",{{"type","string"},{"maxLength",128}},shape.at("name"),renamed))invoke("model_collision_set",{{"modelId",id},{"collisionId",collisionId},{"name",renamed}});
                bool enabled=shape.at("enabled");if(ImGui::Checkbox(tr("Enabled"),&enabled))invoke("model_collision_set",{{"modelId",id},{"collisionId",collisionId},{"enabled",enabled}});
                if(cyui::beginProperties("##collisionProperties",cyui::dp(110))){for(const auto* field:{"center","rotation","halfExtents","radius","halfHeight"})if(shape.contains(field)){cyui::property(tr(field));Json next,spec={{"type",shape[field].is_array()?"array":"number"}};if(!shape[field].is_array())spec["minimum"]=std::string(field)=="radius"?1e-5:0.;if(editValue(id+"/"+collisionId+"/"+field,spec,shape[field],next))invoke("model_collision_set",{{"modelId",id},{"collisionId",collisionId},{field,next}});}cyui::endProperties();}
                if(cyui::dangerButton(tr("Remove collision")))invoke("model_collision_remove",{{"modelId",id},{"collisionId",collisionId}});cyui::endSection();
            }ImGui::PopID();
        }ImGui::EndTabItem();
    }
    if(tab("Retarget")){
        cyui::textDim("%s",tr("Transfer a baked clip using reference-pose bone mapping. No IK solver."));
        const std::string sourceLabel=core.models().contains(view.retargetSource)?core.models().at(view.retargetSource).value("name",core.models().at(view.retargetSource).value("assetPath",view.retargetSource)):tr("Choose a source");
        if(ImGui::BeginCombo(tr("Source model"),sourceLabel.c_str())){for(const auto& [key,source]:core.models().items())if(key!=id&&!source.at("animations").empty()&&ImGui::Selectable(label(source.value("name",source.value("assetPath",key)),key).c_str(),view.retargetSource==key)){view.retargetSource=key;view.retargetAnimation=0;view.mapping=Json::object();for(size_t a=0;a<source["nodes"].size();++a){int match=-1;for(size_t b=0;b<model["nodes"].size();++b)if(source["nodes"][a]["name"]==model["nodes"][b]["name"]){if(match>=0){match=-1;break;}match=int(b);}if(match>=0)view.mapping[std::to_string(a)]=match;}}ImGui::EndCombo();}
        if(core.models().contains(view.retargetSource)){
            const auto& source=core.models().at(view.retargetSource);if(!source["animations"].empty()){
                view.retargetAnimation=std::clamp(view.retargetAnimation,0,int(source["animations"].size())-1);if(ImGui::BeginCombo(tr("Source clip"),source["animations"][view.retargetAnimation].value("name","Animation").c_str())){for(size_t i=0;i<source["animations"].size();++i)if(ImGui::Selectable(source["animations"][i].value("name","Animation").c_str(),int(i)==view.retargetAnimation))view.retargetAnimation=int(i);ImGui::EndCombo();}
                if(cyui::beginProperties("##retargetSettings",cyui::dp(120))){cyui::property(tr("Bake FPS"));ImGui::SliderFloat("##retargetFps",&view.retargetFps,1,120,"%.0f");cyui::property(tr("Translation scale"));ImGui::DragFloat("##retargetScale",&view.retargetScale,.01f,.001f,1000.f);cyui::property(tr("Translation mode"));const char* modes[]={tr("Bind pose"),tr("Root scaled"),tr("All scaled")};ImGui::Combo("##translationMode",&view.translationMode,modes,3);cyui::endProperties();}
                const char* modes[]={"bind","root_scaled","scaled"};if(cyui::ghostButton(tr("Bake retargeted clip")))start("retarget",{{"animation",view.retargetAnimation},{"mapping",view.mapping},{"fps",view.retargetFps},{"translationMode",modes[view.translationMode]},{"translationScale",view.retargetScale}},view.retargetSource);
                cyui::sectionTitle(tr("Bone mapping"));cyui::searchField("##boneFilter",view.mappingFilter,sizeof(view.mappingFilter),tr("Filter bones"),-1);
                for(size_t i=0;i<source["nodes"].size();++i){const auto key=std::to_string(i);const std::string name=source["nodes"][i].value("name",key);if(!matches(name,view.mappingFilter))continue;ImGui::PushID(int(i));int target=view.mapping.value(key,-1);if(cyui::beginProperties("##boneMap",cyui::dp(110))){cyui::property(name.c_str());const std::string preview=target>=0&&target<int(model["nodes"].size())?model["nodes"][target].value("name",""):tr("Unmapped");if(ImGui::BeginCombo("##targetBone",preview.c_str())){if(ImGui::Selectable(tr("Unmapped"),target<0))view.mapping.erase(key);for(size_t j=0;j<model["nodes"].size();++j)if(ImGui::Selectable(label(model["nodes"][j].value("name","Node"),std::to_string(j)).c_str(),target==int(j)))view.mapping[key]=j;ImGui::EndCombo();}cyui::endProperties();}ImGui::PopID();}
            }
        }ImGui::EndTabItem();
    }
}
void EditorApp::drawModelCollisions(const std::string& id,const Json& model,ImVec2 origin,ImVec2 size){
    const auto& view=modelViews_.at(id);if(!view.collisions)return;using namespace DirectX;const auto matrix=modelCameraMatrix(view.frame,view.camera,size);XMFLOAT4X4 m;std::copy(matrix.begin(),matrix.end(),&m._11);const auto projection=XMLoadFloat4x4(&m);auto* draw=ImGui::GetWindowDrawList();draw->PushClipRect(origin,{origin.x+size.x,origin.y+size.y},true);
    for(const auto& shape:model.value("extensions",Json::object()).value("org.cygame.editor",Json::object()).value("collisions",Json::array())){if(!shape.at("enabled").get<bool>())continue;const auto& center=shape["center"];const auto& q=shape["rotation"];const auto transform=XMMatrixRotationQuaternion(XMQuaternionNormalize(XMVectorSet(q[0],q[1],q[2],q[3])))*XMMatrixTranslation(center[0],center[1],center[2])*projection;
        auto line=[&](XMVECTOR a,XMVECTOR b){a=XMVector4Transform(a,transform);b=XMVector4Transform(b,transform);float wa=XMVectorGetW(a),wb=XMVectorGetW(b);if(wa<=0||wb<=0)return;draw->AddLine({origin.x+(XMVectorGetX(a)/wa+1)*size.x*.5f,origin.y+(1-XMVectorGetY(a)/wa)*size.y*.5f},{origin.x+(XMVectorGetX(b)/wb+1)*size.x*.5f,origin.y+(1-XMVectorGetY(b)/wb)*size.y*.5f},shape.value("needsReview",false)?cyui::palette().warning:cyui::palette().success,cyui::dp(1.5f));};
        if(shape["shape"]=="box"){const auto& e=shape["halfExtents"];for(int i=0;i<8;++i)for(int axis=0;axis<3;++axis)if(!(i&(1<<axis))){int j=i|(1<<axis);line(XMVectorSet((i&1?1:-1)*e[0].get<float>(),(i&2?1:-1)*e[1].get<float>(),(i&4?1:-1)*e[2].get<float>(),1),XMVectorSet((j&1?1:-1)*e[0].get<float>(),(j&2?1:-1)*e[1].get<float>(),(j&4?1:-1)*e[2].get<float>(),1));}}
        else{
            const float radius=shape.at("radius"),height=shape.value("halfHeight",0.f),pi=3.14159265359f;
            auto point=[](float x,float y,float z){return XMVectorSet(x,y,z,1);};
            if(shape["shape"]=="sphere"){
                for(int axis=0;axis<3;++axis)for(int i=0;i<48;++i){float a=i*2*pi/48,b=(i+1)*2*pi/48;float pa[3]{},pb[3]{};pa[(axis+1)%3]=std::cos(a)*radius;pa[(axis+2)%3]=std::sin(a)*radius;pb[(axis+1)%3]=std::cos(b)*radius;pb[(axis+2)%3]=std::sin(b)*radius;line(point(pa[0],pa[1],pa[2]),point(pb[0],pb[1],pb[2]));}
            }else{
                for(float sign:{-1.f,1.f}){
                    for(int i=0;i<48;++i){float a=i*2*pi/48,b=(i+1)*2*pi/48;line(point(std::cos(a)*radius,sign*height,std::sin(a)*radius),point(std::cos(b)*radius,sign*height,std::sin(b)*radius));}
                    for(int axis=0;axis<2;++axis)for(int i=0;i<24;++i){float a=i*pi/24,b=(i+1)*pi/24;line(point(axis?0:std::cos(a)*radius,sign*(height+std::sin(a)*radius),axis?std::cos(a)*radius:0),point(axis?0:std::cos(b)*radius,sign*(height+std::sin(b)*radius),axis?std::cos(b)*radius:0));}
                }
                for(int i=0;i<4;++i){float a=i*pi/2;line(point(std::cos(a)*radius,-height,std::sin(a)*radius),point(std::cos(a)*radius,height,std::sin(a)*radius));}
            }
        }
    }draw->PopClipRect();
}
}
