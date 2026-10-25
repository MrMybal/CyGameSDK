#include "Editor.h"
#include "Graph.h"
namespace cygame {
void EditorApp::focusGraphNode(const std::string& graphId,const std::string& function,const std::string& nodeId){
    const auto graph=graphView(core.graphs().at(graphId),function);for(const auto& n:graph.at("nodes"))if(n.at("id")==nodeId){
        auto& view=graphViews_[graphId+(function.empty()?"":"#"+function)];view.selected=nodeId;view.selection={nodeId};view.pan={cyui::dp(40)-n["position"][0].get<float>()*cyui::dp(view.zoom),cyui::dp(40)-n["position"][1].get<float>()*cyui::dp(view.zoom)};
        activeGraph_=graphId;activeFunction_=function;if(function.empty())open(graphId);else openFunction(graphId,function);return;
    }throw std::runtime_error("Node not found");
}
void EditorApp::graphNavigator(){
    cyui::heading(tr("Graph Navigator"),2);
    bool search=cyui::searchField("##graphSearch",graphSearch_,sizeof(graphSearch_),tr("Find nodes, values, variables, comments"),-1);
    search|=ImGui::Checkbox(tr("All loaded graphs"),&findAllGraphs_);
    if(cyui::ghostButton(tr("Search")))search=true;
    const std::string scope=findAllGraphs_?"":activeGraph_;
    if(search||graphSearchResults_.is_null()||graphSearchRevision_!=core.revision()||graphSearchScope_!=scope){
        Json p={{"query",std::string(graphSearch_)}};if(!scope.empty()&&core.graphs().contains(scope))p["graphId"]=scope;auto result=invoke("graph_find",p);graphSearchResults_=result.value("results",Json::array());graphSearchTruncated_=result.value("truncated",false);graphSearchRevision_=core.revision();graphSearchScope_=scope;
    }
    if(graphSearchTruncated_)cyui::callout(cyui::Tone::warning,tr("Results limited to 2000 nodes. Refine the search."));
    if(graphSearchResults_.empty())cyui::emptyState(tr("No results"),tr("Search by node type, value, variable name or comment."),cyui::icon::search);
    for(const auto& r:graphSearchResults_){
        const std::string id=r.at("graphId"),function=r.at("functionName"),node=r.at("nodeId"),title=r.at("label");
        if(ImGui::Selectable(label(std::string(tr(title.c_str()))+" / "+id+(function.empty()?"":" / "+function),id+"#"+function+"/"+node).c_str()))focusGraphNode(id,function,node);
    }
}
void EditorApp::graphNavigationTools(const std::string& graphId,const std::string& function,const Json& graph){
    auto& view=graphViews_[graphId+(function.empty()?"":"#"+function)];
    auto command=[&](const std::string& op,Json p){p["graphId"]=graphId;if(!function.empty())p["functionName"]=function;return invoke(op,p);};
    bool find=ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)&&!ImGui::GetIO().WantTextInput&&ImGui::GetIO().KeyCtrl&&ImGui::IsKeyPressed(ImGuiKey_F);
    if(cyui::iconButton(cyui::icon::search,tr("Find in graph   Ctrl+F")))find=true;
    if(find){activeGraph_=graphId;activeFunction_=function;graphSearchResults_=Json();open("navigator");}
    ImGui::SameLine();if(cyui::iconButton(cyui::icon::star,tr("Bookmark this view")))ImGui::OpenPopup("##newBookmark");
    if(ImGui::BeginPopup("##newBookmark")){
        ImGui::InputText(tr("Name"),bookmarkName_,sizeof(bookmarkName_));if(cyui::primaryButton(tr("Save"))){auto r=command("graph_bookmark_add",{{"name",std::string(bookmarkName_)},{"pan",{view.pan.x/cyui::dp(1),view.pan.y/cyui::dp(1)}},{"zoom",view.zoom}});if(!r.contains("error"))ImGui::CloseCurrentPopup();}ImGui::EndPopup();
    }
    ImGui::SameLine();ImGui::SetNextItemWidth(cyui::dp(160));
    if(ImGui::BeginCombo("##bookmarks",tr("Bookmarks"))){const auto marks=graph.value("bookmarks",Json::array());for(const auto& mark:marks){ImGui::PushID(mark.at("id").get_ref<const std::string&>().c_str());if(ImGui::Selectable(mark.at("name").get_ref<const std::string&>().c_str())){view.pan={cyui::dp(mark["pan"][0].get<float>()),cyui::dp(mark["pan"][1].get<float>())};view.zoom=mark.at("zoom");}ImGui::SameLine();if(cyui::ghostButton(tr("Remove")))command("graph_bookmark_remove",{{"bookmarkId",mark.at("id")}});ImGui::PopID();}ImGui::EndCombo();}
    ImGui::SameLine();
}
void EditorApp::graphCommentProperties(const std::string& graphId,const std::string& function,const Json& comments){
    cyui::sectionTitle(tr("Comment groups"));
    for(const auto& c:comments){
        const std::string id=c.at("id");ImGui::PushID(id.c_str());
        if(cyui::beginSection(c.at("text").get_ref<const std::string&>().c_str())){
            Json p={{"graphId",graphId},{"commentId",id}};if(!function.empty())p["functionName"]=function;
            if(cyui::beginProperties("##commentProps",cyui::dp(72))){
                for(const auto* key:{"text","position","size"}){cyui::property(tr(key));Json next,spec=std::string(key)=="text"?Json{{"type","string"},{"maxLength",2048}}:Json{{"type","array"},{"items",{{"minimum",std::string(key)=="size"?40:-100000},{"maximum",std::string(key)=="size"?10000:100000}}}};if(editValue(graphId+"#"+function+"/comment/"+id+"/"+key,spec,c.at(key),next)){auto request=p;request[key]=next;invoke("graph_comment_edit",request);}}
                cyui::endProperties();
            }
            if(cyui::ghostButton(tr("Remove group")))invoke("graph_comment_remove",p);cyui::endSection();
        }ImGui::PopID();
    }
}
}
