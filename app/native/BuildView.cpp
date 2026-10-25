#include "Editor.h"
#include "BuildClient.h"
namespace cygame {
void EditorApp::buildPanel(){
    cyui::heading(tr("Build and Package"),2);auto descriptor=core.execute("build_describe");const auto& targets=descriptor.at("targets");
    if(targets.empty()){cyui::emptyState(tr("No build targets"),tr("This game adapter has not registered a build provider."),cyui::icon::code);return;}
    if(buildTarget_.empty())buildTarget_=targets[0].at("id");Json target=targets[0];for(const auto& t:targets)if(t.at("id")==buildTarget_)target=t;
    if(cyui::beginProperties("##buildOptions",cyui::dp(160))){
        cyui::property(tr("Target"));if(ImGui::BeginCombo("##target",tr(target.at("name").get_ref<const std::string&>().c_str()))){for(const auto& t:targets)if(ImGui::Selectable(tr(t.at("name").get_ref<const std::string&>().c_str()),t["id"]==buildTarget_)){buildTarget_=t.at("id");buildOptions_=Json::object();buildConfiguration_=t.at("configurations")[0];}ImGui::EndCombo();}
        cyui::property(tr("Configuration"));if(buildConfiguration_.empty())buildConfiguration_=target.at("configurations")[0];if(ImGui::BeginCombo("##configuration",tr(buildConfiguration_.c_str()))){for(const auto& c:target.at("configurations"))if(ImGui::Selectable(tr(c.get_ref<const std::string&>().c_str()),c==buildConfiguration_))buildConfiguration_=c;ImGui::EndCombo();}
        for(const auto& [name,spec]:target.at("optionsSchema").at("properties").items()){
            if(!buildOptions_.contains(name))buildOptions_[name]=spec.value("default",Json());cyui::property(tr(name.c_str()));Json next,viewSpec=spec;if(target.at("providerId")=="sandbox.archive")viewSpec["x-editor-localizeEnum"]=true;if(editValue("build/"+buildTarget_+"/"+name,viewSpec,buildOptions_[name],next))buildOptions_[name]=next;
        }cyui::endProperties();
    }
    if(!target.value("gameExecutable",false))ImGui::TextWrapped("%s",tr("Exports an immutable document archive. It does not produce a game executable."));
    Json parameters={{"targetId",buildTarget_},{"configuration",buildConfiguration_},{"options",buildOptions_},{"expectedRevision",core.revision()}};
    const auto state=core.execute("build_status");const std::string status=state.value("state","Idle");
    const bool active=status!="Idle"&&status!="Completed"&&status!="Failed"&&status!="Cancelled"&&status!="Interrupted";
    if(cyui::ghostButton(tr("Validate request")))buildPlan_=invoke("build_prepare",parameters);
    ImGui::SameLine();{cyui::ScopedDisabled disabled(active);if(cyui::primaryButton(tr("Export package")))invoke("build_request",parameters);}
    cyui::badge(tr(status.c_str()));if(active&&cyui::ghostButton(tr("Cancel job")))invoke("build_cancel");
    if(state.contains("error"))cyui::callout(cyui::Tone::error,state.at("error").get_ref<const std::string&>().c_str());
    if(state.contains("job")){
        const auto& job=state.at("job");cyui::textDim("%s: %s",tr("Job"),job.at("jobId").get_ref<const std::string&>().c_str());
        if(job.contains("progress")&&job["progress"].is_object())cyui::progressBar(job["progress"].value("percent",0.f)/100.f);
        if(job.contains("error")&&job["error"].is_object())ImGui::TextWrapped("%s",job["error"].value("message","").c_str());
        if(status=="Completed"){
            const auto& outputs=job.at("outputs");cyui::sectionTitle(tr("Package ready"));ImGui::TextWrapped("%s",outputs.at("archivePath").get_ref<const std::string&>().c_str());
            if(cyui::ghostButton(tr("Copy package path")))ImGui::SetClipboardText(outputs.at("archivePath").get_ref<const std::string&>().c_str());
            cyui::textDim("%s: %llu",tr("Snapshot revision"),outputs.at("snapshotRevision").get<unsigned long long>());
        }
    }
    if(buildPlan_.contains("diagnostics")){
        if(buildPlan_.value("revision",uint64_t(0))!=core.revision())cyui::callout(cyui::Tone::info,tr("The document has changed since this validation."));
        cyui::sectionTitle(tr("Build diagnostics"));for(const auto& d:buildPlan_["diagnostics"]){cyui::badge(tr(d.at("severity").get_ref<const std::string&>().c_str()));ImGui::SameLine();ImGui::TextWrapped("%s / %s",d.value("graphId","").c_str(),d.at("message").get_ref<const std::string&>().c_str());}
        if(buildPlan_.at("diagnostics").empty())cyui::textDim("%s",tr("No build diagnostics"));
    }
}
}
