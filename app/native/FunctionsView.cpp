#include "Editor.h"
#include "Graph.h"
#include <cstdio>
namespace cygame {
namespace {
Json initialValue(const std::string& type,const Json& types){return graphDefaultValue(type,types);}
Json widget(const std::string& type,const Json& types){return graphWidget(type,types);}
}
Json EditorApp::openFunction(const std::string& graphId,const std::string& name){
    graphView(core.graphs().at(graphId),name);const auto key="fn:"+graphId+"#"+name;
    for(auto& panel:core.panels)if(panel.id==key){panel.open=true;panel.focus=true;return {{"panelId",key}};}
    core.panels.push_back({key,name,"core",[this,graphId,name]{graphEditor(graphId,name);},true,true});return {{"panelId",key}};
}
void EditorApp::functionLibraryUI(){
    auto owner=core.graphs().at(activeGraph_);auto functions=owner.value("functions",Json::object());
    if(!activeFunction_.empty()&&!functions.contains(activeFunction_))activeFunction_.clear();
    cyui::sectionTitle(tr("Functions"));
    if(cyui::iconTextButton(cyui::icon::add,tr("New function")))ImGui::OpenPopup("##newFunction");probeInterface("bp/newFunction");
    if(ImGui::BeginPopup("##newFunction")){
        ImGui::InputText(tr("Name"),functionName_,sizeof(functionName_));
        if(owner.at("domain")=="logic"){ImGui::Checkbox(tr("Execution function"),&newExecutionFunction_);ImGui::Checkbox(tr("Inline macro"),&newMacro_);probeInterface("bp/macro");}
        if(cyui::primaryButton(tr("Create"))){auto result=invoke("graph_function_create",{{"graphId",activeGraph_},{"name",std::string(functionName_)},{"mode",owner.at("domain")=="logic"&&newExecutionFunction_?"execution":"pure"},{"kind",owner.at("domain")=="logic"&&newMacro_?"macro":"function"}});if(!result.contains("error")){activeFunction_=functionName_;functions=core.graphs().at(activeGraph_).value("functions",Json::object());openFunction(activeGraph_,activeFunction_);ImGui::CloseCurrentPopup();}}probeInterface("bp/createFunction");
        ImGui::EndPopup();
    }
    if(!activeFunction_.empty()&&cyui::ghostButton(tr("Main graph"))){activeFunction_.clear();open(activeGraph_);}
    for(const auto& [name,f]:functions.items()){
        if(!matches(name,myGraphSearch_))continue;
        ImGui::PushID(name.c_str());
        if(ImGui::Selectable(name.c_str(),activeFunction_==name)){activeFunction_=name;openFunction(activeGraph_,name);}
        if(f.value("kind","")=="macro"){ImGui::SameLine();cyui::badge(tr("Macro"));}
        if(f.value("mode","pure")=="execution"){ImGui::SameLine();cyui::badge(tr("Execution"));}
        {cyui::ScopedDisabled disabled(activeFunction_==name||(f.value("mode","pure")=="execution"&&!activeFunction_.empty()&&functions.at(activeFunction_).value("mode","pure")=="pure"));if(cyui::ghostButton(tr("Call function"))){
            Json p={{"graphId",activeGraph_},{"type",f.value("kind","")=="macro"?"macro.call":"function.call"},{"functionRef",name},{"position",{100,270}}};
            if(!activeFunction_.empty())p["functionName"]=activeFunction_;
            auto result=invoke("graph_node_add",p);if(!result.contains("error")){if(activeFunction_.empty())open(activeGraph_);else openFunction(activeGraph_,activeFunction_);}
        }
        if(f.value("mode","pure")=="execution"&&f.value("kind","")!="macro"&&f.at("inputs").contains("item")&&f.at("inputs").contains("index")){if(cyui::ghostButton(tr("For each")))invoke("graph_node_add",{{"graphId",activeGraph_},{"type","flow.foreach"},{"functionRef",name},{"position",{100,380}}});}
        }ImGui::PopID();
    }
    if(activeFunction_.empty())return;
    const auto key=activeGraph_+"#"+activeFunction_;const auto& function=functions.at(activeFunction_);
    if(signatureKey_!=key){signatureKey_=key;signatureRenames_=Json::object();signatureDraft_={{"inputs",function.at("inputs")},{"outputs",function.at("outputs")}};}
    if(!cyui::beginSection(label(tr("Function signature"),"signature/"+key).c_str(),function.value("mode","pure")=="pure"))return;
    ImGui::TextWrapped("%s",tr(function.value("mode","pure")=="execution"?"Execution function; local variables are isolated per call.":"Pure function; changes apply to every call."));
    const auto typeNames=owner.at("domain")=="material"?std::vector<std::string>{"float","bool","string","color"}:blueprintTypes(owner.value("types",Json::object()));std::vector<const char*> types;for(const auto& t:typeNames)types.push_back(t.c_str());portType_=std::clamp(portType_,0,int(types.size())-1);
    for(const auto* side:{"inputs","outputs"}){
        cyui::sectionTitle(tr(side));std::string remove;
        for(auto& [name,port]:signatureDraft_[side].items()){
            ImGui::PushID(side);ImGui::PushID(name.c_str());ImGui::TextUnformatted(name.c_str());
            int type=0;bool known=false;for(int i=0;i<int(types.size());++i)if(port["type"]==types[i]){type=i;known=true;}
            ImGui::SetNextItemWidth(-1);if(ImGui::Combo("##portType",&type,types.data(),int(types.size()))){port["type"]=types[type];port["default"]=initialValue(types[type],owner.value("types",Json::object()));known=true;}
            if(!known)cyui::textDim("%s",tr("Draft type unavailable; choose a type or reload."));
            Json next;if(known&&editValue(key+"/"+side+"/"+name,widget(types[type],owner.value("types",Json::object())),port["default"],next))port["default"]=next;
            if(cyui::ghostButton(tr("Remove port")))remove=name;
            ImGui::PopID();ImGui::PopID();
        }
        if(!remove.empty()){signatureDraft_[side].erase(remove);if(signatureRenames_.contains(side)){auto& names=signatureRenames_[side];for(auto it=names.begin();it!=names.end();)if(it.value()==remove)it=names.erase(it);else ++it;}}
    }
    signatureRenameControls(signatureDraft_,signatureRenames_,"functionSignature");
    if(cyui::ghostButton(tr("Preview migration")))previewSignatureMigration({{"graphId",activeGraph_},{"target","function"},{"name",activeFunction_},{"inputs",signatureDraft_["inputs"]},{"outputs",signatureDraft_["outputs"]},{"renames",signatureRenames_}});
    if(cyui::ghostButton(tr("Add port")))ImGui::OpenPopup("##addPort");
    if(ImGui::BeginPopup("##addPort")){
        ImGui::InputText(tr("Name"),portName_,sizeof(portName_));ImGui::Combo(tr("Type"),&portType_,types.data(),int(types.size()));ImGui::Checkbox(tr("Output port"),&outputPort_);
        if(cyui::primaryButton(tr("Add"))){signatureDraft_[outputPort_?"outputs":"inputs"][portName_]={{"type",types[portType_]},{"default",initialValue(types[portType_],owner.value("types",Json::object()))}};ImGui::CloseCurrentPopup();}ImGui::EndPopup();
    }
    if(cyui::primaryButton(tr("Apply signature"))){auto result=invoke("graph_function_signature",{{"graphId",activeGraph_},{"name",activeFunction_},{"inputs",signatureDraft_["inputs"]},{"outputs",signatureDraft_["outputs"]}});if(!result.contains("error"))signatureKey_.clear();}
    if(cyui::ghostButton(tr("Reload signature")))signatureKey_.clear();
    if(cyui::ghostButton(tr("Rename function"))){std::snprintf(functionRename_,sizeof(functionRename_),"%s",activeFunction_.c_str());ImGui::OpenPopup("##renameFunction");}
    if(ImGui::BeginPopup("##renameFunction")){
        ImGui::InputText(tr("New name"),functionRename_,sizeof(functionRename_));
        if(cyui::primaryButton(tr("Apply"))){auto result=invoke("graph_function_rename",{{"graphId",activeGraph_},{"name",activeFunction_},{"newName",std::string(functionRename_)}});if(!result.contains("error")){activeFunction_=functionRename_;openFunction(activeGraph_,activeFunction_);ImGui::CloseCurrentPopup();}}ImGui::EndPopup();
    }
    if(cyui::dangerButton(tr("Remove function"))){auto result=invoke("graph_function_remove",{{"graphId",activeGraph_},{"name",activeFunction_}});if(!result.contains("error")){activeFunction_.clear();open(activeGraph_);}}
    cyui::endSection();
}
}
