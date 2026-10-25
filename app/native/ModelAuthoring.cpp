#include "Scene3D.h"
#include "Graph.h"
#include <cmath>
#include <set>
namespace cygame {
namespace {
const char* authoringNamespace="org.cygame.editor";
void need(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void number(const Json& v,double minimum=0,double maximum=1e8){need(v.is_number()&&std::isfinite(v.get<double>())&&v.get<double>()>=minimum&&v.get<double>()<=maximum,"Invalid authoring number");}
void vector(const Json& v,size_t count,double minimum=-1e8){need(v.is_array()&&v.size()==count,"Invalid authoring vector");for(const auto& x:v)number(x,minimum);}
}
Json modelAtLod(const Json& model,int lod){
    need(lod>=0&&lod<=4,"LOD index must be 0..4");auto copy=model;if(!lod)return copy;
    const auto extensions=model.value("extensions",Json::object()),metadata=extensions.value(authoringNamespace,Json::object()),levels=metadata.value("lods",Json::object());
    for(size_t i=0;i<copy["meshes"].size();++i){const auto key=std::to_string(i);if(levels.contains(key)&&levels[key].size()>=size_t(lod)){need(!levels[key][lod-1].value("stale",false),"Generated LOD is stale; regenerate it after base mesh edits");copy["meshes"][i]["primitives"]=levels[key][lod-1]["primitives"];}}
    return copy;
}
void validateModelAuthoring(const Json& model){
    const auto extensions=model.value("extensions",Json::object());need(extensions.is_object(),"Model extensions must be namespaced metadata");if(!extensions.contains(authoringNamespace))return;
    const auto& meta=extensions.at(authoringNamespace);need(meta.is_object(),"Invalid authoring metadata");const auto lods=meta.value("lods",Json::object());need(lods.is_object()&&lods.size()<=model.at("meshes").size(),"Invalid mesh LOD collection");
    for(const auto& [key,levels]:lods.items()){
        int mesh=-1;try{mesh=std::stoi(key);}catch(...){throw std::runtime_error("Invalid LOD mesh index");}need(mesh>=0&&mesh<int(model.at("meshes").size())&&key==std::to_string(mesh),"Invalid LOD mesh index");need(levels.is_array()&&levels.size()<=4,"LOD limit is four per mesh");
        for(const auto& level:levels){if(level.contains("stale"))need(level["stale"].is_boolean(),"Invalid LOD state");need(level.is_object()&&level.contains("primitives"),"Invalid LOD geometry");auto variant=model;variant["extensions"].erase(authoringNamespace);variant["meshes"][mesh]["primitives"]=level["primitives"];validateScene3D(variant);for(const auto& node:model["nodes"])need(!(node.value("mesh",-1)==mesh&&node.contains("skin")),"Generated LODs require static meshes");}
    }
    const auto motions=meta.value("rootMotion",Json::array());need(motions.is_array()&&motions.size()<=32,"Root motion pair limit is 32");std::set<std::string> motionIds;
    for(const auto& r:motions){need(r.is_object(),"Invalid root motion record");const std::string identity=r.at("id");need(!identity.empty()&&identity.size()<=64&&motionIds.insert(identity).second,"Invalid root motion identity");for(const auto* field:{"source","inPlace","motion"}){need(r.at(field).is_number_integer(),"Invalid motion clip index");int index=r.at(field);need(index>=0&&index<int(model.at("animations").size()),"Motion clip does not exist");}need(r.at("inPlace")!=r.at("motion"),"Motion pair must contain distinct clips");need(r.at("root").is_number_integer(),"Invalid motion root");int root=r.at("root");need(root>=0&&root<int(model.at("nodes").size()),"Invalid motion root");for(const auto* field:{"inPlaceHash","motionHash"}){need(r.at(field).is_string(),"Invalid motion fingerprint");const std::string hash=r.at(field);need(hash.size()==64&&hash.find_first_not_of("0123456789abcdef")==std::string::npos,"Invalid motion fingerprint");}need(r.at("axes").is_array()&&r.at("axes").size()==3&&r.at("rotation").is_boolean(),"Invalid root motion extraction policy");for(const auto& axis:r.at("axes"))need(axis.is_boolean(),"Invalid root motion axis");number(r.at("fps"),1,120);}
    const auto shapes=meta.value("collisions",Json::array());need(shapes.is_array()&&shapes.size()<=64,"Collision limit is 64");std::set<std::string> ids;
    for(const auto& shape:shapes){const std::string id=shape.at("id"),kind=shape.at("shape");if(shape.contains("needsReview"))need(shape["needsReview"].is_boolean(),"Invalid collision review state");need(!id.empty()&&id.size()<=64&&ids.insert(id).second,"Invalid collision identity");need(shape.at("name").is_string()&&shape.at("name").get<std::string>().size()<=128&&shape.at("enabled").is_boolean(),"Invalid collision properties");vector(shape.at("center"),3);vector(shape.at("rotation"),4);double q=0;for(const auto& v:shape["rotation"])q+=v.get<double>()*v.get<double>();need(q>1e-10,"Collision quaternion cannot be zero");
        if(kind=="box"){vector(shape.at("halfExtents"),3,1e-8);}else if(kind=="sphere"||kind=="capsule"){number(shape.at("radius"),1e-8);if(kind=="capsule")number(shape.at("halfHeight"));}else throw std::runtime_error("Unknown collision shape");
    }
}
Json EditorCore::modelAuthoringCommand(const std::string& id,const Json& p){
    if(id=="model_process_commit"){
        const auto base=fs::weakly_canonical(root_),path=fs::weakly_canonical(fs::u8path(p.at("reportPath").get<std::string>())),relative=path.lexically_relative(base);need(!relative.empty()&&!relative.is_absolute()&&*relative.begin()!="..","Authoring report escapes the editor workspace");const auto report=readJson(path);need(report.value("format","")=="cygame-model-process"&&report.value("schemaVersion",0)==1,"Invalid authoring report");const std::string key=report.at("modelId");auto& current=document_.at("models").at(key);need(sha256(current.dump())==report.at("baseHash"),"Model changed while processing; retry on the current document");
        const std::string source=report.value("sourceId","");if(!source.empty())need(sha256(models().at(source).dump())==report.at("sourceHash"),"Source model changed while retargeting");const auto& next=report.at("model");need(next.at("assetId")==key&&next.at("source")==current.at("source")&&next.at("assetPath")==current.at("assetPath")&&next.at("_loadedHash")==current.at("_loadedHash"),"Authoring result changed protected asset provenance");validateScene3D(next);auto collection=models();collection[key]=next;need(collection.dump().size()<=4*1024*1024,"Loaded model budget is 4 MiB");current=next;return {{"modelId",key},{"applied",true},{"summary",report.at("summary")}};
    }
    const std::string key=p.at("modelId");auto& model=document_.at("models").at(key);
    if(id=="model_process_prepare"){
        Json result={{"model",model},{"baseHash",sha256(model.dump())},{"parameters",p}};if(p.value("action","")=="retarget"){const std::string source=p.at("sourceModelId");result["source"]=models().at(source);result["sourceHash"]=sha256(result["source"].dump());}need(result.dump().size()<=3*1024*1024,"Combined authoring snapshot exceeds 3 MiB");return result;
    }
    auto& meta=model["extensions"][authoringNamespace];if(meta.is_null())meta=Json::object();
    if(id=="model_lod_remove"){auto& levels=meta.at("lods").at(std::to_string(p.at("mesh").get<int>()));size_t level=p.at("lod");need(level>0&&level<=levels.size(),"Choose a generated LOD");levels.erase(levels.begin()+level-1);}
    else if(id=="model_collision_set"||id=="model_collision_remove"){
        auto& shapes=meta.at("collisions");size_t index=shapes.size();for(size_t i=0;i<shapes.size();++i)if(shapes[i].at("id")==p.at("collisionId")){index=i;break;}need(index<shapes.size(),"Collision shape not found");if(id=="model_collision_remove")shapes.erase(shapes.begin()+index);else for(const auto* field:{"name","center","rotation","halfExtents","radius","halfHeight","enabled","needsReview"})if(p.contains(field))shapes[index][field]=p[field];
    }else throw std::runtime_error("Unknown model authoring operation");
    validateScene3D(model);return {{"modelId",key},{"updated",true}};
}
}
