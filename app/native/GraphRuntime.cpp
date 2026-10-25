#include "Graph.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <set>
#include <sstream>
namespace cygame {
namespace {
const Json& getNode(const Json& g,const std::string& id){for(const auto& n:g.at("nodes"))if(n.at("id")==id)return n;throw std::runtime_error("Node not found: "+id);}
Json signature(Json g){
    g.erase("assetPath");g.erase("assetMissing");g.erase("references");g.erase("bookmarks");
    for(auto& n:g["nodes"]){n.erase("position");n.erase("breakpoint");n.erase("comment");}
    g.erase("comments");if(g.contains("functions"))for(auto& f:g["functions"])f=signature(f);return g;
}
}
namespace {
Json bodyDiagnostics(const Json& g){
    Json issues=Json::array();
    auto report=[&](const char* severity,const std::string& id,const std::string& message){issues.push_back({{"severity",severity},{"nodeId",id},{"message",message}});};
    try{validateGraph(g);}catch(const std::exception& e){report("Error","",e.what());return {{"valid",false},{"diagnostics",issues}};}
    int entries=0,outputs=0;std::set<std::string> connected,used;
    for(const auto& link:g["links"]){connected.insert(link.at("fromNode"));connected.insert(link.at("toNode"));}
    for(const auto& n:g["nodes"]){
        if(n["type"]=="event.begin"||n["type"]=="event.custom")++entries;
        if(n["type"]=="material.output")++outputs;
        if(n.contains("variable"))used.insert(n.at("variable"));
        if(!connected.count(n.at("id"))&&n["type"]!="event.begin"&&n["type"]!="material.output")report("Warning",n.at("id"),"Node is disconnected");
    }
    if(g["domain"]=="logic"&&entries==0)report("Error","","Logic graph requires a Begin play event");
    if(g["domain"]=="material"&&outputs!=1)report("Error","","Material graph requires exactly one output node");
    if(g.at("domain")=="function"&&g.value("mode","pure")=="execution"){
        for(const auto& n:g.at("nodes"))if(n.at("type")=="function.output"){
            bool wired=false;for(const auto& l:g.at("links"))wired|=l.at("toNode")==n.at("id")&&l.at("toPin")=="exec";
            if(!wired)report("Error",n.at("id"),"Execution function requires a connected return");
        }
    }
    const auto declarations=g.value("variables",Json::object());
    for(const auto& [name,v]:declarations.items())if(!used.count(name))report("Info","","Unused variable: "+name);
    bool valid=true;for(const auto& issue:issues)if(issue["severity"]=="Error")valid=false;
    return {{"valid",valid},{"diagnostics",issues}};
}
}
Json graphDiagnostics(const Json& g){
    auto result=bodyDiagnostics(g);if(g.at("domain")=="function"||!result.at("valid").get<bool>())return result;
    const auto functions=g.value("functions",Json::object());for(const auto& [name,f]:functions.items()){
        auto report=bodyDiagnostics(graphView(g,name));for(auto issue:report.at("diagnostics")){issue["functionName"]=name;result["diagnostics"].push_back(issue);}if(!report.at("valid").get<bool>())result["valid"]=false;
    }return result;
}
void GraphExecution::initialize(){
    if(graph_.contains("interfaces"))run_->interfaces=graph_.at("interfaces");else if(!run_->interfaces.empty())graph_["interfaces"]=run_->interfaces;
    if(graph_.contains("types"))run_->types=graph_.at("types");else if(!run_->types.empty())graph_["types"]=run_->types;
    if(graph_.contains("classes"))run_->classes=graph_.at("classes");
    const auto declarations=graph_.value("variables",Json::object());for(const auto& [name,v]:declarations.items())variables_[name]=v.at("default");
    if(graph_.at("domain")=="logic"){
        for(const auto& n:graph_["nodes"])if(n["type"]=="event.begin")pending_.push_back(n.at("id"));
        if(pending_.empty()){bool custom=false;for(const auto& n:graph_["nodes"])custom|=n.at("type")=="event.custom";if(!custom)throw std::runtime_error("Logic graph requires an event entry");}
    }else if(graph_.at("domain")=="function"&&graph_.value("mode","pure")=="execution"){
        for(const auto& n:graph_["nodes"])if(n["type"]=="function.input")pending_.push_back(n.at("id"));
    }
}
GraphExecution::GraphExecution(Json graph,bool breakpoints):graph_(std::move(graph)),breakpoints_(breakpoints){
    validateGraph(graph_);sourceSignature_=signature(graph_);graph_=expandGraphMacros(graph_);validateGraph(graph_);library_=graph_.contains("functions")?&graph_.at("functions"):nullptr;initialize();
}
GraphExecution::GraphExecution(Json graph,Json arguments,std::shared_ptr<int> budget,int depth,const Json* library,
    std::shared_ptr<RunState> run,std::string name,std::string caller):graph_(std::move(graph)),functionName_(std::move(name)),callerNode_(std::move(caller)),depth_(depth),budget_(std::move(budget)),arguments_(std::move(arguments)),library_(library){
    if(run)run_=std::move(run);initialize();
}
std::string GraphExecution::callTarget(const Json& node){
    if(node.at("type")=="interface.call"){
        const std::string handle=input(node,"self");if(!run_->objects.contains(handle))throw std::runtime_error("Interface receiver is not a valid object");
        const auto definition=blueprintClass(run_->classes,run_->objects.at(handle).at("class"),run_->types);const std::string expected=node.at("interface");
        if(std::find(definition.at("interfaces").begin(),definition.at("interfaces").end(),Json(expected))==definition.at("interfaces").end())throw std::runtime_error("Object does not implement interface: "+expected);
        return definition.at("methods").at(node.at("method").get<std::string>());
    }
    if(node.at("type")!="object.call")return node.at("function");const std::string handle=input(node,"self");const auto& instance=run_->objects.at(handle);const std::string actual=instance.at("class"),expected=node.at("class");std::string current=actual;bool compatible=false;for(int i=0;i<16&&!current.empty();++i){if(current==expected){compatible=true;break;}current=run_->classes.at(current).value("parent","");}if(!compatible)throw std::runtime_error("Method receiver class mismatch");return blueprintClass(run_->classes,actual,run_->types)["methods"].at(node.at("method").get<std::string>());
}
Json GraphExecution::editObject(const std::string& object,const std::string& field,const Json& value){auto objects=run_->objects;auto& instance=objects.at(object);const auto spec=blueprintClass(run_->classes,instance.at("class"),run_->types).at("fields").at(field);validateValue(value,graphValueSchema(spec.at("type"),run_->types),field);instance["fields"][field]=value;if(objects.dump().size()>256*1024)throw std::runtime_error("Runtime object storage exceeds 256 KiB");run_->objects=objects;return status();}
Json GraphExecution::advanceTime(double delta){if(!std::isfinite(delta)||delta<0||delta>60||run_->time+delta>1e6)throw std::runtime_error("Simulation time step must be 0..60 seconds");run_->time+=delta;return state_=="Waiting"?advance(std::string("continue")):status();}
Json GraphExecution::emit(const std::string& event){if(state_=="Failed")throw std::runtime_error("Restart a failed graph before emitting events");std::vector<std::string> targets;for(const auto& node:graph_.at("nodes"))if(node.at("type")=="event.custom"&&node.at("values").at("name")==event)targets.push_back(node.at("id"));if(targets.empty())throw std::runtime_error("Custom event is not declared: "+event);if(pending_.size()+targets.size()>4096)throw std::runtime_error("Graph event queue exceeds 4096 entries");for(const auto& target:targets)pending_.push_back(target);if(state_=="Completed")state_="Paused";return status();}

Json GraphExecution::nodeSpec(const Json& n)const{
    const std::string type=n.at("type");
    // Child bodies deliberately do not serialize/copy their owner's libraries.
    if(type=="interface.call"||type=="interface.supports")return interfaceNodeDefinition(graph_,n);
    if(type=="function.call"||type=="object.call"||type=="macro.call"){if(!library_)throw std::runtime_error("Function library missing");const std::string name=n.at("function");return functionCallDefinition(library_->at(name),name,run_->types);}
    if(type=="flow.foreach")return nodeTypes().at(type); // Its runtime outputs are always count:int.
    return nodeDefinition(graph_,n);
}
Json GraphExecution::input(const Json& n,const std::string& pin){
    if(--*budget_<0)throw std::runtime_error("Graph evaluation budget exceeded");
    for(const auto& link:graph_["links"])if(link["toNode"]==n["id"]&&link["toPin"]==pin)return output(getNode(graph_,link["fromNode"]),link["fromPin"]);
    return n.at("values").at(pin);
}
Json GraphExecution::output(const Json& n,const std::string& pin){
    if(--*budget_<0)throw std::runtime_error("Graph evaluation budget exceeded");
    const std::string type=n.at("type");Json value;
    if(type=="flow.foreach"||type=="object.new"||type.rfind("object.set.",0)==0){if(!callValues_.contains(n.at("id").get<std::string>()))throw std::runtime_error("Read object output before execution");value=callValues_.at(n.at("id").get<std::string>()).at(pin);}
    else if(type=="interface.supports"){const std::string handle=input(n,"self");value=false;if(run_->objects.contains(handle)){const auto definition=blueprintClass(run_->classes,run_->objects.at(handle).at("class"),run_->types);const auto& implemented=definition.at("interfaces");value=std::find(implemented.begin(),implemented.end(),n.at("interface"))!=implemented.end();}}
    else if(type=="object.valid")value=run_->objects.contains(input(n,"object").get<std::string>());
    else if(type.rfind("object.get.",0)==0){const std::string handle=input(n,"object"),field=input(n,"field"),expected=type=="object.get.typed"||type=="object.set.typed"?n.at("typeRef").get<std::string>():type.substr(11);const auto& instance=run_->objects.at(handle);const auto schema=blueprintClass(run_->classes,instance.at("class"),run_->types)["fields"].at(field);if(schema.at("type")!=expected)throw std::runtime_error("Object field type mismatch");value=instance.at("fields").at(field);}
    else if(type.rfind("constant.",0)==0)value=n.at("values").at("value");
    else if(type.rfind("route.",0)==0)value=input(n,"value");
    else if(type=="function.input")value=arguments_.at(pin);
    else if(type=="function.call"||type=="object.call"||type=="interface.call"){
        if(depth_>=16)throw std::runtime_error("Function nesting exceeds 16 calls");
        if(!library_)throw std::runtime_error("Function library missing");const std::string name=callTarget(n);const auto& function=library_->at(name);
        if(function.value("mode","pure")=="execution"){
            if(!callValues_.contains(n.at("id").get<std::string>()))throw std::runtime_error("Read function output before its execution: "+n.at("id").get<std::string>());
            value=callValues_.at(n.at("id").get<std::string>()).at(pin);
        }else{
            Json args=Json::object();for(const auto& [key,t]:function.at("inputs").items())args[key]=input(n,key);
            GraphExecution child(function,args,budget_,depth_+1,library_,run_,name);child.sampleU_=sampleU_;child.sampleV_=sampleV_;child.textureSampler_=textureSampler_;
            for(const auto& result:child.graph_.at("nodes"))if(result.at("type")=="function.output")value=child.input(result,pin);
        }
    }
    else if(type.rfind("variable.",0)==0||type=="typed.get"||type=="typed.set")value=variables_.at(n.at("variable").get<std::string>());
    else if(type=="material.texture"){if(!textureSampler_)throw std::runtime_error("Texture sampling requires a content-root context");const auto rgba=textureSampler_(input(n,"path"),input(n,"u"),input(n,"v"));value=pin=="alpha"?rgba[3]:Json::array({rgba[0],rgba[1],rgba[2]});}
    else if(type=="material.uv")value=pin=="u"?sampleU_:sampleV_;
    else if(type=="uv.transform"){const bool u=pin=="u";value=input(n,u?"u":"v").get<double>()*input(n,u?"tileU":"tileV").get<double>()+input(n,u?"offsetU":"offsetV").get<double>();}
    else if(type=="math.sine"||type=="math.fract"||type=="math.clamp"){const double x=input(n,"value");value=type=="math.sine"?std::sin(x):type=="math.fract"?x-std::floor(x):std::clamp(x,0.,1.);}
    else if(type=="color.compose")value=Json::array({std::clamp(input(n,"r").get<double>(),0.,1.),std::clamp(input(n,"g").get<double>(),0.,1.),std::clamp(input(n,"b").get<double>(),0.,1.)});
    else if(type=="pattern.checker"||type=="pattern.gradient"){
        auto a=input(n,"a"),b=input(n,"b");double factor=0;
        if(type=="pattern.checker"){const double scale=input(n,"scale");factor=std::abs(std::fmod(std::floor(input(n,"u").get<double>()*scale)+std::floor(input(n,"v").get<double>()*scale),2.))>.5?1.:0.;}
        else factor=std::clamp(input(n,"u").get<double>(),0.,1.);
        value=Json::array();for(int i=0;i<3;++i)value.push_back(a[i].get<double>()*(1-factor)+b[i].get<double>()*factor);
    }
    else if(type.rfind("math.",0)==0){
        const auto a=input(n,"a").get<double>(),b=input(n,"b").get<double>();
        if(type=="math.add")value=a+b;else if(type=="math.subtract")value=a-b;else if(type=="math.multiply")value=a*b;
        else if(type=="math.divide"){if(std::abs(b)<1e-12)throw std::runtime_error("Division by zero at node "+n.at("id").get<std::string>());value=a/b;}
        else if(type=="math.less")value=a<b;else if(type=="math.greater")value=a>b;else if(type=="math.equal")value=a==b;
        else throw std::runtime_error("Unsupported math operation");
    }else if(type=="convert.float_string"){std::ostringstream text;text<<std::setprecision(6)<<input(n,"value").get<double>();value=text.str();}
    else if(type=="color.multiply"||type=="color.mix"){
        auto a=input(n,type=="color.mix"?"a":"color");double factor=input(n,"factor");Json b=type=="color.mix"?input(n,"b"):Json::array({0,0,0});value=Json::array();
        if(type=="color.mix")factor=std::clamp(factor,0.,1.);
        for(int i=0;i<3;++i)value.push_back(std::clamp(type=="color.mix"?a[i].get<double>()*(1-factor)+b[i].get<double>()*factor:a[i].get<double>()*factor,0.,1.));
    }else if(!typedNodeDefinition(graph_,n).is_null()){Json resolved=n.at("values");const auto spec=nodeSpec(n);for(const auto& [key,t]:spec.at("inputs").items())if(t!="exec")resolved[key]=input(n,key);value=evaluateTypedNode(n,resolved).at(pin);
    }else{
        Json resolved=n.at("values");
        const auto spec=nodeSpec(n);for(const auto& [key,t]:spec.at("inputs").items())if(t!="exec")resolved[key]=input(n,key);
        value=evaluateCustomNode(type,resolved,pin);
    }
    const auto valueType=nodeSpec(n).at("outputs").at(pin).get<std::string>();if(isNamedGraphType(valueType))validateValue(value,graphValueSchema(valueType,run_->types),pin);
    if(value.is_number()&&!std::isfinite(value.get<double>()))throw std::runtime_error("Non-finite graph result");
    watches_[n.at("id").get<std::string>()+"."+pin]=value;return value;
}
void GraphExecution::executeOne(){
    if(pending_.empty())return;if(++run_->sliceSteps>4096||++run_->steps>100000000)throw std::runtime_error("Graph execution step limit exceeded");
    const std::string id=pending_.front();pending_.pop_front();const auto& n=getNode(graph_,id);const std::string type=n.at("type");
    const std::string sourceId=n.value("macroCall",id);
    run_->trace.push_back({{"runtimeNodeId",id},{"nodeId",sourceId},{"type",type},{"step",run_->steps},{"functionName",functionName_},{"depth",depth_}});if(run_->trace.size()>256)run_->trace.erase(run_->trace.begin());
    std::vector<std::string> pins={"then"};
    if(type=="debug.print"){run_->messages.push_back(input(n,"text"));if(run_->messages.size()>128)run_->messages.erase(run_->messages.begin());}
    else if(type=="flow.branch")pins={input(n,"condition").get<bool>()?"true":"false"};
    else if(type=="enum.switch")pins={input(n,"value").get<std::string>()};
    else if(type=="flow.sequence")pins={"then0","then1","then2"};
    else if(type.rfind("variable.set.",0)==0||type=="typed.set"){auto value=input(n,"value");variables_[n.at("variable").get<std::string>()]=value;watches_[id+".value"]=value;}
    else if(type=="flow.foreach"){
        if(depth_>=16)throw std::runtime_error("Function nesting exceeds 16 calls");const auto items=input(n,"values");const std::string name=n.at("function");const auto& function=library_->at(name);callValues_[id]={{"count",0}};if(!items.empty()){Json args=Json::object();for(const auto& [key,port]:function.at("inputs").items())args[key]=key=="item"?items[0]:key=="index"?Json(0):input(n,key);loops_[id]={{"items",items},{"next",1},{"name",name},{"args",args}};child_.reset(new GraphExecution(function,args,budget_,depth_+1,library_,run_,name,id));}
    }
    else if(type=="flow.delay"){double seconds=input(n,"seconds");if(seconds<0||seconds>3600)throw std::runtime_error("Delay must be 0..3600 seconds");waitUntil_=run_->time+seconds;}
    else if(type=="flow.emit")run_->events.push_back(input(n,"name"));
    else if(type=="object.new"){
        const std::string name=input(n,"class");auto definition=blueprintClass(run_->classes,name,run_->types);if(run_->objects.size()>=256)throw std::runtime_error("Runtime object limit is 256");const std::string handle="object:"+uniqueId();Json fields=Json::object();for(const auto& [key,spec]:definition["fields"].items())fields[key]=spec.at("default");run_->objects[handle]={{"class",name},{"fields",fields}};callValues_[id]={{"object",handle}};
    }else if(type=="object.destroy"){const std::string handle=input(n,"object");if(!run_->objects.erase(handle))throw std::runtime_error("Object reference is invalid");}
    else if(type.rfind("object.set.",0)==0){const std::string handle=input(n,"object"),field=input(n,"field"),expected=type=="object.get.typed"||type=="object.set.typed"?n.at("typeRef").get<std::string>():type.substr(11);auto& instance=run_->objects.at(handle);const auto spec=blueprintClass(run_->classes,instance.at("class"),run_->types)["fields"].at(field);if(spec.at("type")!=expected)throw std::runtime_error("Object field type mismatch");auto value=input(n,"value");validateValue(value,graphValueSchema(expected,run_->types),field);instance["fields"][field]=value;callValues_[id]={{"value",value}};}
    else if(type=="function.call"||type=="object.call"||type=="interface.call"){
        if(depth_>=16)throw std::runtime_error("Function nesting exceeds 16 calls");
        const std::string name=callTarget(n);const auto& function=library_->at(name);
        if(function.value("mode","pure")!="execution")throw std::runtime_error("Pure function has no execution input");
        Json args=Json::object();for(const auto& [key,t]:function.at("inputs").items())args[key]=input(n,key);
        callValues_.erase(id);child_.reset(new GraphExecution(function,args,budget_,depth_+1,library_,run_,name,id));
    }else if(type=="function.output"){
        for(const auto& [key,t]:graph_.at("outputs").items())returns_[key]=input(n,key);
        returned_=true;pending_.clear();return;
    }else if(type!="event.begin"&&type!="event.custom"&&type!="route.exec"&&type!="function.input")throw std::runtime_error("Unsupported execution node: "+type);
    if(run_->objects.dump().size()>256*1024)throw std::runtime_error("Runtime object storage exceeds 256 KiB");
    std::vector<std::string> targets;for(const auto& pin:pins)for(const auto& l:graph_["links"])if(l["fromNode"]==id&&l["fromPin"]==pin)targets.push_back(l.at("toNode"));
    for(auto it=targets.rbegin();it!=targets.rend();++it)pending_.push_front(*it);
}
void GraphExecution::tick(){
    if(child_){child_->tick();if(child_->finished()){
        if(!child_->returned_)throw std::runtime_error("Execution function reached the end without returning: "+child_->functionName_);
        const std::string caller=child_->callerNode_;if(loops_.contains(caller)){
            auto& loop=loops_[caller];int next=loop.at("next");callValues_[caller]={{"count",next}};const bool stop=child_->returns_.value("break",false);child_.reset();if(!stop&&next<int(loop["items"].size())){Json args=loop.at("args");args["index"]=next;args["item"]=loop["items"][next];loop["next"]=next+1;const std::string name=loop.at("name");child_.reset(new GraphExecution(library_->at(name),args,budget_,depth_+1,library_,run_,name,caller));}else loops_.erase(caller);
        }else{callValues_[caller]=child_->returns_;child_.reset();}
    }}else executeOne();
    if(depth_==0)while(!run_->events.empty()){auto event=run_->events.front();run_->events.pop_front();emit(event);}
}
GraphExecution& GraphExecution::active(){return child_?child_->active():*this;}
const GraphExecution& GraphExecution::active()const{return child_?child_->active():*this;}
void GraphExecution::stack(Json& frames)const{
    std::string next=pending_.empty()?"":pending_.front();if(!next.empty())next=getNode(graph_,next).value("macroCall",next);
    frames.push_back({{"depth",depth_},{"functionName",functionName_},{"callerNode",callerNode_},{"suspendedAt",child_?child_->callerNode_:""},{"nextNode",next},{"arguments",arguments_},{"variables",variables_},{"watches",watches_}});
    if(child_)child_->stack(frames);
}
Json GraphExecution::status()const{
    const auto& frame=active();Json frames=Json::array();stack(frames);
    std::string next=frame.pending_.empty()?"":frame.pending_.front();if(!next.empty())next=getNode(frame.graph_,next).value("macroCall",next);
    return {{"time",run_->time},{"wakeTime",frame.waitUntil_},{"objects",run_->objects},{"state",state_},{"nextNode",next},{"activeFunction",frame.functionName_},{"variables",frame.variables_},{"rootVariables",variables_},
        {"messages",run_->messages},{"trace",run_->trace},{"watches",frame.watches_},{"steps",run_->steps},{"error",error_},{"callStack",frames}};
}
Json GraphExecution::advance(bool resume){return advance(std::string(resume?"continue":"over"));}
Json GraphExecution::advance(const std::string& mode){
    if(state_=="Completed"||state_=="Failed")return status();
    run_->sliceSteps=0;*budget_=10000;
    const int initialDepth=active().depth_;const bool resume=mode=="continue";
    const std::string skip=state_=="Breakpoint"?breakpoint_:"";bool first=true;state_="Running";
    try{
        while(!finished()){
            const auto& frame=active();if(frame.waitUntil_>run_->time){state_="Waiting";return status();}if(frame.pending_.empty())throw std::runtime_error("Execution function has no pending return");
            const std::string next=frame.pending_.front(),marker=frame.functionName_+"/"+next;
            if(breakpoints_&&getNode(frame.graph_,next).value("breakpoint",false)&&!(first&&(!resume||marker==skip))){state_="Breakpoint";breakpoint_=marker;return status();}
            tick();first=false;
            if(!resume&&(mode=="into"||(mode=="over"&&active().depth_<=initialDepth)||(mode=="out"&&active().depth_<initialDepth)))break;
        }
        state_=finished()?"Completed":"Paused";
    }catch(const std::exception& e){state_="Failed";error_=e.what();}
    return status();
}
bool GraphExecution::matchesProgram(const Json& graph)const{return signature(graph)==sourceSignature_;}
void GraphExecution::refreshBreakpoints(const Json& graph){
    auto update=[](Json& target,const Json& source){for(auto& n:target["nodes"]){if(n.contains("macroCall"))n["breakpoint"]=n.value("macroEntry",false)&&getNode(source,n.at("macroCall")).value("breakpoint",false);else n["breakpoint"]=getNode(source,n.at("id")).value("breakpoint",false);}};
    update(graph_,graph);if(graph_.contains("functions"))for(auto& [name,f]:graph_["functions"].items())update(f,graph.at("functions").at(name));
    for(auto* frame=child_.get();frame;frame=frame->child_.get())update(frame->graph_,graph.at("functions").at(frame->functionName_));
}
Json GraphExecution::result(){
    if(graph_["domain"]=="material"){
        const auto diagnostics=graphDiagnostics(graph_);if(!diagnostics["valid"].get<bool>())throw std::runtime_error("Material graph requires exactly one output node");
        for(const auto& n:graph_["nodes"])if(n["type"]=="material.output"){
            auto color=input(n,"baseColor");double roughness=input(n,"roughness"),metallic=input(n,"metallic");
            return {{"baseColor",color},{"roughness",std::clamp(roughness,0.,1.)},{"metallic",std::clamp(metallic,0.,1.)}};
        }
    }
    advance(true);if(state_=="Waiting")throw std::runtime_error("Graph is waiting; use the debugger and graph_debug_tick to advance simulation time");if(state_=="Failed")throw std::runtime_error(error_);
    return {{"messages",run_->messages},{"executed",true},{"trace",run_->trace},{"variables",variables_}};
}
Json evaluateFunction(const Json& graph,const std::string& name,const Json& arguments,TextureSampler sampler){
    validateGraph(graph);auto compiled=expandGraphMacros(graph);auto view=graphView(compiled,name);Json args=Json::object();for(const auto& [key,port]:view.at("inputs").items())args[key]=port.at("default");
    for(const auto& [key,v]:arguments.items()){if(!args.contains(key))throw std::runtime_error("Unknown function argument: "+key);args[key]=v;}
    Json probe=view;for(auto& n:probe["nodes"])if(n["type"]=="function.output"){
        // Reuse native pin validation by assigning arguments to a call in a tiny owner.
        Json owner=graph;Json call={{"id","argumentProbe"},{"type",view.value("kind","")=="macro"?"macro.call":"function.call"},{"function",name},{"position",{0,0}},{"values",args}};
        owner["nodes"]=Json::array({call});owner["links"]=Json::array();validateGraph(owner);
    }
    GraphExecution execution(view,args,std::make_shared<int>(10000),1,&view.at("functions"),nullptr,name);execution.textureSampler_=std::move(sampler);Json values=Json::object();
    if(view.value("mode","pure")=="execution"){
        execution.breakpoints_=false;execution.advance(true);if(execution.state_=="Failed")throw std::runtime_error(execution.error_);
        if(!execution.returned_)throw std::runtime_error("Execution function reached the end without returning: "+name);
        return {{"values",execution.returns_},{"messages",execution.run_->messages},{"trace",execution.run_->trace},{"variables",execution.variables_}};
    }
    for(const auto& n:view.at("nodes"))if(n.at("type")=="function.output")for(const auto& [key,port]:view.at("outputs").items())values[key]=execution.input(n,key);
    return {{"values",values}};
}
Json sampleMaterial(const Json& graph,double u,double v,TextureSampler sampler){if(graph.at("domain")!="material")throw std::runtime_error("Sampling requires a material");GraphExecution execution(graph,false);execution.sampleU_=u;execution.sampleV_=v;execution.textureSampler_=std::move(sampler);return execution.result();}
Json evaluateGraph(const Json& graph){return GraphExecution(graph,false).result();}
}
