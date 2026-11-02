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
    auto& view=imageViews_.at(path);
    if(cyui::iconButton(cyui::icon::zoomIn,tr("Zoom in"))){view.fit=false;view.image.fitPending=false;view.image.canvas.view.zoom=std::min(8.,view.image.canvas.view.zoom*1.25);}ImGui::SameLine();
    if(cyui::iconButton(cyui::icon::zoomOut,tr("Zoom out"))){view.fit=false;view.image.fitPending=false;view.image.canvas.view.zoom=std::max(.05,view.image.canvas.view.zoom/1.25);}ImGui::SameLine();
    if(cyui::ghostButton(tr("Fit image"))){view.fit=true;view.image.fitPending=true;}probeInterface("image/fit");ImGui::SameLine();if(cyui::ghostButton("100%")){view.fit=false;view.image.fitPending=false;view.image.canvas.view.zoom=1;view.image.canvas.view.origin={0,0};}ImGui::SameLine();
    const char* channels[]={"RGBA","RGB","R","G","B","A"};ImGui::SetNextItemWidth(cyui::dp(110));ImGui::Combo("##channels",&view.channel,channels,6);ImGui::SameLine();
    if(cyui::iconButton(cyui::icon::refresh,tr("Reload image"))){auto info=invoke("texture_info",{{"path",path}});if(!info.contains("error")){view.info=info;view.refresh=true;}}
    cyui::textDim("%s / %u x %u / %.1f KiB / %.0f%%",view.info.at("format").get_ref<const std::string&>().c_str(),view.info.at("width").get<unsigned>(),view.info.at("height").get<unsigned>(),view.info.at("bytes").get<double>()/1024,view.image.canvas.view.zoom*100);
    if(!textureRenderer){cyui::emptyState(tr("Image preview unavailable"),tr("Connect an image renderer for this platform."),cyui::icon::photo);return;}
    ImTextureID texture{};try{texture=textureRenderer(path,view.channel,view.refresh);view.refresh=false;}catch(const std::exception& e){cyui::callout(cyui::Tone::error,e.what());return;}
    const cyui::ImageData image{texture,view.info.at("width").get<int>(),view.info.at("height").get<int>()};
    const auto start=ImGui::GetCursorScreenPos(),room=ImGui::GetContentRegionAvail();const cyui::Rect area{start,{start.x+room.x,start.y+room.y}};auto& io=ImGui::GetIO();
    const bool hovered=ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)&&area.contains(io.MousePos);
    if(hovered&&(io.MouseWheel!=0||ImGui::IsMouseDown(ImGuiMouseButton_Middle)))view.fit=false;
    if(view.fit&&(view.fittedRoom.x!=room.x||view.fittedRoom.y!=room.y))view.image.fitPending=true;
    const bool fitting=view.image.fitPending||(hovered&&ImGui::IsMouseDoubleClicked(0));
    cyui::imageViewport("##imageCanvas",view.image,image,room);probeInterface("image/canvas");view.fittedRoom=room;
    if(hovered&&ImGui::IsMouseDoubleClicked(0))view.fit=true;
    // Retain the host's documented 5%..800% range while common Images owns gestures.
    const auto zoom=std::clamp(view.image.canvas.view.zoom,.05,8.);
    if(zoom!=view.image.canvas.view.zoom){if(fitting){view.image.canvas.view.zoom=zoom;view.image.canvas.view.origin={image.width*.5-room.x/(2*zoom),image.height*.5-room.y/(2*zoom)};}else view.image.canvas.view.zoomAt(io.MousePos,zoom,start);}
}
}
