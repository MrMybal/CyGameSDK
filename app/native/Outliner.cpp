#include "Editor.h"
#include "UiPropertyAdapter.h"
#include <algorithm>
#include <set>
namespace cygame {
void EditorApp::outliner(){
    cyui::textDim("%zu %s / %zu %s",core.entities().size(),tr("objects"),core.selectedIds().size(),tr("selected"));
    if(cyui::ghostButton(tr(outlinerTable_?"Hierarchy":"Table view"))){outlinerTable_=!outlinerTable_;savePreferences();}
    ImGui::SameLine();if(cyui::ghostButton(tr("Focus selection"))&&!core.selection().empty())invoke("viewport_focus",{{"entityId",core.selection()}});
    const auto entities=core.entities();std::map<cyui::ItemId,std::string> objects,folders;
    std::vector<cyui::HierarchyItem> items;std::set<std::string> folderNames;
    for(const auto& e:entities)if(!e.value("folder","").empty())folderNames.insert(e.at("folder"));
    for(const auto& folder:folderNames){const auto id=uiId("outliner/folder/"+folder);folders[id]=folder;items.push_back({id,0,folder,"Folder",true,false});}
    for(const auto& e:entities){
        const auto name=e.at("id").get<std::string>(),folder=e.value("folder","");const auto id=uiId("outliner/entity/"+name);
        if(folders.count(id)||!objects.emplace(id,name).second)throw std::runtime_error("UI hierarchy identity collision");
        items.push_back({id,folder.empty()?0:uiId("outliner/folder/"+folder),e.at("properties").value("name",name),e.value("type","Object"),e.at("properties").value("enabled",true),e.value("editorLocked",false)});
    }
    auto selectedIds=[&](const std::vector<cyui::ItemId>& selected){std::vector<std::string> ids;for(const auto id:selected)if(objects.count(id))ids.push_back(objects.at(id));return ids;};
    if(outlinerTable_){
        std::vector<cyui::DataColumn> columns;std::vector<std::string> keys;
        const auto schema=core.adapter().propertySchema(Json::object()).at("properties");
        if(schema.contains("name"))keys.push_back("name");for(const auto& [key,spec]:schema.items())if(key!="name")keys.push_back(key);
        for(const auto& key:keys){const auto& spec=schema.at(key);
            cyui::DataColumn column;column.id=uiId("outliner/column/"+key);column.label=tr(spec.value("title",key).c_str());column.editable=!spec.value("readOnly",false);
            uiPropertyMetadata(column.metadata,spec);column.width=key=="name"?180.f:140.f;columns.push_back(column);
        }
        cyui::DataColumn kind;kind.id=uiId("outliner/meta/type");kind.label=tr("Type");kind.width=110;kind.metadata.readOnly=true;
        const size_t kindIndex=std::min(size_t(1),columns.size());columns.insert(columns.begin()+kindIndex,kind);keys.insert(keys.begin()+kindIndex,"");
        std::vector<cyui::DataRow> rows;
        for(const auto& e:entities){
            const auto name=e.at("id").get<std::string>();cyui::DataRow row;row.id=uiId("outliner/entity/"+name);row.readOnly=e.value("editorLocked",false);
            for(const auto& key:keys){if(key.empty())row.cells.push_back(e.value("type",std::string("Object")));else row.cells.push_back(uiPropertyValue(schema.at(key),e.at("properties").at(key),"entity/"+name,[this](const auto& id,const auto& caption){return rawCaption(id,caption);}));}
            rows.push_back(std::move(row));
        }
        if(tableSelection_!=core.selectedIds()){sceneTable_.selection.clear();for(const auto& id:core.selectedIds())sceneTable_.selection.push_back(uiId("outliner/entity/"+id));tableSelection_=core.selectedIds();}
        cyui::DataViewOptions options;options.revision=core.revision();
        options.validate=[&](cyui::ItemId row,cyui::ItemId column,const cyui::DataCell& value){
            try{const auto& e=core.entity(objects.at(row));if(e.value("editorLocked",false))return std::string("Object is locked");
                for(size_t i=0;i<columns.size();++i)if(columns[i].id==column){const auto& key=keys[i];if(key.empty())return std::string("Property is read-only");const auto field=core.adapter().propertySchema(e).at("properties").at(key);if(field.value("readOnly",false))return std::string("Property is read-only");validateValue(jsonPropertyValue(field,value,e.at("properties").at(key)),field,key);return std::string{};}
                return std::string("Unknown property");
            }catch(const std::exception& e){return std::string(e.what());}
        };
        const auto revision=core.revision();const auto edits=cyui::dataView("##sceneTable",sceneTable_,columns,rows,ImGui::GetContentRegionAvail(),options);
        probeInterface("outliner/table");
        Json batch=Json::array();for(const auto& edit:edits)if(edit.edit.committed())for(size_t i=0;i<columns.size();++i)if(columns[i].id==edit.column){
            const auto& e=core.entity(objects.at(edit.row));const auto& key=keys[i];if(key.empty())continue;batch.push_back({{"entityId",e.at("id")},{"propertyId",key},{"value",jsonPropertyValue(schema.at(key),edit.edit.value,e.at("properties").at(key))}});
        }
        if(!batch.empty())invoke("property_set_many",{{"edits",batch},{"expectedRevision",revision}});
        const auto selection=selectedIds(sceneTable_.selection);if(selection!=core.selectedIds())invoke("scene_select_many",{{"entityIds",selection}});tableSelection_=core.selectedIds();
        return;
    }
    if(hierarchySelection_!=core.selectedIds()){hierarchyState_.selection.clear();for(const auto& id:core.selectedIds())hierarchyState_.selection.push_back(uiId("outliner/entity/"+id));hierarchySelection_=core.selectedIds();}
    cyui::HierarchyOptions options;
    options.canReparent=[&](cyui::ArrayView<cyui::ItemId> ids,cyui::ItemId parent){if(parent&&!folders.count(parent))return false;for(size_t i=0;i<ids.count;++i)if(!objects.count(ids[i]))return false;return true;};
    const auto area=ImGui::GetCursorScreenPos(),room=ImGui::GetContentRegionAvail();
    const auto requests=cyui::hierarchy("##worldTree",hierarchyState_,items,room,options);
    if(interfaceProbe){auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();a.x+=ImGui::GetStyle().WindowPadding.x+ImGui::GetFrameHeight()+ImGui::GetStyle().ItemSpacing.x;a.y+=ImGui::GetStyle().WindowPadding.y;b.x-=cyui::dp(70);b.y=a.y+ImGui::GetFrameHeight();interfaceProbe("outliner/first",a,b);}
    auto selection=selectedIds(hierarchyState_.selection);
    if(selection!=core.selectedIds())invoke("scene_select_many",{{"entityIds",selection}});hierarchySelection_=core.selectedIds();
    for(const auto& request:requests){
        const auto ids=selectedIds(request.ids);if(ids.empty()){cyui::notify(tr("Folder actions apply to scene objects."),cyui::Tone::warning);continue;}
        if(request.action==cyui::HierarchyAction::duplicate)invoke("scene_duplicate",{{"entityIds",ids}});
        if(request.action==cyui::HierarchyAction::remove)invoke("scene_delete_many",{{"entityIds",ids}});
        if(request.action==cyui::HierarchyAction::locking)invoke("scene_lock",{{"entityIds",ids},{"locked",request.value}});
        if(request.action==cyui::HierarchyAction::visibility||request.action==cyui::HierarchyAction::rename){
            Json batch=Json::array();for(const auto& id:ids)batch.push_back({{"entityId",id},{"propertyId",request.action==cyui::HierarchyAction::rename?"name":"enabled"},{"value",request.action==cyui::HierarchyAction::rename?Json(request.text):Json(request.value)}});
            invoke("property_set_many",{{"edits",batch},{"expectedRevision",core.revision()}});
        }
        if(request.action==cyui::HierarchyAction::reparent&&(request.parent==0||folders.count(request.parent)))
            invoke("scene_folder_set",{{"entityIds",ids},{"folder",request.parent?folders.at(request.parent):""}});
    }
    if(ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)&&!ImGui::GetIO().WantTextInput){
        if(ImGui::GetIO().KeyCtrl&&ImGui::IsKeyPressed(ImGuiKey_D))invoke("scene_duplicate");
    }
    if(ImGui::IsMouseDoubleClicked(0)&&ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows)&&
       ImGui::IsMouseHoveringRect(area,{area.x+room.x,area.y+room.y})&&objects.count(hierarchyState_.focused))
        invoke("viewport_focus",{{"entityId",objects.at(hierarchyState_.focused)}});
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
