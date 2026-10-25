#include "Editor.h"
#include "Graph.h"
namespace cygame {
void EditorApp::graphCollapseUI(const std::string& graphId,const std::string& functionName,const std::string& viewKey){
    auto& view=graphViews_[viewKey];const auto popup=label(tr("Collapse selection"),"collapseSelection");
    {cyui::ScopedDisabled disabled(view.selection.empty());if(cyui::ghostButton(tr("Collapse selection"))){
        collapseRequest_=Json();collapseReport_=Json();collapseKind_=0;const auto functions=core.graphs().at(graphId).value("functions",Json::object());std::string name="CollapsedGraph";for(int i=1;functions.contains(name);++i)name="CollapsedGraph"+std::to_string(i);std::snprintf(collapseName_,sizeof(collapseName_),"%s",name.c_str());ImGui::OpenPopup(popup.c_str());
    }probeInterface("collapse/open");}
    ImGui::SetNextWindowSizeConstraints(cyui::dp(520,0),cyui::dp(720,850));
    if(!ImGui::BeginPopupModal(popup.c_str(),nullptr,ImGuiWindowFlags_AlwaysAutoResize))return;
    ImGui::TextWrapped("%s",tr("Pure calculations only. Each exported output must use every selected node and incoming wire."));
    if(cyui::beginProperties("##collapseFields")){
        cyui::property(tr("Name"));if(ImGui::InputText("##name",collapseName_,sizeof(collapseName_)))collapseReport_=Json();
        cyui::property(tr("Kind"));if(ImGui::RadioButton(label(tr("Function"),"collapseFunction").c_str(),collapseKind_==0)){collapseKind_=0;collapseReport_=Json();}probeInterface("collapse/kind/function");ImGui::SameLine();
        {cyui::ScopedDisabled disabled(core.graphs().at(graphId).at("domain")!="logic");if(ImGui::RadioButton(label(tr("Macro"),"collapseMacro").c_str(),collapseKind_==1)){collapseKind_=1;collapseReport_=Json();}probeInterface("collapse/kind/macro");}cyui::endProperties();
    }
    if(cyui::ghostButton(tr("Preview collapse"))){
        collapseRequest_={{"graphId",graphId},{"nodeIds",view.selection},{"name",std::string(collapseName_)},{"kind",collapseKind_?"macro":"function"}};if(!functionName.empty())collapseRequest_["functionName"]=functionName;
        collapseReport_=invoke("graph_collapse_plan",collapseRequest_);collapseRevision_=core.revision();collapseStale_=false;
    }probeInterface("collapse/preview");
    if(!collapseReport_.is_null()){
        if(collapseReport_.contains("error"))cyui::callout(cyui::Tone::error,collapseReport_.at("error").get_ref<const std::string&>().c_str());
        else{
            if(collapseRevision_!=core.revision()){collapseRevision_=core.revision();collapseStale_=!core.graphs().contains(graphId)||sha256(core.graphs().at(graphId).dump())!=collapseReport_.at("sourceHash").get<std::string>();}
            if(collapseStale_)cyui::callout(cyui::Tone::warning,tr("The graph changed after this preview. Refresh before applying."));
            if(!collapseReport_.value("valid",false))cyui::callout(cyui::Tone::error,collapseReport_.value("diagnostic","Invalid collapse").c_str());
            if(cyui::beginProperties("##collapseSummary")){
                cyui::property(tr("Selected nodes"));ImGui::Text("%zu",collapseReport_.at("movedNodeIds").size());
                cyui::property(tr("Incoming connections"));ImGui::Text("%d",collapseReport_.at("incomingLinks").get<int>());
                cyui::property(tr("Outgoing connections"));ImGui::Text("%d",collapseReport_.at("outgoingLinks").get<int>());cyui::endProperties();
            }
            if(ImGui::BeginTable("##collapsePorts",4,ImGuiTableFlags_BordersInnerH|ImGuiTableFlags_RowBg|ImGuiTableFlags_ScrollY,cyui::dp(0,180))){
                for(const auto* title:{"Direction","Port","Data type","Connections"})ImGui::TableSetupColumn(tr(title));ImGui::TableHeadersRow();
                for(const auto* side:{"inputs","outputs"})for(const auto& [name,pin]:collapseReport_.at(side).items()){
                    ImGui::TableNextRow();ImGui::TableSetColumnIndex(0);ImGui::TextUnformatted(tr(side));ImGui::TableSetColumnIndex(1);ImGui::TextUnformatted(name.c_str());ImGui::TableSetColumnIndex(2);ImGui::TextUnformatted(pin.at("type").get_ref<const std::string&>().c_str());ImGui::TableSetColumnIndex(3);ImGui::Text("%d",pin.value("connections",1));
                }ImGui::EndTable();
            }
            if(ImGui::TreeNode(tr("Boundary details"))){ImGui::BeginChild("##collapseDetails",cyui::dp(0,140),ImGuiChildFlags_Borders);ImGui::TextWrapped("%s",Json{{"inputs",collapseReport_.at("inputs")},{"outputs",collapseReport_.at("outputs")}}.dump(2).c_str());ImGui::EndChild();ImGui::TreePop();}
        }
    }
    const bool ready=collapseReport_.is_object()&&!collapseReport_.contains("error")&&collapseReport_.value("valid",false)&&!collapseStale_;
    {cyui::ScopedDisabled disabled(!ready);if(cyui::primaryButton(tr("Apply collapse"))){auto request=collapseRequest_;request["planHash"]=collapseReport_.at("planHash");const auto applied=invoke("graph_collapse_apply",request);if(!applied.contains("error")){view.selection={applied.at("callNodeId").get<std::string>()};view.selected=*view.selection.begin();view.wireNode.clear();view.dragging.clear();ImGui::CloseCurrentPopup();}}probeInterface("collapse/apply");}
    ImGui::SameLine();if(cyui::ghostButton(tr("Close")))ImGui::CloseCurrentPopup();ImGui::EndPopup();
}
}
