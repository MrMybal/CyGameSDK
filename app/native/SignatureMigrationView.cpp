#include "Editor.h"
#include "Graph.h"
namespace cygame {
Json EditorApp::previewTypeMigration(const Json& request){return previewSignatureMigration(request);}
Json EditorApp::previewSignatureMigration(const Json& request){
    migrationRequest_=request;migrationReport_=invoke(request.contains("typeId")?"graph_type_migration_plan":"graph_signature_plan",request);migrationAcceptLoss_=false;migrationRevision_=~uint64_t(0);const std::string panel=request.contains("typeId")?"typeMigration":"signatureMigration";for(auto& p:core.panels)if((p.id=="signatureMigration"||p.id=="typeMigration")&&p.id!=panel)p.open=false;open(panel);return migrationReport_;
}
void EditorApp::signatureRenameControls(Json& draft,Json& renames,const std::string& context){
    if(!renames.is_object())renames=Json::object();ImGui::PushID(context.c_str());cyui::sectionTitle(tr("Port renames"));
    for(const auto* side:{"inputs","outputs"}){
        ImGui::PushID(side);std::vector<std::string> names;for(const auto& [name,port]:draft.at(side).items())names.push_back(name);
        if(!names.empty()){
            int& index=std::string(side)=="inputs"?migrationInputIndex_:migrationOutputIndex_;index=std::clamp(index,0,int(names.size())-1);
            ImGui::SetNextItemWidth(-1);if(ImGui::BeginCombo(tr(side),names[index].c_str())){for(int i=0;i<int(names.size());++i)if(ImGui::Selectable(names[i].c_str(),i==index))index=i;ImGui::EndCombo();}
            ImGui::SetNextItemWidth(-1);ImGui::InputText(tr("New port name"),migrationPort_,sizeof(migrationPort_));
            if(cyui::ghostButton(tr("Rename draft port"))){const auto old=names[index];const std::string next=migrationPort_;if(next.empty()||draft[side].contains(next))cyui::notify(tr("Choose a new, unique port name."),cyui::Tone::warning);else{
                auto value=draft[side].at(old);draft[side].erase(old);draft[side][next]=value;if(!renames.contains(side))renames[side]=Json::object();bool found=false;for(auto& [origin,to]:renames[side].items())if(to==old){to=next;found=true;}if(!found)renames[side][old]=next;
            }}
        }ImGui::PopID();
    }ImGui::PopID();
}
void EditorApp::signatureMigrationPanel(bool typePanel){
    const bool typed=migrationRequest_.contains("typeId")||(migrationRequest_.is_null()&&typePanel);
    cyui::heading(tr(typed?"Type Migration":"Signature Migration"),2);
    if(migrationRequest_.is_null()){cyui::emptyState(tr("No migration preview"),tr(typed?"Edit a data type, then preview its migration.":"Edit a function or interface signature, then preview its migration."),cyui::icon::info);return;}
    const std::string graph=migrationRequest_.at("graphId");
    cyui::textDim("%s / %s",graph.c_str(),migrationRequest_.at(migrationRequest_.contains("typeId")?"typeId":"name").get_ref<const std::string&>().c_str());
    bool applied=migrationReport_.value("applied",false);
    {cyui::ScopedDisabled disabled(applied);if(cyui::ghostButton(tr("Refresh migration preview")))previewSignatureMigration(migrationRequest_);}ImGui::SameLine();if(cyui::ghostButton(tr("Close preview")))for(auto& panel:core.panels)if((panel.id=="signatureMigration"||panel.id=="typeMigration"))panel.open=false;
    if(migrationReport_.contains("error")){cyui::callout(cyui::Tone::error,migrationReport_.at("error").get_ref<const std::string&>().c_str());return;}
    if(migrationRevision_!=core.revision()){
        migrationRevision_=core.revision();migrationStale_=!core.graphs().contains(graph)||sha256(core.graphs().at(graph).dump())!=migrationReport_.at("sourceHash").get<std::string>();
    }
    if(applied)cyui::callout(cyui::Tone::success,tr("Migration applied. Edit > Undo restores the complete previous graph."));
    else if(migrationStale_)cyui::callout(cyui::Tone::warning,tr("The graph changed after this preview. Refresh before applying."));
    const auto& summary=migrationReport_.at("summary");
    if(cyui::beginProperties("##migrationSummary")){
        for(const auto& item:std::vector<std::pair<const char*,const char*>>{{"declarations","Declarations updated"},{"nodes","Nodes updated"},{"linksRenamed","Connections renamed"},{"linksRemoved","Connections removed"},{"valuesPreserved","Fallback values kept"},{"valuesAdded","Fallback values added"},{"valuesRemoved","Fallback values removed"},{"valuesReset","Fallback values reset"}}){static const std::map<std::string,const char*> typeLabels={{"valuesPreserved","Values kept"},{"valuesAdded","Values added"},{"valuesRemoved","Values removed"},{"valuesReset","Values reset"}};cyui::property(tr(typed&&typeLabels.count(item.first)?typeLabels.at(item.first):item.second));ImGui::Text("%d",summary.at(item.first).get<int>());}cyui::endProperties();
    }
    if(summary.contains("valuesMigrated")){cyui::textDim("%s: %d",tr("Values migrated"),summary.at("valuesMigrated").get<int>());}
    const bool valid=migrationReport_.value("valid",false),loss=migrationReport_.at("requiresLossAcceptance");
    if(!valid)cyui::callout(cyui::Tone::error,migrationReport_.value("diagnostic","Invalid migration").c_str());
    if(loss&&!applied){cyui::callout(cyui::Tone::warning,tr(typed?"This migration removes connections or removes/resets typed values. Review the details before accepting.":"This migration removes connections or fallback values. Review the details before accepting."));ImGui::Checkbox(tr("Accept the listed losses"),&migrationAcceptLoss_);probeInterface("migration/acceptLoss");}
    {cyui::ScopedDisabled disabled(applied||migrationStale_||!valid||(loss&&!migrationAcceptLoss_));if(cyui::primaryButton(tr("Apply migration"))){auto request=migrationRequest_;request["planHash"]=migrationReport_.at("planHash");request["acceptLoss"]=migrationAcceptLoss_;auto result=invoke(request.contains("typeId")?"graph_type_migration_apply":"graph_signature_apply",request);if(!result.contains("error")){migrationReport_=result;signatureKey_.clear();}}probeInterface("migration/apply");}
    cyui::sectionTitle(tr("Affected declarations"));for(const auto& name:migrationReport_.at("targets").at("functions"))ImGui::BulletText("%s",name.get_ref<const std::string&>().c_str());for(const auto& id:migrationReport_.at("targets").at("interfaces"))ImGui::BulletText("%s.%s",id.at("name").get_ref<const std::string&>().c_str(),id.at("method").get_ref<const std::string&>().c_str());
    for(const auto* category:{"types","classes"})if(migrationReport_.at("targets").contains(category))for(const auto& id:migrationReport_.at("targets").at(category))ImGui::BulletText("%s",id.get_ref<const std::string&>().c_str());
    if(migrationReport_.at("truncated").get<bool>())cyui::callout(cyui::Tone::warning,tr("Details are limited; summary counts include every change."));
    cyui::sectionTitle(tr("Migration details"));int index=0;for(const auto& detail:migrationReport_.at("changes")){
        ImGui::PushID(index++);const auto title=std::string(tr(detail.at("kind").get_ref<const std::string&>().c_str()))+" / "+detail.value("scope",detail.value("declaration",std::string()))+" / "+detail.value("nodeId",detail.value("linkId",std::string()));if(ImGui::TreeNode(label(title,"change").c_str())){ImGui::TextWrapped("%s",detail.dump(2).c_str());ImGui::TreePop();}ImGui::PopID();
    }
}
}
