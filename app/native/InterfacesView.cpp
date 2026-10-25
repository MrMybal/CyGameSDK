#include "Editor.h"
#include "Graph.h"
namespace cygame {
void EditorApp::graphInterfacesUI(){
    const auto owner=core.graphs().at(activeGraph_);if(owner.at("domain")!="logic")return;
    const auto interfaces=owner.value("interfaces",Json::object());cyui::sectionTitle(tr("Interfaces"));bool show=false;
    if(cyui::ghostButton(tr("New interface"))){interfaceRenames_=Json::object();interfaceEditing_=false;std::snprintf(interfaceName_,sizeof(interfaceName_),"NewInterface");interfaceDraft_={{"methods",{{"Execute",{{"mode","execution"},{"inputs",Json::object()},{"outputs",Json::object()}}}}}};show=true;}probeInterface("interfaces/new");
    for(const auto& [name,definition]:interfaces.items()){
        if(!matches(name,myGraphSearch_))continue;ImGui::PushID(name.c_str());cyui::textDim("%s",name.c_str());
        if(cyui::ghostButton(tr("Edit interface"))){interfaceRenames_=Json::object();interfaceEditing_=true;std::snprintf(interfaceName_,sizeof(interfaceName_),"%s",name.c_str());interfaceDraft_=definition;show=true;}probeInterface("interfaces/edit/"+name);
        ImGui::SameLine();if(cyui::ghostButton(tr("Remove interface")))invoke("graph_interface_remove",{{"graphId",activeGraph_},{"name",name}});
        auto add=[&](const char* type,const std::string& method){Json p={{"graphId",activeGraph_},{"type",type},{"interfaceRef",name},{"position",{260,160}}};if(!method.empty())p["methodRef"]=method;if(!activeFunction_.empty())p["functionName"]=activeFunction_;invoke("graph_node_add",p);};
        if(cyui::ghostButton(tr("Check interface")))add("interface.supports","");
        for(const auto& [method,sig]:definition.at("methods").items()){
            const bool allowed=activeFunction_.empty()||owner.at("functions").at(activeFunction_).value("mode","pure")=="execution"||sig.value("mode","pure")=="pure";
            cyui::ScopedDisabled disabled(!allowed);if(cyui::ghostButton(label(std::string(tr("Call interface"))+" / "+method,method).c_str()))add("interface.call",method);
        }ImGui::PopID();
    }
    if(show)ImGui::OpenPopup(label(tr("Interface definition"),"interfaceDefinition").c_str());
    const auto viewport=ImGui::GetMainViewport();ImGui::SetNextWindowViewport(viewport->ID);ImGui::SetNextWindowPos(viewport->GetCenter(),ImGuiCond_Appearing,{.5f,.5f});
    bool open=true;if(ImGui::BeginPopupModal(label(tr("Interface definition"),"interfaceDefinition").c_str(),&open,ImGuiWindowFlags_AlwaysAutoResize)){
        interfaceDefinitionUI();ImGui::EndPopup();
    }
}
void EditorApp::interfaceDefinitionUI(){
    const auto types=core.graphs().at(activeGraph_).value("types",Json::object());const auto names=blueprintTypes(types);std::vector<const char*> labels;for(const auto& name:names)labels.push_back(name.c_str());
    {cyui::ScopedDisabled disabled(interfaceEditing_);if(cyui::beginProperties("##interfaceName")){cyui::property(tr("Name"));ImGui::InputText("##name",interfaceName_,sizeof(interfaceName_));cyui::endProperties();}}
    ImGui::BeginChild("##interfaceMethods",{cyui::dp(500),cyui::dp(390)},ImGuiChildFlags_Borders);std::string removeMethod;
    for(auto& [name,method]:interfaceDraft_["methods"].items()){
        ImGui::PushID(name.c_str());cyui::sectionTitle(name.c_str());bool execution=method.value("mode","pure")=="execution";if(ImGui::Checkbox(tr("Execution method"),&execution))method["mode"]=execution?"execution":"pure";probeInterface("interfaces/execution/"+name);
        for(const auto* side:{"inputs","outputs"}){
            ImGui::PushID(side);cyui::sectionTitle(tr(side));std::string remove;
            for(auto& [pin,port]:method[side].items()){
                ImGui::PushID(pin.c_str());cyui::textDim("%s",pin.c_str());int index=0;bool known=false;for(int i=0;i<int(names.size());++i)if(port["type"]==names[i]){index=i;known=true;}
                if(cyui::beginProperties("##interfacePort")){cyui::property(tr("Type"));if(ImGui::Combo("##type",&index,labels.data(),int(labels.size()))){port["type"]=names[index];port["default"]=graphDefaultValue(names[index],types);known=true;}
                    cyui::property(tr("Default"));Json next;if(known&&editValue("interface/"+name+"/"+side+"/"+pin,graphWidget(names[index],types),port["default"],next))port["default"]=next;if(!known)cyui::textDim("%s",tr("Draft type unavailable; choose a type or reload."));cyui::endProperties();}
                if(cyui::ghostButton(tr("Remove port")))remove=pin;ImGui::PopID();
            }
            if(!remove.empty()){method[side].erase(remove);if(interfaceRenames_.contains(name)&&interfaceRenames_[name].contains(side)){auto& names=interfaceRenames_[name][side];for(auto it=names.begin();it!=names.end();)if(it.value()==remove)it=names.erase(it);else ++it;}}ImGui::SetNextItemWidth(cyui::dp(220));ImGui::InputText("##newPort",interfacePort_,sizeof(interfacePort_));ImGui::SameLine();if(cyui::ghostButton(tr("Add port"))&&!method[side].contains(interfacePort_))method[side][interfacePort_]={{"type","float"},{"default",0.}};probeInterface("interfaces/port/"+name+"/"+side);ImGui::PopID();
        }
        if(interfaceEditing_){if(!interfaceRenames_.contains(name))interfaceRenames_[name]=Json::object();signatureRenameControls(method,interfaceRenames_[name],"interfaceSignature");
            if(cyui::ghostButton(tr("Preview method migration"))){const auto owner=core.graphs().at(activeGraph_);if(!owner.at("interfaces").at(interfaceName_).at("methods").contains(name)||owner.at("interfaces").at(interfaceName_).at("methods").at(name).value("mode","pure")!=method.value("mode","pure"))cyui::notify(tr("Migration keeps the existing method and execution mode."),cyui::Tone::warning);else{auto result=previewSignatureMigration({{"graphId",activeGraph_},{"target","interface"},{"name",std::string(interfaceName_)},{"method",name},{"inputs",method.at("inputs")},{"outputs",method.at("outputs")},{"renames",interfaceRenames_[name]}});if(!result.contains("error"))ImGui::CloseCurrentPopup();}}}
        if(cyui::ghostButton(tr("Remove method")))removeMethod=name;ImGui::PopID();
    }
    if(!removeMethod.empty())interfaceDraft_["methods"].erase(removeMethod);ImGui::SetNextItemWidth(cyui::dp(220));ImGui::InputText(tr("Method name"),interfaceMethod_,sizeof(interfaceMethod_));if(cyui::ghostButton(tr("Add method"))&&!interfaceDraft_["methods"].contains(interfaceMethod_))interfaceDraft_["methods"][interfaceMethod_]={{"mode","execution"},{"inputs",Json::object()},{"outputs",Json::object()}};
    ImGui::EndChild();ImGui::TextWrapped("%s",tr("Interface methods reserve self. Every implementing class must bind matching functions."));
    if(cyui::primaryButton(tr("Apply interface"))){const auto r=invoke("graph_interface_define",{{"graphId",activeGraph_},{"name",std::string(interfaceName_)},{"definition",interfaceDraft_}});if(!r.contains("error"))ImGui::CloseCurrentPopup();}probeInterface("interfaces/apply");ImGui::SameLine();if(cyui::ghostButton(tr("Cancel")))ImGui::CloseCurrentPopup();
}
}
