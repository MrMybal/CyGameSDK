#include "Core.h"
#include <algorithm>
#include <set>
namespace cygame {
Json EditorCore::sceneCommand(const std::string& id,const Json& p){
    auto ids=p.contains("entityIds")?p.at("entityIds").get<std::vector<std::string>>():selectedIds_;
    std::set<std::string> unique;for(const auto& key:ids){entity(key);if(!unique.insert(key).second)throw std::runtime_error("Duplicate object ID");}
    if(id=="scene_select"){
        std::string key=p.at("entityId");entity(key);auto mode=p.value("mode","replace");
        if(mode=="replace")selectedIds_={key};
        else if(mode=="add"){if(!isSelected(key))selectedIds_.push_back(key);}
        else if(mode=="toggle"){if(isSelected(key))selectedIds_.erase(std::remove(selectedIds_.begin(),selectedIds_.end(),key),selectedIds_.end());else selectedIds_.push_back(key);}
        selected_=selectedIds_.empty()?"":selectedIds_.back();return {{"selected",selected_}};
    }
    if(id=="scene_select_many"){
        selectedIds_=p.at("entityIds").get<std::vector<std::string>>();selected_=selectedIds_.empty()?"":selectedIds_.back();
        return {{"selection",selectedIds_}};
    }
    if(id=="scene_create"){
        if(entities().size()>=2000)throw std::runtime_error("Scene limit reached (2000)");
        auto e=adapter_->createEntity(uniqueId(),p.at("name"));document_["entities"].push_back(e);selected_=e.at("id");selectedIds_={selected_};return {{"entity",e}};
    }
    if(id=="scene_delete"||id=="scene_delete_many"){
        if(id=="scene_delete")ids={p.at("entityId")};
        for(const auto& key:ids)if(entity(key).value("editorLocked",false))throw std::runtime_error("Object is locked");
        auto& all=document_["entities"];std::set<std::string> removed(ids.begin(),ids.end());
        all.erase(std::remove_if(all.begin(),all.end(),[&](const Json& e){return removed.count(e.at("id"));}),all.end());
        selectedIds_.erase(std::remove_if(selectedIds_.begin(),selectedIds_.end(),[&](const std::string& key){return removed.count(key);}),selectedIds_.end());
        selected_=selectedIds_.empty()?"":selectedIds_.back();
        if(id=="scene_delete")return {{"deleted",ids[0]}};return {{"deletedCount",ids.size()}};
    }
    if(id=="scene_duplicate"){
        if(entities().size()+ids.size()>2000)throw std::runtime_error("Scene limit reached (2000)");
        Json copies=Json::array();const auto offset=p.value("offset",Json::array({1.,0.,0.}));
        for(const auto& key:ids){
            auto copy=entity(key);copy["id"]=uniqueId();copy["editorLocked"]=false;copy["properties"]["name"]=copy["properties"]["name"].get<std::string>()+"_Copy";
            for(int axis=0;axis<3;++axis)copy["properties"]["position"][axis]=copy["properties"]["position"][axis].get<double>()+offset[axis].get<double>();
            validateValue(copy["properties"],adapter_->propertySchema(copy));copies.push_back(copy);
        }
        selectedIds_.clear();for(const auto& e:copies){document_["entities"].push_back(e);selectedIds_.push_back(e.at("id"));}
        selected_=selectedIds_.empty()?"":selectedIds_.back();return {{"entities",copies}};
    }
    if(id=="scene_folder_set"||id=="scene_lock"){
        std::string folder=p.value("folder","");
        if(id=="scene_folder_set"){for(const auto& part:fs::u8path(folder))if(part==".."||part==".")throw std::runtime_error("Invalid outliner folder");if(folder.find('\\')!=std::string::npos||fs::u8path(folder).is_absolute())throw std::runtime_error("Use relative outliner folder names");}
        for(const auto& key:ids){if(id=="scene_lock")entity(key)["editorLocked"]=p.at("locked");else entity(key)["folder"]=folder;}
        return {{"updatedCount",ids.size()}};
    }
    if(id=="scene_transform"||id=="scene_transform_batch"){
        Json transforms=id=="scene_transform"?Json::array({p}):p.at("transforms");
        std::set<std::string> seen;
        for(const auto& t:transforms){
            const std::string key=t.at("entityId");if(!seen.insert(key).second)throw std::runtime_error("Duplicate transform target");
            const auto& e=entity(key);if(e.value("editorLocked",false))throw std::runtime_error("Object is locked");
            bool any=false;const auto schema=adapter_->propertySchema(e).at("properties");
            for(const auto* field:{"position","rotation","scale"})if(t.contains(field)){any=true;if(!schema.contains(field))throw std::runtime_error("Adapter does not expose this transform property");validateValue(t.at(field),schema.at(field),field);}
            if(!any)throw std::runtime_error("A transform must change at least one property");
        }
        for(const auto& t:transforms)for(const auto* field:{"position","rotation","scale"})if(t.contains(field))entity(t.at("entityId"))["properties"][field]=t.at(field);
        if(id=="scene_transform")return {{"entity",entity(p.at("entityId"))}};
        return {{"updatedCount",transforms.size()}};
    }
    throw std::runtime_error("Unknown scene operation: "+id);
}
}
