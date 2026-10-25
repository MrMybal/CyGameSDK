#include "Editor.h"
namespace cygame {
void EditorApp::interfaceOverview(){
    cyui::heading(tr("Editor Overview"),2);
    ImGui::TextWrapped("%s",tr("Reusable editor foundation. This inventory describes the shipped Sandbox; adapters supply game-specific capabilities."));
    cyui::searchField("##featureFilter",featureSearch_,sizeof(featureSearch_),tr("Search features, status or limitations"),-1);
    cyui::divider();
    const auto& catalogue=interfaceFeatures_;
    if(!catalogue.is_object()||!catalogue.contains("features")||!catalogue["features"].is_array()){
        cyui::emptyState(tr("Inventory unavailable"),tr("Restore app/interface_features.json from this release."),cyui::icon::info);return;
    }
    ImGui::BeginChild("##featureInventory");
    size_t shown=0;
    for(const auto& feature:catalogue["features"]){
        const std::string name=feature.value("name",""),status=feature.value("status",""),category=feature.value("category",""),notes=feature.value("notes",""),panel=feature.value("panel","");
        if(!matches(name+" "+tr(name.c_str())+" "+status+" "+tr(status.c_str())+" "+category+" "+notes+" "+tr(notes.c_str()),featureSearch_))continue;
        ++shown;ImGui::PushID(name.c_str());
        cyui::textDim("%s",tr(category.c_str()));
        cyui::heading(tr(name.c_str()),3);ImGui::SameLine();
        cyui::badge(tr(status.c_str()),status=="Available"?cyui::Tone::success:status=="Not available"?cyui::Tone::neutral:cyui::Tone::warning);
        ImGui::TextWrapped("%s",tr(notes.c_str()));
        if(!panel.empty()&&cyui::ghostButton(tr("Open panel")))open(panel);
        cyui::divider();ImGui::PopID();
    }
    if(!shown)cyui::textDim("%s",tr("No results"));
    ImGui::EndChild();
}
}
