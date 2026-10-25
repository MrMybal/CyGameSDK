#include "Graph.h"
#include <algorithm>
namespace cygame {
Json EditorCore::buildCommand(const std::string& id,const Json& p){
    const auto targets=adapter_->buildTargets();if(id=="build_describe")return {{"adapterId",adapter_->descriptor().at("id")},{"targets",targets}};
    const std::string requested=p.at("targetId");Json target;
    for(const auto& candidate:targets)if(candidate.at("id")==requested)target=candidate;
    if(target.is_null())throw std::runtime_error("Build target is not provided by this adapter");
    const std::string configuration=p.value("configuration",target.at("configurations").at(0).get<std::string>());if(std::find(target.at("configurations").begin(),target.at("configurations").end(),configuration)==target.at("configurations").end())throw std::runtime_error("Unsupported build configuration");
    Json options=p.value("options",Json::object());for(const auto& [name,spec]:target.at("optionsSchema").at("properties").items())if(!options.contains(name)&&spec.contains("default"))options[name]=spec.at("default");validateValue(options,target.at("optionsSchema"));
    Json diagnostics=Json::array(),materialValues=Json::object();auto error=[&](const std::string& graph,const std::string& message){diagnostics.push_back({{"severity","Error"},{"graphId",graph},{"message",message}});};
    try{const auto scene=adapter_->validateScene(entities());if(!scene.value("valid",true))error("","Adapter scene validation failed");}catch(const std::exception& e){error("",e.what());}
    for(const auto& [key,graph]:graphs().items()){
        try{
            if(graph.value("assetMissing",false))throw std::runtime_error("Asset is in Trash");
            const auto report=graphDiagnostics(graph);for(auto entry:report.at("diagnostics")){entry["graphId"]=key;diagnostics.push_back(entry);}
            if(graph.at("domain")=="material"||graph.at("domain")=="material-instance")materialValues[key]=resolveMaterial(root_,graphs(),key).at("preview");
        }catch(const std::exception& e){error(key,e.what());}
    }
    Json snapshot={{"format","cygame-build-snapshot"},{"schemaVersion",1},{"adapterId",adapter_->descriptor().at("id")},{"revision",revision_},{"document",document_},{"materialValues",materialValues}};
    if(snapshot.dump().size()>2*1024*1024){error("","Build snapshot exceeds 2 MiB; use an adapter-specific asset pipeline");snapshot=nullptr;}
    bool valid=true;for(const auto& d:diagnostics)if(d["severity"]=="Error")valid=false;
    return {{"valid",valid},{"revision",revision_},{"target",target},{"configuration",configuration},{"options",options},{"diagnostics",diagnostics},{"snapshot",snapshot}};
}
}
