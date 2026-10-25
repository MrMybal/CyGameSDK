#include "Core.h"
#include "Scene3D.h"
#include "Graph.h"
#include <algorithm>
#include <cmath>
namespace cygame {
namespace {
fs::path modelFile(const fs::path& root,const std::string& path){auto base=fs::weakly_canonical(root/"data/assets"),file=fs::weakly_canonical(base/fs::u8path(path)),rel=file.lexically_relative(base);if(rel.empty()||rel.is_absolute()||*rel.begin()==".."||file.extension()!=L".cy3d")throw std::runtime_error("Choose a .cy3d document inside the content root");return file;}
}
const Json& EditorCore::models()const{static const Json empty=Json::object();return document_.contains("models")?document_.at("models"):empty;}
Json EditorCore::modelCommand(const std::string& id,const Json& p){
    if(id=="model_process_prepare"||id=="model_process_commit"||id=="model_lod_remove"||id=="model_collision_set"||id=="model_collision_remove")return modelAuthoringCommand(id,p);
    if(id=="model_list"){Json list=Json::array();for(const auto& [key,m]:models().items())list.push_back({{"modelId",key},{"name",m.at("name")},{"path",m.at("assetPath")}});return {{"models",list}};}
    if(!document_.contains("models"))document_["models"]=Json::object();auto& models=document_["models"];
    if(id=="model_open"){
        auto file=modelFile(root_,p.at("path"));const auto relative=file.lexically_relative(fs::weakly_canonical(root_/"data/assets")).generic_u8string();auto model=readJson(file);validateScene3D(model);const std::string key=model.at("assetId");
        if(models.contains(key)){if(models[key].value("assetPath","")!=relative)throw std::runtime_error("Asset ID conflict: duplicate or update explicitly");}
        else{if(models.size()>=8)throw std::runtime_error("Close a 3D document before opening more than eight");model["assetPath"]=relative;if(models.dump().size()+model.dump().size()>4*1024*1024)throw std::runtime_error("Open model budget is 4 MiB; close unused model documents");model["_loadedHash"]=sha256(readJson(file).dump());models[key]=model;savedModels_[key]=model;savedModels_[key].erase("assetPath");savedModels_[key].erase("_loadedHash");modelFileHashes_[key]=model["_loadedHash"];}
        return {{"modelId",key},{"nodeCount",models[key]["nodes"].size()},{"meshCount",models[key]["meshes"].size()},{"animationCount",models[key]["animations"].size()}};
    }
    const std::string key=p.at("modelId");auto& model=models.at(key);
    if(id=="model_reload"){
        auto file=modelFile(root_,model.at("assetPath"));auto clean=model;clean.erase("assetPath");clean.erase("_loadedHash");if(savedModels_.count(key)?clean!=savedModels_.at(key):sha256(clean.dump())!=model.value("_loadedHash",""))throw std::runtime_error("Save or undo 3D edits before reloading");auto fresh=readJson(file);validateScene3D(fresh);if(fresh.at("assetId")!=key)throw std::runtime_error("Reloaded portable identity changed");fresh["assetPath"]=model.at("assetPath");fresh["_loadedHash"]=sha256(readJson(file).dump());model=fresh;savedModels_[key]=fresh;savedModels_[key].erase("assetPath");savedModels_[key].erase("_loadedHash");modelFileHashes_[key]=fresh["_loadedHash"];return {{"reloaded",true}};
    }
    if(id=="model_get"||id=="model_export_prepare")return {{"model",model}};
    if(id=="model_sample"){
        const auto frame=sampleScene3D(model,p.value("animation",-1),p.value("time",0.f),p.value("lod",0));Json nodes=Json::array(),positions=Json::array();for(const auto& n:frame.nodes)nodes.push_back(n);for(size_t i=0;i<std::min(size_t(1024),frame.triangles.size());++i){const auto& v=frame.triangles[i];positions.push_back({v.position[0],v.position[1],v.position[2]});}return {{"nodes",nodes},{"positions",positions},{"triangleCount",frame.triangles.size()/3},{"minimum",frame.minimum},{"maximum",frame.maximum}};
    }
    if(id=="model_clip_create"){
        if(model["animations"].size()>=128)throw std::runtime_error("Animation clip limit reached");Json clip={{"name",p.at("name")},{"tracks",Json::array()}};if(p.contains("copyFrom")){clip=model["animations"].at(p.at("copyFrom").get<size_t>());clip["name"]=p.at("name");}model["animations"].push_back(clip);validateScene3D(model);return {{"animation",model["animations"].size()-1},{"modelId",key}};
    }else if(id=="model_track_add"){
        const size_t node=p.at("node");const std::string channel=p.at("path");const auto& n=model["nodes"].at(node);if(n.contains("matrix"))throw std::runtime_error("Animation tracks require TRS nodes");auto& clip=model["animations"].at(p.at("animation").get<size_t>());clip["tracks"].push_back({{"node",node},{"path",channel},{"interpolation","LINEAR"},{"times",Json::array({0.})},{"values",Json::array({n.at(channel)})}});validateScene3D(model);return {{"track",clip["tracks"].size()-1},{"modelId",key}};
    }else if(id=="model_key_add"||id=="model_key_remove"){
        auto& track=model["animations"].at(p.at("animation").get<size_t>())["tracks"].at(p.at("track").get<size_t>());auto& times=track["times"];auto& values=track["values"];const size_t factor=track.at("interpolation")=="CUBICSPLINE"?3:1;
        if(id=="model_key_remove"){size_t index=p.at("key");if(times.size()<=1||index>=times.size())throw std::runtime_error("A track must keep at least one key");times.erase(times.begin()+index);values.erase(values.begin()+index*factor,values.begin()+(index+1)*factor);}
        else{double time=p.at("time");size_t index=0;while(index<times.size()&&times[index].get<double>()<time)++index;if(index<times.size()&&times[index]==p.at("time"))throw std::runtime_error("A key already exists at this time");auto value=p.at("value");times.insert(times.begin()+index,p.at("time"));if(factor==1)values.insert(values.begin()+index,value);else{Json tangent=Json::array();for(size_t i=0;i<value.size();++i)tangent.push_back(0.);Json added=Json::array({tangent,value,tangent});values.insert(values.begin()+index*3,added.begin(),added.end());}}
    }else if(id=="model_node_set"){
        auto& node=model["nodes"].at(p.at("node").get<size_t>());for(const auto* field:{"name","parent","translation","rotation","scale"})if(p.contains(field)){if(node.contains("matrix")&&std::string(field)!="name"&&std::string(field)!="parent")throw std::runtime_error("Matrix nodes require explicit TRS decomposition before transform editing");node[field]=p[field];}
    }else if(id=="model_vertex_set"){
        auto& primitive=model["meshes"].at(p.at("mesh").get<size_t>())["primitives"].at(p.at("primitive").get<size_t>());const size_t vertex=p.at("vertex");
        for(const auto* field:{"positions","normals","uv","joints","weights"})if(p.contains(field))primitive.at(field).at(vertex)=p[field];
    }else if(id=="model_key_set"){
        auto& clip=model["animations"].at(p.at("animation").get<size_t>());auto& track=clip["tracks"].at(p.at("track").get<size_t>());const size_t index=p.at("key");
        if(p.contains("time"))track["times"].at(index)=p["time"];
        if(p.contains("value"))track["values"].at(track.at("interpolation")=="CUBICSPLINE"?index*3+1:index)=p["value"];
    }else if(id=="model_save"){
        validateScene3D(model);auto data=model;data.erase("assetPath");data.erase("_loadedHash");const auto file=modelFile(root_,model.at("assetPath"));const auto expected=modelFileHashes_.count(key)?modelFileHashes_[key]:model.value("_loadedHash","");if(sha256(readJson(file).dump())!=expected)throw std::runtime_error("3D asset changed on disk; reload or preserve external edits");writeJson(file,data);savedModels_[key]=data;modelFileHashes_[key]=sha256(data.dump());model["_loadedHash"]=modelFileHashes_[key];for(auto* history:{&undo_,&redo_})for(auto& state:*history)if(state.document.contains("models")&&state.document["models"].contains(key))state.document["models"][key]["_loadedHash"]=modelFileHashes_[key];return {{"path",model.at("assetPath")},{"saved",true}};
    }else if(id=="model_close"){
        auto saved=readJson(modelFile(root_,model.at("assetPath")));auto compare=model;compare.erase("assetPath");compare.erase("_loadedHash");if(compare!=saved)throw std::runtime_error("Save 3D document changes before closing");models.erase(key);return {{"closed",true}};
    }else throw std::runtime_error("Unknown model operation: "+id);
    auto normalize=[](Json& value){double length=0;for(const auto& v:value)length+=v.get<double>()*v.get<double>();if(!std::isfinite(length)||length<1e-12)throw std::runtime_error("Vector cannot be zero");length=std::sqrt(length);for(auto& v:value)v=v.get<double>()/length;};
    if(id=="model_node_set"&&p.contains("rotation"))normalize(model["nodes"][p.at("node").get<size_t>()]["rotation"]);
    if((id=="model_key_set"||id=="model_key_add")&&p.contains("value")){auto& track=model["animations"][p.at("animation").get<size_t>()]["tracks"][p.at("track").get<size_t>()];if(track.at("path")=="rotation"){size_t index=id=="model_key_add"?0:p.at("key").get<size_t>();if(id=="model_key_add")while(index<track["times"].size()&&track["times"][index]!=p.at("time"))++index;normalize(track["values"][track.at("interpolation")=="CUBICSPLINE"?index*3+1:index]);}}
    if(id=="model_vertex_set"){auto& vertex=model["meshes"][p.at("mesh").get<size_t>()]["primitives"][p.at("primitive").get<size_t>()];const size_t index=p.at("vertex");if(p.contains("normals"))normalize(vertex["normals"][index]);if(p.contains("weights")){auto& weights=vertex["weights"][index];double sum=0;for(const auto& value:weights){double w=value.get<double>();if(!std::isfinite(w)||w<0)throw std::runtime_error("Invalid skin weight");sum+=w;}if(sum<1e-12)throw std::runtime_error("Skin weights cannot be zero");for(auto& value:weights)value=value.get<double>()/sum;}}
    if(model.contains("extensions")&&model["extensions"].contains("org.cygame.editor")){
        auto& meta=model["extensions"]["org.cygame.editor"];const bool geometry=id=="model_vertex_set",pose=id=="model_node_set"&&(p.contains("parent")||p.contains("translation")||p.contains("rotation")||p.contains("scale"));
        if(geometry&&meta.contains("lods")){const auto mesh=std::to_string(p.at("mesh").get<int>());if(meta["lods"].contains(mesh))for(auto& level:meta["lods"][mesh])level["stale"]=true;}
        if((geometry||pose)&&meta.contains("collisions"))for(auto& shape:meta["collisions"])shape["needsReview"]=true;
    }
    validateScene3D(model);return {{"modelId",key},{"updated",true}};
}
}
