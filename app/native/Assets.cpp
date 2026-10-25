#include "Core.h"
#include "Graph.h"
#include "Imports.h"
#include "Scene3D.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
namespace cygame {
namespace {
fs::path inside(const fs::path& base,const std::string& relative,bool rootAllowed=false){
    const auto root=fs::weakly_canonical(base),path=fs::weakly_canonical(root/fs::u8path(relative));
    if(path.extension()==".cyimport")throw std::runtime_error("Import metadata is managed with its asset");
    const auto rel=path.lexically_relative(root);
    if(rel.empty()||rel.is_absolute()||(!rel.empty()&&*rel.begin()=="..")||(!rootAllowed&&path==root))throw std::runtime_error("Path must stay inside the content root");
    return path;
}
Json comparable(Json g){g.erase("assetPath");g.erase("assetMissing");return g;}
bool queryMatch(const std::string& query,const std::string& path,const std::string& type,const std::string& domain,bool imported=false){
    std::istringstream stream(query);std::string word;std::vector<std::string> terms;bool quote=false;
    for(char c:query){if(c=='"'){quote=!quote;continue;}if(std::isspace(static_cast<unsigned char>(c))&&!quote){if(!word.empty()){terms.push_back(word);word.clear();}}else word+=c;}
    if(!word.empty())terms.push_back(word);
    for(auto term:terms){
        bool negative=term.size()>1&&term[0]=='-';if(negative)term.erase(0,1);
        auto colon=term.find(':');std::string key=colon==std::string::npos?"":term.substr(0,colon),needle=colon==std::string::npos?term:term.substr(colon+1),hay=path;
        if(key=="type")hay=type;else if(key=="domain")hay=domain;else if(key=="imported")hay=imported?"true":"false";else if(key=="name")hay=fs::u8path(path).stem().u8string();else if(key=="ext")hay=fs::u8path(path).extension().u8string();
        else if(!key.empty()&&key!="path")throw std::runtime_error("Unknown search field: "+key);
        if(matches(hay,needle)==negative)return false;
    }return true;
}
}
bool EditorCore::graphDirty(const std::string& id)const{
    if(!graphs().contains(id))return false;
    const auto& g=graphs().at(id);auto saved=savedGraphs_.find(id);
    if(saved!=savedGraphs_.end())return comparable(g)!=comparable(saved->second);
    if(g.contains("assetPath")){
        try{return comparable(g)!=comparable(readJson(inside(root_/"data/assets",g.at("assetPath"))).at("graph"));}catch(...){return true;}
    }return comparable(g)!=comparable(initialGraphs().at(g.at("domain").get<std::string>()));
}
Json EditorCore::assetCommand(const std::string& id,const Json& p){
    const auto base=root_/"data/assets";fs::create_directories(base);
    if((id=="asset_move"||id=="asset_trash")&&p.contains("path")){
        const auto selected=inside(base,p.at("path"));auto isSelected=[&](const std::string& relative){std::error_code error;return fs::equivalent(inside(base,relative),selected,error)&&!error;};
        if(document_.contains("jsonDocuments"))for(const auto& [path,doc]:document_["jsonDocuments"].items())if(isSelected(path))throw std::runtime_error("Close the JSON document before moving its asset");
        for(const auto& model:models())if(isSelected(model.at("assetPath")))throw std::runtime_error("Close the 3D document before moving or deleting its asset");
        std::function<bool(const Json&)> used=[&](const Json& body){for(const auto& node:body.value("nodes",Json::array()))if(node.value("type","")=="material.texture"&&isSelected(node.at("values").at("path")))return true;if(body.contains("functions"))for(const auto& f:body["functions"])if(used(f))return true;return false;};for(const auto& graph:graphs())if(used(graph))throw std::runtime_error("Asset is referenced by a loaded material texture node");
    }
    if(id=="graph_diff"){
        const std::string key=p.at("graphId");const auto& live=graphs().at(key);const auto relative=live.value("assetPath","graphs/"+key+".cyasset");
        const auto path=inside(base,relative);Json baseline;std::string source="Session";bool external=false;
        if(fs::is_regular_file(path)){
            const auto file=readJson(path);if(file.value("format","")!="cygame-graph"||file.value("schemaVersion",0)!=1)throw std::runtime_error("Unsupported saved graph format");
            baseline=file.at("graph");validateGraph(baseline);source="Disk";
            if(savedGraphs_.count(key))external=comparable(baseline)!=comparable(savedGraphs_.at(key));
        }else if(live.contains("assetPath"))throw std::runtime_error("Saved asset file is missing");
        else baseline=savedGraphs_.at(key);
        auto report=compareGraphDocuments(baseline,live,p.value("ignorePresentation",false));
        report["graphId"]=key;report["revision"]=revision_;report["baselineSource"]=source;report["path"]=source=="Disk"?relative:"";report["externalChanges"]=external;return report;
    }

    if(id=="asset_dependencies")return assetDependencyReport(root_,graphs(),p.at("path"));
    if(id=="asset_references_set"){
        const std::string key=p.at("graphId");auto& graph=document_["graphs"].at(key);graph["references"]=p.at("paths");validateGraph(graph);
        for(const auto& ref:graph["references"])inside(base,ref);
        return {{"references",graph["references"]}};
    }
    auto loaded=[&](const std::string& path)->std::string{for(const auto& [key,g]:graphs().items())if(g.value("assetPath","")==path)return key;return "";};
    // File operations are persistent actions, separate from document Undo.
    // Keep their references valid in every retained snapshot as well.
    auto updateReferences=[&](const std::string& oldPath,const std::string& newPath,int missing){
        auto folded=[](std::string value){for(auto& c:value)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));return value;};
        auto graph=[&](Json& g){
            // A persistent rename also updates references retained in Undo/Redo.
            // Otherwise a later Undo could resurrect a path that no longer exists.
            if(oldPath!=newPath&&g.value("domain","")=="material-instance"&&folded(g.at("parent").get<std::string>())==folded(oldPath))g["parent"]=newPath;
            if(oldPath!=newPath&&g.contains("references"))for(auto& ref:g["references"])if(folded(ref.get<std::string>())==folded(oldPath))ref=newPath;
            if(g.value("assetPath","")!=oldPath)return;g["assetPath"]=newPath;if(missing==1)g["assetMissing"]=true;else if(missing==0)g.erase("assetMissing");
        };
        auto document=[&](Json& d){for(auto& g:d["graphs"])graph(g);};
        document(document_);document(saved_);for(auto& snap:undo_)document(snap.document);for(auto& snap:redo_)document(snap.document);
        for(auto& [key,g]:savedGraphs_)graph(g);
    };
    auto clean=[&](const std::string& path){auto key=loaded(path);if(!key.empty()&&graphDirty(key))throw std::runtime_error("Save or reload this asset before moving it or sending it to Trash");};
    if(id=="asset_save_graph"){
        std::string key=p.at("graphId");const auto& g=graphs().at(key);validateGraph(g);
        if(g.value("assetMissing",false))throw std::runtime_error("Restore this asset from Trash before saving it");
        auto path=inside(base,g.value("assetPath","graphs/"+key+".cyasset"));
        writeJson(path,{{"format","cygame-graph"},{"schemaVersion",1},{"graph",g}});
        savedGraphs_[key]=g;return {{"path",path.u8string()}};
    }
    if(id=="asset_new_material_instance"){
        const std::string name=p.at("name");for(char c:name)if(!std::isalnum(static_cast<unsigned char>(c))&&c!='_'&&c!='-')throw std::runtime_error("Asset names use letters, numbers, underscore or dash");
        const auto relative=(fs::u8path(p.value("folder","materials"))/(name+".cyasset")).generic_u8string(),key="asset:"+relative;auto path=inside(base,relative);
        if(fs::exists(path)||graphs().contains(key))throw std::runtime_error("Asset already exists");
        Json graph={{"domain","material-instance"},{"assetPath",relative},{"parent",p.at("parent")},{"overrides",Json::object()},{"variables",Json::object()},{"nodes",Json::array()},{"links",Json::array()}};
        document_["graphs"][key]=graph;resolveMaterial(root_,graphs(),key);writeJson(path,{{"format","cygame-graph"},{"schemaVersion",1},{"graph",graph}});savedGraphs_[key]=graph;return {{"graphId",key},{"path",relative}};
    }
    if(id=="asset_new_graph"){
        std::string name=p.at("name"),domain=p.at("domain");
        for(auto c:name)if(!std::isalnum(static_cast<unsigned char>(c))&&c!='_'&&c!='-')throw std::runtime_error("Asset names use letters, numbers, underscore or dash");
        auto folder=p.value("folder","graphs");auto relative=(fs::u8path(folder)/(name+".cyasset")).generic_u8string(),key="asset:"+relative;
        auto path=inside(base,relative);if(fs::exists(path)||graphs().contains(key))throw std::runtime_error("Asset already exists");
        Json g=initialGraphs().at(domain);g["assetPath"]=relative;
        writeJson(path,{{"format","cygame-graph"},{"schemaVersion",1},{"graph",g}});
        document_["graphs"][key]=g;savedGraphs_[key]=g;return {{"graphId",key},{"path",relative}};
    }
    if(id=="asset_open"||id=="asset_reload"){
        const std::string relative3d=p.at("path");if(fs::u8path(relative3d).extension()==".json"){auto opened=jsonCommand("json_open",{{"path",relative3d}});return {{"path",relative3d},{"jsonId",opened.at("jsonId")},{"editor","json"}};}if(fs::u8path(relative3d).extension()==".cy3d"){auto opened=modelCommand("model_open",{{"path",relative3d}});return {{"path",relative3d},{"modelId",opened.at("modelId")},{"editor","model"}};}
        auto path=inside(base,p.at("path"));if(!fs::is_regular_file(path))throw std::runtime_error("Asset file not found");
        auto relative=path.lexically_relative(fs::weakly_canonical(base)).generic_u8string(),key=loaded(relative);
        if(path.extension()==".cyasset"){
            if(!key.empty()&&id=="asset_open")return {{"graphId",key},{"path",relative}};
            if(!key.empty()&&graphDirty(key)&&!p.value("discardChanges",false))throw std::runtime_error("Reload would discard unsaved asset changes");
            auto a=readJson(path);if(a.value("format","")!="cygame-graph"||a.value("schemaVersion",0)!=1)throw std::runtime_error("Unsupported asset format");
            validateGraph(a.at("graph"));if(key.empty())key="asset:"+relative;
            Json g=a.at("graph");g["assetPath"]=relative;g.erase("assetMissing");
            document_["graphs"][key]=g;savedGraphs_[key]=g;return {{"graphId",key},{"path",relative}};
        }
        return {{"path",relative},{"extension",path.extension().u8string()},{"bytes",fs::file_size(path)},{"editor","metadata"}};
    }
    if(id=="asset_folder_create"){
        auto path=inside(base,p.at("path"));if(fs::exists(path))throw std::runtime_error("Folder already exists");
        fs::create_directories(path);return {{"path",path.lexically_relative(fs::weakly_canonical(base)).generic_u8string()}};
    }
    if(id=="asset_copy"||id=="asset_move"||id=="asset_trash"){
        auto source=inside(base,p.at("path"));if(!fs::is_regular_file(source))throw std::runtime_error("Asset file not found");
        auto relative=source.lexically_relative(fs::weakly_canonical(base)).generic_u8string(),key=loaded(relative);
        if(id!="asset_copy"){
            clean(relative);const auto dependencies=assetDependencyReport(root_,graphs(),relative);
            if(dependencies.at("incomplete").get<bool>())throw std::runtime_error("Reference scan incomplete: inspect Reference Viewer before changing files");
            if(!dependencies.at("referencers").empty())throw std::runtime_error("Asset is referenced: update and save its consumers before moving it to another path or Trash");
        }
        if(id=="asset_trash"){
            const auto ticket=uniqueId();auto directory=root_/"data/trash"/ticket;
            auto destination=inside(directory/"files",relative);fs::create_directories(destination.parent_path());
            writeJson(directory/"ticket.json",{{"ticket",ticket},{"path",relative}});
            try{transferAssetPair(source,destination,false);}catch(...){std::error_code ec;fs::remove(directory/"ticket.json",ec);throw;}
            updateReferences(relative,relative,1);
            return {{"ticket",ticket},{"path",relative}};
        }
        auto target=inside(base,p.at("destination"));if(fs::exists(target))throw std::runtime_error("Destination already exists");
        if(id=="asset_copy"&&fs::file_size(source)>64*1024*1024)throw std::runtime_error("Files above 64 MiB require an asynchronous import provider");
        fs::create_directories(target.parent_path());auto targetRelative=target.lexically_relative(fs::weakly_canonical(base)).generic_u8string();
        if(id=="asset_copy"){
            if(source.extension()==".cy3d"){auto duplicate=readJson(source);validateScene3D(duplicate);auto identity=uniqueId();for(auto& c:identity)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));duplicate["assetId"]=identity;duplicate["name"]=target.stem().u8string();writeJson(target,duplicate);}
            else transferAssetPair(source,target,true);
        }
        else{
            transferAssetPair(source,target,false);
            updateReferences(relative,targetRelative,-1);
        }
        return {{"path",targetRelative}};
    }
    if(id=="asset_restore"){
        std::string ticket=p.at("ticket");for(char c:ticket)if(!std::isalnum(static_cast<unsigned char>(c))&&c!='-')throw std::runtime_error("Invalid Trash ticket");
        auto directory=inside(root_/"data/trash",ticket);auto record=readJson(directory/"ticket.json");
        const std::string relative=record.at("path");auto target=inside(base,relative),source=inside(directory/"files",relative);
        if(!fs::is_regular_file(source)||fs::exists(target))throw std::runtime_error("Cannot restore asset: source missing or destination exists");
        fs::create_directories(target.parent_path());transferAssetPair(source,target,false);
        updateReferences(relative,relative,0);
        std::error_code ec;fs::remove(directory/"ticket.json",ec);return {{"path",relative}};
    }
    if(id=="asset_trash_list"){
        Json records=Json::array();auto trash=root_/"data/trash";fs::create_directories(trash);
        for(const auto& e:fs::directory_iterator(trash)){
            if(e.is_symlink()||!e.is_directory())continue;
            try{auto ticket=readJson(e.path()/"ticket.json");if(fs::is_regular_file(inside(e.path()/"files",ticket.at("path"))))records.push_back(ticket);}catch(...){}
        }return {{"tickets",records}};
    }
    if(id=="assets_list"){
        Json results=Json::array(),folders=Json::array();std::vector<std::string> paths;size_t scanned=0;bool truncated=false;
        for(fs::recursive_directory_iterator it(base,fs::directory_options::skip_permission_denied),end;it!=end;++it){
            if(++scanned>50000){truncated=true;break;}
            if(it->is_symlink()){if(it->is_directory())it.disable_recursion_pending();continue;}
            auto path=it->path().lexically_relative(fs::weakly_canonical(base)).generic_u8string();
            if(it->is_directory()){paths.push_back(path);continue;}if(!it->is_regular_file())continue;
            auto ext=it->path().extension().u8string();for(auto& c:ext)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            if(ext==".cyimport")continue;
            std::string kind="Other",domain="";
            if(ext==".cyasset"){kind="Graph";try{domain=readJson(it->path()).at("graph").value("domain","");}catch(...){}}
            else if(ext==".png"||ext==".jpg"||ext==".jpeg"||ext==".bmp"||ext==".dds"||ext==".tga")kind="Texture";
            else if(ext==".cy3d"||ext==".cypack"||ext==".fbx"||ext==".obj"||ext==".gltf"||ext==".glb")kind="Mesh";
            else if(ext==".wav"||ext==".ogg"||ext==".mp3")kind="Audio";
            if((p.value("type","All")=="All"||p.at("type")==kind)&&queryMatch(p.value("query",""),path,kind,domain,fs::is_regular_file(importSidecar(it->path()))))
                results.push_back({{"path",path},{"type",kind},{"bytes",it->file_size()},{"domain",domain},{"extension",ext},{"imported",fs::is_regular_file(importSidecar(it->path()))}});
            if(results.size()>=5000){truncated=true;break;}
        }
        std::sort(paths.begin(),paths.end());for(const auto& path:paths)folders.push_back(path);
        std::sort(results.begin(),results.end(),[](const Json& a,const Json& b){return a.at("path").get<std::string>()<b.at("path").get<std::string>();});
        return {{"assets",results},{"folders",folders},{"truncated",truncated}};
    }
    throw std::runtime_error("Unknown asset command: "+id);
}
}
