#include "Editor.h"
#include "Graph.h"
namespace cygame {
void EditorApp::graphExpandUI(const std::string& graphId,const std::string& functionName,const std::string& viewKey){
    auto& view=graphViews_[viewKey];const auto popup=label(tr("Expand call"),"expandCall");const auto body=graphView(core.graphs().at(graphId),functionName);bool eligible=false;
    if(view.selection.size()==1)for(const auto& n:body.at("nodes"))if(n.at("id")==*view.selection.begin()&&(n.at("type")=="function.call"||n.at("type")=="macro.call"))eligible=true;
    {cyui::ScopedDisabled disabled(!eligible);if(cyui::ghostButton(tr("Expand call"))){expandRequest_={{"graphId",graphId},{"nodeId",*view.selection.begin()}};if(!functionName.empty())expandRequest_["functionName"]=functionName;expandReport_=Json();ImGui::OpenPopup(popup.c_str());}probeInterface("expand/open");}
    ImGui::SetNextWindowSizeConstraints(cyui::dp(540,0),cyui::dp(760,850));if(!ImGui::BeginPopupModal(popup.c_str(),nullptr,ImGuiWindowFlags_AlwaysAutoResize))return;
    ImGui::TextWrapped("%s",tr("Expand one pure call into editable nodes. The local definition stays available. Preview checks wired argument evaluation."));
    if(cyui::ghostButton(tr("Preview expansion"))){expandReport_=invoke("graph_expand_plan",expandRequest_);expandRevision_=core.revision();expandStale_=false;}probeInterface("expand/preview");
    if(expandReport_.is_object()){
        if(expandReport_.contains("error"))cyui::callout(cyui::Tone::error,expandReport_.at("error").get_ref<const std::string&>().c_str());
        else{
            if(expandRevision_!=core.revision()){expandRevision_=core.revision();expandStale_=!core.graphs().contains(graphId)||sha256(core.graphs().at(graphId).dump())!=expandReport_.at("sourceHash").get<std::string>();}
            if(expandStale_)cyui::callout(cyui::Tone::warning,tr("The graph changed after this preview. Refresh before applying."));if(!expandReport_.value("valid",false))cyui::callout(cyui::Tone::error,expandReport_.value("diagnostic","Invalid expansion").c_str());
            if(cyui::beginProperties("##expandSummary")){
                cyui::property(tr("Name"));ImGui::TextUnformatted(expandReport_.at("name").get_ref<const std::string&>().c_str());cyui::property(tr("Kind"));ImGui::TextUnformatted(tr(expandReport_.at("kind")=="macro"?"Macro":"Function"));
                cyui::property(tr("Cloned nodes"));ImGui::Text("%d",expandReport_.at("clonedNodes").get<int>());cyui::property(tr("Typed reroutes"));ImGui::Text("%zu",expandReport_.at("inputs").size()+expandReport_.at("outputs").size());
                cyui::property(tr("Incoming connections"));ImGui::Text("%d",expandReport_.at("incomingLinks").get<int>());cyui::property(tr("Outgoing connections"));ImGui::Text("%d",expandReport_.at("outgoingLinks").get<int>());cyui::endProperties();
            }
            if(ImGui::BeginTable("##expandPorts",4,ImGuiTableFlags_BordersInnerH|ImGuiTableFlags_RowBg|ImGuiTableFlags_ScrollY,cyui::dp(0,180))){for(const auto* title:{"Direction","Port","Data type","Connected"})ImGui::TableSetupColumn(tr(title));ImGui::TableHeadersRow();for(const auto* side:{"inputs","outputs"})for(const auto& [name,pin]:expandReport_.at(side).items()){ImGui::TableNextRow();ImGui::TableSetColumnIndex(0);ImGui::TextUnformatted(tr(side));ImGui::TableSetColumnIndex(1);ImGui::TextUnformatted(name.c_str());ImGui::TableSetColumnIndex(2);ImGui::TextUnformatted(pin.at("type").get_ref<const std::string&>().c_str());ImGui::TableSetColumnIndex(3);ImGui::TextUnformatted(tr(pin.value("connected",false)?"Yes":"No"));}ImGui::EndTable();}
            ImGui::TextWrapped("%s",tr("Typed reroutes retain call defaults and external wire metadata. A comment group keeps the original call data. Execution calls are not supported."));
        }
    }
    const bool ready=expandReport_.is_object()&&!expandReport_.contains("error")&&expandReport_.value("valid",false)&&!expandStale_;
    {cyui::ScopedDisabled disabled(!ready);if(cyui::primaryButton(tr("Apply expansion"))){auto request=expandRequest_;request["planHash"]=expandReport_.at("planHash");const auto applied=invoke("graph_expand_apply",request);if(!applied.contains("error")){view.selection.clear();for(const auto& id:applied.at("addedNodeIds"))view.selection.insert(id);view.selected=view.selection.empty()?"":*view.selection.begin();view.wireNode.clear();view.dragging.clear();ImGui::CloseCurrentPopup();}}probeInterface("expand/apply");}ImGui::SameLine();if(cyui::ghostButton(tr("Close")))ImGui::CloseCurrentPopup();ImGui::EndPopup();
}
}
