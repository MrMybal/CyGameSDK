#include "Editor.h"
#include "Graph.h"
#include <algorithm>
#include <cstdio>
namespace cygame {
void EditorApp::myGraph(){
    if(!core.graphs().contains(activeGraph_))activeGraph_="logic";
    auto graph=core.graphs().at(activeGraph_);auto domain=graph.at("domain").get<std::string>();
    cyui::heading(tr("My Graph"),2);
    cyui::searchField("##myGraphSearch",myGraphSearch_,sizeof(myGraphSearch_),tr("Search graphs, functions, variables"),-1);
    cyui::sectionTitle(tr("Graphs"));
    for(const auto& [id,g]:core.graphs().items()){
        auto title=id=="logic"?std::string(tr("Event Graph")):id=="material"?std::string(tr("Material Graph")):fs::u8path(g.value("assetPath",id)).stem().u8string();
        if(!matches(title+" "+id,myGraphSearch_))continue;
        if(ImGui::Selectable(label(title,id).c_str(),activeGraph_==id)){activeGraph_=id;activeFunction_.clear();open(id);}
    }
    if(domain=="material-instance"){if(cyui::ghostButton(tr("Open parent"))){auto r=invoke("asset_open",{{"path",graph.at("parent")}});if(r.contains("graphId"))open(r.at("graphId"));}return;}
    functionLibraryUI();graphTypesUI();graphInterfacesUI();blueprintLibraryUI();
    graph=graphView(core.graphs().at(activeGraph_),activeFunction_);domain=graph.at("domain").get<std::string>();
    if(!activeFunction_.empty()&&graph.value("mode","pure")=="pure")return;
    auto variableCommand=[&](const std::string& id,Json p){if(!activeFunction_.empty())p["functionName"]=activeFunction_;return invoke(id,p);};
    cyui::sectionTitle(tr(domain=="function"?"Local variables":"Variables"));
    if(cyui::iconTextButton(cyui::icon::add,tr("New variable")))ImGui::OpenPopup("##newVariable");probeInterface("bp/newVariable");
    if(ImGui::BeginPopup("##newVariable")){
        ImGui::InputText(tr("Name"),variableName_,sizeof(variableName_));
        const auto names=core.graphs().at(activeGraph_).at("domain")=="material"?std::vector<std::string>{"float","bool","string","color"}:blueprintTypes(graph.value("types",Json::object()));std::vector<const char*> types;for(const auto& name:names)types.push_back(name.c_str());if(variableType_>=int(types.size())){variableType_=0;variableDraft_=graphDefaultValue(types[0],graph.value("types",Json::object()));}ImGui::SetNextItemWidth(cyui::dp(140));
        if(ImGui::BeginCombo(tr("Type"),types[variableType_],ImGuiComboFlags_HeightLarge)){for(size_t i=0;i<types.size();++i){if(ImGui::Selectable(types[i],variableType_==int(i))){variableType_=int(i);variableDraft_=graphDefaultValue(types[i],graph.value("types",Json::object()));}probeInterface(std::string("bp/type/")+types[i]);}ImGui::EndCombo();}probeInterface("bp/type");
        try{validateValue(variableDraft_,graphValueSchema(types[variableType_],graph.value("types",Json::object())));}catch(...){variableDraft_=graphDefaultValue(types[variableType_],graph.value("types",Json::object()));}
        Json spec=graphWidget(types[variableType_],graph.value("types",Json::object()));
        Json next;if(editValue("newVariableDefault",spec,variableDraft_,next))variableDraft_=next;probeInterface("bp/default");
        if(cyui::primaryButton(tr("Create"))){
            auto r=variableCommand("graph_variable_add",{{"graphId",activeGraph_},{"name",std::string(variableName_)},{"type",types[variableType_]},{"default",variableDraft_}});
            if(!r.contains("error"))ImGui::CloseCurrentPopup();
        }probeInterface("bp/createVariable");ImGui::EndPopup();
    }
    const auto variables=graph.value("variables",Json::object());
    if(variables.empty())cyui::textDim("%s",tr("Declare values shared by nodes."));
    for(const auto& [name,v]:variables.items()){
        if(!matches(name,myGraphSearch_))continue;
        ImGui::PushID(name.c_str());
        if(cyui::beginSection(name.c_str())){
            const std::string type=v.at("type");cyui::badge(type.c_str());
            Json spec=graphWidget(type,graph.value("types",Json::object())),next;
            if(editValue("variable/"+activeGraph_+"#"+activeFunction_+"/"+name,spec,v.at("default"),next))variableCommand("graph_variable_default",{{"graphId",activeGraph_},{"name",name},{"value",next}});
            if(cyui::ghostButton(tr("Get")))variableCommand("graph_node_add",{{"graphId",activeGraph_},{"type",isNamedGraphType(type)?"typed.get":"variable.get."+type},{"variableName",name},{"typeRef",type},{"position",{80,280}}});
            if(domain=="logic"||domain=="function"){ImGui::SameLine();if(cyui::ghostButton(tr("Set")))variableCommand("graph_node_add",{{"graphId",activeGraph_},{"type",isNamedGraphType(type)?"typed.set":"variable.set."+type},{"variableName",name},{"typeRef",type},{"position",{340,280}}});}
            if(cyui::ghostButton(tr("Rename variable"))){std::snprintf(variableRename_,sizeof(variableRename_),"%s",name.c_str());ImGui::OpenPopup("##renameVariable");}
            if(ImGui::BeginPopup("##renameVariable")){ImGui::InputText(tr("New name"),variableRename_,sizeof(variableRename_));if(cyui::primaryButton(tr("Apply"))){auto result=variableCommand("graph_variable_rename",{{"graphId",activeGraph_},{"name",name},{"newName",std::string(variableRename_)}});if(!result.contains("error"))ImGui::CloseCurrentPopup();}ImGui::EndPopup();}
            if(cyui::ghostButton(tr("Remove variable")))variableCommand("graph_variable_remove",{{"graphId",activeGraph_},{"name",name}});
            cyui::endSection();
        }ImGui::PopID();
    }
}
void EditorApp::debuggerPanel(){
    if(!core.graphs().contains(debugGraph_))debugGraph_="logic";
    ImGui::SetNextItemWidth(cyui::dp(240));
    if(ImGui::BeginCombo("##debugGraph",debugGraph_.c_str())){
        for(const auto& [key,g]:core.graphs().items())if(g["domain"]=="logic"&&ImGui::Selectable(key.c_str(),key==debugGraph_)){debugGraph_=key;debugFrame_=-1;}
        ImGui::EndCombo();
    }
    ImGui::SameLine();if(cyui::ghostButton(tr("Start debugging"))){invoke("graph_debug_start",{{"graphId",debugGraph_}});debugFrame_=-1;}
    ImGui::SameLine();if(cyui::ghostButton(tr("Step over   F10")))invoke("graph_debug_step",{{"graphId",debugGraph_}});
    ImGui::SameLine();if(cyui::ghostButton(tr("Step into   F11")))invoke("graph_debug_step_into",{{"graphId",debugGraph_}});
    ImGui::SameLine();if(cyui::ghostButton(tr("Step out   Shift+F11")))invoke("graph_debug_step_out",{{"graphId",debugGraph_}});
    ImGui::SameLine();if(cyui::ghostButton(tr("Continue")))invoke("graph_debug_continue",{{"graphId",debugGraph_}});
    ImGui::SameLine();if(cyui::ghostButton(tr("Stop")))invoke("graph_debug_stop",{{"graphId",debugGraph_}});
    if(ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)&&!ImGui::GetIO().WantTextInput){
        if(ImGui::IsKeyPressed(ImGuiKey_F10))invoke("graph_debug_step",{{"graphId",debugGraph_}});
        if(ImGui::IsKeyPressed(ImGuiKey_F11))invoke(ImGui::GetIO().KeyShift?"graph_debug_step_out":"graph_debug_step_into",{{"graphId",debugGraph_}});
        if(ImGui::IsKeyPressed(ImGuiKey_F5))invoke("graph_debug_continue",{{"graphId",debugGraph_}});
    }
    ImGui::SetNextItemWidth(cyui::dp(90));ImGui::InputFloat(tr("Simulation delta"),&debugDelta_,0,0,"%.3f");ImGui::SameLine();if(cyui::ghostButton(tr("Advance time")))invoke("graph_debug_tick",{{"graphId",debugGraph_},{"delta",debugDelta_}});
    ImGui::SetNextItemWidth(cyui::dp(200));ImGui::InputText(tr("Custom event"),debugEvent_,sizeof(debugEvent_));ImGui::SameLine();if(cyui::ghostButton(tr("Emit event")))invoke("graph_debug_event",{{"graphId",debugGraph_},{"event",std::string(debugEvent_)}});
    const auto state=core.execute("graph_debug_status",{{"graphId",debugGraph_}});cyui::badge(tr(state.value("state","Idle").c_str()));cyui::textDim("%s %.3f",tr("Simulation time"),state.value("time",0.));if(debuggerProbe)debuggerProbe(debugGraph_,state);
    if(state.contains("error")&&!state["error"].get<std::string>().empty())cyui::callout(cyui::Tone::error,state.at("error").get_ref<const std::string&>().c_str());
    const auto frames=state.value("callStack",Json::array());Json inspected=state;
    if(!frames.empty()){
        cyui::sectionTitle(tr("Call stack"));
        if(cyui::ghostButton(tr("Follow active frame")))debugFrame_=-1;
        for(int i=static_cast<int>(frames.size())-1;i>=0;--i){const auto& f=frames[i];const std::string name=f.at("functionName"),next=f.value("suspendedAt","").empty()?f.at("nextNode").get<std::string>():f.at("suspendedAt").get<std::string>();
            const auto title=std::to_string(i)+"  "+(name.empty()?std::string(tr("Main graph")):name)+"  /  "+next;
            if(ImGui::Selectable(label(title,"frame"+std::to_string(i)).c_str(),debugFrame_==i||(debugFrame_<0&&i==frames.size()-1))){debugFrame_=i;if(!next.empty())focusGraphNode(debugGraph_,name,next);}
        }
        if(debugFrame_>=static_cast<int>(frames.size()))debugFrame_=-1;inspected=frames[debugFrame_<0?frames.size()-1:debugFrame_];
    }
    if(ImGui::BeginTable("##debugViews",3,ImGuiTableFlags_Resizable|ImGuiTableFlags_BordersInnerV)){
        ImGui::TableNextRow();ImGui::TableNextColumn();cyui::sectionTitle(tr("Execution trace"));
        for(const auto& entry:state.value("trace",Json::array())){
            const std::string id=entry.at("nodeId"),type=entry.at("type"),function=entry.value("functionName","");
            const auto body=graphView(core.graphs().at(debugGraph_),function);std::string nodeTitle=type;
            for(const auto& n:body.at("nodes"))if(n.at("id")==id){const auto spec=nodeDefinition(body,n);nodeTitle=type=="function.call"?spec.at("title").get<std::string>():std::string(tr(spec.at("title").get_ref<const std::string&>().c_str()));break;}
            const auto title=std::to_string(entry.at("step").get<int>())+"  "+(function.empty()?"":function+" / ")+nodeTitle;
            if(ImGui::Selectable(label(title,"trace"+std::to_string(entry.at("step").get<int>())).c_str()))focusGraphNode(debugGraph_,function,id);
        }
        ImGui::TableNextColumn();cyui::sectionTitle(tr("Runtime variables"));
        const auto variables=inspected.value("variables",Json::object());for(const auto& [name,v]:variables.items())ImGui::TextWrapped("%s = %s",name.c_str(),v.dump().c_str());
        const auto arguments=inspected.value("arguments",Json::object());if(!arguments.empty())cyui::sectionTitle(tr("Arguments"));for(const auto& [name,v]:arguments.items())ImGui::TextWrapped("%s = %s",name.c_str(),v.dump().c_str());
        ImGui::TableNextColumn();cyui::sectionTitle(tr("Observed values"));
        const auto watches=inspected.value("watches",Json::object());for(const auto& [name,v]:watches.items())ImGui::TextWrapped("%s = %s",name.c_str(),v.dump().c_str());
        ImGui::EndTable();
    }
    const auto objects=state.value("objects",Json::object());if(!objects.empty())cyui::sectionTitle(tr("Runtime objects"));for(const auto& [id,instance]:objects.items())if(cyui::beginSection(label(instance.at("class").get<std::string>()+" / "+id,id).c_str())){const auto schema=blueprintClass(core.graphs().at(debugGraph_).value("classes",Json::object()),instance.at("class"),core.graphs().at(debugGraph_).value("types",Json::object()));if(cyui::beginProperties(id.c_str(),cyui::dp(120))){for(const auto& [field,value]:instance.at("fields").items()){cyui::property(field.c_str());Json next;if(editValue(debugGraph_+"/"+id+"/"+field,graphWidget(schema.at("fields").at(field).at("type"),core.graphs().at(debugGraph_).value("types",Json::object())),value,next))invoke("graph_debug_object_set",{{"graphId",debugGraph_},{"object",id},{"field",field},{"value",next}});}cyui::endProperties();}cyui::endSection();}
    if(state.contains("messages")&&!state["messages"].empty()){cyui::sectionTitle(tr("Graph messages"));for(const auto& message:state["messages"])ImGui::TextUnformatted(message.get_ref<const std::string&>().c_str());}
}
void EditorApp::graphDiagnosticsPanel(){
    if(!core.graphs().contains(activeGraph_))return;
    if(cyui::ghostButton(tr("Check graph"))){
        Json p={{"graphId",activeGraph_}};if(!activeFunction_.empty())p["functionName"]=activeFunction_;auto result=invoke("graph_diagnose",p);graphViews_[activeGraph_+(activeFunction_.empty()?"":"#"+activeFunction_)].diagnostics=result.value("diagnostics",Json::array());
    }
    const auto key=activeGraph_+(activeFunction_.empty()?"":"#"+activeFunction_);auto& view=graphViews_[key];if(view.diagnostics.empty())cyui::textDim("%s",tr("No graph diagnostics"));
    for(const auto& entry:view.diagnostics){
        const std::string severity=entry.at("severity"),id=entry.at("nodeId"),message=entry.at("message"),function=entry.value("functionName",activeFunction_);
        cyui::badge(tr(severity.c_str()),severity=="Error"?cyui::Tone::error:severity=="Warning"?cyui::Tone::warning:cyui::Tone::info);
        ImGui::SameLine();if(ImGui::Selectable((id.empty()?std::string(tr(message.c_str())):std::string(tr(message.c_str()))+"###"+id).c_str())&&!id.empty()){
            focusGraphNode(activeGraph_,function,id);
        }
    }
}
}
