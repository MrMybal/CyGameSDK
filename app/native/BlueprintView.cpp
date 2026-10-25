#include "Editor.h"
#include "Graph.h"
namespace cygame {
void EditorApp::blueprintLibraryUI(){
    const auto owner=core.graphs().at(activeGraph_);if(owner.at("domain")!="logic"||!activeFunction_.empty())return;cyui::sectionTitle(tr("Classes"));
    if(cyui::ghostButton(tr("Define class"))){classInterfaces_=Json::array();ImGui::OpenPopup("##classDefinition");}
    const auto classes=owner.value("classes",Json::object());for(const auto& [name,definition]:classes.items()){
        ImGui::PushID(name.c_str());ImGui::TextUnformatted(name.c_str());if(cyui::ghostButton(tr("Construct")))invoke("graph_node_add",{{"graphId",activeGraph_},{"type","object.new"},{"classRef",name},{"position",{100,180}}});ImGui::SameLine();if(cyui::ghostButton(tr("Edit"))){classInterfaces_=definition.value("interfaces",Json::array());std::snprintf(className_,sizeof(className_),"%s",name.c_str());std::snprintf(classParent_,sizeof(classParent_),"%s",definition.value("parent","").c_str());std::snprintf(classFields_,sizeof(classFields_),"%s",definition.at("fields").dump().c_str());std::snprintf(classMethods_,sizeof(classMethods_),"%s",definition.value("methods",Json::object()).dump().c_str());ImGui::OpenPopup("##editClass");}
        if(ImGui::BeginPopup("##editClass")){classDefinitionUI();ImGui::EndPopup();}
        const auto resolved=blueprintClass(owner.at("classes"),name,owner.value("types",Json::object()));for(const auto& [method,function]:resolved.at("methods").items())if(cyui::ghostButton(label(method,name+"/"+method).c_str()))invoke("graph_node_add",{{"graphId",activeGraph_},{"type","object.call"},{"classRef",name},{"methodRef",method},{"position",{300,180}}});
        if(cyui::ghostButton(tr("Remove class")))invoke("graph_class_remove",{{"graphId",activeGraph_},{"name",name}});ImGui::PopID();
    }
    if(ImGui::BeginPopup("##classDefinition")){classDefinitionUI();ImGui::EndPopup();}
}
void EditorApp::classDefinitionUI(){
    ImGui::InputText(tr("Name"),className_,sizeof(className_));ImGui::InputText(tr("Parent class"),classParent_,sizeof(classParent_));ImGui::TextUnformatted(tr("Typed fields (JSON)"));ImGui::InputTextMultiline("##classFields",classFields_,sizeof(classFields_),{cyui::dp(340),cyui::dp(120)});ImGui::TextUnformatted(tr("Method to function bindings (JSON)"));ImGui::InputTextMultiline("##classMethods",classMethods_,sizeof(classMethods_),{cyui::dp(340),cyui::dp(70)});
    const auto owner=core.graphs().at(activeGraph_),interfaces=owner.value("interfaces",Json::object());
    Json inherited=Json::object(),inheritedInterfaces=Json::array();if(owner.value("classes",Json::object()).contains(classParent_)){const auto parent=blueprintClass(owner.at("classes"),classParent_,owner.value("types",Json::object()));inherited=parent.at("methods");inheritedInterfaces=parent.at("interfaces");}
    if(!interfaces.empty())cyui::sectionTitle(tr("Implemented interfaces"));
    Json required=Json::object();for(const auto& [name,contract]:interfaces.items()){
        const bool fromParent=std::find(inheritedInterfaces.begin(),inheritedInterfaces.end(),Json(name))!=inheritedInterfaces.end();bool enabled=fromParent||std::find(classInterfaces_.begin(),classInterfaces_.end(),Json(name))!=classInterfaces_.end();
        {cyui::ScopedDisabled disabled(fromParent);const auto title=name+(fromParent?" / "+std::string(tr("Inherited")):"");if(ImGui::Checkbox(title.c_str(),&enabled)){if(enabled)classInterfaces_.push_back(name);else classInterfaces_.erase(std::remove(classInterfaces_.begin(),classInterfaces_.end(),Json(name)),classInterfaces_.end());}}
        if(enabled)for(const auto& [method,spec]:contract.at("methods").items())required[method]=spec;
    }
    if(!required.empty())try{
        auto bindings=Json::parse(classMethods_);if(!bindings.is_object())throw std::runtime_error("Invalid method bindings");
        cyui::sectionTitle(tr("Interface method bindings"));
        for(const auto& [name,spec]:required.items()){
            const auto selected=bindings.value(name,inherited.value(name,std::string()));ImGui::PushID(name.c_str());
            if(ImGui::BeginCombo(name.c_str(),selected.empty()?tr("Choose matching function"):selected.c_str())){
                if(inherited.contains(name)&&ImGui::Selectable(tr("Use inherited binding"))){bindings.erase(name);std::snprintf(classMethods_,sizeof(classMethods_),"%s",bindings.dump().c_str());}
                const auto functions=owner.value("functions",Json::object());for(const auto& [function,f]:functions.items()){
                    bool match=f.value("kind","function")!="macro"&&f.value("mode","pure")==spec.value("mode","pure")&&f.at("inputs").contains("self")&&f.at("inputs").at("self").at("type")=="object"&&f.at("inputs").size()==spec.at("inputs").size()+1&&f.at("outputs").size()==spec.at("outputs").size();
                    for(const auto* side:{"inputs","outputs"})for(const auto& [pin,port]:spec.at(side).items())match&=f.at(side).contains(pin)&&f.at(side).at(pin).at("type")==port.at("type");
                    if(match&&ImGui::Selectable(function.c_str(),function==selected)){auto next=bindings;next[name]=function;const auto text=next.dump();if(text.size()<sizeof(classMethods_)){bindings=next;std::snprintf(classMethods_,sizeof(classMethods_),"%s",text.c_str());}}
                }ImGui::EndCombo();
            }ImGui::PopID();
        }
    }catch(const std::exception&){cyui::textDim("%s",tr("Correct method bindings JSON before choosing functions."));}
    cyui::textDim("%s",tr("Methods require a typed object input named self."));
    if(cyui::primaryButton(tr("Apply class"))){try{auto result=invoke("graph_class_define",{{"graphId",activeGraph_},{"name",std::string(className_)},{"parent",std::string(classParent_)},{"fields",Json::parse(classFields_)},{"methods",Json::parse(classMethods_)},{"interfaces",classInterfaces_}});if(!result.contains("error"))ImGui::CloseCurrentPopup();}catch(const std::exception& e){cyui::notify(e.what(),cyui::Tone::error);}}
}
}
