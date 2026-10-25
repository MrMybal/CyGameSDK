#include "Graph.h"
#include <algorithm>
#include <set>
namespace cygame {
void validateMaterialInstance(const Json& graph){
    if(graph.at("domain")!="material-instance")throw std::runtime_error("Not a material instance");
    validateAssetReferences({{"references",Json::array({graph.at("parent")})}});
    if(!graph.at("overrides").is_object()||graph["overrides"].size()>256)throw std::runtime_error("Invalid material overrides");
    if(!graph.at("nodes").is_array()||!graph.at("links").is_array()||!graph["nodes"].empty()||!graph["links"].empty()||!graph.value("variables",Json::object()).empty()||!graph.value("functions",Json::object()).empty())throw std::runtime_error("Edit material topology in the parent graph");
}
namespace {
std::string fold(std::string value){for(auto& c:value)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));return value;}
fs::path contentPath(const fs::path& root,const std::string& path){
    validateAssetReferences({{"references",Json::array({path})}});const auto base=fs::weakly_canonical(root/"data/assets"),absolute=fs::weakly_canonical(base/fs::u8path(path));auto relative=absolute.lexically_relative(base);
    if(relative.empty()||relative.is_absolute()||*relative.begin()=="..")throw std::runtime_error("Parent must stay inside the content root");return absolute;
}
Json parentGraph(const fs::path& root,const Json& graphs,const std::string& path){
    const auto absolute=contentPath(root,path);if(!fs::is_regular_file(absolute))throw std::runtime_error("Material parent file is missing: "+path);
    const auto key=fold(absolute.generic_u8string());for(const auto& g:graphs)if(g.contains("assetPath")&&!g.value("assetMissing",false)&&fold(contentPath(root,g.at("assetPath")).generic_u8string())==key)return g;
    const auto file=readJson(absolute);if(file.value("format","")!="cygame-graph"||file.value("schemaVersion",0)!=1)throw std::runtime_error("Unsupported material parent format");return file.at("graph");
}
Json resolve(const fs::path& root,const Json& graphs,const Json& source,std::set<std::string>& visited,int depth,Json& parents){
    if(depth>8)throw std::runtime_error("Material inheritance exceeds eight parent links");
    validateGraph(source);if(source.at("domain")=="material")return source;
    if(source.at("domain")!="material-instance")throw std::runtime_error("Parent must be a material or material instance");
    const std::string path=source.at("parent"),key=fold(contentPath(root,path).generic_u8string());if(!visited.insert(key).second)throw std::runtime_error("Cyclic material inheritance");parents.push_back(path);
    auto graph=resolve(root,graphs,parentGraph(root,graphs,path),visited,depth+1,parents);
    const auto& overrides=source.at("overrides");for(const auto& [name,value]:overrides.items()){
        if(!graph.at("variables").contains(name))throw std::runtime_error("Unknown or removed material parameter: "+name);graph["variables"][name]["default"]=value;
    }validateGraph(graph);return graph;
}
}
Json resolveMaterial(const fs::path& root,const Json& graphs,const std::string& id,bool includePreview){
    const auto& source=graphs.at(id);std::set<std::string> visited;if(source.contains("assetPath"))visited.insert(fold(contentPath(root,source.at("assetPath")).generic_u8string()));Json parents=Json::array();
    auto effective=resolve(root,graphs,source,visited,0,parents);auto inherited=effective;
    if(source.at("domain")=="material-instance"){
        Json clean=source;clean["overrides"]=Json::object();std::set<std::string> seen;Json unused=Json::array();inherited=resolve(root,graphs,clean,seen,0,unused);
    }
    Json parameters=Json::array();const auto variables=effective.value("variables",Json::object());
    for(const auto& [name,v]:variables.items())parameters.push_back({{"name",name},{"type",v.at("type")},{"value",v.at("default")},{"inherited",inherited.at("variables").at(name).at("default")},{"overridden",source.value("overrides",Json::object()).contains(name)}});
    Json result={{"graphId",id},{"parameters",parameters},{"parents",parents},{"graph",effective}};if(includePreview)result["preview"]=sampleMaterial(effective,.5,.5,textureSampler(root));return result;
}
Json EditorCore::materialCommand(const std::string& id,const Json& p){
    const std::string key=p.at("graphId");auto& graph=document_["graphs"].at(key);
    if(id=="material_prepare")return prepareMaterial(root_,graphs(),key);
    if(id=="material_sample"){auto effective=resolveMaterial(root_,graphs(),key,false).at("graph");return sampleMaterial(effective,p.value("u",.5),p.value("v",.5),textureSampler(root_));}
    if(id=="material_get")return resolveMaterial(root_,graphs(),key);
    if(graph.at("domain")!="material-instance")throw std::runtime_error("Parameter overrides belong to a material instance");
    if(id=="material_override_remove"){
        const std::string name=p.at("name");if(!graph["overrides"].contains(name))throw std::runtime_error("Override not found");graph["overrides"].erase(name);return {{"overrides",graph["overrides"]}};
    }
    if(id=="material_override_set")graph["overrides"][p.at("name").get<std::string>()]=p.at("value");
    else if(id=="material_parent_set")graph["parent"]=p.at("parent");else throw std::runtime_error("Unknown material operation");
    return resolveMaterial(root_,graphs(),key);
}
}
