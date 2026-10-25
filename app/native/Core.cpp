#include "Core.h"
#include "Scene3D.h"
#include "Graph.h"
#include "Image.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <windows.h>
#include <objbase.h>

namespace cygame {
bool matches(const std::string& text, const std::string& query) {
    auto lower = [](std::string s) { for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); return s; };
    return lower(text).find(lower(query)) != std::string::npos;
}
Json readJson(const fs::path& path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("Cannot read: " + path.u8string());
    if (fs::file_size(path) > 8 * 1024 * 1024) throw std::runtime_error("JSON exceeds 8 MiB");
    return Json::parse(file);
}
void writeJson(const fs::path& path, const Json& value, size_t byteLimit) {
    std::string serialized=value.dump(2);if(serialized.size()+1>byteLimit)serialized=value.dump();if(serialized.size()+1>byteLimit)throw std::runtime_error("JSON output exceeds the reader size limit");
    fs::create_directories(path.parent_path()); const fs::path temp = path.wstring() + L".tmp";if(fs::is_symlink(temp))throw std::runtime_error("Refusing a linked JSON temporary file");
    { std::ofstream f(temp, std::ios::binary | std::ios::trunc); f << serialized << '\n'; f.flush();
      if (!f) throw std::runtime_error("Cannot write: " + path.u8string()); }
    if (!MoveFileExW(temp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        throw std::runtime_error("Cannot commit: " + path.u8string());
}
std::string uniqueId() {
    GUID id{}; if (FAILED(CoCreateGuid(&id))) throw std::runtime_error("Cannot create ID");
    wchar_t text[40]{}; StringFromGUID2(id, text, 40); return std::string(text + 1, text + 37);
}
// Native boundary enforces the subset used in the shipped descriptors.
// CyToolsCore performs full Draft 2020-12 validation at its own boundary.
void validateValue(const Json& v, const Json& s, const std::string& path) {
    const auto t = s.value("type", "");
    bool valid = t.empty() || (t == "object" && v.is_object()) || (t == "array" && v.is_array()) ||
        (t == "string" && v.is_string()) || (t == "boolean" && v.is_boolean()) ||
        (t == "number" && v.is_number()) || (t == "integer" && v.is_number_integer());
    if (!valid) throw std::runtime_error(path + ": expected " + t);
    if (s.contains("enum") && std::find(s["enum"].begin(), s["enum"].end(), v) == s["enum"].end()) throw std::runtime_error(path + ": invalid enum");
    if (v.is_number()) {
        auto n = v.get<double>();
        if (!std::isfinite(n) || (s.contains("minimum") && n < s["minimum"].get<double>()) || (s.contains("maximum") && n > s["maximum"].get<double>())) throw std::runtime_error(path + ": out of range");
    }
    if (v.is_string()) {
        auto n = v.get_ref<const std::string&>().size();
        if (n < s.value("minLength", size_t(0)) || n > s.value("maxLength", size_t(65536))) throw std::runtime_error(path + ": invalid length");
    }
    if(s.contains("x-editor-max-bytes")&&v.dump().size()>s["x-editor-max-bytes"].get<size_t>())throw std::runtime_error(path+": value size limit");
    if (v.is_array()) {
        if (v.size() < s.value("minItems", size_t(0)) || v.size() > s.value("maxItems", size_t(10000))) throw std::runtime_error(path + ": invalid item count");
        if(s.value("uniqueItems",false))for(size_t i=0;i<v.size();++i)for(size_t j=0;j<i;++j)if(v[i]==v[j])throw std::runtime_error(path+": duplicate set item");
        if (s.contains("items")) for (const auto& x : v) validateValue(x, s["items"], path + "[]");
    }
    if (v.is_object()) {
        if(v.size()>s.value("maxProperties",size_t(10000)))throw std::runtime_error(path+": property count limit");
        for (const auto& k : s.value("required", Json::array())) if (!v.contains(k.get<std::string>())) throw std::runtime_error(path + ": missing " + k.get<std::string>());
        const auto props = s.value("properties", Json::object());
        for (auto it = v.begin(); it != v.end(); ++it) {
            if(s.contains("propertyNames"))validateValue(it.key(),s["propertyNames"],path+" key");
            if (props.contains(it.key())) validateValue(it.value(), props[it.key()], path + "." + it.key());
            else if(s.contains("additionalProperties")&&s["additionalProperties"].is_object())validateValue(it.value(),s["additionalProperties"],path+"."+it.key());
            else if (s.contains("additionalProperties") && s["additionalProperties"] == false) throw std::runtime_error(path + ": unknown field " + it.key());
        }
    }
}
namespace {
class SandboxAdapter final : public IGameAdapter {
public:
    Json descriptor() const override { return {{"id","sandbox"},{"name","Sandbox"},{"version","0.28.0"},{"capabilities",{{"scene",true},{"properties",true},{"preview",true},{"build",false},{"package",true},{"play",false}}}}; }
    Json buildTargets()const override {
        Json options={{"type","object"},{"properties",{{"compression",{{"type","string"},{"enum",{"Stored","Deflated"}},{"default","Deflated"}}},{"includeMaterialValues",{{"type","boolean"},{"default",true}}}}},{"additionalProperties",false}};
        return Json::array({{{"id","sandbox.document"},{"name","Sandbox document archive"},{"providerId","sandbox.archive"},{"platform","Any"},{"configurations",{"Development"}},{"optionsSchema",options},{"gameExecutable",false}}});
    }
    Json propertySchema(const Json&) const override {
        const Json vec3 = {{"type","array"},{"minItems",3},{"maxItems",3},{"items",{{"type","number"},{"minimum",-10000},{"maximum",10000}}},{"x-editor-widget","vector3"}};
        Json props = {
            {"name",{{"type","string"},{"minLength",1},{"maxLength",128},{"title","Name"},{"x-editor-group","Object"}}},
            {"enabled",{{"type","boolean"},{"title","Enabled"},{"x-editor-group","Object"}}},
            {"mobility",{{"type","string"},{"enum",{"Static","Movable"}},{"title","Mobility"},{"x-editor-group","Object"}}},
            {"position",vec3},{"rotation",vec3},{"scale",vec3},
            {"tint",{{"type","array"},{"minItems",3},{"maxItems",3},{"items",{{"type","number"},{"minimum",0},{"maximum",1}}},{"title","Tint"},{"x-editor-widget","color"},{"x-editor-group","Rendering"}}}
        };
        for (const auto& key : {"position","rotation","scale"}) {
            props[key]["title"] = key == std::string("position") ? "Position" : key == std::string("rotation") ? "Rotation" : "Scale";
            props[key]["x-editor-group"] = "Transform";
        }
        props["scale"]["items"]["minimum"] = 0.01;
        return {{"type","object"},{"properties",props},{"required",{"name","enabled","mobility","position","rotation","scale","tint"}},{"additionalProperties",false}};
    }
    Json createEntity(const std::string& id, const std::string& name) const override {
        Json properties={{"name",name},{"enabled",true},{"mobility","Static"},{"position",{0.,0.,0.5}},{"rotation",{0.,0.,0.}},{"scale",{1.,1.,1.}},{"tint",{0.47,0.52,0.6}}};
        return {{"id",id},{"type","StaticMesh"},{"properties",properties}};
    }
    Json initialScene() const override {
        Json a = Json::array();
        auto e = createEntity("platform","Platform"); e["properties"]["scale"] = {7.,5.,0.25}; e["properties"]["position"] = {0.,0.,-0.125}; a.push_back(e);
        e = createEntity("hero","SM_Cube"); e["properties"]["position"] = {0.,0.,0.75}; e["properties"]["scale"] = {1.5,1.5,1.5}; a.push_back(e);
        e = createEntity("pillar","SM_Pillar"); e["properties"]["position"] = {-2.,1.,1.}; e["properties"]["scale"] = {0.6,0.6,2.}; a.push_back(e);
        e = createEntity("block","SM_Block"); e["properties"]["position"] = {2.,-0.5,0.4}; e["properties"]["scale"] = {1.2,1.2,0.8}; a.push_back(e);
        return a;
    }
    Json validateScene(const Json& a) const override {
        if (!a.is_array() || a.size() > 2000) throw std::runtime_error("Invalid scene size (maximum 2000)");
        std::vector<std::string> ids;
        for (const auto& e : a) {
            if (!e.is_object() || !e.contains("id") || !e["id"].is_string() || e.value("type","") != "StaticMesh" || !e.contains("properties")) throw std::runtime_error("Invalid sandbox entity");
            const auto id = e.at("id").get<std::string>();
            if (id.empty() || std::find(ids.begin(),ids.end(),id) != ids.end()) throw std::runtime_error("Duplicate/empty entity ID");
            ids.push_back(id); validateValue(e.at("properties"),propertySchema(e),id);
        }
        return {{"valid",true},{"entityCount",a.size()},{"adapterId","sandbox"}};
    }
};
}
EditorCore::EditorCore(fs::path root,std::unique_ptr<IGameAdapter> adapter) : root_(std::move(root)), adapter_(adapter?std::move(adapter):std::make_unique<SandboxAdapter>()) {
    document_ = {{"entities",adapter_->initialScene()},{"graphs",initialGraphs()}}; saved_ = document_; selected_ = "hero";
    selectedIds_={selected_};
    for(const auto& [id,g]:graphs().items())savedGraphs_[id]=g;
    // Keep the manifest alive for the duration of iteration.
    const auto manifest = readJson(root_ / "app/CyTool.json");
    for (const auto& descriptor : manifest.at("operations")) {
        auto id = descriptor.at("id").get<std::string>();
        registerCommand({descriptor,[this,id](const Json& p){ return nativeCommand(id,p); },descriptor.value("x-editor-undoable",false)});
    }
    registerImporter({{{"id","scene3d.glb"},{"name","Portable 3D (GLB / CyPack)"},{"extensions",{".glb",".cypack"}},{"outputExtensions",{".cy3d"}},{"maxBytes",32*1024*1024}},[](const fs::path& file){if(file.extension()==".cy3d"){auto model=readJson(file);validateScene3D(model);return Json{{"format","cygame-scene3d"},{"assetId",model.at("assetId")}};}return Json{{"format","portable-3d-source"},{"bytes",fs::file_size(file)}};}});
    registerImporter({{{"id","image.copy"},{"name","Image (original file)"},{"extensions",{".png",".jpg",".jpeg",".bmp"}},{"maxBytes",32*1024*1024}},[](const fs::path& file){return readImageFile(file,true).info;}});
    log("CyGameSDK 0.28.0 | Sandbox adapter | editor API ready");
}
void EditorCore::registerCommand(Command c) {
    const std::string id = c.descriptor.at("id");
    if (commands_.count(id)) throw std::runtime_error("Duplicate command: " + id);
    commands_.emplace(id,std::move(c));
}
void EditorCore::bindCommand(const std::string& id,std::function<Json(const Json&)> handler){
    auto found=commands_.find(id);if(found==commands_.end()||!handler)throw std::runtime_error("Declare the operation in CyTool.json before binding it");
    found->second.run=std::move(handler);
}
void EditorCore::log(const std::string& message) {
    log_ += message + '\n';
    if (log_.size() > 256*1024) log_.erase(0,log_.find('\n',64*1024)+1);
    const auto path = root_ / "data/logs/editor.log"; fs::create_directories(path.parent_path());
    if (fs::exists(path) && fs::file_size(path) > 2*1024*1024) {
        std::error_code ec; fs::remove(path.wstring()+L".1",ec); fs::rename(path,path.wstring()+L".1",ec);
    }
    std::ofstream(path,std::ios::app) << message << '\n';
}
bool EditorCore::isSelected(const std::string& id)const{return std::find(selectedIds_.begin(),selectedIds_.end(),id)!=selectedIds_.end();}
Json& EditorCore::entity(const std::string& id) {
    for (auto& e : document_["entities"]) if (e.at("id") == id) return e;
    throw std::runtime_error("Entity not found: " + id);
}
Json EditorCore::describeCommands() const {
    Json out = Json::array(); for (const auto& [id,c] : commands_) out.push_back(c.descriptor); return out;
}
Json EditorCore::search(const std::string& q) const {
    Json out = Json::array();
    for (const auto& [id,c] : commands_) if (matches(id+" "+c.descriptor.value("name",""),q)) out.push_back({{"kind","command"},{"id",id},{"label",c.descriptor.value("name",id)}});
    for (const auto& p : panels) if (matches(p.title,q)) out.push_back({{"kind","panel"},{"id",p.id},{"label",p.title}});
    for (const auto& e : entities()) if (matches(e.at("properties").at("name"),q)) out.push_back({{"kind","entity"},{"id",e.at("id")},{"label",e.at("properties").at("name")}});
    const auto schema = adapter_->propertySchema(Json::object());
    for (const auto& [key,s] : schema.at("properties").items()) if (matches(key+" "+s.value("title",key),q)) out.push_back({{"kind","property"},{"id",key},{"label",s.value("title",key)}});
    for (const auto& [key,s] : nodeTypes().items()) if (matches(key+" "+s.value("title",key),q)) out.push_back({{"kind","node"},{"id",key},{"label",s.value("title",key)}});
    return out;
}
Json EditorCore::execute(const std::string& id, const Json& p) {
    auto it = commands_.find(id); if (it == commands_.end()) throw std::runtime_error("Unknown command: " + id);
    validateValue(p,it->second.descriptor.at("inputSchema"));
    if(p.contains("expectedRevision")&&p.at("expectedRevision").get<uint64_t>()!=revision_)throw std::runtime_error("The editor changed; refresh the state before applying this edit");
    Snapshot before{document_,selected_,selectedIds_,it->second.descriptor.value("name",id)};
    try {
        auto result = it->second.run(p);
        validateValue(result,it->second.descriptor.at("outputSchema"),"outputs");
        if (it->second.undoable && before.document != document_) {
            undo_.push_back(before); if (undo_.size()>128) undo_.erase(undo_.begin()); redo_.clear();
        }
        if(before.document!=document_||before.ids!=selectedIds_)++revision_;
        if (id!="json_get" && id!="model_get" && id!="model_sample" && id!="asset_importers" && id!="asset_import_status" && id!="material_compile_status" && id!="build_status" && id!="build_describe" && id!="editor_status" && id!="scene_list" && id!="editor_search" && id!="graph_get" && id!="assets_list" && id!="edit_history" && id!="asset_trash_list" && id!="graph_debug_status") log(id+" | OK");
        return result;
    } catch (...) { document_=before.document; selected_=before.selected;selectedIds_=before.ids; throw; }
}
Json EditorCore::nativeCommand(const std::string& id, const Json& p) {
    if (id=="editor_status") {
        Json visible=Json::array();for(const auto& panel:panels)visible.push_back({{"id",panel.id},{"title",panel.title},{"owner",panel.owner},{"open",panel.open}});
        return {{"adapter",adapter_->descriptor()},{"selected",selected_},{"dirty",dirty()},{"canUndo",canUndo()},{"canRedo",canRedo()},{"commands",describeCommands()},{"panels",visible},{"revision",revision_}};
    }
    if(id.rfind("json_",0)==0)return jsonCommand(id,p);
    if(id.rfind("model_",0)==0)return modelCommand(id,p);
    if(id=="game_codec_describe")return {{"adapterId",adapter_->descriptor().at("id")},{"declarations",adapter_->assetCodecs()},{"note","Declarations are not proof of codec execution or game compatibility"}};
    if (id=="scene_list") return {{"entities",entities()},{"selected",selected_},{"selection",selectedIds_},{"revision",revision_},{"propertySchema",adapter_->propertySchema(Json::object())}};
    if(id.rfind("scene_",0)==0&&id!="scene_validate"&&id!="scene_list")return sceneCommand(id,p);
    if (id=="property_set") {
        auto& e=entity(p.at("entityId"));if(e.value("editorLocked",false))throw std::runtime_error("Object is locked"); std::string key=p.at("propertyId"); const auto schema=adapter_->propertySchema(e).at("properties");
        if (!schema.contains(key) || schema.at(key).value("readOnly",false)) throw std::runtime_error("Property unavailable: "+key);
        validateValue(p.at("value"),schema.at(key),key); e["properties"][key]=p.at("value"); return {{"entity",e}};
    }
    if (id=="edit_undo" || id=="edit_redo") {
        auto& from=id=="edit_undo"?undo_:redo_; auto& to=id=="edit_undo"?redo_:undo_;
        if (from.empty()) throw std::runtime_error("History is empty");
        to.push_back({document_,selected_,selectedIds_,from.back().label}); document_=from.back().document; selected_=from.back().selected;selectedIds_=from.back().ids; from.pop_back(); return {{"applied",true}};
    }
    if(id=="edit_history"){
        Json undo=Json::array(),redo=Json::array();for(auto it=undo_.rbegin();it!=undo_.rend();++it)undo.push_back(it->label);
        for(auto it=redo_.rbegin();it!=redo_.rend();++it)redo.push_back(it->label);return {{"undo",undo},{"redo",redo}};
    }
    if (id=="project_save") {
        adapter_->validateScene(entities()); for (const auto& g : graphs()) validateGraph(g);
        if(document_.dump().size()>7*1024*1024)throw std::runtime_error("Project exceeds the 7 MiB editable document budget");
        const auto path=root_/"data/projects/Sandbox.cyproject.json";
        writeJson(path,{{"format","cygame-project"},{"schemaVersion",1},{"adapterId",adapter_->descriptor().at("id")},{"document",document_}});
        saved_=document_; return {{"path",path.u8string()}};
    }
    if (id=="project_load") {
        auto doc=readJson(root_/"data/projects/Sandbox.cyproject.json");
        if (doc.value("format","")!="cygame-project" || doc.value("schemaVersion",0)!=1 || doc.value("adapterId","")!=adapter_->descriptor().at("id")) throw std::runtime_error("Unsupported project format/adapter");
        const auto& d=doc.at("document"); adapter_->validateScene(d.at("entities"));
        if (!d.at("graphs").is_object() || !d["graphs"].contains("logic") || !d["graphs"].contains("material") || d["graphs"].size()>1000) throw std::runtime_error("Invalid graph collection");
        for (const auto& g : d.at("graphs")) validateGraph(g);
        if(d.contains("models")){if(!d["models"].is_object()||d["models"].size()>8||d["models"].dump().size()>4*1024*1024)throw std::runtime_error("Invalid model collection");for(const auto& [key,model]:d["models"].items()){validateScene3D(model);validateAssetReferences({{"references",Json::array({model.at("assetPath")})}});if(model.at("assetId")!=key)throw std::runtime_error("Model identity mismatch");}}
        if(d.contains("jsonDocuments")){if(!d["jsonDocuments"].is_object()||d["jsonDocuments"].size()>4||d["jsonDocuments"].dump().size()>3*1024*1024)throw std::runtime_error("Invalid JSON document collection");for(const auto& [jsonPath,entry]:d["jsonDocuments"].items()){validateAssetReferences({{"references",Json::array({jsonPath})}});validateEditableJson(entry.at("value"));if(!entry.contains("value")||!entry.contains("savedHash")||!entry["savedHash"].is_string()||entry["value"].dump().size()>512*1024)throw std::runtime_error("Invalid JSON document state");}}
        document_=d;jsonFileHashes_.clear();modelFileHashes_.clear();savedModels_.clear(); saved_=document_; selected_=entities().empty()?"":entities()[0].at("id").get<std::string>(); selectedIds_=selected_.empty()?std::vector<std::string>{}:std::vector<std::string>{selected_};return {{"loaded",true}};
    }
    if (id=="scene_validate") return adapter_->validateScene(entities());
    if (id=="editor_search") return {{"results",search(p.at("query"))}};
    if (id=="viewport_new" && openPanel)return openPanel("viewport",true);
    if (id=="window_reset_layout" && resetLayout){resetLayout();return {{"reset",true}};}
    if (id=="editor_close" && requestClose){requestClose();return {{"requested",true}};}
    if (id=="panel_open" && openPanel) return openPanel(p.at("panelId"),p.value("newInstance",false));
    if (id=="plugin_set_enabled" && setPlugin) return setPlugin(p.at("pluginId"),p.at("enabled"));
    if (id=="viewport_focus" && focusEntity) { entity(p.at("entityId")); focusEntity(p.at("entityId")); return {{"focused",p.at("entityId")}}; }
    if(id=="build_describe"||id=="build_prepare")return buildCommand(id,p);
    if((id=="build_request"||id=="build_status"||id=="build_cancel")&&buildUi)return buildUi(id,p);
    if((id=="material_compile_request"||id=="material_compile_status"||id=="material_compile_cancel"||id=="material_preview_apply"||id=="material_compiler_view"||id=="material_preview_settings")&&materialUi)return materialUi(id,p);
    if(id.rfind("material_",0)==0)return materialCommand(id,p);
    if(id=="graph_diff_view"&&showGraphDiff)return showGraphDiff(p.at("graphId"));
    if(id=="graph_diff")return assetCommand(id,p);
    if(id=="texture_info")return readImage(root_,p.at("path"),false).info;
    if(id=="texture_view_settings"&&textureUi)return textureUi(p);
    if(id=="texture_open"&&showTexture)return showTexture(p.at("path"));
    if(id=="graph_find")return findGraphNodes(graphs(),p);
    if (id.rfind("graph_",0)==0) {
        const std::string key=p.at("graphId");const auto& graph=document_.at("graphs").at(key);
        if(id=="graph_diagnose")return graphDiagnostics(graphView(graph,p.value("functionName","")));
        if(id=="graph_function_evaluate")return evaluateFunction(graph,p.at("name"),p.value("arguments",Json::object()),textureSampler(root_));
        if(id=="graph_debug_view")throw std::runtime_error("The graph debugger view requires a graphical host");
        if(id.rfind("graph_debug_",0)==0){
            if(graph.at("domain")!="logic")throw std::runtime_error("Stepping is available for logic graphs");
            if(id=="graph_debug_stop"){debugSessions_.erase(key);return {{"state","Idle"}};}
            if(id=="graph_debug_start"){
                const auto diagnostics=graphDiagnostics(graph);if(!diagnostics.at("valid").get<bool>())throw std::runtime_error("Graph has validation errors");
                debugSessions_[key]=std::make_shared<GraphExecution>(graph,true);return debugSessions_[key]->status();
            }
            auto found=debugSessions_.find(key);if(found==debugSessions_.end()){if(id=="graph_debug_status")return {{"state","Idle"}};throw std::runtime_error("Start a debug session first");}
            if(!found->second->matchesProgram(graph))return {{"state","Stale"},{"error","Graph changed: restart debugging"}};
            found->second->refreshBreakpoints(graph);
            if(id=="graph_debug_status")return found->second->status();
            if(id=="graph_debug_tick")return found->second->advanceTime(p.at("delta"));
            if(id=="graph_debug_object_set")return found->second->editObject(p.at("object"),p.at("field"),p.at("value"));
            if(id=="graph_debug_event")return found->second->emit(p.at("event"));
            return found->second->advance(std::string(id=="graph_debug_continue"?"continue":id=="graph_debug_step_into"?"into":id=="graph_debug_step_out"?"out":"over"));
        }
        if (id=="graph_run") {
            auto result=graph.at("domain")=="material-instance"?resolveMaterial(root_,graphs(),key).at("preview"):graph.at("domain")=="material"?sampleMaterial(graph,.5,.5,textureSampler(root_)):evaluateGraph(graph);
            if (result.contains("messages")) for (const auto& message:result["messages"]) log(message.get<std::string>());
            return result;
        }
        return graphCommand(document_["graphs"],id,p);
    }

    if((id=="asset_import_request"||id=="asset_import_status"||id=="asset_import_cancel"||id=="asset_import_view")&&importUi)return importUi(id,p);
    if(id=="asset_importers"||id=="asset_import_prepare"||id=="asset_import_commit"||id=="asset_import_info"||id=="asset_import_check")return importCommand(id,p);
    if(id=="asset_reference_view"&&showReferences)return showReferences(p.at("path"));
    if(id=="asset_favorite"&&workspaceAction)return workspaceAction(id,p);
    if(id.rfind("asset_",0)==0||id=="assets_list")return assetCommand(id,p);
    if((id.rfind("workspace_",0)==0||id=="content_drawer_toggle")&&workspaceAction)return workspaceAction(id,p);
    throw std::runtime_error("Command unavailable in this host: "+id);
}
}
