#include "Editor.h"
#include "Graph.h"
#include <cstdio>
namespace cygame {
void EditorApp::materialInstanceEditor(const std::string& id){
    const auto source=core.graphs().at(id);auto& state=materialViews_[id];bool refresh=false;
    if(cyui::primaryButton(tr("Save asset")))invoke("asset_save_graph",{{"graphId",id}});
    ImGui::SameLine();if(cyui::ghostButton(tr("Open parent"))){auto r=invoke("asset_open",{{"path",source.at("parent")}});if(r.contains("graphId"))open(r.at("graphId"));}
    ImGui::SameLine();if(cyui::ghostButton(tr("Refresh material")))refresh=true;
    ImGui::SameLine();if(cyui::ghostButton(tr("Compare saved")))invoke("graph_diff_view",{{"graphId",id}});
    ImGui::SameLine();cyui::badge(tr("MATERIAL INSTANCE"));
    if(refresh)shaderViews_[id].revision=~uint64_t(0);
    if(refresh||state.revision!=core.revision()||state.report.is_null()){state.report=invoke("material_get",{{"graphId",id}});state.revision=core.revision();}
    if(materialParentKey_!=id){materialParentKey_=id;std::snprintf(materialParent_,sizeof(materialParent_),"%s",source.at("parent").get_ref<const std::string&>().c_str());}
    ImGui::BeginChild("##instanceParameters",{ImGui::GetContentRegionAvail().x*.66f,0});
    cyui::heading(tr("Material parameters"),2);
    if(cyui::beginProperties("##instanceParent",cyui::dp(90))){cyui::property(tr("Parent"));if(ImGui::InputText("##parent",materialParent_,sizeof(materialParent_),ImGuiInputTextFlags_EnterReturnsTrue)){invoke("material_parent_set",{{"graphId",id},{"parent",std::string(materialParent_)}});refresh=true;}cyui::endProperties();}
    ImGui::TextWrapped("%s",tr("Enable an override to change a value. Disable it to inherit the parent."));
    const auto report=state.report;
    if(report.contains("error")){
        cyui::callout(cyui::Tone::error,report.at("error").get_ref<const std::string&>().c_str());
        for(const auto& [name,value]:source.at("overrides").items()){ImGui::PushID(name.c_str());ImGui::Text("%s = %s",name.c_str(),value.dump().c_str());if(cyui::ghostButton(tr("Remove override")))invoke("material_override_remove",{{"graphId",id},{"name",name}});ImGui::PopID();}
    }else{
        for(const auto& parameter:report.at("parameters")){
            const std::string name=parameter.at("name"),type=parameter.at("type");ImGui::PushID(name.c_str());bool enabled=parameter.at("overridden");
            if(ImGui::Checkbox(name.c_str(),&enabled)){if(enabled)invoke("material_override_set",{{"graphId",id},{"name",name},{"value",parameter.at("value")}});else invoke("material_override_remove",{{"graphId",id},{"name",name}});}
            cyui::ScopedDisabled inherited(!enabled);Json spec=type=="float"?Json{{"type","number"},{"minimum",-1e9},{"maximum",1e9}}:type=="bool"?Json{{"type","boolean"}}:type=="string"?Json{{"type","string"}}:Json{{"type","array"},{"x-editor-widget","color"}},next;
            if(editValue(id+"/instance/"+name,spec,parameter.at("value"),next))invoke("material_override_set",{{"graphId",id},{"name",name},{"value",next}});
            cyui::textDim("%s: %s",tr("Inherited"),parameter.at("inherited").dump().c_str());ImGui::PopID();
        }
        if(report.at("parameters").empty())cyui::emptyState(tr("No material parameters"),tr("Declare variables in the parent material to expose editable parameters."),cyui::icon::info);
        cyui::sectionTitle(tr("Parent chain"));for(const auto& path:report.at("parents"))ImGui::BulletText("%s",path.get_ref<const std::string&>().c_str());
    }
    ImGui::EndChild();ImGui::SameLine();ImGui::BeginChild("##instancePreview",{0,0});
    materialPreviewUI(id);
    ImGui::EndChild();
}
}
