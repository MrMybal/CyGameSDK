#include "Editor.h"
#include <algorithm>
#include <cmath>
namespace cygame {
Json EditorApp::openTexture(const std::string& path){
    auto info=core.execute("texture_info",{{"path",path}});const std::string relative=info.at("path"),key="texture:"+relative;imageViews_[relative].info=info;
    for(auto& panel:core.panels)if(panel.id==key){panel.open=true;panel.focus=true;return {{"panelId",key}};}
    core.panels.push_back({key,fs::u8path(relative).filename().u8string(),"core",[this,relative]{textureEditor(relative);},true,true});return {{"panelId",key}};
}
void EditorApp::textureEditor(const std::string& path){
    auto& view=imageViews_.at(path);const auto& pal=cyui::palette();
    if(cyui::iconButton(cyui::icon::zoomIn,tr("Zoom in"))){view.fit=false;view.zoom=std::min(8.f,view.zoom*1.25f);}ImGui::SameLine();
    if(cyui::iconButton(cyui::icon::zoomOut,tr("Zoom out"))){view.fit=false;view.zoom=std::max(.05f,view.zoom/1.25f);}ImGui::SameLine();
    if(cyui::ghostButton(tr("Fit image")))view.fit=true;ImGui::SameLine();if(cyui::ghostButton("100%")){view.fit=false;view.zoom=1;view.pan={0,0};}ImGui::SameLine();
    const char* channels[]={"RGBA","RGB","R","G","B","A"};ImGui::SetNextItemWidth(cyui::dp(110));ImGui::Combo("##channels",&view.channel,channels,6);ImGui::SameLine();
    if(cyui::iconButton(cyui::icon::refresh,tr("Reload image"))){auto info=invoke("texture_info",{{"path",path}});if(!info.contains("error")){view.info=info;view.refresh=true;}}
    cyui::textDim("%s / %u x %u / %.1f KiB / %.0f%%",view.info.at("format").get_ref<const std::string&>().c_str(),view.info.at("width").get<unsigned>(),view.info.at("height").get<unsigned>(),view.info.at("bytes").get<double>()/1024,view.zoom*100);
    if(!textureRenderer){cyui::emptyState(tr("Image preview unavailable"),tr("Connect an image renderer for this platform."),cyui::icon::photo);return;}
    ImTextureID texture{};try{texture=textureRenderer(path,view.channel,view.refresh);view.refresh=false;}catch(const std::exception& e){cyui::callout(cyui::Tone::error,e.what());return;}
    ImGui::BeginChild("##imageCanvas",{0,0},ImGuiChildFlags_Borders,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
    const auto start=ImGui::GetCursorScreenPos(),size=ImGui::GetContentRegionAvail();const ImVec2 end{start.x+size.x,start.y+size.y};const float width=view.info.at("width"),height=view.info.at("height");
    if(view.fit){view.zoom=std::clamp(std::min(size.x/width,size.y/height),.05f,8.f);view.pan={(size.x-width*view.zoom)*.5f,(size.y-height*view.zoom)*.5f};}
    ImGui::InvisibleButton("##imageInteraction",size,ImGuiButtonFlags_MouseButtonMiddle);auto& io=ImGui::GetIO();
    if(ImGui::IsItemHovered()){
        if(ImGui::IsMouseDragging(ImGuiMouseButton_Middle)){view.fit=false;view.pan.x+=io.MouseDelta.x;view.pan.y+=io.MouseDelta.y;}
        if(io.MouseWheel){view.fit=false;const float next=std::clamp(view.zoom*std::pow(1.1f,io.MouseWheel),.05f,8.f),ratio=next/view.zoom;view.pan={io.MousePos.x-start.x-(io.MousePos.x-start.x-view.pan.x)*ratio,io.MousePos.y-start.y-(io.MousePos.y-start.y-view.pan.y)*ratio};view.zoom=next;}
    }
    auto* draw=ImGui::GetWindowDrawList();draw->PushClipRect(start,end,true);const float cell=cyui::dp(16);for(float y=0;y<size.y;y+=cell)for(float x=0;x<size.x;x+=cell)draw->AddRectFilled({start.x+x,start.y+y},{std::min(end.x,start.x+x+cell),std::min(end.y,start.y+y+cell)},(int(x/cell)+int(y/cell))%2?pal.bg2:pal.bg4);
    draw->AddImage(texture,{start.x+view.pan.x,start.y+view.pan.y},{start.x+view.pan.x+width*view.zoom,start.y+view.pan.y+height*view.zoom});draw->PopClipRect();ImGui::EndChild();
}
}
