#include "Editor.h"
#include "BuildClient.h"
#include <windows.h>
#include <commdlg.h>
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
    {cyui::ScopedDisabled disabled(running);if(cyui::beginProperties("##importFields",cyui::dp(150))){
        cyui::property(tr("Reimport existing asset"));if(ImGui::Checkbox("##reimport",&reimport_))importReport_=Json();
        if(!reimport_){cyui::property(tr("Importer"));if(ImGui::BeginCombo("##importProvider",importProvider_.empty()?tr("Automatic"):importProvider_.c_str())){if(ImGui::Selectable(tr("Automatic"),importProvider_.empty()))importProvider_.clear();for(const auto& definition:definitions.at("importers"))if(ImGui::Selectable(tr(definition.at("name").get_ref<const std::string&>().c_str()),definition.at("id")==importProvider_))importProvider_=definition.at("id");ImGui::EndCombo();}}
        cyui::property(tr("Source file"));ImGui::InputText("##importSource",importSource_,sizeof(importSource_),reimport_?ImGuiInputTextFlags_ReadOnly:0);
        cyui::property(tr("Destination asset"));ImGui::InputText("##importDestination",importDestination_,sizeof(importDestination_));cyui::endProperties();
    }
    if(!reimport_&&cyui::ghostButton(tr("Browse source..."))){
        wchar_t path[32768]{};OPENFILENAMEW dialog{};dialog.lStructSize=sizeof(dialog);dialog.hwndOwner=GetActiveWindow();dialog.lpstrFile=path;dialog.nMaxFile=32768;std::wstring filters;auto filter=[&](const char* title,const wchar_t* pattern){filters+=fs::u8path(title).wstring();filters.push_back(0);filters+=pattern;filters.push_back(0);};filter(tr("Supported source files"),L"*.png;*.jpg;*.jpeg;*.bmp;*.glb;*.cypack");filter(tr("Image files"),L"*.png;*.jpg;*.jpeg;*.bmp");filter(tr("3D asset files"),L"*.glb;*.cypack");filter(tr("All files"),L"*.*");filters.push_back(0);dialog.lpstrFilter=filters.c_str();dialog.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
        if(GetOpenFileNameW(&dialog)){auto file=fs::path(path);std::snprintf(importSource_,sizeof(importSource_),"%s",file.u8string().c_str());auto filename=file.filename();if(file.extension()==L".glb"||file.extension()==L".cypack")filename.replace_extension(L".cy3d");auto dest=(fs::u8path(assetFolder_.empty()?"Imported":assetFolder_)/filename).generic_u8string();std::snprintf(importDestination_,sizeof(importDestination_),"%s",dest.c_str());}
    }
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
            if(cyui::beginProperties("##importInfo",cyui::dp(150))){
                for(const auto& pair:std::vector<std::pair<const char*,const char*>>{{"importerId","Importer"},{"sourcePath","Source file"},{"importedAt","Imported at"}})if(record.contains(pair.first)){cyui::property(tr(pair.second));ImGui::TextWrapped("%s",record.at(pair.first).get_ref<const std::string&>().c_str());}
                if(record.contains("info")){auto info=record.at("info");if(info.contains("width")){cyui::property(tr("Dimensions"));ImGui::Text("%u x %u",info.at("width").get<unsigned>(),info.at("height").get<unsigned>());}if(info.contains("format")){cyui::property(tr("Format"));ImGui::TextUnformatted(info.at("format").get_ref<const std::string&>().c_str());}}
                cyui::endProperties();
            }
        }
    }
    cyui::sectionTitle(tr("Registered importers"));for(const auto& d:definitions.at("importers")){ImGui::TextUnformatted(tr(d.at("name").get_ref<const std::string&>().c_str()));cyui::textDim("%s",d.at("extensions").dump().c_str());}
}
}
