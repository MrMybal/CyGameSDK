#include "Editor.h"
namespace cygame {
namespace {std::string escaped(std::string key){std::string out;for(char c:key)if(c=='~')out+="~0";else if(c=='/')out+="~1";else out+=c;return out;}}
Json EditorApp::openJson(const std::string& path){
    auto result=core.execute("json_open",{{"path",path}});const std::string id=result.at("jsonId"),panel="json:"+id;
    for(auto& p:core.panels)if(p.id==panel){p.open=true;p.focus=true;return {{"panelId",panel}};}
    core.panels.push_back({panel,fs::u8path(path).filename().u8string(),"core",[this,id]{jsonEditor(id);},true,true});return {{"panelId",panel}};
}
void EditorApp::jsonEditor(const std::string& id){
    auto data=invoke("json_get",{{"jsonId",id}});if(!data.contains("value")){cyui::textDim("%s",tr("JSON document unavailable"));return;}
    if(cyui::primaryButton(tr("Save JSON")))invoke("json_save",{{"jsonId",id}});
    ImGui::SameLine();if(cyui::ghostButton(tr("Close document"))){auto result=invoke("json_close",{{"jsonId",id}});if(!result.contains("error")){for(auto& panel:core.panels)if(panel.id=="json:"+id)panel.open=false;return;}}
    ImGui::SameLine();if(cyui::ghostButton(tr("Undo"))&&core.canUndo())invoke("edit_undo");ImGui::SameLine();if(cyui::ghostButton(tr("Redo"))&&core.canRedo())invoke("edit_redo");
    ImGui::SameLine();if(cyui::ghostButton(tr("Copy JSON")))ImGui::SetClipboardText(data["value"].dump(2).c_str());
    cyui::searchField("##jsonFilter",jsonSearch_,sizeof(jsonSearch_),tr("Filter JSON keys"),-1);
    Json edit;bool changed=false;
    std::function<void(const Json&,const std::string&,const std::string&,int)> draw=[&](const Json& value,const std::string& key,const std::string& pointer,int depth){
        if(depth>64)return;ImGui::PushID(pointer.c_str());
        if(value.is_object()||value.is_array()){
            const bool expanded=ImGui::TreeNodeEx("##container",depth<2?ImGuiTreeNodeFlags_DefaultOpen:0,"%s [%zu]",key.c_str(),value.size());
            if(expanded){for(auto it=value.begin();it!=value.end();++it){const std::string child=value.is_object()?it.key():std::to_string(it-value.begin());draw(it.value(),child,pointer+"/"+escaped(child),depth+1);}ImGui::TreePop();}
        }else if(jsonSearch_[0]==0||matches(key+" "+pointer,jsonSearch_)){
            if(value.is_string()&&value.get_ref<const std::string&>().size()>4096){cyui::textDim("%s: %s",key.c_str(),tr("Large string (read-only in this view)"));if(cyui::ghostButton(tr("Copy JSON value")))ImGui::SetClipboardText(value.dump().c_str());}
            else if(cyui::beginProperties("##jsonProperty",cyui::dp(150))){cyui::property(key.c_str());Json spec={{"type",value.is_boolean()?"boolean":value.is_string()?"string":"json"}},next;if(editValue("json/"+id+pointer,spec,value,next)){edit={{"jsonId",id},{"pointer",pointer},{"value",next}};changed=true;}cyui::endProperties();}
        }
        if(ImGui::BeginPopupContextItem("##jsonContext")){
            if(!pointer.empty()&&ImGui::MenuItem(tr("Remove JSON value"))){invoke("json_remove",{{"jsonId",id},{"pointer",pointer}});}
            if((value.is_object()||value.is_array())&&ImGui::MenuItem(tr("Add JSON value"))){jsonParent_=pointer;jsonTarget_=id;jsonAdd_=true;}
            ImGui::EndPopup();
        }
        ImGui::PopID();
    };
    ImGui::BeginChild("##jsonTree");draw(data["value"],tr("Root"),"",0);ImGui::EndChild();if(changed)invoke("json_value_set",edit);
    if(jsonAdd_&&jsonTarget_==id){ImGui::OpenPopup("##jsonAdd");jsonAdd_=false;}
    if(ImGui::BeginPopup("##jsonAdd")){ImGui::InputText(tr("Name"),jsonName_,sizeof(jsonName_));ImGui::InputText(tr("Value (JSON)"),jsonValue_,sizeof(jsonValue_));if(cyui::primaryButton(tr("Add"))){try{auto value=Json::parse(jsonValue_);auto result=invoke("json_add",{{"jsonId",id},{"pointer",jsonParent_},{"name",std::string(jsonName_)},{"value",value}});if(!result.contains("error"))ImGui::CloseCurrentPopup();}catch(const std::exception& e){cyui::notify(e.what(),cyui::Tone::error);}}ImGui::EndPopup();}
}
}
