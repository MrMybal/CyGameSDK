#include "Editor.h"
#include "BuildClient.h"
#include <windows.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
namespace cygame {
namespace {bool active(const Json& state){auto s=state.value("state","Idle");return s!="Idle"&&s!="Completed"&&s!="Failed"&&s!="Cancelled"&&s!="Interrupted";}}
Json EditorApp::importAction(const std::string& operation,const Json& p){
    if(operation=="asset_import_view"){
        if(p.contains("path")){auto info=core.execute("asset_import_info",p);reimport_=info.value("imported",false);std::snprintf(importDestination_,sizeof(importDestination_),"%s",p.at("path").get_ref<const std::string&>().c_str());if(reimport_)std::snprintf(importSource_,sizeof(importSource_),"%s",info.at("metadata").at("sourcePath").get_ref<const std::string&>().c_str());importReport_=reimport_?core.execute("asset_import_check",p):info;}
        return open("imports");
    }
    if(operation=="asset_import_request"){
        const bool redo=p.value("reimport",false);auto plan=core.execute("asset_import_prepare",p);reimport_=redo;importProvider_=plan.at("importerId");
        std::snprintf(importSource_,sizeof(importSource_),"%s",plan.at("sourcePath").get_ref<const std::string&>().c_str());std::snprintf(importDestination_,sizeof(importDestination_),"%s",plan.at("destination").get_ref<const std::string&>().c_str());importReport_=plan;
        return importClient_->start(redo?Json{{"path",p.at("path")}}:p,redo?"asset_reimport":"asset_import");
    }
    if(operation=="asset_import_cancel")return importClient_->cancel();
    auto state=importClient_->status();if(state.value("state","")=="Completed"&&state.contains("job")){
        const auto job=state.at("job");if(importCompleted_!=job.at("jobId")){importCompleted_=job.at("jobId");importReport_=job.at("outputs");assetsRefresh();const auto path=importReport_.at("path").get<std::string>();if(imageViews_.count(path)){imageViews_[path].refresh=true;imageViews_[path].info=core.execute("texture_info",{{"path",path}});}reimport_=true;}
    }return state;
}
void EditorApp::importPanel(){
    cyui::heading(tr("Asset Import"),2);ImGui::TextWrapped("%s",tr("Import a source file into the project. Original files remain unchanged."));
    auto definitions=core.execute("asset_importers");
    auto state=importAction("asset_import_status",{});bool running=active(state);
    {cyui::ScopedDisabled disabled(running);if(cyui::beginProperties("##importFields",150)){
        cyui::property(tr("Reimport existing asset"));if(ImGui::Checkbox("##reimport",&reimport_))importReport_=Json();
        if(!reimport_){cyui::property(tr("Importer"));if(ImGui::BeginCombo("##importProvider",importProvider_.empty()?tr("Automatic"):importProvider_.c_str())){if(ImGui::Selectable(tr("Automatic"),importProvider_.empty()))importProvider_.clear();for(const auto& definition:definitions.at("importers"))if(ImGui::Selectable(tr(definition.at("name").get_ref<const std::string&>().c_str()),definition.at("id")==importProvider_))importProvider_=definition.at("id");ImGui::EndCombo();}}
        cyui::property(tr("Source file"));ImGui::InputText("##importSource",importSource_,sizeof(importSource_),reimport_?ImGuiInputTextFlags_ReadOnly:0);
        cyui::property(tr("Destination asset"));ImGui::InputText("##importDestination",importDestination_,sizeof(importDestination_));cyui::endProperties();
    }
    if(!reimport_&&cyui::ghostButton(tr("Browse source..."))){
        importPickerOpen_=true;assistantAttachmentPicker_=false;
        auto initial=importSource_[0]?fs::u8path(importSource_).parent_path():core.root();
        std::error_code initialError;if(!initial.is_absolute())initial=core.root()/initial;if(!fs::is_directory(initial,initialError)||initialError)initial=core.root();
        importPickerRoot_=initial.root_path().generic_u8string();
        importPicker_.directory=importPicker_.pathText=initial.generic_u8string();importPicker_.selection.clear();importListing_={};
    }
    probeInterface("imports/browse");
    Json parameters=reimport_?Json{{"reimport",true},{"path",std::string(importDestination_)}}:Json{{"sourcePath",std::string(importSource_)},{"destination",std::string(importDestination_)}};
    if(!reimport_&&!importProvider_.empty())parameters["importerId"]=importProvider_;
    if(cyui::ghostButton(tr("Validate import")))importReport_=invoke("asset_import_prepare",parameters);
    ImGui::SameLine();if(cyui::primaryButton(tr(reimport_?"Reimport asset":"Import asset")))invoke("asset_import_request",parameters);
    ImGui::SameLine();if(cyui::ghostButton(tr("Check source")))importReport_=invoke("asset_import_check",{{"path",std::string(importDestination_)}});
    }
    if(running&&cyui::ghostButton(tr("Cancel job")))invoke("asset_import_cancel");
    cyui::badge(tr(state.value("state","Idle").c_str()));
    if(state.contains("error"))cyui::callout(cyui::Tone::error,state.at("error").get_ref<const std::string&>().c_str());
    if(state.contains("job")){auto job=state.at("job");if(job.contains("progress")&&job["progress"].is_object())cyui::progressBar(job["progress"].value("percent",0.f)/100.f);if(job.contains("error")&&job["error"].is_object())cyui::callout(cyui::Tone::error,job["error"].value("message","").c_str());}
    if(!importReport_.is_null()){
        if(importReport_.contains("error"))cyui::callout(cyui::Tone::error,importReport_.at("error").get_ref<const std::string&>().c_str());
        else{
            if(importReport_.contains("state"))cyui::badge(tr((std::map<std::string,std::string>{{"UpToDate","Up to date"},{"SourceChanged","Source changed"},{"LocalChanges","Local changes"},{"SourceMissing","Source missing"},{"AssetMissing","Asset missing"},{"Untracked","Untracked"}}).at(importReport_.at("state").get<std::string>()).c_str()));
            if(importReport_.value("state","")=="LocalChanges")cyui::callout(cyui::Tone::warning,tr("Reimport is blocked because this asset was edited locally."));
            if(importReport_.value("state","")=="SourceMissing")cyui::callout(cyui::Tone::warning,tr("Restore the source file to reimport this asset."));
            cyui::sectionTitle(tr("Import details"));auto record=importReport_.value("metadata",importReport_);
            if(cyui::beginProperties("##importInfo",150)){
                for(const auto& pair:std::vector<std::pair<const char*,const char*>>{{"importerId","Importer"},{"sourcePath","Source file"},{"importedAt","Imported at"}})if(record.contains(pair.first)){cyui::property(tr(pair.second));ImGui::TextWrapped("%s",record.at(pair.first).get_ref<const std::string&>().c_str());}
                if(record.contains("info")){auto info=record.at("info");if(info.contains("width")){cyui::property(tr("Dimensions"));ImGui::Text("%u x %u",info.at("width").get<unsigned>(),info.at("height").get<unsigned>());}if(info.contains("format")){cyui::property(tr("Format"));ImGui::TextUnformatted(info.at("format").get_ref<const std::string&>().c_str());}}
                cyui::endProperties();
            }
        }
    }
    cyui::sectionTitle(tr("Registered importers"));for(const auto& d:definitions.at("importers")){ImGui::TextUnformatted(tr(d.at("name").get_ref<const std::string&>().c_str()));cyui::textDim("%s",d.at("extensions").dump().c_str());}
}

void EditorApp::importFilePicker(){
    if(!importPickerOpen_)return;
    ImGui::SetNextWindowSize(cyui::dp(760,540),ImGuiCond_FirstUseEver);
    if(ImGui::Begin(label(tr(assistantAttachmentPicker_?"Choose an attachment":"Choose a source file"),"importSourcePicker").c_str(),&importPickerOpen_)){
        if(ImGui::BeginCombo("##volume",importPickerRoot_.c_str())){
            const auto mask=GetLogicalDrives();
            for(int index=0;index<26;++index)if(mask&(1u<<index)){
                const std::string root=std::string(1,char('A'+index))+":/";
                if(ImGui::Selectable(root.c_str(),importPickerRoot_==root)){
                    importPickerRoot_=root;importPicker_.directory=importPicker_.pathText=root;importPicker_.selection.clear();
                }
            }ImGui::EndCombo();
        }
        ImGui::SameLine();{cyui::ScopedDisabled disabled(importListingFuture_.valid());if(cyui::ghostButton(tr("Refresh assets")))importListing_.directory.clear();}
        if(importListingFuture_.valid()&&importListingFuture_.wait_for(std::chrono::seconds(0))==std::future_status::ready){
            try{auto completed=importListingFuture_.get();if(completed.directory==importPicker_.directory)importListing_=std::move(completed);}
            catch(const std::exception& e){importListing_={};importListing_.directory=importPicker_.directory;importListing_.error=e.what();}
        }
        if(importListing_.directory!=importPicker_.directory&&!importListingFuture_.valid()){
            const auto directory=importPicker_.directory;importListing_={};importListing_.directory=directory;importListing_.loading=true;
            importListingFuture_=std::async(std::launch::async,[directory]{
                cyui::FileListing result;result.directory=directory;std::error_code error;
                fs::directory_iterator iterator(fs::u8path(directory),error),end;
                if(error){result.error=error.message();return result;}
                for(size_t count=0;iterator!=end&&!error;iterator.increment(error)){
                    if(++count>10000){result.error="Directory exceeds the 10000-entry listing budget";break;}
                    const auto entry=*iterator;cyui::FileEntry row;row.name=entry.path().filename().u8string();
                    row.id=EditorApp::uiId("source/"+entry.path().generic_u8string());row.directory=entry.is_directory(error);
                    if(error)break;if(!row.directory){row.bytes=entry.file_size(error);if(error){error.clear();continue;}}
                    result.entries.push_back(std::move(row));
                }
                if(error)result.error=error.message();
                return result;
            });
        }
        cyui::FilePickerOptions options;options.root=importPickerRoot_;
        options.filters={{"Supported source files",{".png",".jpg",".jpeg",".bmp",".glb",".cypack"}},{"Image files",{".png",".jpg",".jpeg",".bmp"}},{"3D asset files",{".glb",".cypack"}},{"All files",{}}};
        if(assistantAttachmentPicker_)options.filters={{"All files",{}}};
        cyui::FileListing waiting;waiting.directory=importPicker_.directory;waiting.loading=true;const auto& listing=importListing_.directory==importPicker_.directory?importListing_:waiting;
        const auto result=cyui::filePicker("##sourceFiles",importPicker_,listing,ImGui::GetContentRegionAvail(),options);
        if(interfaceProbe){auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();const auto width=cyui::buttonWidth(tr("Open"));b.x=a.x-ImGui::GetStyle().ItemSpacing.x;a.x=b.x-width;interfaceProbe("imports/picker/open",a,b);}
        if(result.accepted&&!result.paths.empty()){
            const auto file=fs::u8path(result.paths.front());std::error_code error;
            const auto source=file.u8string();
            if(assistantAttachmentPicker_&&fs::is_regular_file(file,error)&&!error){if(assistant_)assistant_->attach(source);assistantAttachmentPicker_=false;importPickerOpen_=false;}
            else if(source.size()>=sizeof(importSource_)||!fs::is_regular_file(file,error)||error)cyui::notify(tr("Source file is unavailable."),cyui::Tone::error);
            else{
                auto filename=file.filename();auto extension=file.extension().u8string();std::transform(extension.begin(),extension.end(),extension.begin(),[](unsigned char c){return char(std::tolower(c));});
                if(extension==".glb"||extension==".cypack")filename.replace_extension(".cy3d");
                const auto destination=(fs::u8path(assetFolder_.empty()?"Imported":assetFolder_)/filename).generic_u8string();
                if(destination.size()>=sizeof(importDestination_))cyui::notify(tr("Text exceeds the field limit."),cyui::Tone::error);
                else{std::snprintf(importSource_,sizeof(importSource_),"%s",source.c_str());std::snprintf(importDestination_,sizeof(importDestination_),"%s",destination.c_str());importPickerOpen_=false;}
            }
        }
        if(result.cancelled)importPickerOpen_=false;
    }ImGui::End();
}
}
