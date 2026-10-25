#include "Core.h"
#include "Graph.h"
namespace cygame {
namespace {fs::path jsonFile(const fs::path& root,const std::string& path){auto base=fs::weakly_canonical(root/"data/assets"),file=fs::weakly_canonical(base/fs::u8path(path)),rel=file.lexically_relative(base);if(rel.empty()||rel.is_absolute()||*rel.begin()==".."||file.extension()!=L".json")throw std::runtime_error("JSON document must stay inside the content root");return file;}}
void validateEditableJson(const Json& value){std::vector<std::pair<const Json*,int>> pending={{&value,0}};size_t count=0;while(!pending.empty()){auto current=pending.back();pending.pop_back();if(current.second>64||++count>50000)throw std::runtime_error("JSON editor limit is 64 levels / 50,000 values");if(current.first->is_object()||current.first->is_array())for(const auto& child:*current.first)pending.push_back({&child,current.second+1});}}
Json EditorCore::jsonCommand(const std::string& id,const Json& p){
    if(!document_.contains("jsonDocuments"))document_["jsonDocuments"]=Json::object();auto& documents=document_["jsonDocuments"];
    if(id=="json_open"){const auto file=jsonFile(root_,p.at("path"));const std::string path=file.lexically_relative(fs::weakly_canonical(root_/"data/assets")).generic_u8string();if(documents.contains(path))return {{"jsonId",path}};if(fs::file_size(file)>512*1024)throw std::runtime_error("Editable JSON is limited to 512 KiB");if(!documents.contains(path)){if(documents.size()>=4)throw std::runtime_error("Close a JSON document before opening more than four");auto value=readJson(file);validateEditableJson(value);documents[path]={{"value",value},{"savedHash",sha256(value.dump())}};}return {{"jsonId",path}};}
    const std::string path=p.at("jsonId");auto& doc=documents.at(path);auto& value=doc.at("value");
    if(id=="json_close"){if(sha256(value.dump())!=sha256(readJson(jsonFile(root_,path)).dump()))throw std::runtime_error("Save or undo JSON changes before closing");documents.erase(path);jsonFileHashes_.erase(path);return {{"closed",true}};}
    if(id=="json_get")return {{"value",value},{"jsonId",path}};
    if(id=="json_save"){
        const auto file=jsonFile(root_,path);if(sha256(readJson(file).dump())!=(jsonFileHashes_.count(path)?Json(jsonFileHashes_[path]):doc.at("savedHash")))throw std::runtime_error("JSON changed on disk; preserve external edits before saving");writeJson(file,value,512*1024);jsonFileHashes_[path]=sha256(value.dump());doc["savedHash"]=sha256(value.dump());return {{"saved",true},{"path",path}};
    }
    const std::string pointer=p.value("pointer","");Json::json_pointer target(pointer);
    if(id=="json_value_set"){if(!value.contains(target))throw std::runtime_error("JSON pointer is missing");value.at(target)=p.at("value");}
    else if(id=="json_add"){auto& parent=value.at(target);if(parent.is_array())parent.push_back(p.at("value"));else if(parent.is_object()){std::string name=p.at("name");if(parent.contains(name))throw std::runtime_error("JSON property already exists");parent[name]=p.at("value");}else throw std::runtime_error("Add requires a JSON container");}
    else if(id=="json_remove"){if(pointer.empty())throw std::runtime_error("Cannot remove the JSON root");value=value.patch(Json::array({{{"op","remove"},{"path",pointer}}}));}
    else throw std::runtime_error("Unknown JSON operation");
    validateEditableJson(value);if(value.dump().size()>512*1024)throw std::runtime_error("Editable JSON exceeds 512 KiB");return {{"updated",true}};
}
}
