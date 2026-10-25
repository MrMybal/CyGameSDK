#include "Editor.h"
#include <algorithm>
#include <set>
namespace cygame {
void EditorApp::outliner(){
    cyui::searchField("##objects",objectSearch_,sizeof(objectSearch_),tr("Search objects"),-1);
    cyui::textDim("%zu %s / %zu %s",core.entities().size(),tr("objects"),core.selectedIds().size(),tr("selected"));
    if(ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)&&!ImGui::GetIO().WantTextInput){
        if(ImGui::GetIO().KeyCtrl&&ImGui::IsKeyPressed(ImGuiKey_D))invoke("scene_duplicate");
        if(ImGui::IsKeyPressed(ImGuiKey_Delete))invoke("scene_delete_many");
    }
    auto folderDrop=[&](const std::string& folder){
        if(ImGui::BeginDragDropTarget()){
            if(const auto* payload=ImGui::AcceptDragDropPayload("CYGAME_ACTORS")){
                try{auto ids=Json::parse(std::string(static_cast<const char*>(payload->Data),payload->DataSize-1));invoke("scene_folder_set",{{"entityIds",ids},{"folder",folder}});}
                catch(const std::exception& e){cyui::notify(e.what(),cyui::Tone::error);}
            }ImGui::EndDragDropTarget();
        }
    };
    if(ImGui::Selectable(tr("World"),false))invoke("scene_select_many",{{"entityIds",Json::array()}});
    folderDrop("");
    std::set<std::string> folders={""};const auto entities=core.entities();
    for(const auto& e:entities)folders.insert(e.value("folder",""));
    for(const auto& folder:folders){
        bool expanded=true;
        if(!folder.empty()){expanded=ImGui::TreeNodeEx(folder.c_str(),ImGuiTreeNodeFlags_DefaultOpen);folderDrop(folder);}
        if(expanded&&ImGui::BeginTable(("##actors"+folder).c_str(),4,ImGuiTableFlags_RowBg|ImGuiTableFlags_Resizable)){
            ImGui::TableSetupColumn(tr("Label"),ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn(tr("Type"),ImGuiTableColumnFlags_WidthFixed,cyui::dp(65));
            ImGui::TableSetupColumn("V",ImGuiTableColumnFlags_WidthFixed,cyui::dp(22));
            ImGui::TableSetupColumn("L",ImGuiTableColumnFlags_WidthFixed,cyui::dp(22));ImGui::TableHeadersRow();
            for(const auto& e:entities){
                std::string id=e.at("id"),name=e.at("properties").at("name"),type=e.value("type","Object");
                if(e.value("folder","")!=folder||!matches(name+" "+type,objectSearch_))continue;
                ImGui::PushID(id.c_str());ImGui::TableNextRow();ImGui::TableNextColumn();
                if(ImGui::Selectable(label(name,id).c_str(),core.isSelected(id),ImGuiSelectableFlags_AllowDoubleClick))invoke("scene_select",{{"entityId",id},{"mode",ImGui::GetIO().KeyCtrl?"toggle":"replace"}});
                if(ImGui::IsItemHovered()&&ImGui::IsMouseDoubleClicked(0))invoke("viewport_focus",{{"entityId",id}});
                if(ImGui::BeginDragDropSource()){
                    auto ids=core.isSelected(id)?Json(core.selectedIds()):Json::array({id});auto text=ids.dump();
                    ImGui::SetDragDropPayload("CYGAME_ACTORS",text.c_str(),text.size()+1);ImGui::Text("%zu %s",ids.size(),tr("objects"));ImGui::EndDragDropSource();
                }
                if(ImGui::BeginPopupContextItem("##actorMenu")){
                    if(ImGui::MenuItem(tr("Select object")))invoke("scene_select",{{"entityId",id}});
                    if(ImGui::MenuItem(tr("Duplicate selected objects"),"Ctrl+D"))invoke("scene_duplicate",{{"entityIds",core.isSelected(id)?Json(core.selectedIds()):Json::array({id})}});
                    if(ImGui::MenuItem(tr("Delete selection"),"Delete"))invoke("scene_delete_many",{{"entityIds",core.isSelected(id)?Json(core.selectedIds()):Json::array({id})}});
                    ImGui::EndPopup();
                }
                ImGui::TableNextColumn();cyui::textFaint("%s",type.c_str());
                ImGui::TableNextColumn();bool enabled=e.at("properties").value("enabled",true),locked=e.value("editorLocked",false);
                {cyui::ScopedDisabled disabled(locked);if(ImGui::Checkbox("##visible",&enabled))invoke("property_set",{{"entityId",id},{"propertyId","enabled"},{"value",enabled}});}
                ImGui::TableNextColumn();if(ImGui::Checkbox("##locked",&locked))invoke("scene_lock",{{"entityIds",Json::array({id})},{"locked",locked}});
                ImGui::PopID();
            }ImGui::EndTable();
        }
        if(!folder.empty()&&expanded)ImGui::TreePop();
    }
}
void EditorApp::historyPanel(){
    if(cyui::ghostButton(tr("Undo"))&&core.canUndo())invoke("edit_undo");ImGui::SameLine();
    if(cyui::ghostButton(tr("Redo"))&&core.canRedo())invoke("edit_redo");
    auto history=core.execute("edit_history");cyui::sectionTitle(tr("Undo history"));
    for(const auto& item:history.at("undo"))ImGui::BulletText("%s",tr(item.get_ref<const std::string&>().c_str()));
    if(history["undo"].empty())cyui::textDim("%s",tr("No edits yet"));
    cyui::sectionTitle(tr("Redo history"));for(const auto& item:history["redo"])ImGui::BulletText("%s",tr(item.get_ref<const std::string&>().c_str()));
}
}
