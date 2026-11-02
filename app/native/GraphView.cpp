#include "Editor.h"
#include "Graph.h"
#include <algorithm>
#include <cmath>
#include <cstring>
namespace cygame {
namespace {
float distance(ImVec2 a,ImVec2 b){return std::hypot(a.x-b.x,a.y-b.y);}
}
void EditorApp::graphEditor(const std::string& graphId,const std::string& functionName) {
    const auto viewKey=graphId+(functionName.empty()?"":"#"+functionName);auto& view=graphViews_[viewKey];const auto& pal=cyui::palette();
    if(!core.graphs().contains(graphId)||(!functionName.empty()&&(!core.graphs().at(graphId).contains("functions")||!core.graphs().at(graphId).at("functions").contains(functionName)))){
        cyui::emptyState(tr("Asset Editor"),tr("This asset is not loaded in the current project."),cyui::icon::info);return;
    }
    if(core.graphs().at(graphId).at("domain")=="material-instance"){materialInstanceEditor(graphId);return;}
    auto graph=graphView(core.graphs().at(graphId),functionName);
    auto command=[&](const std::string& id,Json p){if(id.rfind("graph_",0)==0&&id.rfind("graph_function_",0)!=0&&id.rfind("graph_debug_",0)!=0&&!functionName.empty())p["functionName"]=functionName;return invoke(id,p);};const std::string domain=graph.at("domain");const bool execution=domain=="logic"||(domain=="function"&&graph.value("mode","pure")=="execution");
    std::set<std::string> existing;for(const auto& n:graph["nodes"])existing.insert(n.at("id"));
    for(auto it=view.selection.begin();it!=view.selection.end();)if(!existing.count(*it))it=view.selection.erase(it);else ++it;
    if(!existing.count(view.selected))view.selected.clear();
    if(!view.selected.empty()&&!view.selection.count(view.selected))view.selection={view.selected};
    auto selectCopies=[&](const Json& result){if(result.contains("nodes")){view.selection.clear();for(const auto& n:result["nodes"])view.selection.insert(n.at("id"));view.selected=view.selection.empty()?"":*view.selection.begin();}};
    auto copy=[&](){if(view.selection.empty())return;auto result=command("graph_fragment_copy",{{"graphId",graphId},{"nodeIds",view.selection}});if(result.contains("fragment"))ImGui::SetClipboardText(result["fragment"].dump().c_str());};
    auto paste=[&](){const char* text=ImGui::GetClipboardText();if(!text||std::strlen(text)>512*1024)return;try{selectCopies(command("graph_fragment_paste",{{"graphId",graphId},{"fragment",Json::parse(text)}}));}catch(const std::exception& e){cyui::notify(e.what(),cyui::Tone::error);}};
    auto duplicate=[&](){if(!view.selection.empty())selectCopies(command("graph_nodes_duplicate",{{"graphId",graphId},{"nodeIds",view.selection}}));};
    auto remove=[&](){if(!view.selection.empty()){auto result=command("graph_nodes_remove",{{"graphId",graphId},{"nodeIds",view.selection}});if(!result.contains("error")){view.selection.clear();view.selected.clear();}}};
    bool frameAll=ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)&&!ImGui::GetIO().WantTextInput&&ImGui::IsKeyPressed(ImGuiKey_Home);
    if(cyui::primaryButton(tr("Add node")))ImGui::OpenPopup("##nodePalette");
    ImGui::SameLine();if(cyui::ghostButton(tr("Check graph"))){
        auto report=command("graph_diagnose",{{"graphId",graphId}});view.diagnostics=report.value("diagnostics",Json::array());activeGraph_=graphId;activeFunction_=functionName;open("graphDiagnostics");
    }
    ImGui::SameLine();if(cyui::ghostButton(tr(domain=="function"?"Evaluate function":domain=="logic"?"Run graph":"Preview material"))){
        const auto result=domain=="function"?command("graph_function_evaluate",{{"graphId",graphId},{"name",functionName}}):command("graph_run",{{"graphId",graphId}});
        view.result=result.dump(2);
        if(result.contains("baseColor"))view.preview=result;
    }
    ImGui::SameLine();if(cyui::ghostButton(tr("Save asset")))command("asset_save_graph",{{"graphId",graphId}});
    ImGui::SameLine();if(cyui::ghostButton(tr("Compare saved")))invoke("graph_diff_view",{{"graphId",graphId}});
    ImGui::SameLine();if(cyui::ghostButton(tr("Frame all   Home")))frameAll=true;
    ImGui::SameLine();cyui::badge(tr(domain=="function"?"FUNCTION":domain=="logic"?"LOGIC":"MATERIAL"));
    if(execution){
        view.debug=core.execute("graph_debug_status",{{"graphId",graphId}});if(view.debug.value("activeFunction","")!=functionName)view.debug.erase("nextNode");
        if(cyui::ghostButton(tr("Debug"))){view.debug=command("graph_debug_start",{{"graphId",graphId}});debugGraph_=graphId;open("debugger");}
        ImGui::SameLine();if(cyui::ghostButton(tr("Step over   F10")))view.debug=command("graph_debug_step",{{"graphId",graphId}});
        ImGui::SameLine();if(cyui::ghostButton(tr("Step into   F11")))view.debug=command("graph_debug_step_into",{{"graphId",graphId}});
        ImGui::SameLine();if(cyui::ghostButton(tr("Step out   Shift+F11")))view.debug=command("graph_debug_step_out",{{"graphId",graphId}});
        ImGui::SameLine();if(cyui::ghostButton(tr("Continue   F5")))view.debug=command("graph_debug_continue",{{"graphId",graphId}});
        ImGui::SameLine();if(cyui::ghostButton(tr("Stop")))view.debug=command("graph_debug_stop",{{"graphId",graphId}});
        ImGui::SameLine();cyui::badge(tr(view.debug.value("state","Idle").c_str()));
    }
    if(ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)&&!ImGui::GetIO().WantTextInput){
        if(execution&&ImGui::IsKeyPressed(ImGuiKey_F10))view.debug=command("graph_debug_step",{{"graphId",graphId}});
        if(execution&&ImGui::IsKeyPressed(ImGuiKey_F5))view.debug=command("graph_debug_continue",{{"graphId",graphId}});
        if(execution&&ImGui::IsKeyPressed(ImGuiKey_F11))view.debug=command(ImGui::GetIO().KeyShift?"graph_debug_step_out":"graph_debug_step_into",{{"graphId",graphId}});
        const auto& input=ImGui::GetIO();
        if(input.KeyCtrl&&ImGui::IsKeyPressed(ImGuiKey_A)){view.selection=existing;view.selected=view.selection.empty()?"":*view.selection.begin();}
        if(input.KeyCtrl&&ImGui::IsKeyPressed(ImGuiKey_C))copy();
        if(input.KeyCtrl&&ImGui::IsKeyPressed(ImGuiKey_V))paste();
        if(input.KeyCtrl&&ImGui::IsKeyPressed(ImGuiKey_D))duplicate();

        if(!view.selected.empty()&&ImGui::IsKeyPressed(ImGuiKey_F9))for(const auto& n:graph["nodes"])if(n["id"]==view.selected)command("graph_breakpoint_set",{{"graphId",graphId},{"nodeId",view.selected},{"enabled",!n.value("breakpoint",false)}});
    }
    if(execution&&view.debug.value("activeFunction","")!=functionName)view.debug.erase("nextNode");
    graphNavigationTools(graphId,functionName,graph);
    if(cyui::iconButton(cyui::icon::copy,tr("Copy nodes   Ctrl+C")))copy();ImGui::SameLine();
    if(cyui::iconButton(cyui::icon::paste,tr("Paste nodes   Ctrl+V")))paste();ImGui::SameLine();
    if(cyui::ghostButton(tr("Duplicate selection")))duplicate();ImGui::SameLine();
    if(cyui::ghostButton(tr("Align left"))&&!view.selection.empty()){
        double x=1e9;for(const auto& n:graph["nodes"])if(view.selection.count(n.at("id")))x=std::min(x,n["position"][0].get<double>());
        Json positions=Json::array();for(const auto& n:graph["nodes"])if(view.selection.count(n.at("id")))positions.push_back({{"nodeId",n.at("id")},{"position",{x,n["position"][1]}}});command("graph_nodes_move",{{"graphId",graphId},{"positions",positions}});
    }ImGui::SameLine();if(cyui::ghostButton(tr("Comment group")))ImGui::OpenPopup("##groupComment");
    if(ImGui::BeginPopup("##groupComment")){
        ImGui::InputText(tr("Comment"),groupComment_,sizeof(groupComment_));
        if(cyui::primaryButton(tr("Create"))&&!view.selection.empty()){
            ImVec2 low{1e9f,1e9f},high{-1e9f,-1e9f};for(const auto& n:graph["nodes"])if(view.selection.count(n.at("id"))){const float x=n["position"][0],y=n["position"][1];const auto spec=nodeDefinition(graph,n);low.x=std::min(low.x,x-20);low.y=std::min(low.y,y-45);high.x=std::max(high.x,x+225);high.y=std::max(high.y,y+54+27*(1+float(std::max(spec["inputs"].size(),spec["outputs"].size()))));}
            auto result=command("graph_comment_add",{{"graphId",graphId},{"text",std::string(groupComment_)},{"position",{low.x,low.y}},{"size",{high.x-low.x,high.y-low.y}}});if(!result.contains("error"))ImGui::CloseCurrentPopup();
        }ImGui::EndPopup();
    }
    ImGui::SameLine();cyui::textFaint("%zu %s",view.selection.size(),tr("selected"));
    ImGui::SameLine();graphCollapseUI(graphId,functionName,viewKey);
    ImGui::SameLine();graphExpandUI(graphId,functionName,viewKey);
    if(ImGui::BeginPopup("##nodePalette")) {
        cyui::searchField("##nodeSearch",view.filter,sizeof(view.filter),tr("Search nodes"),cyui::dp(240));
        for(const auto& [type,spec]:nodeTypes().items()) {
            if(type=="typed.literal"||type=="typed.get"||type=="typed.set"||type=="route.typed"||type.rfind("struct.",0)==0||type.rfind("enum.",0)==0||type=="object.get.typed"||type=="object.set.typed"||type=="interface.supports")continue;
            if(type.rfind("function.",0)==0||type=="macro.call"||(type=="object.call"||type=="interface.call")||type=="flow.foreach"||(domain=="function"&&(type=="event.begin"||type=="material.output")))continue;
            if(domain=="function"&&!execution) {bool pure=type.rfind("variable.",0)!=0&&type!="material.output";for(const auto* side:{"inputs","outputs"})for(const auto& t:spec[side])pure&=t!="exec";if(!pure)continue;}
            else if(domain!="function"&&spec["domain"]!="both"&&spec["domain"]!=domain)continue;
            if(spec.contains("variableType")){
                bool match=false;const auto variables=graph.value("variables",Json::object());for(const auto& v:variables)match|=v.at("type")==spec["variableType"];if(!match)continue;
            }
            const auto rawTitle=spec.at("title").get<std::string>();const auto title=std::string(tr(rawTitle.c_str()))+(type.rfind("route.",0)==0?" / "+std::string(tr(type.substr(6).c_str())):"");
            if(matches(title+" "+rawTitle+" "+type,view.filter)&&ImGui::Selectable(label(title,type).c_str())) {
                command("graph_node_add",{{"graphId",graphId},{"type",type},{"position",{view.newNodeAt.x,view.newNodeAt.y}}});
                ImGui::CloseCurrentPopup();
            }
        }
        const auto interfaces=graph.value("interfaces",Json::object());for(const auto& [name,definition]:interfaces.items()){
            auto offer=[&](const char* type,const std::string& method){Json n={{"type",type},{"interface",name}};if(!method.empty())n["method"]=method;const auto spec=interfaceNodeDefinition(graph,n);if(!execution&&spec.value("execution",false))return;const auto title=std::string(tr(type))+" / "+name+(method.empty()?"":"."+method);if(matches(title+" "+type,view.filter)&&ImGui::Selectable(label(title,title).c_str())){Json p={{"graphId",graphId},{"type",type},{"interfaceRef",name},{"position",{view.newNodeAt.x,view.newNodeAt.y}}};if(!method.empty())p["methodRef"]=method;command("graph_node_add",p);ImGui::CloseCurrentPopup();}};
            offer("interface.supports","");for(const auto& [method,signature]:definition.at("methods").items())offer("interface.call",method);
        }
        const auto dataTypes=graph.value("types",Json::object());for(const auto& [id,d]:dataTypes.items()){
            std::vector<Json> entries;auto offer=[&](const char* op,const std::string& field=""){Json n={{"type",op},{"typeRef",id}};if(!field.empty())n["field"]=field;entries.push_back(n);};
            offer("typed.literal");offer("route.typed");offer("object.get.typed");if(execution)offer("object.set.typed");if(d.at("kind")=="enum"){offer("enum.equal");offer("enum.to_string");if(execution)offer("enum.switch");}else{offer("struct.make");offer("struct.break");for(const auto& [field,spec]:d.at("fields").items())offer("struct.set",field);}
            for(const auto& n:entries){const auto spec=typedNodeDefinition(graph,n);const std::string title=std::string(tr(spec.at("title").get_ref<const std::string&>().c_str()))+" / "+id+(n.contains("field")?" / "+n.at("field").get<std::string>():"");if(matches(title+" "+n.at("type").get<std::string>()+" "+spec.at("title").get<std::string>(),view.filter)&&ImGui::Selectable(label(title,title).c_str())){Json p={{"graphId",graphId},{"type",n.at("type")},{"typeRef",id},{"position",{view.newNodeAt.x,view.newNodeAt.y}}};if(n.contains("field"))p["fieldRef"]=n.at("field");command("graph_node_add",p);ImGui::CloseCurrentPopup();}}
        }ImGui::EndPopup();
    }
    // Toolbar/modal actions can replace the selected region in this very frame.
    // Build the common Nodes model from the new document, retaining new selection IDs.
    graph=graphView(core.graphs().at(graphId),functionName);
    const float sidebar=cyui::dp(235);
    ImGui::BeginChild("##nodeCanvas",{std::max(cyui::dp(150),ImGui::GetContentRegionAvail().x-sidebar-cyui::dp(8)),0},ImGuiChildFlags_Borders,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
    const auto start=ImGui::GetCursorScreenPos(),size=ImGui::GetContentRegionAvail();const float dpi=cyui::dp(1);
    auto& state=view.nodes;if(view.wireRevision!=core.revision()){view.wireNode.clear();view.wireRevision=core.revision();}state.canvas.view.zoom=view.zoom;state.canvas.view.origin={-view.pan.x/view.zoom,-view.pan.y/view.zoom};
    state.selection.clear();std::map<cyui::ItemId,std::string> nodeNames,linkNames;
    struct Pin {std::string node,key,type;bool output;cyui::ItemId id;ImVec2 point;};std::map<cyui::ItemId,Pin> pins;
    std::vector<cyui::Node> nodes;std::vector<cyui::NodeLink> links;
    auto pinId=[&](const std::string& node,const std::string& pin,bool out){return uiId("pin/"+viewKey+Json::array({node,pin,out}).dump());};
    for(const auto& n:graph.at("nodes")){
        const std::string id=n.at("id"),type=n.at("type");const auto spec=nodeDefinition(graph,n);const auto numeric=uiId("node/"+viewKey+id);
        if(!nodeNames.emplace(numeric,id).second)throw std::runtime_error("UI node identity collision");
        const bool named=type=="function.call"||type=="macro.call"||type=="object.call"||type=="interface.call";
        cyui::Node node;node.id=numeric;node.position={n.at("position")[0].get<double>()*dpi,n.at("position")[1].get<double>()*dpi};node.label=named?spec.at("title").get<std::string>():tr(spec.at("title").get_ref<const std::string&>().c_str());
        if(n.contains("variable"))node.label+=" / "+n.at("variable").get<std::string>();node.colorIndex=0;
        for(const auto* side:{"inputs","outputs"}){const bool out=std::string(side)=="outputs";for(const auto& [key,t]:spec.at(side).items()){
            const auto pid=pinId(id,key,out);const std::string portType=t;const bool user=named||type=="function.input"||type=="function.output"||type=="struct.make"||type=="struct.break"||type=="enum.switch";
            if(!pins.emplace(pid,Pin{id,key,portType,out,pid,{}}).second)throw std::runtime_error("UI pin identity collision");
            (out?node.outputs:node.inputs).push_back({pid,user?key:std::string(tr(key.c_str())),portType=="exec"?0:portType=="bool"?2:1});
        }}nodes.push_back(node);if(view.selection.count(id))state.selection.push_back(numeric);
    }
    for(const auto& l:graph.at("links")){const auto id=uiId("link/"+viewKey+l.at("id").get<std::string>());linkNames[id]=l.at("id");links.push_back({id,pinId(l.at("fromNode"),l.at("fromPin"),true),pinId(l.at("toNode"),l.at("toPin"),false),0});}
    if(frameAll&&!nodes.empty()){
        cyui::Point low{1e9,1e9},high{-1e9,-1e9};for(const auto& n:nodes){low.x=std::min(low.x,n.position.x);low.y=std::min(low.y,n.position.y);high.x=std::max(high.x,n.position.x+cyui::dp(205));high.y=std::max(high.y,n.position.y+cyui::dp(34+27*float(std::max(n.inputs.size(),n.outputs.size()))+8));}state.canvas.view.fit(low,high,size,cyui::dp(35));
    }
    cyui::NodeOptions options;options.width=205;options.header=34;options.row=27;options.revision=core.revision();
    options.canConnect=[&](cyui::ItemId from,cyui::ItemId to){return pins.count(from)&&pins.count(to)&&pins.at(from).type==pins.at(to).type;};
    options.drawBody=[&](const cyui::Node& node,const cyui::Rect& rect){
        const auto& id=nodeNames.at(node.id);for(const auto& n:graph.at("nodes"))if(n.at("id")==id){auto* draw=ImGui::GetWindowDrawList();const float zoom=float(state.canvas.view.zoom);
            if(n.value("breakpoint",false))draw->AddCircleFilled({rect.max.x-cyui::dp(5),rect.min.y-cyui::dp(17)*zoom},cyui::dp(5),pal.error);
            if(view.debug.value("nextNode","")==id)draw->AddRect({rect.min.x-cyui::dp(8)*zoom,rect.min.y-cyui::dp(34)*zoom},rect.max,pal.warning,cyui::dp(4),0,cyui::dp(2));
            if(!n.value("comment","").empty()&&rect.contains(ImGui::GetIO().MousePos))ImGui::SetTooltip("%s",n.at("comment").get_ref<const std::string&>().c_str());
        }
    };
    const bool wasBoxing=state.boxing;const auto boxAnchor=state.boxStart;const auto result=cyui::nodeEditor("##canvas",state,nodes,links,size,options);
    auto* draw=ImGui::GetWindowDrawList();auto& io=ImGui::GetIO();const bool hovered=ImGui::IsItemHovered();
    if(result.moved.committed()){
        Json positions=Json::array();bool changed=false;for(const auto& moved:result.moved.value){const auto& name=nodeNames.at(moved.id);positions.push_back({{"nodeId",name},{"position",{moved.position.x/dpi,moved.position.y/dpi}}});for(const auto& n:graph.at("nodes"))if(n.at("id")==name)changed|=std::abs(n.at("position")[0].get<double>()-moved.position.x/dpi)>.001||std::abs(n.at("position")[1].get<double>()-moved.position.y/dpi)>.001;}
        if(changed)command("graph_nodes_move",{{"graphId",graphId},{"positions",positions},{"expectedRevision",state.revision}});
    }
    if(result.connect){const auto& from=pins.at(result.connectFrom);const auto& to=pins.at(result.connectTo);command("graph_link_add",{{"graphId",graphId},{"fromNode",from.node},{"fromPin",from.key},{"toNode",to.node},{"toPin",to.key}});view.wireNode.clear();}
    if(result.linkRemoved&&linkNames.count(result.removeLink))command("graph_link_remove",{{"graphId",graphId},{"linkId",linkNames.at(result.removeLink)}});
    if(!result.removeNodes.empty()){Json names=Json::array();for(const auto id:result.removeNodes)if(nodeNames.count(id))names.push_back(nodeNames.at(id));if(!names.empty())command("graph_nodes_remove",{{"graphId",graphId},{"nodeIds",names}});}
    Json boxes=Json::array();
    // Application overlays supply type colors, breakpoints and authored comment
    // groups; common Nodes owns cards, wires, hit testing, navigation and gestures.
    for(const auto& n:nodes){auto pos=n.position;for(const auto& moved:state.preview)if(moved.id==n.id)pos=moved.position;const auto origin=state.canvas.view.screen(pos,start);const float zoom=float(state.canvas.view.zoom),width=cyui::dp(205)*zoom,height=cyui::dp(34+27*float(std::max<size_t>(1,std::max(n.inputs.size(),n.outputs.size())))+8)*zoom;const ImVec2 end{origin.x+width,origin.y+height};
        boxes.push_back({{"id",nodeNames.at(n.id)},{"a",{origin.x,origin.y}},{"b",{end.x,end.y}}});
        if(wasBoxing&&ImGui::IsMouseReleased(0)){const auto anchor=state.canvas.view.screen(boxAnchor,start);const cyui::Rect selection{{std::min(anchor.x,io.MousePos.x),std::min(anchor.y,io.MousePos.y)},{std::max(anchor.x,io.MousePos.x),std::max(anchor.y,io.MousePos.y)}};if(origin.x<selection.max.x&&end.x>selection.min.x&&origin.y<selection.max.y&&end.y>selection.min.y&&!cyui::selected(state.selection,n.id))state.selection.push_back(n.id);}
        for(const bool out:{false,true}){const auto& ports=out?n.outputs:n.inputs;for(size_t i=0;i<ports.size();++i){auto& pin=pins.at(ports[i].id);pin.point={origin.x+(out?width:0),origin.y+cyui::dp(34+27*(float(i)+.5f))*zoom};const auto color=pin.type=="exec"?pal.text:pin.type=="bool"?pal.warning:pin.type=="color"?pal.info:pal.accent;draw->AddCircleFilled(pin.point,cyui::dp(4)*std::max(.6f,zoom),color);}}
    }
    view.selection.clear();for(const auto id:state.selection)if(nodeNames.count(id))view.selection.insert(nodeNames.at(id));if(!view.selection.count(view.selected))view.selected=view.selection.empty()?"":*view.selection.begin();
    if(hovered&&ImGui::IsMouseClicked(0)){
        bool clicked=false;for(const auto& [id,pin]:pins)if(distance(pin.point,io.MousePos)<cyui::dp(9)){
            clicked=true;if(!view.wireNode.empty()&&view.wireOutput!=pin.output){const auto from=view.wireOutput?view.wireNode:pin.node,fp=view.wireOutput?view.wirePin:pin.key,to=view.wireOutput?pin.node:view.wireNode,tp=view.wireOutput?pin.key:view.wirePin;command("graph_link_add",{{"graphId",graphId},{"fromNode",from},{"fromPin",fp},{"toNode",to},{"toPin",tp}});view.wireNode.clear();state.wiring=false;}else{view.wireNode=pin.node;view.wirePin=pin.key;view.wireOutput=pin.output;}break;
        }if(!clicked)view.wireNode.clear();
    }
    if(ImGui::IsKeyPressed(ImGuiKey_Escape)||result.moved.phase==cyui::EditPhase::cancel)view.wireNode.clear();
    const auto comments=graph.value("comments",Json::array());draw->PushClipRect(start,{start.x+size.x,start.y+size.y},true);
    for(const auto& comment:comments){const auto low=state.canvas.view.screen({comment.at("position")[0].get<double>()*dpi,comment.at("position")[1].get<double>()*dpi},start),high=state.canvas.view.screen({(comment.at("position")[0].get<double>()+comment.at("size")[0].get<double>())*dpi,(comment.at("position")[1].get<double>()+comment.at("size")[1].get<double>())*dpi},start);draw->AddRect(low,high,cyui::alpha(pal.info,.5f),cyui::dp(4));draw->AddText({low.x+cyui::dp(8),low.y+cyui::dp(8)},pal.textDim,comment.at("text").get_ref<const std::string&>().c_str());}
    if(!view.wireNode.empty())for(const auto& [id,pin]:pins)if(pin.node==view.wireNode&&pin.key==view.wirePin&&pin.output==view.wireOutput&&!state.wiring)draw->AddLine(pin.point,io.MousePos,pal.accent,cyui::dp(2));
    draw->AddText({start.x+cyui::dp(12),start.y+size.y-cyui::dp(24)},pal.textFaint,tr("MMB pan | Wheel zoom | Click pins to connect | RMB add"));draw->PopClipRect();
    if(hovered&&ImGui::IsMouseClicked(1)&&!result.linkRemoved){const auto point=state.canvas.view.world(io.MousePos,start);view.newNodeAt={float(point.x/dpi),float(point.y/dpi)};ImGui::OpenPopup("##canvasMenu");}
    if(ImGui::BeginPopup("##canvasMenu")) {
        for(const auto& [type,spec]:nodeTypes().items())if(type.rfind("function.",0)!=0&&(domain=="function"||spec["domain"]=="both"||spec["domain"]==domain)) {
            if(domain=="function"&&(type=="event.begin"||type=="material.output"))continue;
            if(domain=="function"&&!execution){bool pure=type.rfind("variable.",0)!=0&&type!="material.output";for(const auto* side:{"inputs","outputs"})for(const auto& t:spec[side])pure&=t!="exec";if(!pure)continue;}
            if(spec.contains("variableType")){bool match=false;const auto variables=graph.value("variables",Json::object());for(const auto& v:variables)match|=v.at("type")==spec["variableType"];if(!match)continue;}
            const std::string title=std::string(tr(spec.at("title").get_ref<const std::string&>().c_str()))+(type.rfind("route.",0)==0?" / "+std::string(tr(type.substr(6).c_str())):"");
            if(ImGui::MenuItem(label(title,type).c_str()))
                command("graph_node_add",{{"graphId",graphId},{"type",type},{"position",{view.newNodeAt.x,view.newNodeAt.y}}});
        }ImGui::EndPopup();
    }
    view.zoom=float(state.canvas.view.zoom);view.pan={float(-state.canvas.view.origin.x*view.zoom),float(-state.canvas.view.origin.y*view.zoom)};
    if(graphProbe)graphProbe(viewKey,boxes,view.selection);
    ImGui::EndChild();ImGui::SameLine();
    ImGui::BeginChild("##nodeDetails",{0,0});
    cyui::sectionTitle(tr("Node properties"));
    if(view.selected.empty())cyui::textDim("%s",tr("Select a node to edit its values."));
    for(const auto& n:graph["nodes"])if(n["id"]==view.selected){
        std::string type=n.at("type");const auto definition=nodeDefinition(graph,n);cyui::heading(type=="function.call"?definition.at("title").get_ref<const std::string&>().c_str():tr(definition.at("title").get_ref<const std::string&>().c_str()),3);
        if((type=="function.call"||type=="macro.call")&&cyui::ghostButton(tr("Open function")))openFunction(graphId,n.at("function"));
        cyui::textFaint("%s",type.c_str());
        if(n.contains("variable"))cyui::textDim("%s: %s",tr("Variable"),n.at("variable").get_ref<const std::string&>().c_str());
        if(execution&&(type=="event.begin"||type=="function.input"||definition.at("inputs").contains("exec"))){
            bool breakpoint=n.value("breakpoint",false);if(ImGui::Checkbox(tr("Breakpoint   F9"),&breakpoint))command("graph_breakpoint_set",{{"graphId",graphId},{"nodeId",view.selected},{"enabled",breakpoint}});
        }
        if(commentNode_!=viewKey+"/"+view.selected){commentNode_=viewKey+"/"+view.selected;std::snprintf(nodeComment_,sizeof(nodeComment_),"%s",n.value("comment","").c_str());}
        if(ImGui::InputText(tr("Comment"),nodeComment_,sizeof(nodeComment_),ImGuiInputTextFlags_EnterReturnsTrue))command("graph_node_comment",{{"graphId",graphId},{"nodeId",view.selected},{"text",std::string(nodeComment_)}});
        if(cyui::ghostButton(tr("Duplicate node"))){auto copies=command("graph_nodes_duplicate",{{"graphId",graphId},{"nodeIds",Json::array({view.selected})}});if(copies.contains("nodes"))view.selected=copies["nodes"][0].at("id");}
        if(n.contains("typeRef"))cyui::textDim("%s: %s",tr("Data type"),n.at("typeRef").get_ref<const std::string&>().c_str());
        if(n.contains("interface"))cyui::textDim("%s: %s",tr("Interface"),n.at("interface").get_ref<const std::string&>().c_str());
        if(n.contains("function"))cyui::textDim("%s: %s",tr("Definition"),n.at("function").get_ref<const std::string&>().c_str());
        for(const auto& [key,v]:n["values"].items()){
            const auto definition=nodeDefinition(graph,n);const std::string portType=definition.at("inputs").contains(key)?definition["inputs"][key].get<std::string>():definition.at("outputs").at(key).get<std::string>();Json spec=graphWidget(portType,graph.value("types",Json::object()));
            const std::string caption=type.rfind("function.",0)==0||type=="macro.call"||(type=="object.call"||type=="interface.call")||type=="struct.make"?key:tr(key.c_str());Json changed;
            if(editProperty(viewKey+"/"+view.selected+"/"+key,caption,spec,v,changed))
                command("graph_node_value",{{"graphId",graphId},{"nodeId",view.selected},{"key",key},{"value",changed}});
        }
        if(cyui::dangerButton(tr("Delete node")))remove();
        cyui::sectionTitle(tr("Connections"));
        for(const auto& l:graph["links"])if(l["fromNode"]==n["id"]||l["toNode"]==n["id"]){
            ImGui::PushID(l.at("id").get_ref<const std::string&>().c_str());
            cyui::textDim("%s > %s",l.at("fromPin").get_ref<const std::string&>().c_str(),l.at("toPin").get_ref<const std::string&>().c_str());
            if(cyui::ghostButton(tr("Insert reroute"))){ImVec2 midpoint{};for(const auto& peer:graph["nodes"])if(peer["id"]==l["fromNode"]||peer["id"]==l["toNode"]){const bool output=peer["id"]==l["fromNode"],route=peer.at("type").get<std::string>().rfind("route.",0)==0;const auto d=nodeDefinition(graph,peer);int index=0;for(const auto& [key,t]:d.at(output?"outputs":"inputs").items()){++index;if(key==l.at(output?"fromPin":"toPin"))break;}midpoint.x+=(peer["position"][0].get<float>()+(output?(route?36:205):0))*.5f;midpoint.y+=(peer["position"][1].get<float>()+(route?12:34+27*index))*.5f;}midpoint.x-=18;midpoint.y-=12;command("graph_link_reroute",{{"graphId",graphId},{"linkId",l["id"]},{"position",{midpoint.x,midpoint.y}}});}
            if(cyui::ghostButton(tr("Disconnect")))command("graph_link_remove",{{"graphId",graphId},{"linkId",l["id"]}});
            ImGui::PopID();
        }
    }
    if(!comments.empty())graphCommentProperties(graphId,functionName,comments);
    if(domain=="material")materialPreviewUI(graphId);
    if(!view.result.empty()){cyui::sectionTitle(tr("Result"));ImGui::TextWrapped("%s",view.result.c_str());}
    ImGui::EndChild();
}
}
