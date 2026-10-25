#include "Editor.h"
#include <imgui_internal.h>
#include <algorithm>
#include <cctype>
namespace cygame {
std::string EditorApp::panelTitle(const Panel& p)const{
    auto graph=p.id.rfind("graph:",0)==0?p.id.substr(6):(p.id=="logic"||p.id=="material"?p.id:"");
    if(!graph.empty()&&core.graphs().contains(graph)){
        const auto& g=core.graphs().at(graph);
        std::string title=p.id.rfind("graph:",0)==0?fs::u8path(g.value("assetPath",graph)).stem().u8string():tr(p.title.c_str());
        return title+(core.graphDirty(graph)?" *":"");
    }return tr(p.title.c_str());
}
Json EditorApp::workspaceAction(const std::string& id,const Json& p){
    if(id=="asset_favorite"){
        std::string path=p.at("path");bool enabled=p.at("enabled");if(enabled)favorites_.insert(path);else favorites_.erase(path);
        savePreferences();return {{"path",path},{"enabled",enabled}};
    }
    if(id=="content_drawer_toggle"){drawerOpen_=!drawerOpen_;drawerFocus_=drawerOpen_;return {{"open",drawerOpen_}};}
    if(id=="workspace_layout"){
        selectWorkspace(p.at("preset"));savePreferences();return {{"preset",workspacePreset_}};
    }
    std::string name=p.at("name");for(char c:name)if(!std::isalnum(static_cast<unsigned char>(c))&&c!='-'&&c!='_')throw std::runtime_error("Layout names use letters, numbers, underscore or dash");
    const auto path=core.root()/("data/layouts/"+name+".json");
    if(id=="workspace_save"){
        size_t bytes=0;const char* ini=ImGui::SaveIniSettingsToMemory(&bytes);Json panels=Json::object();for(const auto& panel:core.panels)panels[panel.id]=panel.open;
        writeJson(path,{{"format","cygame-layout"},{"schemaVersion",1},{"preset",workspacePreset_},{"ini",std::string(ini,bytes)},{"panels",panels}});
    }else if(id=="workspace_load"){
        auto layout=readJson(path);
        if(layout.value("format","")!="cygame-layout"||layout.value("schemaVersion",0)!=1||!layout.at("ini").is_string()||!layout.at("panels").is_object())throw std::runtime_error("Invalid layout");
        const auto preset=layout.value("preset","Level");if(preset!="Level"&&preset!="Scripting"&&preset!="Materials")throw std::runtime_error("Unknown workspace");
        for(const auto& [key,open]:layout["panels"].items())if(!open.is_boolean())throw std::runtime_error("Invalid panel state");
        workspacePreset_=preset;
        for(auto& panel:core.panels)if(layout["panels"].contains(panel.id)){
            panel.open=layout["panels"][panel.id];if(panel.owner=="example.diagnostics"&&!pluginEnabled_)panel.open=false;
        }
        const auto& ini=layout["ini"].get_ref<const std::string&>();ImGui::LoadIniSettingsFromMemory(ini.c_str(),ini.size());
        resetLayout=false;layoutChecked_=true;savePreferences();
    }else throw std::runtime_error("Unknown workspace action");
    return {{"name",name}};
}
void EditorApp::selectWorkspace(const std::string& preset){
        workspacePreset_=preset;drawerOpen_=false;
        for(auto& panel:core.panels){
            panel.open=panel.id=="modules"||panel.id=="content"||panel.id=="log";
            if(workspacePreset_=="Level")panel.open|=panel.id=="viewport"||panel.id=="outliner"||panel.id=="details";
            if(workspacePreset_=="Scripting")panel.open|=panel.id=="logic"||panel.id=="debugger"||panel.id=="mygraph"||panel.id=="graphDiagnostics";
            if(workspacePreset_=="Materials")panel.open|=panel.id=="material"||panel.id=="mygraph"||panel.id=="graphDiagnostics";
        }
        if(workspacePreset_!="Level"){activeGraph_=workspacePreset_=="Scripting"?"logic":"material";for(auto& panel:core.panels)if(panel.id=="modules")panel.open=false;}
        open(workspacePreset_=="Level"?"viewport":workspacePreset_=="Scripting"?"logic":"material");
        resetLayout=true;
}
void EditorApp::contentDrawer(){
    if(!drawerOpen_)return;const auto* vp=ImGui::GetMainViewport();
    float height=std::min(cyui::dp(410),vp->WorkSize.y*0.48f);
    ImGui::SetNextWindowPos({vp->WorkPos.x,vp->WorkPos.y+vp->WorkSize.y-height});
    ImGui::SetNextWindowSize({vp->WorkSize.x,height});ImGui::SetNextWindowViewport(vp->ID);
    if(drawerFocus_)ImGui::SetNextWindowFocus();
    if(ImGui::Begin(label(tr("Content Drawer"),"contentDrawer").c_str(),&drawerOpen_,ImGuiWindowFlags_NoDocking|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize)){
        if(cyui::iconTextButton(cyui::icon::folder,tr("Dock in Layout"))){open("content");drawerOpen_=false;savePreferences();}
        probeInterface("drawer/dock");ImGui::Separator();contentBrowser();
        if(!drawerFocus_&&!ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)&&!ImGui::IsPopupOpen(nullptr,ImGuiPopupFlags_AnyPopupId|ImGuiPopupFlags_AnyPopupLevel))drawerOpen_=false;
    }ImGui::End();drawerFocus_=false;
}
}
