#include "Editor.h"
#include "Graph.h"
namespace cygame {
void EditorApp::graphDiffPanel(){
    cyui::heading(tr("Graph Changes"),2);
    if(diffGraph_.empty()||!core.graphs().contains(diffGraph_)){diffGraph_=core.graphs().contains(activeGraph_)?activeGraph_:"logic";diffReport_=Json();}
    bool refresh=false;ImGui::SetNextItemWidth(cyui::dp(340));
    if(ImGui::BeginCombo("##diffGraph",diffGraph_.c_str())){for(const auto& [key,g]:core.graphs().items())if(ImGui::Selectable(key.c_str(),key==diffGraph_)){diffGraph_=key;refresh=true;}ImGui::EndCombo();}
    ImGui::SameLine();if(cyui::ghostButton(tr("Refresh comparison")))refresh=true;
    ImGui::SameLine();if(cyui::ghostButton(tr("Save asset"))){invoke("asset_save_graph",{{"graphId",diffGraph_}});refresh=true;}
    refresh|=ImGui::Checkbox(tr("Ignore layout and comments"),&diffIgnorePresentation_);
    if(refresh||diffReport_.is_null()||diffRevision_!=core.revision()){
        diffReport_=invoke("graph_diff",{{"graphId",diffGraph_},{"ignorePresentation",diffIgnorePresentation_}});diffRevision_=core.revision();diffSelection_=-1;
    }
    if(diffReport_.contains("error")){cyui::callout(cyui::Tone::error,diffReport_.at("error").get_ref<const std::string&>().c_str());return;}
    cyui::textDim("%s: %s %s",tr("Baseline"),tr(diffReport_.at("baselineSource").get_ref<const std::string&>().c_str()),diffReport_.at("path").get_ref<const std::string&>().c_str());
    if(diffReport_.at("externalChanges").get<bool>())cyui::callout(cyui::Tone::info,tr("The file on disk changed since it was loaded or saved."));
    if(diffReport_.at("truncated").get<bool>())cyui::callout(cyui::Tone::warning,tr("Comparison details are limited. The summary counts all changes."));
    const auto& summary=diffReport_.at("summary");for(const auto* key:{"Added","Removed","Changed"}){cyui::badge((std::string(tr(key))+": "+std::to_string(summary.at(key).get<int>())).c_str());ImGui::SameLine();}ImGui::NewLine();
    const auto& entries=diffReport_.at("entries");if(entries.empty()){cyui::emptyState(tr("No saved differences"),tr("The current graph matches the selected baseline."),cyui::icon::check);return;}
    if(diffSelection_<0)diffSelection_=0;
    ImGui::BeginChild("##diffEntries",{ImGui::GetContentRegionAvail().x*.42f,0});
    for(int i=0;i<entries.size();++i){const auto& e=entries[i];const std::string function=e.at("functionName");const auto text=std::string(tr(e.at("change").get_ref<const std::string&>().c_str()))+" / "+tr(e.at("kind").get_ref<const std::string&>().c_str())+" / "+(function.empty()?"":function+" / ")+e.at("key").get<std::string>();
        if(ImGui::Selectable(label(text,std::to_string(i)).c_str(),diffSelection_==i))diffSelection_=i;
    }ImGui::EndChild();ImGui::SameLine();ImGui::BeginChild("##diffDetails",{0,0});
    if(diffSelection_>=0&&diffSelection_<entries.size()){
        const auto& e=entries[diffSelection_];const std::string function=e.at("functionName"),node=e.at("key");
        if(e.at("kind")=="nodes"&&e.at("change")!="Removed"&&cyui::ghostButton(tr("Locate current node")))focusGraphNode(diffGraph_,function,node);
        for(const auto* key:{"beforePreview","afterPreview"}){
            cyui::sectionTitle(tr(std::string(key)=="beforePreview"?"Saved version":"Current version"));
            std::string value=e.at(key);std::vector<char> buffer(value.begin(),value.end());buffer.push_back(0);
            ImGui::InputTextMultiline(key,buffer.data(),buffer.size(),{-1,cyui::dp(180)},ImGuiInputTextFlags_ReadOnly);
        }
    }else cyui::textDim("%s",tr("Select a change to inspect its saved and current values."));
    ImGui::EndChild();
}
}
