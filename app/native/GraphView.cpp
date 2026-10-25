#include "Editor.h"
#include "Graph.h"
#include <algorithm>
#include <cmath>
#include <cstring>
namespace cygame {
namespace {
ImVec2 add(ImVec2 a,ImVec2 b){return{a.x+b.x,a.y+b.y};}
float distance(ImVec2 a,ImVec2 b){return std::hypot(a.x-b.x,a.y-b.y);}
}
void EditorApp::graphEditor(const std::string& graphId,const std::string& functionName) {
    const auto viewKey=graphId+(functionName.empty()?"":"#"+functionName);auto& view=graphViews_[viewKey];const auto& pal=cyui::palette();
    if(!core.graphs().contains(graphId)||(!functionName.empty()&&(!core.graphs().at(graphId).contains("functions")||!core.graphs().at(graphId).at("functions").contains(functionName)))){
        cyui::emptyState(tr("Asset Editor"),tr("This asset is not loaded in the current project."),cyui::icon::info);return;
    }
    if(core.graphs().at(graphId).at("domain")=="material-instance"){materialInstanceEditor(graphId);return;}
    const auto graph=graphView(core.graphs().at(graphId),functionName);
    auto command=[&](const std::string& id,Json p){if(id.rfind("graph_",0)==0&&id.rfind("graph_function_",0)!=0&&id.rfind("graph_debug_",0)!=0&&!functionName.empty())p["functionName"]=functionName;return invoke(id,p);};const std::string domain=graph.at("domain");const bool execution=domain=="logic"||(domain=="function"&&graph.value("mode","pure")=="execution");
    std::set<std::string> existing;for(const auto& n:graph["nodes"])existing.insert(n.at("id"));
    for(auto it=view.selection.begin();it!=view.selection.end();)if(!existing.count(*it))it=view.selection.erase(it);else ++it;
    if(!existing.count(view.selected))view.selected.clear();
    if(!view.selected.empty()&&!view.selection.count(view.selected))view.selection={view.selected};
    if(!view.dragging.empty()&&(core.revision()!=view.dragRevision||ImGui::IsKeyPressed(ImGuiKey_Escape))){view.dragging.clear();view.dragPositions=Json::array();}
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
        if(ImGui::IsKeyPressed(ImGuiKey_Delete))remove();
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
    const float sidebar=cyui::dp(235);
    ImGui::BeginChild("##nodeCanvas",{std::max(cyui::dp(150),ImGui::GetContentRegionAvail().x-sidebar-cyui::dp(8)),0},ImGuiChildFlags_Borders,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
    ImVec2 start=ImGui::GetCursorScreenPos(),size=ImGui::GetContentRegionAvail(),end=add(start,size);
    if(frameAll&&!graph["nodes"].empty()){
        ImVec2 low{1e9f,1e9f},high{-1e9f,-1e9f};
        for(const auto& n:graph["nodes"]){const float x=n["position"][0],y=n["position"][1];const auto type=nodeDefinition(graph,n);
            low.x=std::min(low.x,x);low.y=std::min(low.y,y);high.x=std::max(high.x,x+205);high.y=std::max(high.y,y+34+27*(1+float(std::max(type["inputs"].size(),type["outputs"].size()))));}
        view.zoom=std::clamp(std::min((size.x-cyui::dp(70))/(high.x-low.x),(size.y-cyui::dp(70))/(high.y-low.y))/cyui::dp(1),.4f,1.8f);
        const float scale=cyui::dp(view.zoom);view.pan={size.x*.5f-(low.x+high.x)*.5f*scale,size.y*.5f-(low.y+high.y)*.5f*scale};
    }
    ImGui::InvisibleButton("##canvas",size,ImGuiButtonFlags_MouseButtonLeft|ImGuiButtonFlags_MouseButtonMiddle|ImGuiButtonFlags_MouseButtonRight);
    const bool hovered=ImGui::IsItemHovered();auto& io=ImGui::GetIO();
    if(hovered) {
        if(ImGui::IsMouseDragging(ImGuiMouseButton_Middle))view.pan=add(view.pan,io.MouseDelta);
        if(io.MouseWheel!=0){
            const float next=std::clamp(view.zoom*std::pow(1.1f,io.MouseWheel),0.4f,1.8f),ratio=next/view.zoom;
            view.pan={io.MousePos.x-start.x-(io.MousePos.x-start.x-view.pan.x)*ratio,io.MousePos.y-start.y-(io.MousePos.y-start.y-view.pan.y)*ratio};view.zoom=next;
        }
        if(ImGui::IsMouseClicked(ImGuiMouseButton_Right)){
            view.newNodeAt={(io.MousePos.x-start.x-view.pan.x)/view.zoom,(io.MousePos.y-start.y-view.pan.y)/view.zoom};
            ImGui::OpenPopup("##canvasMenu");
        }
    }
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
    auto* draw=ImGui::GetWindowDrawList();draw->PushClipRect(start,end,true);draw->AddRectFilled(start,end,pal.bg0);
    const float z=cyui::dp(view.zoom),width=cyui::dp(205)*view.zoom,header=cyui::dp(34)*view.zoom,row=cyui::dp(27)*view.zoom;
    const float spacing=cyui::dp(32)*view.zoom;
    for(float x=std::fmod(view.pan.x,spacing);x<size.x;x+=spacing)draw->AddLine({start.x+x,start.y},{start.x+x,end.y},pal.grid);
    for(float y=std::fmod(view.pan.y,spacing);y<size.y;y+=spacing)draw->AddLine({start.x,start.y+y},{end.x,start.y+y},pal.grid);
    const auto comments=graph.value("comments",Json::array());for(const auto& c:comments){ImVec2 a{start.x+view.pan.x+c["position"][0].get<float>()*z,start.y+view.pan.y+c["position"][1].get<float>()*z},b{a.x+c["size"][0].get<float>()*z,a.y+c["size"][1].get<float>()*z};draw->AddRectFilled(a,b,cyui::alpha(pal.info,.08f),cyui::dp(4));draw->AddRect(a,b,cyui::alpha(pal.info,.4f),cyui::dp(4));draw->AddText({a.x+cyui::dp(10),a.y+cyui::dp(8)},pal.textDim,c.at("text").get_ref<const std::string&>().c_str());}
    struct Pin {std::string node,key,type;bool output;ImVec2 p;bool userName=false;};
    std::vector<Pin> pins;
    struct Box{std::string id,type;ImVec2 a,b;};std::vector<Box> boxes;
    for(const auto& n:graph.at("nodes")){
        const std::string id=n.at("id"),type=n.at("type");const auto spec=nodeDefinition(graph,n);
        float px=n["position"][0],py=n["position"][1];
        if(!view.dragging.empty()&&ImGui::IsMouseDown(ImGuiMouseButton_Left))for(const auto& original:view.dragPositions)if(original["nodeId"]==id){px=original["position"][0].get<float>()+(io.MousePos.x-view.dragOrigin.x)/z;py=original["position"][1].get<float>()+(io.MousePos.y-view.dragOrigin.y)/z;}
        const ImVec2 a{start.x+view.pan.x+px*z,start.y+view.pan.y+py*z};const bool route=type.rfind("route.",0)==0;
        ImVec2 b{a.x+(route?cyui::dp(36)*view.zoom:width),a.y+(route?cyui::dp(24)*view.zoom:header+row*(1+std::max(spec["inputs"].size(),spec["outputs"].size())))};
        boxes.push_back({id,type,a,b});
        int i=0;for(const auto& [key,t]:spec["inputs"].items())pins.push_back({id,key,t,false,{a.x,a.y+(route?cyui::dp(12)*view.zoom:header+(++i)*row)},((type.rfind("function.",0)==0||type=="macro.call"||(type=="object.call"||type=="interface.call"))&&t!="exec")||type=="struct.make"});
        i=0;for(const auto& [key,t]:spec["outputs"].items())pins.push_back({id,key,t,true,{b.x,a.y+(route?cyui::dp(12)*view.zoom:header+(++i)*row)},((type.rfind("function.",0)==0||type=="macro.call"||(type=="object.call"||type=="interface.call"))&&t!="exec")||type=="struct.break"||type=="enum.switch"});
    }
    auto findPin=[&](const std::string& n,const std::string& p,bool out)->const Pin*{for(const auto& pin:pins)if(pin.node==n&&pin.key==p&&pin.output==out)return &pin;return nullptr;};
    for(const auto& link:graph.at("links")){
        const auto* a=findPin(link.at("fromNode"),link.at("fromPin"),true);const auto* b=findPin(link.at("toNode"),link.at("toPin"),false);
        if(a&&b){const float d=std::max(cyui::dp(12),std::abs(b->p.x-a->p.x)*0.5f);draw->AddBezierCubic(a->p,add(a->p,{d,0}),add(b->p,{-d,0}),b->p,a->type=="exec"?pal.text:pal.accent,cyui::dp(2));}
    }
    if(!view.wireNode.empty()){
        if(const auto* pin=findPin(view.wireNode,view.wirePin,view.wireOutput))
            draw->AddBezierCubic(pin->p,add(pin->p,{cyui::dp(60),0}),add(io.MousePos,{-cyui::dp(60),0}),io.MousePos,pal.accent,cyui::dp(2));
    }
    bool pinClicked=false,boxClicked=false;
    for(const auto& box:boxes){
        bool selected=view.selection.count(box.id)!=0;draw->AddRectFilled(box.a,box.b,pal.bg2,cyui::dp(4));
        const bool reroute=box.type.rfind("route.",0)==0;
        if(!reroute)draw->AddRectFilled(box.a,{box.b.x,box.a.y+header},selected?pal.accentDim:pal.bg4,cyui::dp(4));
        draw->AddRect(box.a,box.b,view.debug.value("nextNode","")==box.id?pal.warning:selected?pal.accent:pal.lineStrong,cyui::dp(4),0,cyui::dp(selected?2.f:1.f));
        Json spec;for(const auto& n:graph["nodes"])if(n["id"]==box.id)spec=nodeDefinition(graph,n);const auto title=box.type=="function.call"||box.type=="macro.call"||(box.type=="object.call"||box.type=="interface.call")?spec.at("title").get<std::string>():std::string(tr(spec.at("title").get_ref<const std::string&>().c_str()));
        if(!reroute)draw->AddText(nullptr,ImGui::GetFontSize()*view.zoom,{box.a.x+cyui::dp(12)*view.zoom,box.a.y+cyui::dp(9)*view.zoom},pal.text,title.c_str());
        else draw->AddLine({box.a.x+cyui::dp(6)*view.zoom,(box.a.y+box.b.y)/2},{box.b.x-cyui::dp(6)*view.zoom,(box.a.y+box.b.y)/2},pal.textDim,cyui::dp(2));
        for(const auto& n:graph["nodes"])if(n["id"]==box.id){
            if(n.contains("variable"))draw->AddText(nullptr,ImGui::GetFontSize()*view.zoom,{box.a.x+cyui::dp(12)*view.zoom,box.b.y-row*.65f},pal.info,n.at("variable").get_ref<const std::string&>().c_str());
            if(n.value("breakpoint",false))draw->AddCircleFilled({box.b.x-cyui::dp(12),box.a.y+header/2},cyui::dp(5),pal.error);
            if(!n.value("comment","").empty()&&hovered&&io.MousePos.x>box.a.x&&io.MousePos.x<box.b.x&&io.MousePos.y>box.a.y&&io.MousePos.y<box.b.y)ImGui::SetTooltip("%s",n.at("comment").get_ref<const std::string&>().c_str());
        }
        if(hovered&&io.MousePos.x>box.a.x&&io.MousePos.x<box.b.x&&io.MousePos.y>box.a.y&&io.MousePos.y<box.b.y&&ImGui::IsMouseClicked(ImGuiMouseButton_Left)){
            if(io.KeyCtrl){if(view.selection.count(box.id))view.selection.erase(box.id);else view.selection.insert(box.id);}else if(!view.selection.count(box.id)){view.selection={box.id};}
            view.selected=view.selection.count(box.id)?box.id:view.selection.empty()?"":*view.selection.begin();boxClicked=true;
            if(io.MousePos.y<box.a.y+header&&view.selection.count(box.id)){
                view.dragging=box.id;view.dragOrigin=io.MousePos;view.dragRevision=core.revision();view.dragPositions=Json::array();
                for(const auto& n:graph["nodes"])if(view.selection.count(n.at("id")))view.dragPositions.push_back({{"nodeId",n.at("id")},{"position",n.at("position")}});
                for(const auto& n:graph["nodes"])if(n["id"]==box.id)view.nodeOrigin={n["position"][0],n["position"][1]};
            }
        }
    }
    for(const auto& pin:pins){
        const auto color=pin.type=="exec"?pal.text:pin.type=="bool"?pal.warning:pin.type=="color"?pal.info:pal.accent;
        draw->AddCircleFilled(pin.p,cyui::dp(5)*view.zoom,color);
        const auto text=pin.userName?pin.key:std::string(tr(pin.key.c_str()));const auto textWidth=ImGui::CalcTextSize(text.c_str()).x*view.zoom;
        if(pin.node.empty()||std::none_of(boxes.begin(),boxes.end(),[&](const Box& b){return b.id==pin.node&&b.type.rfind("route.",0)==0;}))draw->AddText(nullptr,ImGui::GetFontSize()*view.zoom,{pin.p.x+(pin.output?-textWidth-cyui::dp(12)*view.zoom:cyui::dp(12)*view.zoom),pin.p.y-ImGui::GetFontSize()*view.zoom/2},pal.textDim,text.c_str());
        if(hovered&&distance(pin.p,io.MousePos)<cyui::dp(11)&&ImGui::IsMouseClicked(ImGuiMouseButton_Left)){
            pinClicked=true;view.dragging.clear();
            if(!view.wireNode.empty()&&view.wireOutput!=pin.output){
                const auto from=view.wireOutput?view.wireNode:pin.node,fp=view.wireOutput?view.wirePin:pin.key;
                const auto to=view.wireOutput?pin.node:view.wireNode,tp=view.wireOutput?pin.key:view.wirePin;
                command("graph_link_add",{{"graphId",graphId},{"fromNode",from},{"fromPin",fp},{"toNode",to},{"toPin",tp}});
                view.wireNode.clear();
            }else{view.wireNode=pin.node;view.wirePin=pin.key;view.wireOutput=pin.output;}
        }
    }
    if(hovered&&ImGui::IsMouseClicked(ImGuiMouseButton_Left)&&!pinClicked&&!boxClicked){if(!io.KeyCtrl)view.selection.clear();view.selected.clear();view.wireNode.clear();view.marquee=true;view.marqueeStart=io.MousePos;}
    if(view.marquee){
        const ImVec2 a{std::min(view.marqueeStart.x,io.MousePos.x),std::min(view.marqueeStart.y,io.MousePos.y)},b{std::max(view.marqueeStart.x,io.MousePos.x),std::max(view.marqueeStart.y,io.MousePos.y)};
        draw->AddRectFilled(a,b,cyui::alpha(pal.selection,.4f));draw->AddRect(a,b,pal.accent);
        if(ImGui::IsMouseReleased(ImGuiMouseButton_Left)){if(distance(a,b)>cyui::dp(5))for(const auto& box:boxes)if(box.a.x<b.x&&box.b.x>a.x&&box.a.y<b.y&&box.b.y>a.y)view.selection.insert(box.id);view.selected=view.selection.empty()?"":*view.selection.begin();view.marquee=false;}
    }
    if(!view.dragging.empty()&&ImGui::IsMouseReleased(ImGuiMouseButton_Left)){
        const float dx=(io.MousePos.x-view.dragOrigin.x)/z,dy=(io.MousePos.y-view.dragOrigin.y)/z;
        if(std::hypot(dx,dy)>.1f){auto positions=view.dragPositions;for(auto& original:positions){original["position"][0]=original["position"][0].get<float>()+dx;original["position"][1]=original["position"][1].get<float>()+dy;}command("graph_nodes_move",{{"graphId",graphId},{"positions",positions},{"expectedRevision",view.dragRevision}});}
        view.dragging.clear();view.dragPositions=Json::array();
    }
    if(graphProbe){Json probe=Json::array();for(const auto& box:boxes)probe.push_back({{"id",box.id},{"a",{box.a.x,box.a.y}},{"b",{box.b.x,box.b.y}}});graphProbe(viewKey,probe,view.selection);}
    draw->AddText({start.x+cyui::dp(12),end.y-cyui::dp(24)},pal.textFaint,tr("MMB pan | Wheel zoom | Click pins to connect | RMB add"));
    draw->PopClipRect();ImGui::EndChild();ImGui::SameLine();
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
            ImGui::TextUnformatted(type.rfind("function.",0)==0||type=="macro.call"||(type=="object.call"||type=="interface.call")||type=="struct.make"?key.c_str():tr(key.c_str()));Json changed;
            if(editValue(graphId+"/"+view.selected+"/"+key,spec,v,changed))
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
