#include "Editor.h"
#include "Graph.h"
#include <algorithm>
#include <cstdio>
namespace cygame {
void EditorApp::referenceViewer(){
    bool refresh=false;cyui::heading(tr("Reference Viewer"),2);
    cyui::textDim("%s",tr("Asset path"));
    ImGui::SetNextItemWidth(std::max(cyui::dp(100),ImGui::GetContentRegionAvail().x-cyui::dp(100)));
    refresh=ImGui::InputText("##referencePath",referencePath_,sizeof(referencePath_),ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();if(cyui::ghostButton(tr("Refresh")))refresh=true;
    if(refresh||referenceReport_.is_null()){auto result=invoke("asset_dependencies",{{"path",std::string(referencePath_)}});if(!result.contains("error"))referenceReport_=result;}
    if(!referenceReport_.is_object()||!referenceReport_.contains("dependencies")){cyui::emptyState(tr("No asset selected"),tr("Open References from an asset context menu."),cyui::icon::info);return;}
    const auto report=referenceReport_;
    ImGui::TextWrapped("%s",tr("References combine saved assets and live edits. Save consumers after changing their references."));
    if(report.value("incomplete",false))cyui::callout(cyui::Tone::warning,tr("Reference scan incomplete. File moves are blocked until errors or limits are resolved."));
    ImGui::BeginChild("##referenceDiagram",{0,cyui::dp(240)},ImGuiChildFlags_Borders,ImGuiWindowFlags_NoScrollbar);
    const auto origin=ImGui::GetCursorScreenPos(),size=ImGui::GetContentRegionAvail();auto* draw=ImGui::GetWindowDrawList();const auto& pal=cyui::palette();
    const float width=std::max(cyui::dp(90),std::min(cyui::dp(230),size.x*.28f)),height=cyui::dp(40);
    ImVec2 center{origin.x+size.x*.5f-width*.5f,origin.y+size.y*.5f-height*.5f};
    int diagramId=0;auto box=[&](const std::string& path,ImVec2 point,bool exists,bool selected){
        draw->AddRectFilled(point,{point.x+width,point.y+height},selected?pal.selection:pal.bg3,cyui::dp(4));draw->AddRect(point,{point.x+width,point.y+height},!exists?pal.error:selected?pal.accent:pal.lineStrong,cyui::dp(4));
        cyui::clippedText(draw,{point.x+cyui::dp(8),point.y+cyui::dp(12)},width-cyui::dp(16),pal.text,path.c_str());
        ImGui::PushID(diagramId++);ImGui::SetCursorScreenPos(point);if(ImGui::InvisibleButton(path.c_str(),{width,height})&&!selected){std::snprintf(referencePath_,sizeof(referencePath_),"%s",path.c_str());referenceReport_=Json();}ImGui::PopID();
    };
    for(int side=0;side<2;++side){const auto& items=report.at(side==0?"referencers":"dependencies");const int count=std::min(4,int(items.size()));
        for(int i=0;i<count;++i){ImVec2 point{side==0?origin.x:origin.x+size.x-width,origin.y+cyui::dp(12)+i*cyui::dp(48)};ImVec2 a{side==0?point.x+width:center.x+width,side==0?point.y+height*.5f:center.y+height*.5f},b{side==0?center.x:point.x,side==0?center.y+height*.5f:point.y+height*.5f};draw->AddBezierCubic(a,{a.x+cyui::dp(30),a.y},{b.x-cyui::dp(30),b.y},b,pal.info,cyui::dp(2));box(items[i].at("path"),point,items[i].at("exists"),false);}
    }
    box(report.at("path"),center,report.value("exists",false),true);ImGui::EndChild();
    for(const auto* direction:{"referencers","dependencies"}){
        cyui::sectionTitle(tr(direction));for(const auto& entry:report.at(direction)){const std::string path=entry.at("path");ImGui::PushID(direction);if(ImGui::Selectable(path.c_str())){std::snprintf(referencePath_,sizeof(referencePath_),"%s",path.c_str());referenceReport_=Json();}if(!entry.at("exists").get<bool>()){ImGui::SameLine();cyui::badge(tr("Missing"),cyui::Tone::error);}ImGui::PopID();}
    }
    std::string graphId;for(const auto& [id,g]:core.graphs().items())if(g.value("assetPath","")==report.at("path").get<std::string>())graphId=id;
    if(!graphId.empty()){
        cyui::sectionTitle(tr("Declared references"));const auto graph=core.graphs().at(graphId);const auto refs=graph.value("references",Json::array());
        for(size_t i=0;i<refs.size();++i){ImGui::PushID(int(i));ImGui::TextUnformatted(refs[i].get_ref<const std::string&>().c_str());ImGui::SameLine();if(cyui::ghostButton(tr("Remove reference"))){auto next=refs;next.erase(next.begin()+i);auto result=invoke("asset_references_set",{{"graphId",graphId},{"paths",next}});if(!result.contains("error"))referenceReport_=Json();}ImGui::PopID();}
        ImGui::InputText(tr("Reference target"),newReference_,sizeof(newReference_));
        if(cyui::ghostButton(tr("Add reference"))){auto next=refs;next.push_back(std::string(newReference_));auto result=invoke("asset_references_set",{{"graphId",graphId},{"paths",next}});if(!result.contains("error"))referenceReport_=Json();}
        ImGui::SameLine();if(cyui::primaryButton(tr("Save asset"))){invoke("asset_save_graph",{{"graphId",graphId}});referenceReport_=Json();}
    }else if(report.value("exists",false)&&fs::u8path(report.at("path").get<std::string>()).extension()==".cyasset"){
        if(cyui::ghostButton(tr("Open asset"))){auto result=invoke("asset_open",{{"path",report.at("path")}});if(result.contains("graphId"))open(result.at("graphId"));}
    }
    for(const auto& error:report.value("errors",Json::array()))ImGui::TextWrapped("%s: %s",error.at("path").get_ref<const std::string&>().c_str(),error.at("message").get_ref<const std::string&>().c_str());
}
}
