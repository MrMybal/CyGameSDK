#include "Graph.h"
#include <set>
namespace cygame {
void validateAssetReferences(const Json& graph){
    const auto refs=graph.value("references",Json::array());if(!refs.is_array()||refs.size()>256)throw std::runtime_error("Asset references must be an array of at most 256 paths");std::set<std::string> unique;
    for(const auto& v:refs){validateValue(v,{{"type","string"},{"minLength",1},{"maxLength",1024}});const std::string value=v;
        const auto path=fs::u8path(value);if(path.is_absolute()||value.find(':')!=std::string::npos||value.find('\\')!=std::string::npos||value.find('\0')!=std::string::npos||path.lexically_normal().generic_u8string()!=value)throw std::runtime_error("Use normalized paths relative to the content root");
        for(const auto& part:path)if(part==".."||part==".")throw std::runtime_error("Reference cannot leave the content root");
        if(!unique.insert(value).second)throw std::runtime_error("Duplicate asset reference");
    }
}
namespace {
Json allReferences(const Json& graph){Json refs=graph.value("references",Json::array());std::set<std::string> paths;for(const auto& ref:refs)paths.insert(ref.get<std::string>());std::function<void(const Json&)> visit=[&](const Json& body){for(const auto& node:body.value("nodes",Json::array()))if(node.value("type","")=="material.texture"){const std::string path=node.at("values").at("path");validateAssetReferences({{"references",Json::array({path})}});paths.insert(path);}if(body.contains("functions"))for(const auto& f:body["functions"])visit(f);};visit(graph);return Json(paths);}
}
Json assetDependencyReport(const fs::path& root,const Json& graphs,const std::string& selected){
    const auto base=fs::weakly_canonical(root/"data/assets");fs::create_directories(base);
    validateAssetReferences({{"references",Json::array({selected})}});
    auto exists=[&](const std::string& p){const auto absolute=fs::weakly_canonical(base/fs::u8path(p));auto relative=absolute.lexically_relative(base);return !relative.empty()&&!relative.is_absolute()&&*relative.begin()!=".."&&fs::is_regular_file(absolute);};
    std::map<std::string,std::string> display;
    auto canonical=[&](const std::string& value){const auto absolute=fs::weakly_canonical(base/fs::u8path(value));const auto relative=absolute.lexically_relative(base);if(relative.empty()||relative.is_absolute()||*relative.begin()=="..")throw std::runtime_error("Reference resolves outside the content root");const auto path=relative.generic_u8string();auto key=path;for(auto& c:key)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));display.emplace(key,path);return key;};
    const auto selectedKey=canonical(selected);
    std::map<std::string,std::set<std::string>> edges;Json errors=Json::array();size_t scanned=0,files=0,bytes=0;bool incomplete=false;
    for(fs::recursive_directory_iterator it(base,fs::directory_options::skip_permission_denied),end;it!=end;++it){
        if(++scanned>10000){incomplete=true;break;}
        if(it->is_symlink()){if(it->is_directory())it.disable_recursion_pending();continue;}if(!it->is_regular_file()||it->path().extension()!=".cyasset")continue;
        const auto path=it->path().lexically_relative(base).generic_u8string();const auto size=it->file_size();bytes+=size;
        if(++files>1024||bytes>16*1024*1024){incomplete=true;break;}
        try{
            if(size>1024*1024)throw std::runtime_error("Reference scan requires assets below 1 MiB");
            const auto data=readJson(it->path());if(data.value("format","")!="cygame-graph"||data.value("schemaVersion",0)!=1)throw std::runtime_error("Unknown asset format");
            const auto& graph=data.at("graph");validateAssetReferences(graph);edges[canonical(path)];
            for(const auto& ref:allReferences(graph))edges[canonical(path)].insert(canonical(ref));if(graph.value("domain","")=="material-instance")edges[canonical(path)].insert(canonical(graph.at("parent")));
        }catch(const std::exception& e){errors.push_back({{"path",path},{"message",e.what()}});incomplete=true;}
    }
    // Union saved and live references. An unsaved edit cannot make disk consumers disappear.
    for(const auto& graph:graphs)if(graph.contains("assetPath")&&!graph.value("assetMissing",false)){
        const std::string path=graph.at("assetPath");validateAssetReferences(graph);edges[canonical(path)];for(const auto& ref:allReferences(graph))edges[canonical(path)].insert(canonical(ref));if(graph.value("domain","")=="material-instance")edges[canonical(path)].insert(canonical(graph.at("parent")));
    }
    Json dependencies=Json::array(),referencers=Json::array(),missing=Json::array();
    for(const auto& ref:edges[selectedKey]){const auto path=display.at(ref);const bool found=exists(path);dependencies.push_back({{"path",path},{"exists",found}});if(!found)missing.push_back(path);}
    for(const auto& [path,refs]:edges)if(refs.count(selectedKey))referencers.push_back({{"path",display.at(path)},{"exists",exists(display.at(path))}});
    std::set<std::string> visited;std::vector<std::string> todo={selectedKey};Json transitive=Json::array();
    while(!todo.empty()&&visited.size()<4096){auto path=todo.back();todo.pop_back();if(!visited.insert(path).second)continue;for(const auto& ref:edges[path]){if(ref!=selectedKey&&!visited.count(ref)){todo.push_back(ref);}}}
    for(const auto& path:visited)if(path!=selectedKey)transitive.push_back(display.at(path));
    return {{"path",selected},{"exists",exists(selected)},{"dependencies",dependencies},{"referencers",referencers},{"missing",missing},{"transitive",transitive},{"incomplete",incomplete},{"errors",errors},{"scannedAssets",files},{"source","saved and live references"}};
}
}
