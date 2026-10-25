#include "Editor.h"
#include "BuildClient.h"
#include <DirectXMath.h>
#include <algorithm>
#include <cmath>
namespace cygame {
namespace {const char* fieldTitle(const char* field){static const std::map<std::string,const char*> titles={{"translation","Translation"},{"rotation","Rotation"},{"scale","Scale"},{"positions","Position"},{"normals","Normal"},{"uv","UV"},{"joints","Joint indices"},{"weights","Skin weights"}};auto found=titles.find(field);return found==titles.end()?field:found->second;}}

Json EditorApp::openModel(const std::string& path){
    auto info=core.execute("model_open",{{"path",path}});return openLoadedModel(info.at("modelId"));
}
Json EditorApp::openLoadedModel(const std::string& id){
    core.models().at(id);const std::string panel="model:"+id;
    if(modelViews_.find(id)==modelViews_.end())modelViews_[id].animation=core.models().at(id)["animations"].empty()?-1:0;
    for(auto& p:core.panels)if(p.id==panel){p.open=true;p.focus=true;return {{"panelId",panel}};}
    core.panels.push_back({panel,core.models().at(id).value("name","3D Asset"),"core",[this,id]{modelEditor(id);},true,true});return {{"panelId",panel}};
}
void EditorApp::modelLibrary(){
    cyui::heading(tr("3D Assets"),2);ImGui::TextWrapped("%s",tr("Import a self-contained GLB or CyPack to edit meshes, skeletons and animation clips."));
    if(cyui::primaryButton(tr("Asset Import")))open("imports");
    if(core.models().empty())cyui::emptyState(tr("No 3D documents loaded"),tr("Open a .cy3d asset from the Content Browser."),cyui::icon::folder);
    const auto loaded=core.models();for(const auto& [id,model]:loaded.items()){
        ImGui::PushID(id.c_str());if(cyui::ghostButton(model.value("name",id).c_str()))openLoadedModel(id);
        ImGui::SameLine();if(cyui::ghostButton(tr("Close document")))invoke("model_close",{{"modelId",id}});ImGui::PopID();
    }
}
void EditorApp::modelEditor(const std::string& id){
    if(!core.models().contains(id)){cyui::emptyState(tr("3D document closed"),tr("Open a .cy3d asset from the Content Browser."),cyui::icon::folder);return;}
    const auto model=core.models().at(id);auto& view=modelViews_[id];view.animation=std::clamp(view.animation,-1,int(model["animations"].size())-1);if(!model["meshes"].empty())view.mesh=std::clamp(view.mesh,0,int(model["meshes"].size())-1);
    const auto authoring=model.value("extensions",Json::object()).value("org.cygame.editor",Json::object());bool needsReview=false;for(const auto& levels:authoring.value("lods",Json::object()))for(const auto& level:levels)needsReview|=level.value("stale",false);for(const auto& shape:authoring.value("collisions",Json::array()))needsReview|=shape.value("needsReview",false);if(needsReview)cyui::callout(cyui::Tone::warning,tr("Generated data needs review after geometry changes."));if(view.lod>0)for(const auto& levels:authoring.value("lods",Json::object()))if(levels.size()>=size_t(view.lod)&&levels[view.lod-1].value("stale",false))view.lod=0;
    auto command=[&](const char* operation,Json p){p["modelId"]=id;return invoke(operation,p);};
    if(cyui::primaryButton(tr("Save asset")))command("model_save",{});
    ImGui::SameLine();if(cyui::ghostButton(tr("Reload from disk")))command("model_reload",{});
    ImGui::SameLine();if(cyui::ghostButton(tr("Undo"))&&core.canUndo())invoke("edit_undo");ImGui::SameLine();if(cyui::ghostButton(tr("Redo"))&&core.canRedo())invoke("edit_redo");
    ImGui::SameLine();if(cyui::ghostButton(tr("Export GLB")))command("model_export",{{"format","glb"}});ImGui::SameLine();if(cyui::ghostButton(tr("Export CyPack")))command("model_export",{{"format","cypack"}});
    if(view.exporter){const auto state=view.exporter->status();cyui::badge(tr(state.value("state","Idle").c_str()));if(state.value("state","")=="Completed"&&state.contains("job"))ImGui::TextWrapped("%s",state["job"]["outputs"].value("outputPath","").c_str());if(state.value("state","")=="Failed"){if(state.contains("error"))cyui::textDim("%s",state["error"].get_ref<const std::string&>().c_str());else if(state.contains("job"))ImGui::TextWrapped("%s",state["job"].value("error",Json::object()).value("message","").c_str());}}
    if(view.processor){const auto state=view.processor->status();if(state.value("state","")=="Completed"&&view.processedRun!=state.value("runId","")&&state.contains("job")){const auto summary=state["job"].value("outputs",Json::object()).value("summary",Json::object());if(summary.contains("animation")){view.animation=summary["animation"];view.time=0;view.playing=false;view.track=view.key=0;}view.processedRun=state.value("runId","");}cyui::badge(tr(state.value("state","Idle").c_str()));if(state.contains("job")&&state["job"].contains("progress")){const auto progress=state["job"]["progress"];cyui::progressBar(float(progress.value("percent",0.))/100.f,{0,cyui::dp(18)},tr(progress.value("currentStep","Processing asset").c_str()));}if(state.value("state","")=="Failed")ImGui::TextWrapped("%s",state.value("error",state.value("job",Json::object()).value("error",Json::object()).value("message","")).c_str());if(state.value("state","")=="Running"&&cyui::ghostButton(tr("Cancel")))view.processor->cancel();}
    cyui::textDim("%s  /  %zu %s  /  %zu %s",model.value("assetId","").c_str(),model["meshes"].size(),tr("meshes"),model["skins"].size(),tr("skeletons"));
    const float inspector=std::min(cyui::dp(340),ImGui::GetContentRegionAvail().x*.4f);
    ImGui::BeginChild("##modelLeft",{std::max(cyui::dp(140),ImGui::GetContentRegionAvail().x-inspector-cyui::dp(8)),0});
    if(ImGui::BeginCombo("##clip",view.animation<0?tr("Reference pose"):model["animations"][view.animation].value("name","Animation").c_str())){
        if(ImGui::Selectable(tr("Reference pose"),view.animation<0)){view.animation=-1;view.playing=false;view.time=0;}
        for(size_t i=0;i<model["animations"].size();++i)if(ImGui::Selectable(model["animations"][i].value("name","Animation").c_str(),view.animation==int(i))){view.animation=int(i);view.time=0;view.track=view.key=0;}
        ImGui::EndCombo();
    }
    ImGui::SetNextItemWidth(cyui::dp(140));ImGui::InputText("##clipName",view.clipName,sizeof(view.clipName));ImGui::SameLine();if(cyui::ghostButton(tr("New clip"))){auto result=command("model_clip_create",{{"name",std::string(view.clipName)}});if(result.contains("animation")){view.animation=result["animation"];view.time=0;view.track=view.key=0;ImGui::EndChild();return;}}
    float duration=0;if(view.animation>=0)for(const auto& track:model["animations"][view.animation]["tracks"])duration=std::max(duration,track["times"].back().get<float>());
    if(view.playing&&duration>0){view.time+=ImGui::GetIO().DeltaTime*view.speed;if(view.time>duration){if(view.loop)view.time=std::fmod(view.time,duration);else{view.time=duration;view.playing=false;}}}
    {cyui::ScopedDisabled disabled(view.animation<0);if(cyui::ghostButton(tr(view.playing?"Pause":"Play clip")))view.playing=!view.playing;probeInterface("model/play");ImGui::SameLine();ImGui::Checkbox(tr("Loop"),&view.loop);ImGui::SameLine();ImGui::SetNextItemWidth(cyui::dp(70));ImGui::DragFloat("##speed",&view.speed,.05f,.05f,4.f,"%.2fx",ImGuiSliderFlags_AlwaysClamp);ImGui::SliderFloat(tr("Time (seconds)"),&view.time,0,std::max(1.f,duration),"%.3f");}
    ImGui::Checkbox(tr("Wireframe"),&view.camera.wireframe);ImGui::SameLine();ImGui::Checkbox(tr("Skeleton overlay"),&view.skeleton);ImGui::SameLine();if(cyui::ghostButton(tr("Frame all")))view.camera=ModelCamera{};
    if(view.revision!=core.revision()||view.sampledTime!=view.time||view.sampledAnimation!=view.animation||view.sampledLod!=view.lod){try{view.frame=sampleScene3D(model,view.animation,view.time,view.lod);}catch(const std::exception& e){cyui::callout(cyui::Tone::error,e.what());ImGui::EndChild();return;}view.revision=core.revision();view.sampledTime=view.time;view.sampledAnimation=view.animation;view.sampledLod=view.lod;}
    // Switching between clips at the same time must refresh the pose as well.
    if(view.frame.nodes.size()!=model["nodes"].size())view.frame=sampleScene3D(model,view.animation,view.time,view.lod);
    ImVec2 size=ImGui::GetContentRegionAvail();size.x=std::max(size.x,1.f);size.y=std::max(size.y,1.f);const auto origin=ImGui::GetCursorScreenPos();
    if(meshRenderer){ImGui::Image(meshRenderer(id,view.frame,view.camera,size),size);if(ImGui::IsItemHovered()){auto& io=ImGui::GetIO();view.camera.distance=std::clamp(view.camera.distance*std::pow(.9f,io.MouseWheel),.2f,30.f);if(ImGui::IsMouseDragging(ImGuiMouseButton_Right)){view.camera.yaw+=io.MouseDelta.x*.008f;view.camera.pitch=std::clamp(view.camera.pitch+io.MouseDelta.y*.008f,-1.5f,1.5f);}}
        if(view.skeleton){using namespace DirectX;auto matrix=modelCameraMatrix(view.frame,view.camera,size);XMFLOAT4X4 m;std::copy(matrix.begin(),matrix.end(),&m._11);auto transform=XMLoadFloat4x4(&m);auto* draw=ImGui::GetWindowDrawList();draw->PushClipRect(origin,{origin.x+size.x,origin.y+size.y},true);std::vector<ImVec2> points;std::vector<bool> visible;
            for(const auto& point:view.frame.nodes){auto v=XMVector4Transform(XMVectorSet(point[0],point[1],point[2],1),transform);float w=XMVectorGetW(v);visible.push_back(w>0);points.push_back({origin.x+(XMVectorGetX(v)/std::max(.001f,w)+1)*size.x*.5f,origin.y+(1-XMVectorGetY(v)/std::max(.001f,w))*size.y*.5f});}
            for(size_t i=0;i<points.size();++i){int parent=model["nodes"][i].value("parent",-1);if(visible[i]){if(parent>=0&&visible[parent])draw->AddLine(points[i],points[parent],cyui::palette().info,cyui::dp(1.5f));draw->AddCircleFilled(points[i],cyui::dp(view.node==int(i)?5.f:3.f),view.node==int(i)?cyui::palette().accent:cyui::palette().text);}}draw->PopClipRect();}
        drawModelCollisions(id,model,origin,size);
    }else cyui::emptyState(tr("3D preview unavailable"),tr("Register a mesh renderer for this platform."),cyui::icon::info);
    ImGui::EndChild();ImGui::SameLine();ImGui::BeginChild("##modelInspector",{0,0});
    if(ImGui::BeginTabBar("##modelTools")){
        auto tab=[&](const char* source){bool selected=ImGui::BeginTabItem(tr(source),nullptr,view.requestedTab==source?ImGuiTabItemFlags_SetSelected:0);probeInterface(std::string("model/tab/")+source);if(selected&&view.requestedTab==source)view.requestedTab.clear();return selected;};
        if(tab("Skeleton")){
            cyui::sectionTitle(tr("Node hierarchy"));ImGui::BeginChild("##bones",{0,cyui::dp(165)},ImGuiChildFlags_Borders);
            for(size_t i=0;i<model["nodes"].size();++i){const auto& node=model["nodes"][i];int depth=0,parent=node.value("parent",-1);while(parent>=0){++depth;parent=model["nodes"][parent].value("parent",-1);}ImGui::Indent(cyui::dp(float(std::min(depth,12))*8));if(ImGui::Selectable(label(node.value("name","Node"),std::to_string(i)).c_str(),view.node==int(i)))view.node=int(i);ImGui::Unindent(cyui::dp(float(std::min(depth,12))*8));}ImGui::EndChild();
            if(!model["nodes"].empty()){view.node=std::clamp(view.node,0,int(model["nodes"].size())-1);const auto& node=model["nodes"][view.node];char name[513]{};std::snprintf(name,sizeof(name),"%s",node.value("name","").c_str());if(ImGui::InputText(tr("Name"),name,sizeof(name),ImGuiInputTextFlags_EnterReturnsTrue))command("model_node_set",{{"node",view.node},{"name",name}});
                if(cyui::beginProperties("##boneProperties",cyui::dp(90))){for(const auto* field:{"translation","rotation","scale"}){cyui::property(tr(fieldTitle(field)));Json next;Json spec={{"type","array"}};if(editValue("model/"+id+"/node/"+std::to_string(view.node)+"/"+field,spec,node.at(field),next))command("model_node_set",{{"node",view.node},{field,next}});}cyui::endProperties();}
                int parent=node.value("parent",-1);if(ImGui::InputInt(tr("Parent node"),&parent,0,0,ImGuiInputTextFlags_EnterReturnsTrue))command("model_node_set",{{"node",view.node},{"parent",parent}});
                if(node.contains("matrix"))cyui::textDim("%s",tr("Matrix node: transform editing requires decomposition."));
            }ImGui::EndTabItem();
        }
        if(tab("Mesh")){
            if(!model["meshes"].empty()){if(cyui::beginProperties("##meshSelection",cyui::dp(75))){cyui::property(tr("Mesh"));if(ImGui::BeginCombo("##meshIndex",model["meshes"][view.mesh].value("name","Mesh").c_str())){for(size_t i=0;i<model["meshes"].size();++i)if(ImGui::Selectable(label(model["meshes"][i].value("name","Mesh"),std::to_string(i)).c_str(),view.mesh==int(i)))view.mesh=int(i);ImGui::EndCombo();}cyui::endProperties();}const auto& mesh=model["meshes"][view.mesh];view.primitive=mesh["primitives"].empty()?0:std::clamp(view.primitive,0,int(mesh["primitives"].size())-1);
                if(!mesh["primitives"].empty()){if(cyui::beginProperties("##primitiveSelection",cyui::dp(75))){cyui::property(tr("Primitive"));if(ImGui::InputInt("##primitiveIndex",&view.primitive,1,10))view.primitive=std::clamp(view.primitive,0,int(mesh["primitives"].size())-1);cyui::endProperties();}const auto& primitive=mesh["primitives"][view.primitive];ImGui::Text("%zu %s / %zu %s",primitive["positions"].size(),tr("vertices"),primitive["indices"].size()/3,tr("triangles"));view.vertex=std::clamp(view.vertex,0,int(primitive["positions"].size())-1);if(cyui::beginProperties("##vertexSelection",cyui::dp(75))){cyui::property(tr("Vertex"));if(ImGui::InputInt("##vertexIndex",&view.vertex,1,10))view.vertex=std::clamp(view.vertex,0,int(primitive["positions"].size())-1);cyui::endProperties();}
                    if(cyui::beginProperties("##vertexProps",cyui::dp(90))){for(const auto* field:{"positions","normals","uv","joints","weights"})if(primitive.contains(field)){cyui::property(tr(fieldTitle(field)));Json next;if(editValue("model/"+id+"/vertex/"+std::to_string(view.vertex)+"/"+field,Json{{"type","array"},{"items",std::string(field)=="joints"?Json{{"type","integer"},{"minimum",0},{"maximum",255}}:Json::object()}},primitive[field][view.vertex],next))command("model_vertex_set",{{"mesh",view.mesh},{"primitive",view.primitive},{"vertex",view.vertex},{field,next}});if(std::string(field)=="positions")probeInterface("model/vertex");}cyui::endProperties();}
                }}ImGui::EndTabItem();
        }
        if(tab("Animation")){
            if(view.animation>=0){const char* channels[]={tr("translation"),tr("rotation"),tr("scale")};ImGui::SetNextItemWidth(-1);ImGui::Combo("##newTrackPath",&view.trackChannel,channels,3);const char* fields[]={"translation","rotation","scale"};if(cyui::ghostButton(tr("Add track for selected node"))){auto result=command("model_track_add",{{"animation",view.animation},{"node",view.node},{"path",fields[view.trackChannel]}});if(result.contains("track")){view.track=result["track"];view.key=0;ImGui::EndTabItem();ImGui::EndTabBar();ImGui::EndChild();return;}}
                const auto& tracks=model["animations"][view.animation]["tracks"];if(!tracks.empty()){view.track=std::clamp(view.track,0,int(tracks.size())-1);if(cyui::beginProperties("##trackSelection",cyui::dp(75))){cyui::property(tr("Track"));if(ImGui::InputInt("##trackIndex",&view.track,1,10))view.track=std::clamp(view.track,0,int(tracks.size())-1);cyui::endProperties();}const auto& track=tracks[view.track];view.key=std::clamp(view.key,0,int(track["times"].size())-1);ImGui::Text("%s / %s",track.at("path").get_ref<const std::string&>().c_str(),track.at("interpolation").get_ref<const std::string&>().c_str());if(cyui::beginProperties("##keySelection",cyui::dp(75))){cyui::property(tr("Keyframe"));if(ImGui::InputInt("##keyIndex",&view.key,1,10))view.key=std::clamp(view.key,0,int(track["times"].size())-1);cyui::endProperties();}
                float time=track["times"][view.key];if(ImGui::InputFloat(tr("Key time"),&time,0,0,"%.3f",ImGuiInputTextFlags_EnterReturnsTrue))command("model_key_set",{{"animation",view.animation},{"track",view.track},{"key",view.key},{"time",time}});
                const size_t index=track.at("interpolation")=="CUBICSPLINE"?size_t(view.key)*3+1:size_t(view.key);if(cyui::beginProperties("##keyValue")){cyui::property(tr("Value"));Json next;if(editValue("model/"+id+"/track/"+std::to_string(view.track)+"/key/"+std::to_string(view.key),{{"type","array"}},track["values"][index],next))command("model_key_set",{{"animation",view.animation},{"track",view.track},{"key",view.key},{"value",next}});cyui::endProperties();}
                if(cyui::ghostButton(tr("Add key at current time")))command("model_key_add",{{"animation",view.animation},{"track",view.track},{"time",view.time},{"value",track["values"][index]}});ImGui::SameLine();if(cyui::ghostButton(tr("Remove key")))command("model_key_remove",{{"animation",view.animation},{"track",view.track},{"key",view.key}});
                cyui::sectionTitle(tr("Timeline"));const auto a=ImGui::GetCursorScreenPos();ImVec2 region{ImGui::GetContentRegionAvail().x,cyui::dp(64)};ImGui::InvisibleButton("##timeline",region);auto* d=ImGui::GetWindowDrawList();d->AddRectFilled(a,{a.x+region.x,a.y+region.y},cyui::palette().bg0);for(const auto& t:track["times"]){float x=a.x+t.get<float>()/std::max(1.f,duration)*region.x;d->AddLine({x,a.y+cyui::dp(16)},{x,a.y+region.y-cyui::dp(16)},cyui::palette().textDim);}float cursor=a.x+view.time/std::max(1.f,duration)*region.x;d->AddLine({cursor,a.y},{cursor,a.y+region.y},cyui::palette().accent,cyui::dp(2));if(ImGui::IsItemHovered()&&ImGui::IsMouseDown(0)){view.playing=false;view.time=std::clamp((ImGui::GetIO().MousePos.x-a.x)/region.x,0.f,1.f)*duration;}
            }}else cyui::textDim("%s",tr("This asset has no animation clips."));ImGui::EndTabItem();
        }
        modelAuthoringUI(id,model);
        modelMotionUI(id,model);
        ImGui::EndTabBar();
    }ImGui::EndChild();
}
}
