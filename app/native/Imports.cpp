#include "Imports.h"
#include "Graph.h"
#include <fstream>
#include <algorithm>
namespace cygame {
namespace {
fs::path within(const fs::path& base,const fs::path& input){auto root=fs::weakly_canonical(base),path=fs::weakly_canonical(input),relative=path.lexically_relative(root);if(relative.empty()||relative.is_absolute()||*relative.begin()==".."||path==root)throw std::runtime_error("Import path escapes its workspace");return path;}
std::string hashFile(const fs::path& path){if(!fs::is_regular_file(path)||fs::file_size(path)>32*1024*1024)throw std::runtime_error("Import file missing or above 32 MiB");std::ifstream stream(path,std::ios::binary);if(!stream)throw std::runtime_error("Cannot read import file");return sha256(std::string(std::istreambuf_iterator<char>(stream),{}));}
Json record(const fs::path& asset){auto path=importSidecar(asset);if(fs::file_size(path)>65536)throw std::runtime_error("Import metadata exceeds 64 KiB");auto value=readJson(path);if(value.value("format","")!="cygame-import"||value.value("schemaVersion",0)!=1)throw std::runtime_error("Unsupported import metadata");return value;}
std::string stateHash(const fs::path& path){return fs::exists(path)?hashFile(path):"";}
bool digest(const Json& value){if(!value.is_string())return false;auto s=value.get<std::string>();return s.size()==64&&s.find_first_not_of("0123456789abcdef")==std::string::npos;}
}
fs::path importSidecar(const fs::path& asset){return fs::path(asset.wstring()+L".cyimport");}
void transferAssetPair(const fs::path& source,const fs::path& target,bool copy){
    auto meta=importSidecar(source),dest=importSidecar(target);if(fs::exists(target)||fs::exists(dest))throw std::runtime_error("Asset or import metadata destination already exists");
    if(fs::exists(meta)&&(fs::is_symlink(meta)||!fs::is_regular_file(meta)||fs::file_size(meta)>65536))throw std::runtime_error("Import metadata is invalid or above 64 KiB");
    fs::create_directories(target.parent_path());if(copy)fs::copy_file(source,target);else fs::rename(source,target);
    try{if(fs::exists(meta)){if(copy)fs::copy_file(meta,dest);else fs::rename(meta,dest);}}
    catch(...){if(copy){std::error_code ec;fs::remove(target,ec);}else fs::rename(target,source);throw;}
}
void EditorCore::registerImporter(ImportDefinition definition){
    const std::string id=definition.descriptor.at("id");if(id.empty()||importers_.count(id)||!definition.inspect)throw std::runtime_error("Invalid importer registration");
    const auto max=definition.descriptor.at("maxBytes").get<uint64_t>();if(!max||max>32*1024*1024||!definition.descriptor.at("extensions").is_array())throw std::runtime_error("Importers must declare bounded input files");
    importers_.emplace(id,std::move(definition));
}
Json EditorCore::importCommand(const std::string& operation,const Json& p){
    const auto content=fs::weakly_canonical(root_/"data/assets");fs::create_directories(content);
    auto protectLive=[&](const fs::path& target){const auto relative=target.lexically_relative(content).generic_u8string();for(const auto& [key,model]:models().items())if(model.value("assetPath","")==relative){auto clean=model;clean.erase("assetPath");clean.erase("_loadedHash");auto baseline=savedModels_.count(key)?savedModels_.at(key):readJson(target);if(clean!=baseline)throw std::runtime_error("Save or undo loaded 3D edits before reimporting");}};
    auto assetPath=[&](const std::string& relative){auto path=fs::u8path(relative);if(path.is_absolute())throw std::runtime_error("Destination must be content-relative");return within(content,content/path);};
    if(operation=="asset_importers"){Json definitions=Json::array();for(const auto& [id,d]:importers_)definitions.push_back(d.descriptor);return {{"importers",definitions}};}
    if(operation=="asset_import_info"||operation=="asset_import_check"){
        auto asset=assetPath(p.at("path"));if(!fs::exists(importSidecar(asset)))return {{"imported",false},{"path",p.at("path")},{"state","Untracked"}};
        auto metadata=record(asset);Json result={{"imported",true},{"path",p.at("path")},{"metadata",metadata}};
        if(operation=="asset_import_check"){
            auto source=fs::u8path(metadata.at("sourcePath").get<std::string>());std::string state;
            if(!fs::is_regular_file(asset))state="AssetMissing";
            else if(hashFile(asset)!=metadata.at("importedSha256"))state="LocalChanges";
            else if(!fs::is_regular_file(source))state="SourceMissing";
            else state=hashFile(source)==metadata.at("sourceSha256")?"UpToDate":"SourceChanged";
            result["state"]=state;
        }return result;
    }
    if(operation=="asset_import_prepare"){
        const bool reimport=p.value("reimport",false);Json metadata;fs::path target,source;std::string id;
        if(reimport){target=assetPath(p.at("path"));protectLive(target);metadata=record(target);source=fs::u8path(metadata.at("sourcePath").get<std::string>());id=metadata.at("importerId");if(hashFile(target)!=metadata.at("importedSha256"))throw std::runtime_error("Imported asset has local changes; preserve them before reimporting");}
        else{target=assetPath(p.at("destination"));source=fs::u8path(p.at("sourcePath").get<std::string>());id=p.value("importerId","");if(fs::exists(target)||fs::exists(importSidecar(target)))throw std::runtime_error("Import destination already exists");}
        if(!source.is_absolute()||!fs::is_regular_file(source))throw std::runtime_error("Choose an existing absolute source file");source=fs::weakly_canonical(source);
        const auto relativeSource=source.lexically_relative(content);if(!relativeSource.empty()&&!relativeSource.is_absolute()&&*relativeSource.begin()!="..")throw std::runtime_error("Import sources must be outside the content root");
        auto extension=source.extension().u8string();std::transform(extension.begin(),extension.end(),extension.begin(),[](unsigned char c){return char(std::tolower(c));});
        auto targetExt=target.extension().u8string();std::transform(targetExt.begin(),targetExt.end(),targetExt.begin(),[](unsigned char c){return char(std::tolower(c));});
        if(id.empty())for(const auto& [key,d]:importers_)for(const auto& ext:d.descriptor.at("extensions"))if(ext==extension){id=key;break;}
        if(!importers_.count(id))throw std::runtime_error("No registered importer for this source");const auto& provider=importers_.at(id);
        bool outputAccepted=false;for(const auto& ext:provider.descriptor.value("outputExtensions",Json::array({extension})))outputAccepted|=ext==targetExt;if(!outputAccepted)throw std::runtime_error("Destination extension is unsupported by this importer");
        bool accepted=false;for(const auto& ext:provider.descriptor.at("extensions"))accepted|=ext==extension;if(!accepted||fs::file_size(source)>provider.descriptor.at("maxBytes").get<uint64_t>())throw std::runtime_error("Source is outside the importer limits");
        auto info=provider.inspect(source);info.erase("path");
        return {{"format","cygame-import-plan"},{"schemaVersion",1},{"sourcePath",source.u8string()},{"destination",target.lexically_relative(content).generic_u8string()},{"importerId",id},{"reimport",reimport},{"sourceBytes",fs::file_size(source)},{"maxBytes",provider.descriptor.at("maxBytes")},{"preservedAssetId",reimport?metadata.value("info",Json::object()).value("assetId",""):""},{"expectedAsset",stateHash(target)},{"expectedMetadata",stateHash(importSidecar(target))},{"info",info}};
    }
    if(operation!="asset_import_commit")throw std::runtime_error("Unknown import operation");
    auto reportPath=within(root_,fs::u8path(p.at("reportPath").get<std::string>()));auto report=readJson(reportPath);if(report.value("format","")!="cygame-import-result"||report.value("schemaVersion",0)!=1)throw std::runtime_error("Invalid import report");
    const auto plan=report.at("plan");if(plan.value("format","")!="cygame-import-plan"||plan.value("schemaVersion",0)!=1)throw std::runtime_error("Invalid import plan");
    const std::string id=plan.at("importerId");if(!importers_.count(id)||!digest(report.at("sourceSha256"))||!digest(report.at("importedSha256")))throw std::runtime_error("Invalid importer or hashes");
    auto target=assetPath(plan.at("destination"));protectLive(target);auto sidecar=importSidecar(target),stage=within(reportPath.parent_path(),fs::u8path(report.at("stagedPath").get<std::string>()));
    if(stage==reportPath||stage==target||stage==sidecar)throw std::runtime_error("Staged asset must be independent of persistent files");
    within(content,sidecar);within(content,fs::path(sidecar.wstring()+L".tmp"));
    if(plan.at("reimport").get<bool>()){auto current=record(target);if(current.at("importedSha256")!=plan.at("expectedAsset")||current.at("sourcePath")!=plan.at("sourcePath")||current.at("importerId")!=id)throw std::runtime_error("Reimport source or local contents changed");}
    else if(plan.at("expectedAsset")!=""||plan.at("expectedMetadata")!="")throw std::runtime_error("New imports cannot overwrite files");
    if(!fs::u8path(plan.at("sourcePath").get<std::string>()).is_absolute())throw std::runtime_error("Import source must be absolute");
    validateValue(plan.at("sourcePath"),{{"type","string"},{"minLength",1},{"maxLength",1024}});
    validateValue(report.at("importedAt"),{{"type","string"},{"maxLength",64}});validateValue(report.at("sourceBytes"),{{"type","integer"},{"minimum",0},{"maximum",32*1024*1024}});
    auto destinationExt=target.extension().u8string();std::transform(destinationExt.begin(),destinationExt.end(),destinationExt.begin(),[](unsigned char c){return char(std::tolower(c));});
    bool supported=false;for(const auto& ext:importers_.at(id).descriptor.value("outputExtensions",importers_.at(id).descriptor.at("extensions")))supported|=destinationExt==ext;if(!supported)throw std::runtime_error("Unsupported destination extension");
    if(stateHash(target)!=plan.at("expectedAsset")||stateHash(sidecar)!=plan.at("expectedMetadata"))throw std::runtime_error("Import destination changed since preparation");
    if(hashFile(stage)!=report.at("importedSha256")||fs::file_size(stage)>importers_.at(id).descriptor.at("maxBytes").get<uint64_t>())throw std::runtime_error("Staged import checksum or size mismatch");
    const auto info=importers_.at(id).inspect(stage); // Validate actual staged bytes before any persistent mutation.
    auto metadata=Json{{"format","cygame-import"},{"schemaVersion",1},{"importerId",id},{"sourcePath",plan.at("sourcePath")},{"sourceSha256",report.at("sourceSha256")},{"importedSha256",report.at("importedSha256")},{"sourceBytes",report.at("sourceBytes")},{"info",info},{"importedAt",report.at("importedAt")}};metadata["info"].erase("path");
    auto backup=within(root_,root_/"data/import-recovery"/fs::u8path(uniqueId()));fs::create_directories(backup);fs::create_directories(target.parent_path());
    writeJson(backup/"transaction.json",{{"destination",plan.at("destination")},{"format","cygame-import-recovery"},{"schemaVersion",1}});
    auto cleanup=[&]{std::error_code ec;for(const auto* name:{"a","m","transaction.json"})fs::remove(backup/name,ec);fs::remove(backup,ec);};
    bool oldAsset=false,oldMeta=false,installed=false;
    try{if(fs::exists(target)){fs::rename(target,backup/"a");oldAsset=true;}if(fs::exists(sidecar)){fs::rename(sidecar,backup/"m");oldMeta=true;}fs::rename(stage,target);installed=true;writeJson(sidecar,metadata);}
    catch(...){
        std::error_code ec;bool restored=true;
        if(installed){fs::rename(target,stage,ec);restored&=!ec;ec.clear();fs::remove(sidecar,ec);ec.clear();fs::remove(fs::path(sidecar.wstring()+L".tmp"),ec);}
        if(oldAsset){ec.clear();fs::rename(backup/"a",target,ec);restored&=!ec;}
        if(oldMeta){ec.clear();fs::rename(backup/"m",sidecar,ec);restored&=!ec;}
        if(restored){cleanup();throw;}
        throw std::runtime_error("Import rollback needs recovery from "+backup.u8string());
    }
    cleanup();
    return {{"path",plan.at("destination")},{"imported",true},{"reimport",plan.at("reimport")},{"metadata",metadata}};
}
}
