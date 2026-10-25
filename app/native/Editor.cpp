#include "Editor.h"
#include "Graph.h"
#include "BuildClient.h"
#include "EditorTheme.h"
#include <imgui_internal.h>
#include <algorithm>
#include <fstream>
#include <cstdio>
#include <windows.h>

namespace cygame {
namespace {
class DiagnosticsPlugin final:public IEditorPlugin {
public:
    explicit DiagnosticsPlugin(EditorApp& app):app_(app){}
    EditorApp& app_;
    std::string id()const override{return "example.diagnostics";}
    void activate(EditorCore& h) override {
        for(auto& p:h.panels)if(p.id=="diagnostics"){p.open=true;return;}
        h.panels.push_back({"diagnostics","Diagnostics",id(),[this,&h]{
            cyui::heading(app_.tr("Diagnostics"),2);
            ImGui::Text("%.1f FPS / %.2f ms",ImGui::GetIO().Framerate,1000.f/std::max(1.f,ImGui::GetIO().Framerate));
            ImGui::Text("%zu %s / %zu %s",h.entities().size(),app_.tr("objects"),h.panels.size(),app_.tr("panels"));
            cyui::textDim("%s: app/native/Editor.cpp",app_.tr("Source plugin"));
        },false,false});
    }
    void deactivate(EditorCore& h)override{for(auto& p:h.panels)if(p.owner==id())p.open=false;}
};
void bufferCopy(char* out,size_t size,const std::string& in){std::snprintf(out,size,"%s",in.c_str());}
}
EditorApp::EditorApp(EditorCore& h):core(h) {
    try{translations_=readJson(core.root()/"app/locales/ui.json");}catch(...){translations_=Json::object();}
    try{interfaceFeatures_=readJson(core.root()/"app/interface_features.json");}catch(...){interfaceFeatures_=Json();}
    try{preferences_=readJson(core.root()/"data/settings/ui-preferences.json");}catch(...){preferences_=Json::object();}
    if(!preferences_.is_object())preferences_=Json::object();
    for(const auto& key:{"language","theme"})if(preferences_.contains(key)&&!preferences_[key].is_string())preferences_.erase(key);
    for(const auto& key:{"autosave","diagnostics"})if(preferences_.contains(key)&&!preferences_[key].is_boolean())preferences_.erase(key);
    if(preferences_.contains("textScale")&&!preferences_["textScale"].is_number())preferences_.erase("textScale");
    if(preferences_.contains("panels")&&!preferences_["panels"].is_object())preferences_.erase("panels");
    language_=preferences_.value("language","en");if(language_!="fr")language_="en";
    theme_=preferences_.value("theme","unreal-dark");
    if(!editorThemeKnown(theme_))theme_="unreal-dark";
    textScale_=std::clamp(preferences_.value("textScale",1.f),0.8f,1.6f);
    autoSave_=preferences_.value("autosave",false);pluginEnabled_=preferences_.value("diagnostics",true);
    core.panels={
        {"viewport","Viewport","core",[this]{viewport("viewport");}},
        {"outliner","World Outliner","core",[this]{outliner();}},
        {"details","Details","core",[this]{details();}},
        {"content","Content Browser","core",[this]{contentBrowser();}},
        {"log","Output Log","core",[this]{logPanel();}},
        {"modules","Modules","core",[this]{modules();}},
        {"logic","Logic Graph","core",[this]{graphEditor("logic");}},
        {"material","Material Graph","core",[this]{graphEditor("material");}},
        {"operations","MCP / Operations","core",[this]{operations();},false},
        {"options","Options","core",[this]{options();},false},
        {"asset","Asset Editor","core",[this]{assetEditor();},false},
        {"interface","Editor Overview","core",[this]{interfaceOverview();},false},
        {"mesh","3D Assets","core",[this]{modelLibrary();},false}
    };
    for(auto& p:core.panels)if(preferences_.contains("panels")&&preferences_["panels"].contains(p.id)){if(preferences_["panels"][p.id].is_boolean())p.open=preferences_["panels"][p.id].get<bool>();}
    core.resetLayout=[this]{workspaceAction("workspace_layout",{{"preset",workspacePreset_}});};
    core.requestClose=[this]{requestClose();};
    core.openPanel=[this](const std::string& id,bool instance){return open(id,instance);};
    core.focusEntity=[this](const std::string& id){
        const auto pos=core.entity(id).at("properties").at("position");
        for(auto& [key,c]:cameras_){for(int i=0;i<3;++i)c.target[i]=pos[i];c.distance=6;}
    };
    diagnostics_=std::make_unique<DiagnosticsPlugin>(*this);
    core.setPlugin=[this](const std::string& id,bool enabled){
        if(id!=diagnostics_->id())throw std::runtime_error("Unknown plugin");
        pluginEnabled_=enabled;if(enabled)diagnostics_->activate(core);else diagnostics_->deactivate(core);
        savePreferences();return Json{{"pluginId",id},{"enabled",enabled}};
    };
    if(pluginEnabled_)diagnostics_->activate(core);
    int restored=1;if(preferences_.contains("viewportCount")&&preferences_["viewportCount"].is_number_integer())restored=std::clamp(preferences_["viewportCount"].get<int>(),1,16);
    for(int i=1;i<restored;++i)open("viewport",true);
    bufferCopy(recipePath_,sizeof(recipePath_),(core.root()/"data/configurations/operation.json").u8string());
    registerAssetEditor({"cygame.graph",{".cyasset"},[this](const Json& descriptor){
        auto info=invoke("asset_open",{{"path",descriptor.at("path")}});
        if(info.contains("graphId"))open(info.at("graphId"));
    }});
    if(preferences_.contains("favorites")&&preferences_["favorites"].is_array())for(const auto& f:preferences_["favorites"])if(f.is_string())favorites_.insert(f.get<std::string>());
    if(preferences_.contains("assetGrid")&&preferences_["assetGrid"].is_boolean())assetGrid_=preferences_["assetGrid"];
    if(preferences_.contains("workspace")&&preferences_["workspace"].is_string())workspacePreset_=preferences_["workspace"];
    if(workspacePreset_!="Level"&&workspacePreset_!="Scripting"&&workspacePreset_!="Materials")workspacePreset_="Level";
    core.workspaceAction=[this](const std::string& id,const Json& p){return workspaceAction(id,p);};
    core.panels.push_back({"mygraph","My Graph","core",[this]{myGraph();},false});
    core.panels.push_back({"debugger","Graph Debugger","core",[this]{debuggerPanel();},false});
    core.panels.push_back({"graphDiagnostics","Graph Diagnostics","core",[this]{graphDiagnosticsPanel();},false});
    core.panels.push_back({"history","History","core",[this]{historyPanel();},false});
    buildClient_=std::make_unique<BuildClient>(core.root());core.buildUi=[this](const std::string& id,const Json& p){if(id=="build_request")return buildClient_->start(p);if(id=="build_cancel")return buildClient_->cancel();return buildClient_->status();};
    // Local Operations-panel execution submits the same official job through the
    // desktop client. SDK build_execute is registered by build_service.py.
    core.bindCommand("build_execute",[this](const Json& p){auto result=buildClient_->start(p);open("build");return result;});
    core.panels.push_back({"build","Build and Package","core",[this]{buildPanel();},false});
    core.panels.push_back({"navigator","Graph Navigator","core",[this]{graphNavigator();},false});
    core.showGraphDiff=[this](const std::string& graph){core.graphs().at(graph);diffGraph_=graph;diffReport_=Json();diffSelection_=-1;return open("changes");};
    core.panels.push_back({"typeMigration","Type Migration","core",[this]{signatureMigrationPanel(true);},false});
    core.panels.push_back({"signatureMigration","Signature Migration","core",[this]{signatureMigrationPanel();},false});
    core.panels.push_back({"changes","Graph Changes","core",[this]{graphDiffPanel();},false});
    core.panels.push_back({"references","Reference Viewer","core",[this]{referenceViewer();},false});
    core.panels.push_back({"trash","Trash","core",[this]{trashBrowser();},false});
    core.showReferences=[this](const std::string& path){bufferCopy(referencePath_,sizeof(referencePath_),path);referenceReport_=Json();return open("references");};
    core.materialUi=[this](const std::string& id,const Json& p){return shaderAction(id,p);};
    core.bindCommand("material_compile",[this](const Json& p){return shaderAction("material_compile_request",p);});
    core.panels.push_back({"compiler","Material Compiler","core",[this]{compilerPanel();},false});
    core.textureUi=[this](const Json& p){auto opened=openTexture(p.at("path"));auto info=core.execute("texture_info",{{"path",p.at("path")}});auto& v=imageViews_.at(info.at("path").get<std::string>());const std::vector<std::string> channels={"RGBA","RGB","R","G","B","A"};if(p.contains("channel"))v.channel=int(std::find(channels.begin(),channels.end(),p.at("channel").get<std::string>())-channels.begin());if(p.contains("zoom")){v.zoom=p.at("zoom");v.fit=false;v.pan={0,0};}if(p.contains("fit"))v.fit=p.at("fit");opened["channel"]=channels.at(v.channel);opened["fit"]=v.fit;opened["zoom"]=v.zoom;return opened;};
    core.showTexture=[this](const std::string& path){return openTexture(path);};
    registerAssetEditor({"cygame.image",{".png",".jpg",".jpeg",".bmp"},[this](const Json& d){openTexture(d.at("path"));}});
    core.bindCommand("asset_browser_settings",[this](const Json& p){
        if(p.contains("folder")){const std::string folder=p.at("folder");bool exists=folder.empty();const auto folderList=core.execute("assets_list").at("folders");for(const auto& f:folderList)exists|=f==folder;if(!exists)throw std::runtime_error("Content folder is missing");assetFolder_=folder;}
        if(p.contains("query"))bufferCopy(assetSearch_,sizeof(assetSearch_),p.at("query"));
        if(p.contains("type")){const std::vector<std::string> types={"All","Graph","Texture","Mesh","Audio","Other"};assetType_=int(std::find(types.begin(),types.end(),p.at("type").get<std::string>())-types.begin());}
        if(p.contains("grid"))assetGrid_=p.at("grid");assetsRefresh();return Json{{"query",std::string(assetSearch_)},{"type",std::vector<std::string>{"All","Graph","Texture","Mesh","Audio","Other"}.at(assetType_)},{"folder",assetFolder_},{"grid",assetGrid_}};
    });
    importClient_=std::make_unique<BuildClient>(core.root());core.importUi=[this](const std::string& id,const Json& p){return importAction(id,p);};
    core.bindCommand("asset_import",[this](const Json& p){auto args=p;args["reimport"]=false;auto result=importAction("asset_import_request",args);open("imports");return result;});
    core.bindCommand("asset_reimport",[this](const Json& p){auto args=p;args["reimport"]=true;auto result=importAction("asset_import_request",args);open("imports");return result;});
    core.panels.push_back({"imports","Asset Import","core",[this]{importPanel();},false});
    registerAssetEditor({"cygame.json",{".json"},[this](const Json& asset){openJson(asset.at("path"));}});
    registerAssetEditor({"cygame.model",{".cy3d"},[this](const Json& asset){openModel(asset.at("path"));}});
    core.bindCommand("graph_debug_view",[this](const Json& p){const std::string id=p.at("graphId");if(core.graphs().at(id).at("domain")!="logic")throw std::runtime_error("Choose a logic asset for the debugger");debugGraph_=id;debugFrame_=-1;return open("debugger");});
    codecClient_=std::make_unique<BuildClient>(core.root());core.panels.push_back({"codecs","Game Codecs","core",[this]{gameCodecPanel();},false});
    for(const auto* operation:{"game_codec_list","game_codec_export"}){const std::string op=operation;core.bindCommand(op,[this,op](const Json& p){return codecClient_->start(p,op);});}
    core.bindCommand("model_process",[this](const Json& p){const std::string id=p.at("modelId");core.models().at(id);auto& view=modelViews_[id];if(!view.processor)view.processor=std::make_shared<BuildClient>(core.root());return view.processor->start(p,"model_process");});
    core.bindCommand("model_view_settings",[this](const Json& p){const std::string id=p.at("modelId");auto& v=modelViews_[id];const auto& m=core.models().at(id);if(p.contains("animation")){int a=p.at("animation");if(a>=int(m.at("animations").size()))throw std::runtime_error("Unknown animation clip");v.animation=a;}if(p.contains("time"))v.time=p.at("time");if(p.contains("playing"))v.playing=p.at("playing");if(p.contains("tab"))v.requestedTab=p.at("tab");if(p.contains("lod"))v.lod=p.at("lod");if(p.contains("wireframe"))v.camera.wireframe=p.at("wireframe");if(p.contains("collisions"))v.collisions=p.at("collisions");if(p.contains("sourceModelId")){core.models().at(p.at("sourceModelId").get<std::string>());v.retargetSource=p.at("sourceModelId");}if(p.contains("mapping"))v.mapping=p.at("mapping");if(v.processor){const auto status=v.processor->status(true);if(status.value("state","")=="Completed")v.processedRun=status.value("runId","");}if(p.contains("motionTool"))v.motionTool=p.at("motionTool");for(auto pair:{std::pair<const char*,int*>{"ikRoot",&v.ikRoot},{"ikTip",&v.ikTip},{"motionRoot",&v.motionRoot}})if(p.contains(pair.first)){int i=p.at(pair.first);if(i>=int(m["nodes"].size()))throw std::runtime_error("Unknown motion node");*pair.second=i;v.motionInitialized=true;}for(auto pair:{std::pair<const char*,float*>{"ikTarget",v.ikTarget},{"ikPole",v.ikPole}})if(p.contains(pair.first))for(size_t i=0;i<3;++i)pair.second[i]=p[pair.first][i];return Json{{"lod",v.lod},{"animation",v.animation},{"time",v.time},{"playing",v.playing}};});
    core.bindCommand("model_export",[this](const Json& p){const std::string id=p.at("modelId");auto& view=modelViews_[id];if(!view.exporter)view.exporter=std::make_shared<BuildClient>(core.root());return view.exporter->start(p,"model_export");});
    if(!preferences_.contains("panels"))selectWorkspace(workspacePreset_);
    else for(auto& panel:core.panels)if(preferences_["panels"].contains(panel.id)&&preferences_["panels"][panel.id].is_boolean())panel.open=preferences_["panels"][panel.id];
    if(!pluginEnabled_)for(auto& panel:core.panels)if(panel.owner==diagnostics_->id())panel.open=false;
    assetsRefresh();
}
EditorApp::~EditorApp(){try{if(persistSettings)savePreferences();}catch(...){}}
void EditorApp::registerAssetEditor(AssetEditorDefinition definition){
    if(definition.id.empty()||!definition.open)throw std::runtime_error("Invalid asset editor");
    for(const auto& handler:assetEditors_)if(handler.id==definition.id)throw std::runtime_error("Duplicate asset editor");
    assetEditors_.push_back(std::move(definition));
}
void EditorApp::openAsset(const Json& descriptor){
    assetPath_=descriptor.at("path");
    auto extension=fs::u8path(assetPath_).extension().u8string();for(auto& c:extension)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    for(const auto& handler:assetEditors_)if(std::find(handler.extensions.begin(),handler.extensions.end(),extension)!=handler.extensions.end()){
        handler.open(descriptor);return;
    }
    auto info=invoke("asset_open",{{"path",assetPath_}});
    bufferCopy(assetInspect_,sizeof(assetInspect_),info.dump(2));open("asset");
}
void EditorApp::configureStyle(float dpi) {
    styleDpi_=dpi;applyEditorTheme(theme_,dpi,textScale_);stylePending_=false;
}
const char* EditorApp::tr(const char* s)const{
    if(language_=="fr"&&translations_.contains(s)&&translations_[s].contains("fr"))return translations_[s]["fr"].get_ref<const std::string&>().c_str();
    return s;
}
std::string EditorApp::label(const std::string& text,const std::string& id)const{return text+"###"+id;}
void EditorApp::savePreferences(){
    if(!persistSettings)return;
    Json panels=Json::object();for(const auto& p:core.panels)panels[p.id]=p.open;
    writeJson(core.root()/"data/settings/ui-preferences.json",{{"language",language_},{"theme",theme_},{"textScale",textScale_},{"autosave",autoSave_},{"diagnostics",pluginEnabled_},{"viewportCount",viewportCount_},{"panels",panels},{"favorites",favorites_},{"assetGrid",assetGrid_},{"workspace",workspacePreset_}});
}
Json EditorApp::invoke(const std::string& id,const Json& p){
    try{
        auto result=core.execute(id,p);
        if(id.rfind("graph_debug_",0)==0){debugGraph_=p.at("graphId");if(id=="graph_debug_start")debugFrame_=-1;}
        if(id.rfind("graph_debug_",0)==0&&result.contains("activeFunction")&&!result.value("nextNode","").empty()&&(result.value("state","")=="Paused"||result.value("state","")=="Breakpoint"))focusGraphNode(p.at("graphId"),result.at("activeFunction"),result.at("nextNode"));
        lastResult_=result;resultText_=result.dump(2);
        if(id=="project_save"||id=="scene_validate"||id=="graph_validate"||id=="asset_save_graph")cyui::notify(tr("Done"),cyui::Tone::success);
        if(id.rfind("asset_",0)==0)assetsRefresh();
        if(id=="asset_move"&&result.contains("path")){
            std::string old=p.at("path"),next=result.at("path");if(favorites_.erase(old)){favorites_.insert(next);savePreferences();}
            if(assetPath_==old)assetPath_=next;
        }
        return result;
    }catch(const std::exception& e){
        core.log(std::string("ERROR | ")+e.what());cyui::notify(e.what(),cyui::Tone::error);
        lastResult_={{"error",e.what()}};resultText_=lastResult_.dump(2,' ',false,Json::error_handler_t::replace);return lastResult_;
    }
}
Json EditorApp::open(const std::string& id,bool instance){
    if(!instance&&id.rfind("model:",0)==0)return openLoadedModel(id.substr(6));
    if(!instance&&id.rfind("json:",0)==0)return openJson(id.substr(5));
    if(id.rfind("fn:",0)==0){const auto split=id.rfind('#');if(split==std::string::npos)throw std::runtime_error("Invalid function panel ID");return openFunction(id.substr(3,split-3),id.substr(split+1));}
    if(instance){
        if(id!="viewport")throw std::runtime_error("Only viewports support multiple instances in this foundation");
        if(viewportCount_>=16)throw std::runtime_error("Viewport limit reached (16)");
        auto key="viewport."+std::to_string(++viewportCount_);
        core.panels.push_back({key,"Viewport "+std::to_string(viewportCount_),"core",[this,key]{viewport(key);},true,true});
        return {{"panelId",key}};
    }
    for(auto& p:core.panels)if(p.id==id){
        if(p.owner=="example.diagnostics"&&!pluginEnabled_)throw std::runtime_error("Plugin disabled");
        p.open=true;p.focus=true;return {{"panelId",id}};
    }
    if(core.graphs().contains(id)){
        auto key="graph:"+id;for(auto& p:core.panels)if(p.id==key){p.open=true;p.focus=true;return {{"panelId",key}};}
        const std::string title=fs::u8path(core.graphs().at(id).value("assetPath",id)).stem().u8string();
        core.panels.push_back({key,title,"core",[this,id]{graphEditor(id);},true,true});return {{"panelId",key}};
    }
    throw std::runtime_error("Panel not found: "+id);
}
void EditorApp::probeInterface(const std::string& id){if(interfaceProbe)interfaceProbe(id,ImGui::GetItemRectMin(),ImGui::GetItemRectMax());}
void EditorApp::menuBar(){
    if(!ImGui::BeginMainMenuBar())return;
    {cyui::ScopedFont font(cyui::boldFont());ImGui::TextUnformatted("CY");}
    ImGui::SameLine();cyui::toolbarSeparator();
    const char* filter="";
    auto beginMenu=[&](const char* title,int index){
        const bool opened=ImGui::BeginMenu(tr(title));probeInterface(std::string("menu/")+title);
        if(!opened)return false;
        auto* buffer=menuFilters_[index];
        cyui::searchField((std::string("##menuFilter/")+title).c_str(),buffer,sizeof(menuFilters_[index]),tr("Filter menu"),cyui::dp(230));
        probeInterface(std::string("filter/")+title);ImGui::Separator();filter=buffer;return true;
    };
    auto item=[&](const char* title,const char* shortcut=nullptr,bool selected=false,bool enabled=true){
        if(!matches(std::string(title)+" "+tr(title),filter))return false;
        const bool clicked=ImGui::MenuItem(tr(title),shortcut,selected,enabled);probeInterface(std::string("action/")+title);return clicked;
    };
    if(beginMenu("File",0)){
        if(item("Save project","Ctrl+S"))invoke("project_save");
        if(item("Load project","Ctrl+O"))invoke("project_load");
        if(item("Exit","Alt+F4"))invoke("editor_close");
        ImGui::EndMenu();
    }
    if(beginMenu("Edit",1)){
        if(item("Undo","Ctrl+Z",false,core.canUndo()))invoke("edit_undo");
        if(item("Redo","Ctrl+Y",false,core.canRedo()))invoke("edit_redo");
        if(item("History"))open("history");
        if(item("Search everything","Ctrl+P")){searchOpen_=true;focusSearch_=true;}
        ImGui::EndMenu();
    }
    if(beginMenu("Window",2)){
        // Copy descriptors: opening a viewport can grow the registry.
        const auto panels=core.panels;
        for(const auto& p:panels)if(p.owner!="example.diagnostics"||pluginEnabled_){
            if(!matches(p.title+" "+panelTitle(p)+" "+p.id,filter))continue;
            const bool toggled=ImGui::MenuItem(label(panelTitle(p),p.id).c_str(),nullptr,p.open);probeInterface("panel/"+p.id);
            if(toggled){
                if(p.open){for(auto& live:core.panels)if(live.id==p.id)live.open=false;savePreferences();}
                else open(p.id);
            }
        }
        ImGui::Separator();if(item("New viewport"))invoke("viewport_new");
        if(item("Reset layout"))invoke("window_reset_layout");
        ImGui::EndMenu();
    }
    if(beginMenu("Workspace",3)){
        for(const auto* preset:{"Level","Scripting","Materials"})if(item(preset,nullptr,workspacePreset_==preset))invoke("workspace_layout",{{"preset",preset}});
        ImGui::Separator();ImGui::SetNextItemWidth(cyui::dp(165));ImGui::InputText(tr("Layout name"),layoutName_,sizeof(layoutName_));
        if(item("Save layout"))invoke("workspace_save",{{"name",std::string(layoutName_)}});
        if(item("Load layout"))invoke("workspace_load",{{"name",std::string(layoutName_)}});
        ImGui::EndMenu();
    }
    if(beginMenu("Tools",4)){
        if(item("3D Assets"))open("mesh");
        if(item("Asset Import"))open("imports");
        if(item("Build and Package"))open("build");
        if(item("Material Compiler"))open("compiler");
        if(item("Graph Navigator"))open("navigator");
        if(item("Reference Viewer"))open("references");
        if(item("MCP / Operations"))open("operations");
        if(item("Options"))open("options");
        ImGui::EndMenu();
    }
    if(beginMenu("Help",5)){
        if(item("Editor Overview"))open("interface");
        if(item("Diagnostics",nullptr,false,pluginEnabled_))open("diagnostics");
        ImGui::EndMenu();
    }
    if(ImGui::GetWindowWidth()>cyui::dp(950)){
        ImGui::SameLine();cyui::alignRight(cyui::dp(330));
        cyui::textFaint("CyGameSDK   /   %s%s",core.adapter().descriptor().value("name","Game adapter").c_str(),core.dirty()?" *":"");
    }
    ImGui::EndMainMenuBar();
}
void EditorApp::topBar(){
    if(cyui::beginTopBar("##toolbar")){
        ImGui::SetNextItemWidth(cyui::dp(115));
        if(ImGui::BeginCombo("##workspace",tr(workspacePreset_.c_str()))){
            for(const auto* preset:{"Level","Scripting","Materials"})if(ImGui::Selectable(tr(preset),workspacePreset_==preset))invoke("workspace_layout",{{"preset",preset}});
            ImGui::EndCombo();
        }cyui::toolbarSeparator();
        if(cyui::iconButton(cyui::icon::save,tr("Save project (Ctrl+S)")))invoke("project_save");
        ImGui::SameLine();if(cyui::iconButton(cyui::icon::undo,tr("Undo (Ctrl+Z)"),false)){if(core.canUndo())invoke("edit_undo");}
        ImGui::SameLine();if(cyui::iconButton(cyui::icon::redo,tr("Redo (Ctrl+Y)"),false)){if(core.canRedo())invoke("edit_redo");}
        cyui::toolbarSeparator();
        if(cyui::iconTextButton(cyui::icon::add,tr("Add cube")))invoke("scene_create",{{"name","SM_Cube"}});
        ImGui::SameLine();if(cyui::ghostButton(tr("Validate scene")))invoke("scene_validate");
        cyui::toolbarSeparator();
        {cyui::ScopedDisabled disabled(true);cyui::iconButton(cyui::icon::play,tr("Play: requires a game adapter"));}
        ImGui::SameLine();if(cyui::iconTextButton(cyui::icon::code,tr("Build / Package")))open("build");
        cyui::tooltip(tr("The Sandbox adapter has no game runtime or compiler."));
        ImGui::SameLine();cyui::alignRight(cyui::dp(330));
        if(cyui::ghostButton(tr("Search everything   Ctrl+P"),{cyui::dp(330),0})){searchOpen_=true;focusSearch_=true;}
    }cyui::endTopBar();
}
void EditorApp::layout(ImGuiID id){
    ImGui::DockBuilderRemoveNode(id);ImGui::DockBuilderAddNode(id,ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(id,ImGui::GetMainViewport()->WorkSize);
    ImGuiID center=id,right=0,rightTop=0;
    if(workspacePreset_=="Level"){
        right=ImGui::DockBuilderSplitNode(center,ImGuiDir_Right,0.24f,nullptr,&center);
        rightTop=ImGui::DockBuilderSplitNode(right,ImGuiDir_Up,0.33f,nullptr,&right);
    }
    auto bottom=ImGui::DockBuilderSplitNode(center,ImGuiDir_Down,workspacePreset_=="Level"?0.29f:0.24f,nullptr,&center);
    auto left=ImGui::DockBuilderSplitNode(center,ImGuiDir_Left,0.18f,nullptr,&center);
    for(const auto& p:core.panels){
        auto target=center;
        if(p.id=="outliner"&&rightTop)target=rightTop;
        else if((p.id=="details"||p.id=="options"||p.id=="diagnostics")&&right)target=right;
        else if(p.id=="content"||p.id=="log"||p.id=="operations"||p.id=="trash"||p.id=="debugger"||p.id=="graphDiagnostics"||p.id=="history"||p.id=="navigator")target=bottom;
        else if(p.id=="modules"||p.id=="mygraph")target=left;
        ImGui::DockBuilderDockWindow(label(panelTitle(p),p.id).c_str(),target);
    }
    ImGui::DockBuilderFinish(id);
}
bool EditorApp::editValue(const std::string& key,const Json& spec,const Json& original,Json& committed){
    if(spec.value("x-editor-widget","")=="record"){
        if(!original.is_object()){cyui::textDim("%s",tr("Draft type unavailable; choose a type or reload."));return false;}for(const auto& field:spec.at("properties").items())if(!original.contains(field.key())){cyui::textDim("%s",tr("Draft type unavailable; choose a type or reload."));return false;}
        Json next=original;bool changed=false;ImGui::PushID(key.c_str());
        if(ImGui::TreeNodeEx(tr("Fields"),ImGuiTreeNodeFlags_DefaultOpen)){
            if(cyui::beginProperties("##recordValues",cyui::dp(95))){for(const auto& [name,field]:spec.at("properties").items()){cyui::property(name.c_str());Json value;if(editValue(key+"/"+name,field,original.at(name),value)){next[name]=value;changed=true;}}cyui::endProperties();}ImGui::TreePop();
        }ImGui::PopID();if(changed)committed=next;return changed;
    }
    if(activeField_!=key)drafts_[key]=original;
    auto& v=drafts_[key];bool changed=false,immediate=false;
    ImGui::PushID(key.c_str());ImGui::SetNextItemWidth(-1);
    const auto type=spec.value("type","");
    Json draftSpec=spec;if(type=="json"){draftSpec.erase("type");if(v.type()!=original.type()&&!(v.is_number()&&original.is_number()))v=original;}
    try{validateValue(v,draftSpec,key);}catch(...){v=original;if(activeField_==key)activeField_.clear();}
    if(spec.contains("enum")){
        const auto value=v.is_string()?v.get<std::string>():v.dump();
        if(ImGui::BeginCombo("##v",spec.value("x-editor-localizeEnum",false)?tr(value.c_str()):value.c_str())){
            for(const auto& item:spec["enum"]){auto text=item.is_string()?item.get<std::string>():item.dump();if(ImGui::Selectable(spec.value("x-editor-localizeEnum",false)?tr(text.c_str()):text.c_str(),item==v)){v=item;changed=immediate=true;}}
            ImGui::EndCombo();
        }
    }else if(type=="boolean"){
        bool b=v.get<bool>();if(ImGui::Checkbox("##v",&b)){v=b;changed=immediate=true;}
    }else if(type=="number"||type=="integer"){
        if(type=="integer"){int n=v.get<int>();if(ImGui::DragInt("##v",&n,1.f,spec.value("minimum",-100000),spec.value("maximum",100000))){v=n;changed=true;}}
        else{float n=v.get<float>();if(ImGui::DragFloat("##v",&n,0.05f,spec.value("minimum",-10000.f),spec.value("maximum",10000.f),"%.3f",ImGuiSliderFlags_AlwaysClamp)){v=n;changed=true;}}
    }else if(type=="array"&&v.size()>=2&&v.size()<=4&&spec.value("items",Json::object()).value("type","")=="integer"){
        int a[4]{};for(size_t i=0;i<v.size();++i)a[i]=v[i].get<int>();const auto item=spec.at("items");changed=ImGui::DragScalarN("##v",ImGuiDataType_S32,a,int(v.size()),1.f,nullptr,nullptr,"%d");if(changed){v=Json::array();for(size_t i=0;i<original.size();++i)v.push_back(std::clamp(a[i],item.value("minimum",-100000),item.value("maximum",100000)));}
    }else if(type=="array"&&v.size()==2){
        float a[2]={v[0],v[1]};changed=ImGui::DragFloat2("##v",a,.5f,spec.value("items",Json::object()).value("minimum",-100000.f),spec.value("items",Json::object()).value("maximum",100000.f),"%.1f",ImGuiSliderFlags_AlwaysClamp);if(changed)v={a[0],a[1]};
    }else if(type=="array"&&v.size()==3){
        float a[3]={v[0],v[1],v[2]};
        if(spec.value("x-editor-widget","")=="color")changed=ImGui::ColorEdit3("##v",a,ImGuiColorEditFlags_Float);
        else changed=ImGui::DragFloat3("##v",a,0.05f,spec.value("items",Json::object()).value("minimum",-10000.f),spec.value("items",Json::object()).value("maximum",10000.f),"%.2f",ImGuiSliderFlags_AlwaysClamp);
        if(changed)v={a[0],a[1],a[2]};
    }else if(type=="array"&&v.size()==4){
        float a[4]={v[0],v[1],v[2],v[3]};changed=ImGui::DragFloat4("##v",a,.01f,0,0,"%.3f");if(changed)v={a[0],a[1],a[2],a[3]};
    }else if(type=="string"){
        char text[4097]{};bufferCopy(text,sizeof(text),v.get<std::string>());
        if(ImGui::InputText("##v",text,std::min(sizeof(text),size_t(spec.value("maxLength",4096))+1))){v=text;changed=true;}
    }else{
        if(v.dump().size()>4096){ImGui::TextUnformatted(tr("Large JSON value (use SDK to edit)"));ImGui::PopID();return false;}
        char text[4097]{};bufferCopy(text,sizeof(text),v.dump());
        if(ImGui::InputText("##v",text,sizeof(text),ImGuiInputTextFlags_EnterReturnsTrue)){try{v=Json::parse(text);changed=immediate=true;}catch(...){cyui::notify(tr("Invalid JSON value"),cyui::Tone::error);}}
    }
    probeInterface("types/value/"+key);
    const bool active=ImGui::IsItemActive(),finished=ImGui::IsItemDeactivatedAfterEdit(),deactivated=ImGui::IsItemDeactivated();
    if(active)activeField_=key;
    if(deactivated&&activeField_==key)activeField_.clear();
    ImGui::PopID();
    if(immediate||finished){committed=v;return true;}
    return false;
}
void EditorApp::details(){
    if(core.selection().empty()){cyui::emptyState(tr("No selection"),tr("Select an object in the viewport or outliner."),cyui::icon::info);return;}
    const auto e=core.entity(core.selection());const auto name=e.at("properties").at("name").get<std::string>();
    cyui::heading(name.c_str(),2);cyui::badge(e.value("type","Object").c_str());
    if(core.selectedIds().size()>1)cyui::callout(cyui::Tone::neutral,tr("Multiple selection: properties below show the primary object."));
    bool locked=e.value("editorLocked",false);
    if(ImGui::Checkbox(label(tr("Locked"),"detailsLock").c_str(),&locked))invoke("scene_lock",{{"locked",locked}});
    std::snprintf(actorFolder_,sizeof(actorFolder_),"%s",e.value("folder","").c_str());
    if(cyui::beginProperties("##objectFolder",cyui::dp(95))){
        cyui::property(tr("Outliner folder"));if(ImGui::InputText("##folder",actorFolder_,sizeof(actorFolder_),ImGuiInputTextFlags_EnterReturnsTrue))invoke("scene_folder_set",{{"folder",std::string(actorFolder_)}});
        cyui::endProperties();
    }
    cyui::ScopedDisabled lockGuard(locked);
    cyui::searchField("##properties",propertySearch_,sizeof(propertySearch_),tr("Search properties"),-1);
    const auto schema=core.adapter().propertySchema(e).at("properties");
    std::vector<std::string> groups={"Object","Transform","Rendering"};
    for(const auto& [key,s]:schema.items()){
        auto group=s.value("x-editor-group","Object");if(std::find(groups.begin(),groups.end(),group)==groups.end())groups.push_back(group);
    }
    for(const auto& group:groups){
        bool any=false;for(const auto& [key,s]:schema.items())if(s.value("x-editor-group","Object")==group&&matches(tr(s.value("title",key).c_str()),propertySearch_))any=true;
        if(any&&cyui::beginSection(tr(group.c_str()))){
            if(cyui::beginProperties(group.c_str(),cyui::dp(82))){
                for(const auto& [key,s]:schema.items())if(s.value("x-editor-group","Object")==group&&matches(tr(s.value("title",key).c_str()),propertySearch_)){
                    cyui::property(tr(s.value("title",key).c_str()));Json value;
                    if(editValue(core.selection()+"/"+key,s,e.at("properties").at(key),value))
                        invoke("property_set",{{"entityId",core.selection()},{"propertyId",key},{"value",value}});
                }cyui::endProperties();
            }cyui::endSection();
        }
    }
    cyui::divider();if(cyui::dangerButton(tr("Delete selection")))invoke("scene_delete_many");
}
void EditorApp::modules(){
    cyui::overline(tr("PROJECT"));cyui::heading(core.adapter().descriptor().value("name","Game adapter").c_str(),2);cyui::textDim("%s",tr("Common editor foundation"));
    cyui::divider();cyui::sectionTitle(tr("Create"));
    if(cyui::iconTextButton(cyui::icon::add,tr("Cube")))invoke("scene_create",{{"name","SM_Cube"}});
    if(cyui::iconTextButton(cyui::icon::code,tr("Logic Graph")))open("logic");
    if(cyui::iconTextButton(cyui::icon::photo,tr("Material Graph")))open("material");
    cyui::divider();cyui::sectionTitle(tr("Game adapter"));
    cyui::badge((core.adapter().descriptor().value("name","Game adapter")+" / "+core.adapter().descriptor().value("version","unknown")).c_str());
    ImGui::TextWrapped("%s",tr("Scene, properties and preview are available. Connect a game adapter for engine formats, build and play."));
    cyui::divider();cyui::sectionTitle(tr("Editor plugins"));
    bool enabled=pluginEnabled_;
    if(ImGui::Checkbox(label(tr("Diagnostics"),"diagnosticsToggle").c_str(),&enabled))invoke("plugin_set_enabled",{{"pluginId","example.diagnostics"},{"enabled",enabled}});
    cyui::textFaint("%s",tr("Source plugin"));
    if(enabled&&cyui::ghostButton(tr("Open diagnostics")))open("diagnostics");
    cyui::divider();if(cyui::iconTextButton(cyui::icon::settings,tr("Options")))open("options");
    if(cyui::iconTextButton(cyui::icon::globe,tr("MCP / Operations")))open("operations");
}
void EditorApp::assetEditor(){
    cyui::heading(assetPath_.empty()?tr("Asset Editor"):assetPath_.c_str(),2);
    ImGui::TextWrapped("%s",tr("Graph and supported image assets have specialized views. Other formats expose metadata until an adapter registers an editor."));
    ImGui::InputTextMultiline("##assetInfo",assetInspect_,sizeof(assetInspect_),{-1,-1},ImGuiInputTextFlags_ReadOnly);
}
void EditorApp::logPanel(){
    if(cyui::ghostButton(tr("Copy log")))ImGui::SetClipboardText(core.logText().c_str());
    ImGui::SameLine();ImGui::Checkbox(label(tr("Pause"),"logPause").c_str(),&logPaused_);
    if(!logPaused_)logSnapshot_=core.logText();
    std::vector<char> text(logSnapshot_.begin(),logSnapshot_.end());text.push_back(0);
    ImGui::InputTextMultiline("##log",text.data(),text.size(),{-1,-1},ImGuiInputTextFlags_ReadOnly);
}
void EditorApp::saveRecipe(){
    auto parameters=Json::parse(operationParams_);bool valid=true;std::string reason;
    for(const auto& d:core.describeCommands())if(d.at("id")==operation_){try{validateValue(parameters,d["inputSchema"]);}catch(const std::exception& e){valid=false;reason=e.what();}}
    Json recipe={{"format","cytools-generation-config"},{"schemaVersion",1},{"toolId","cy.editor.gamesdk"},{"toolVersion","0.21.0"},{"operationId",operation_},{"parameters",parameters},{"backendId",""},{"modelId",""},{"validParameters",valid},{"validationMessage",reason},{"uiState",Json::object()},{"jobId",nullptr}};
    // Parameters only; connection credentials and editor endpoint are not included.
    auto path=core.root()/("data/configurations/"+uniqueId()+".json");writeJson(path,recipe);
    bufferCopy(recipePath_,sizeof(recipePath_),path.u8string());cyui::notify(tr("Configuration saved"),cyui::Tone::success);
}
void EditorApp::loadRecipe(){
    auto recipe=readJson(fs::u8path(recipePath_));
    if(recipe.value("format","")!="cytools-generation-config"||recipe.value("schemaVersion",0)!=1||recipe.value("toolId","")!="cy.editor.gamesdk")throw std::runtime_error("Unsupported configuration");
    std::string id=recipe.at("operationId");bool found=false;
    for(const auto& d:core.describeCommands())if(d.at("id")==id){validateValue(recipe.at("parameters"),d.at("inputSchema"));found=true;}
    if(!found)throw std::runtime_error("Unknown operation");
    operation_=id;bufferCopy(operationParams_,sizeof(operationParams_),recipe.at("parameters").dump(2));
    cyui::notify(tr("Configuration loaded without execution"),cyui::Tone::success);
}
void EditorApp::operations(){
    cyui::textDim("%s",tr("CyToolsCore provides MCP, SDK, validation, jobs and transports."));
    ImGui::SetNextItemWidth(cyui::dp(230));
    if(ImGui::BeginCombo("##operation",operation_.c_str())){
        cyui::searchField("##operationFilter",operationSearch_,sizeof(operationSearch_),tr("Filter operations"),-1);
        for(const auto& d:core.describeCommands()){
            std::string id=d.at("id");
            if(!matches(id+" "+d.value("name",id)+" "+tr(d.value("name",id).c_str()),operationSearch_))continue;
            if(ImGui::Selectable(tr(d.value("name",id).c_str()),operation_==id)){operation_=id;bufferCopy(operationParams_,sizeof(operationParams_),"{}");}
        }ImGui::EndCombo();
    }
    ImGui::SameLine();if(cyui::primaryButton(tr("Execute"))){
        try{auto result=invoke(operation_,Json::parse(operationParams_));if(autoSave_&&!result.contains("error"))saveRecipe();}
        catch(const std::exception& e){cyui::notify(e.what(),cyui::Tone::error);}
    }
    ImGui::SameLine();if(cyui::ghostButton(tr("Copy contract"))){
        for(const auto& d:core.describeCommands())if(d.at("id")==operation_)ImGui::SetClipboardText(d.dump(2).c_str());
    }
    ImGui::BeginChild("##params",{ImGui::GetContentRegionAvail().x*0.48f,0});
    ImGui::TextUnformatted(tr("Parameters (JSON)"));
    ImGui::InputTextMultiline("##paramsJson",operationParams_,sizeof(operationParams_),{-1,cyui::dp(130)});
    if(cyui::ghostButton(tr("Save configuration"))){try{saveRecipe();}catch(const std::exception& e){cyui::notify(e.what(),cyui::Tone::error);}}
    ImGui::SameLine();if(cyui::ghostButton(tr("Load configuration"))){try{loadRecipe();}catch(const std::exception& e){cyui::notify(e.what(),cyui::Tone::error);}}
    ImGui::SetNextItemWidth(-1);ImGui::InputText("##recipePath",recipePath_,sizeof(recipePath_));
    ImGui::EndChild();ImGui::SameLine();ImGui::BeginChild("##results",{0,0});
    ImGui::TextUnformatted(tr("Result"));
    std::vector<char> text(resultText_.begin(),resultText_.end());text.push_back(0);
    ImGui::InputTextMultiline("##resultJson",text.data(),text.size(),{-1,-1},ImGuiInputTextFlags_ReadOnly);ImGui::EndChild();
}
void EditorApp::options(){
    cyui::heading(tr("Options"),2);
    if(cyui::beginProperties("##options",cyui::dp(100))){
        cyui::property(tr("Language"));const char* languages[]={"English","Francais"};int current=language_=="fr"?1:0;
        if(ImGui::Combo("##language",&current,languages,2)){language_=current==1?"fr":"en";savePreferences();}
        cyui::property(tr("Theme"));const bool themeOpen=ImGui::BeginCombo("##theme",tr(editorThemeName(theme_)));probeInterface("theme/combo");if(themeOpen){
            if(ImGui::Selectable(tr("Unreal-inspired Dark"),theme_=="unreal-dark")){theme_="unreal-dark";stylePending_=true;savePreferences();}probeInterface("theme/unreal-dark");
            for(const auto& t:cyui::builtInThemes()){if(ImGui::Selectable(tr(t.name.c_str()),t.key==theme_)){theme_=t.key;stylePending_=true;savePreferences();}probeInterface("theme/"+t.key);}
            ImGui::EndCombo();
        }
        cyui::property(tr("Text size"));if(ImGui::SliderFloat("##scale",&textScale_,0.8f,1.6f,"%.2f")){ImGui::GetStyle().FontScaleMain=textScale_;savePreferences();}
        cyui::property(tr("Autosave"),tr("Save executed operation parameters from the Operations panel."));if(ImGui::Checkbox("##autosave",&autoSave_))savePreferences();
        cyui::endProperties();
    }
    cyui::sectionTitle(tr("Configuration"));
    if(cyui::ghostButton(tr("Save configuration"))){try{saveRecipe();}catch(const std::exception& e){cyui::notify(e.what(),cyui::Tone::error);}}
    if(cyui::ghostButton(tr("Load configuration"))){try{loadRecipe();}catch(const std::exception& e){cyui::notify(e.what(),cyui::Tone::error);}}
    ImGui::SetNextItemWidth(-1);ImGui::InputText("##recipe",recipePath_,sizeof(recipePath_));
    cyui::sectionTitle(tr("Licenses"));
    ImGui::TextWrapped("CyUI: MIT\nDear ImGui: MIT\nImGuizmo: MIT\nCyToolsCore: MIT\nnlohmann/json 3.11.3: MIT\nWindows SDK / Direct3D: Microsoft SDK terms\nSegoe UI: Windows system font");
    cyui::textDim("%s",tr("Full notices: app/LICENCES"));
    cyui::sectionTitle(tr("Development status"));
    ImGui::TextWrapped("%s",tr("Windows foundation. Sandbox adapter, preview shaders and source plugins. Game compilation and Blueprint compatibility require further development."));
}
void EditorApp::searchPopup(){
    if(searchOpen_){ImGui::OpenPopup("##globalSearch");searchOpen_=false;}
    ImGui::SetNextWindowSize(cyui::dp(650,480),ImGuiCond_Appearing);
    if(ImGui::BeginPopup("##globalSearch")){
        if(focusSearch_){ImGui::SetKeyboardFocusHere();focusSearch_=false;}
        cyui::searchField("##global",globalSearch_,sizeof(globalSearch_),tr("Commands, panels, objects, properties, nodes..."),-1);
        cyui::divider();ImGui::BeginChild("##searchResults");
        const auto results=core.search("");size_t displayed=0;
        for(const auto& r:results){
            const std::string kind=r.at("kind"),id=r.at("id"),title=r.at("label");
            if(!matches(std::string(tr(title.c_str()))+" "+title+" "+id,globalSearch_))continue;++displayed;
            if(ImGui::Selectable(label(std::string(tr(title.c_str()))+"    / "+kind,kind+"/"+id).c_str())){
                if(kind=="panel")open(id);
                else if(kind=="entity")invoke("scene_select",{{"entityId",id}});
                else if(kind=="property"){bufferCopy(propertySearch_,sizeof(propertySearch_),tr(title.c_str()));open("details");}
                else if(kind=="node"){
                    auto domain=nodeTypes().at(id).value("domain","logic");if(domain=="both")domain=core.graphs().contains(activeGraph_)?core.graphs().at(activeGraph_).at("domain").get<std::string>():"logic";
                    const auto graph=core.graphs().contains(activeGraph_)&&core.graphs().at(activeGraph_).at("domain")==domain?activeGraph_:domain;
                    invoke("graph_node_add",{{"graphId",graph},{"type",id},{"position",{100,250}}});open(graph);
                }else{
                    bool required=false;for(const auto& d:core.describeCommands())if(d.at("id")==id)required=!d.at("inputSchema").value("required",Json::array()).empty();
                    if(required){operation_=id;bufferCopy(operationParams_,sizeof(operationParams_),"{}");open("operations");}else invoke(id);
                }
                ImGui::CloseCurrentPopup();
            }
        }
        if(displayed==0)cyui::textDim("%s",tr("No results"));
        ImGui::EndChild();ImGui::EndPopup();
    }
}
void EditorApp::frame(){
    if(stylePending_)configureStyle(styleDpi_);
    auto& io=ImGui::GetIO();
    if(io.KeyCtrl&&ImGui::IsKeyPressed(ImGuiKey_Space))invoke("content_drawer_toggle");
    if(io.KeyCtrl&&ImGui::IsKeyPressed(ImGuiKey_P)){searchOpen_=true;focusSearch_=true;}
    if(!io.WantTextInput){
        if(io.KeyCtrl&&ImGui::IsKeyPressed(ImGuiKey_S))invoke("project_save");
        if(io.KeyCtrl&&ImGui::IsKeyPressed(ImGuiKey_O))invoke("project_load");
        if(io.KeyCtrl&&ImGui::IsKeyPressed(ImGuiKey_Z)&&core.canUndo())invoke("edit_undo");
        if(io.KeyCtrl&&ImGui::IsKeyPressed(ImGuiKey_Y)&&core.canRedo())invoke("edit_redo");
    }
    menuBar();topBar();
    if(cyui::beginStatusBar("##status")){
        if(cyui::ghostButton(tr("Content Drawer   Ctrl+Space")))invoke("content_drawer_toggle");
        ImGui::SameLine();if(cyui::ghostButton(tr("Output Log")))open("log");
        ImGui::SameLine();cyui::statusText(cyui::Tone::success,"%s",tr("EDITOR API READY"));
        ImGui::SameLine(0,cyui::dp(24));cyui::statusText(cyui::Tone::neutral,"%s",tr("CyToolsCore MCP / SDK"));
        ImGui::SameLine(0,cyui::dp(24));cyui::statusText(cyui::Tone::neutral,"%zu %s",core.entities().size(),tr("objects"));
        ImGui::SameLine();cyui::alignRight(cyui::dp(175));cyui::statusText(cyui::Tone::neutral,"%.0f FPS  /  %s",io.Framerate,core.dirty()?tr("Unsaved changes"):tr("Saved"));
    }cyui::endStatusBar();
    const auto dock=cyui::dockSpace("##EditorDock");
    if(!layoutChecked_||resetLayout){
        if(resetLayout||!ImGui::DockBuilderGetNode(dock)||ImGui::DockBuilderGetNode(dock)->IsLeafNode())layout(dock);
        layoutChecked_=true;resetLayout=false;
    }
    // Resolve each panel at the time it is submitted. Earlier callbacks may
    // open/close/focus another panel, and callbacks may grow the registry.
    std::vector<std::string> ids;for(const auto& p:core.panels)ids.push_back(p.id);
    for(const auto& id:ids){
        auto found=std::find_if(core.panels.begin(),core.panels.end(),[&](const Panel& p){return p.id==id;});
        if(found==core.panels.end()||!found->open)continue;
        const Panel panel=*found;found->focus=false;
        bool opened=true;if(panel.focus)ImGui::SetNextWindowFocus();
        ImGui::SetNextWindowSize(cyui::dp(500,400),ImGuiCond_FirstUseEver);
        if(panel.id.rfind("json:",0)==0||panel.id=="mesh"||panel.id.rfind("model:",0)==0||panel.id.rfind("viewport.",0)==0||panel.id=="interface"||panel.id=="codecs"||panel.id.rfind("graph:",0)==0||panel.id.rfind("fn:",0)==0||panel.id=="references"||panel.id=="build"||panel.id=="typeMigration"||panel.id=="signatureMigration"||panel.id=="changes"||panel.id=="compiler"||panel.id=="imports"||panel.id.rfind("texture:",0)==0){auto* target=ImGui::FindWindowByName(workspacePreset_=="Materials"?"###material":workspacePreset_=="Scripting"?"###logic":"###viewport");if(target&&target->DockId)ImGui::SetNextWindowDockID(target->DockId,ImGuiCond_FirstUseEver);}
        if(ImGui::Begin(label(panelTitle(panel),panel.id).c_str(),&opened)){
            if((panel.id=="logic"||panel.id=="material")&&ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)){activeGraph_=panel.id;activeFunction_.clear();}
            if(panel.id.rfind("graph:",0)==0&&ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)){activeGraph_=panel.id.substr(6);activeFunction_.clear();}
            if(panel.id.rfind("fn:",0)==0&&ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)){const auto split=panel.id.rfind('#');activeGraph_=panel.id.substr(3,split-3);activeFunction_=panel.id.substr(split+1);}
            panel.draw();
        }ImGui::End();
        if(!opened){for(auto& p:core.panels)if(p.id==id)p.open=false;savePreferences();}
    }
    contentDrawer();
    searchPopup();
    if(closePrompt_){ImGui::OpenPopup("##close");closePrompt_=false;}
    if(ImGui::BeginPopupModal("##close",nullptr,ImGuiWindowFlags_AlwaysAutoResize)){
        ImGui::TextUnformatted(tr("Close editor?"));
        if(core.dirty())ImGui::TextUnformatted(tr("The project has unsaved changes."));
        if(cyui::primaryButton(tr("Save and close"))){if(!invoke("project_save").contains("error"))shouldClose=true;}
        ImGui::SameLine();if(cyui::ghostButton(tr("Close without saving")))shouldClose=true;
        ImGui::SameLine();if(cyui::ghostButton(tr("Cancel")))ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    cyui::renderNotifications();
}
}
