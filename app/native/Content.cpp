#include "Editor.h"
#include <algorithm>
#include <cstdio>
namespace cygame {
namespace { void copyTo(char* dst,size_t size,const std::string& value){std::snprintf(dst,size,"%s",value.c_str());} }
void EditorApp::assetsRefresh(){
    if(clearThumbnails)clearThumbnails();
    const char* types[]={"All","Graph","Texture","Mesh","Audio","Other"};
    try{
        auto result=core.execute("assets_list",{{"query",std::string(assetSearch_)},{"type",types[assetType_]}});
        assets_=result.at("assets");assetFolders_=result.value("folders",Json::array());
    }catch(const std::exception& e){core.log(e.what());cyui::notify(e.what(),cyui::Tone::error);}
}
void EditorApp::assetContext(const Json& asset){
    std::string path=asset.at("path");
    if(ImGui::BeginPopupContextItem("##assetContext")){
        if(ImGui::MenuItem(tr("Open asset")))openAsset(asset);
        if(asset.value("imported",false)&&ImGui::MenuItem(tr("Import settings / Reimport")))invoke("asset_import_view",{{"path",path}});
        if(asset.at("type")=="Graph"&&ImGui::MenuItem(tr("Compare saved"))){auto result=invoke("asset_open",{{"path",path}});if(result.contains("graphId"))invoke("graph_diff_view",{{"graphId",result.at("graphId")}});}
        if(ImGui::MenuItem(tr("References")))invoke("asset_reference_view",{{"path",path}});
        if((asset.value("domain","")=="material"||asset.value("domain","")=="material-instance")&&ImGui::MenuItem(tr("Create material instance"))){contextAsset_=path;assetOperation_="new_instance";assetDialog_=true;}
        bool favorite=favorites_.count(path)!=0;
        if(ImGui::MenuItem(tr(favorite?"Remove favorite":"Add favorite")))invoke("asset_favorite",{{"path",path},{"enabled",!favorite}});
        for(const auto& operation:{"asset_copy","asset_move"}){
            if(ImGui::MenuItem(tr(std::string(operation)=="asset_copy"?"Duplicate asset":"Rename / Move"))){
                contextAsset_=path;assetOperation_=operation;copyTo(assetDestination_,sizeof(assetDestination_),path);assetDialog_=true;
            }
        }
        if(asset.at("type")=="Graph"&&ImGui::MenuItem(tr("Reload from disk")))invoke("asset_reload",{{"path",path}});
        ImGui::Separator();
        if(ImGui::MenuItem(tr("Move to Trash")))invoke("asset_trash",{{"path",path}});
        ImGui::EndPopup();
    }
    if(ImGui::BeginDragDropSource()){
        ImGui::SetDragDropPayload("CYGAME_ASSET",path.c_str(),path.size()+1);ImGui::TextUnformatted(path.c_str());ImGui::EndDragDropSource();
    }
}
void EditorApp::contentBrowser(){
    const auto& palette=cyui::palette();
    if(cyui::primaryButton(tr("Add"))){ImGui::OpenPopup("##addContent");}
    if(ImGui::BeginPopup("##addContent")){
        if(ImGui::MenuItem(tr("Import source file..."))){reimport_=false;open("imports");}
        if(ImGui::MenuItem(tr("Logic Graph"))){assetOperation_="new_logic";assetDialog_=true;}
        if(ImGui::MenuItem(tr("Material Graph"))){assetOperation_="new_material";assetDialog_=true;}
        if(ImGui::MenuItem(tr("New folder"))){assetOperation_="folder";assetDialog_=true;}
        ImGui::EndPopup();
    }
    if(ImGui::GetWindowWidth()>cyui::dp(720))ImGui::SameLine();
    bool changed=cyui::searchField("##assetSearch",assetSearch_,sizeof(assetSearch_),tr("Search content: type:Graph domain:material"),ImGui::GetWindowWidth()>cyui::dp(720)?std::max(cyui::dp(150),ImGui::GetContentRegionAvail().x-cyui::dp(330)):-1);
    if(ImGui::GetWindowWidth()>cyui::dp(720))ImGui::SameLine();const char* types[]={tr("All"),tr("Graph"),tr("Texture"),tr("Mesh"),tr("Audio"),tr("Other")};
    ImGui::SetNextItemWidth(cyui::dp(95));changed|=ImGui::Combo("##types",&assetType_,types,6);
    ImGui::SameLine();if(cyui::iconButton(cyui::icon::refresh,tr("Refresh assets")))changed=true;
    ImGui::SameLine();if(cyui::iconButton(cyui::icon::star,tr("Favorites"),favoritesOnly_))favoritesOnly_=!favoritesOnly_;
    ImGui::SameLine();if(cyui::ghostButton(tr(assetGrid_?"List":"Tiles"))){assetGrid_=!assetGrid_;savePreferences();}
    ImGui::SameLine();if(cyui::iconButton(cyui::icon::trash,tr("Trash")))open("trash");
    if(changed)assetsRefresh();
    if(assetDialog_){ImGui::OpenPopup("##assetAction");assetDialog_=false;}
    if(ImGui::BeginPopup("##assetAction")){
        bool create=assetOperation_=="new_instance"||assetOperation_=="new_logic"||assetOperation_=="new_material",folder=assetOperation_=="folder";
        if(create)ImGui::InputText(tr("Name"),newAssetName_,sizeof(newAssetName_));
        else if(folder)ImGui::InputText(tr("Folder name"),newFolder_,sizeof(newFolder_));
        else{cyui::textDim("%s",contextAsset_.c_str());ImGui::InputText(tr("Destination"),assetDestination_,sizeof(assetDestination_));}
        if(cyui::primaryButton(tr(create||folder?"Create":"Apply"))){
            Json result;
            if(assetOperation_=="new_instance")result=invoke("asset_new_material_instance",{{"name",std::string(newAssetName_)},{"folder",assetFolder_.empty()?"materials":assetFolder_},{"parent",contextAsset_}});
            else if(create)result=invoke("asset_new_graph",{{"name",std::string(newAssetName_)},{"folder",assetFolder_.empty()?"graphs":assetFolder_},{"domain",assetOperation_=="new_logic"?"logic":"material"}});
            else if(folder)result=invoke("asset_folder_create",{{"path",(fs::u8path(assetFolder_)/newFolder_).generic_u8string()}});
            else result=invoke(assetOperation_,{{"path",contextAsset_},{"destination",std::string(assetDestination_)}});
            if(!result.contains("error")){if(result.contains("graphId"))open(result["graphId"]);ImGui::CloseCurrentPopup();}
        }ImGui::SameLine();if(cyui::ghostButton(tr("Cancel")))ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    if(cyui::ghostButton(tr("Content")))assetFolder_.clear();
    std::string breadcrumb;
    for(const auto& part:fs::u8path(assetFolder_)){
        if(!breadcrumb.empty())breadcrumb+="/";breadcrumb+=part.u8string();ImGui::SameLine();cyui::textFaint(">");ImGui::SameLine();
        ImGui::PushID(breadcrumb.c_str());if(cyui::ghostButton(part.u8string().c_str()))assetFolder_=breadcrumb;ImGui::PopID();
    }
    ImGui::BeginChild("##contentSources",{std::min(cyui::dp(165),ImGui::GetContentRegionAvail().x*0.30f),0},ImGuiChildFlags_Borders);
    cyui::overline(tr("SOURCES"));
    auto folderRow=[&](const std::string& path,const std::string& title){
        ImGui::PushID(path.c_str());
        if(ImGui::Selectable(title.c_str(),assetFolder_==path))assetFolder_=path;
        if(ImGui::BeginDragDropTarget()){
            if(const auto* payload=ImGui::AcceptDragDropPayload("CYGAME_ASSET")){
                std::string source=static_cast<const char*>(payload->Data),destination=(fs::u8path(path)/fs::u8path(source).filename()).generic_u8string();
                invoke("asset_move",{{"path",source},{"destination",destination}});
            }ImGui::EndDragDropTarget();
        }ImGui::PopID();
    };
    folderRow("",tr("All content"));
    const auto folderSnapshot=assetFolders_;
    for(const auto& entry:folderSnapshot){
        const std::string path=entry;const auto depth=std::count(path.begin(),path.end(),'/');
        ImGui::Indent(cyui::dp(8.f+depth*8.f));folderRow(path,fs::u8path(path).filename().u8string());ImGui::Unindent(cyui::dp(8.f+depth*8.f));
    }
    ImGui::EndChild();ImGui::SameLine();ImGui::BeginChild("##contentFiles",{0,0});
    std::vector<Json> visible;
    for(const auto& asset:assets_){
        std::string path=asset.at("path");
        if(!assetFolder_.empty()&&path.rfind(assetFolder_+"/",0)!=0)continue;
        if(favoritesOnly_&&!favorites_.count(path))continue;visible.push_back(asset);
    }
    if(visible.empty())cyui::emptyState(tr("No matching assets"),tr("Create an asset, change the filters, or drop a file into the content root."),cyui::icon::folder);
    if(assetGrid_&&!visible.empty()){
        const int columns=std::max(1,int(ImGui::GetContentRegionAvail().x/cyui::dp(133)));
        if(ImGui::BeginTable("##assetTiles",columns,ImGuiTableFlags_SizingStretchSame)){
            for(const auto& asset:visible){
                ImGui::TableNextColumn();std::string path=asset.at("path"),type=asset.at("type"),domain=asset.value("domain","");
                ImGui::PushID(path.c_str());const auto a=ImGui::GetCursorScreenPos();ImVec2 size{ImGui::GetContentRegionAvail().x,cyui::dp(102)},b{a.x+size.x,a.y+size.y};
                if(ImGui::InvisibleButton("##tile",size)){assetPath_=path;copyTo(assetInspect_,sizeof(assetInspect_),asset.dump(2));if(ImGui::IsMouseDoubleClicked(0))openAsset(asset);}
                auto* draw=ImGui::GetWindowDrawList();draw->AddRectFilled(a,b,path==assetPath_?palette.selection:ImGui::IsItemHovered()?palette.bg4:palette.bg2,cyui::dp(3));
                draw->AddRect(a,b,path==assetPath_?palette.accent:palette.line,cyui::dp(3));
                const char* glyph=type=="Graph"?cyui::icon::code:type=="Texture"?cyui::icon::photo:type=="Audio"?cyui::icon::play:type=="Mesh"?cyui::icon::grid:cyui::icon::document;
                ImTextureID thumb{};auto ext=asset.value("extension","");if(thumbnailRenderer&&(ext==".png"||ext==".jpg"||ext==".jpeg"||ext==".bmp")&&ImGui::IsItemVisible())thumb=thumbnailRenderer(path);
                if(thumb){const float w=std::min(size.x-cyui::dp(18),cyui::dp(106)),x=a.x+(size.x-w)*.5f;draw->AddImage(thumb,{x,a.y+cyui::dp(7)},{x+w,a.y+cyui::dp(7)+w*.5f});}
                else draw->AddText(nullptr,ImGui::GetFontSize()*2.2f,{a.x+cyui::dp(13),a.y+cyui::dp(12)},palette.textDim,glyph);
                auto name=fs::u8path(path).stem().u8string();cyui::clippedText(draw,{a.x+cyui::dp(9),a.y+cyui::dp(66)},size.x-cyui::dp(18),palette.text,name.c_str());
                const auto kind=domain=="logic"?tr("Logic"):domain=="material-instance"?tr("Material instance"):domain=="material"?tr("Material"):tr(type.c_str());
                draw->AddText({a.x+cyui::dp(9),a.y+cyui::dp(84)},palette.textFaint,kind);
                if(asset.value("imported",false))draw->AddText({b.x-cyui::dp(21),a.y+cyui::dp(42)},palette.info,cyui::icon::download);
                if(favorites_.count(path))draw->AddText({b.x-cyui::dp(21),a.y+cyui::dp(8)},palette.warning,cyui::icon::star);
                assetContext(asset);ImGui::PopID();
            }ImGui::EndTable();
        }
    }else if(!visible.empty()&&ImGui::BeginTable("##assetRows",3,ImGuiTableFlags_RowBg|ImGuiTableFlags_Resizable)){
        ImGui::TableSetupColumn(tr("Asset"));ImGui::TableSetupColumn(tr("Type"));ImGui::TableSetupColumn(tr("Size"));ImGui::TableHeadersRow();
        for(const auto& asset:visible){
            std::string path=asset.at("path");ImGui::TableNextRow();ImGui::TableNextColumn();ImGui::PushID(path.c_str());
            if(ImGui::Selectable(path.c_str(),assetPath_==path,ImGuiSelectableFlags_SpanAllColumns|ImGuiSelectableFlags_AllowDoubleClick)){assetPath_=path;copyTo(assetInspect_,sizeof(assetInspect_),asset.dump(2));if(ImGui::IsMouseDoubleClicked(0))openAsset(asset);}
            assetContext(asset);ImGui::TableNextColumn();ImGui::TextUnformatted(tr(asset.at("type").get_ref<const std::string&>().c_str()));ImGui::TableNextColumn();cyui::textDim("%.1f KB",asset.at("bytes").get<double>()/1024);ImGui::PopID();
        }ImGui::EndTable();
    }ImGui::EndChild();
}
void EditorApp::trashBrowser(){
    cyui::heading(tr("Trash"),2);cyui::textDim("%s",tr("Deleted assets can be restored without overwriting existing files."));
    auto result=core.execute("asset_trash_list");
    for(const auto& ticket:result["tickets"]){
        ImGui::PushID(ticket.at("ticket").get_ref<const std::string&>().c_str());ImGui::TextUnformatted(ticket.at("path").get_ref<const std::string&>().c_str());
        ImGui::SameLine();if(cyui::ghostButton(tr("Restore")))invoke("asset_restore",{{"ticket",ticket["ticket"]}});
        ImGui::PopID();
    }
    if(result["tickets"].empty())cyui::emptyState(tr("Trash is empty"),tr("Assets moved to Trash will appear here."),cyui::icon::trash);
}
}
