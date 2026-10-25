#include "Graph.h"
#include <algorithm>
#include <set>
namespace cygame {
namespace { std::map<std::string,NodeEvaluator> customEvaluators; }
Json& mutableNodeTypes() {
    static Json types = {
        {"event.begin",{{"title","Begin play"},{"domain","logic"},{"inputs",Json::object()},{"outputs",{{"then","exec"}}},{"defaults",Json::object()}}},
        {"debug.print",{{"title","Print string"},{"domain","logic"},{"inputs",{{"exec","exec"},{"text","string"}}},{"outputs",{{"then","exec"}}},{"defaults",{{"text","Hello from the graph"}}}}},
        {"flow.branch",{{"title","Branch"},{"domain","logic"},{"inputs",{{"exec","exec"},{"condition","bool"}}},{"outputs",{{"true","exec"},{"false","exec"}}},{"defaults",{{"condition",true}}}}},
        {"constant.float",{{"title","Float"},{"domain","both"},{"inputs",Json::object()},{"outputs",{{"value","float"}}},{"defaults",{{"value",0.5}}}}},
        {"constant.bool",{{"title","Boolean"},{"domain","logic"},{"inputs",Json::object()},{"outputs",{{"value","bool"}}},{"defaults",{{"value",true}}}}},
        {"constant.color",{{"title","Color"},{"domain","material"},{"inputs",Json::object()},{"outputs",{{"value","color"}}},{"defaults",{{"value",{0.2,0.45,0.8}}}}}},
        {"math.add",{{"title","Add"},{"domain","both"},{"inputs",{{"a","float"},{"b","float"}}},{"outputs",{{"value","float"}}},{"defaults",{{"a",0.0},{"b",1.0}}}}},
        {"math.multiply",{{"title","Multiply"},{"domain","both"},{"inputs",{{"a","float"},{"b","float"}}},{"outputs",{{"value","float"}}},{"defaults",{{"a",0.5},{"b",2.0}}}}},
        {"material.output",{{"title","Material output"},{"domain","material"},{"inputs",{{"baseColor","color"},{"roughness","float"},{"metallic","float"}}},{"outputs",Json::object()},{"defaults",{{"baseColor",{0.5,0.5,0.5}},{"roughness",0.5},{"metallic",0.0}}}}}
    };
    static const bool enriched=[&]{
        for(const auto* t:{"float","bool","string","color"}){
            const std::string type=t;Json value=type=="float"?Json(0.0):type=="bool"?Json(false):type=="string"?Json(""):Json::array({0.5,0.5,0.5});
            types["variable.get."+type]={{"title","Get "+type},{"domain","both"},{"variableType",type},{"inputs",Json::object()},{"outputs",{{"value",type}}},{"defaults",Json::object()}};
            types["variable.set."+type]={{"title","Set "+type},{"domain","logic"},{"variableType",type},{"inputs",{{"exec","exec"},{"value",type}}},{"outputs",{{"then","exec"},{"value",type}}},{"defaults",{{"value",value}}}};
        }
        types["constant.string"]={{"title","String"},{"domain","both"},{"inputs",Json::object()},{"outputs",{{"value","string"}}},{"defaults",{{"value","Hello"}}}};
        for(const auto* op:{"subtract","divide","less","greater","equal"}){
            std::string name=op;types["math."+name]={{"title",name},{"domain","both"},{"inputs",{{"a","float"},{"b","float"}}},{"outputs",{{"value",name=="less"||name=="greater"||name=="equal"?"bool":"float"}}},{"defaults",{{"a",0.0},{"b",1.0}}}};
        }
        types["convert.float_string"]={{"title","Float to string"},{"domain","logic"},{"inputs",{{"value","float"}}},{"outputs",{{"value","string"}}},{"defaults",{{"value",0.0}}}};
        types["flow.sequence"]={{"title","Sequence"},{"domain","logic"},{"inputs",{{"exec","exec"}}},{"outputs",{{"then0","exec"},{"then1","exec"},{"then2","exec"}}},{"defaults",Json::object()}};
        types["color.multiply"]={{"title","Multiply color"},{"domain","material"},{"inputs",{{"color","color"},{"factor","float"}}},{"outputs",{{"value","color"}}},{"defaults",{{"color",{0.5,0.5,0.5}},{"factor",1.0}}}};
        types["color.mix"]={{"title","Mix colors"},{"domain","material"},{"inputs",{{"a","color"},{"b","color"},{"factor","float"}}},{"outputs",{{"value","color"}}},{"defaults",{{"a",{0.1,0.2,0.5}},{"b",{0.8,0.3,0.1}},{"factor",0.5}}}};
        for(const auto* t:{"float","bool","string","color","exec"}){
            const std::string type=t;const bool exec=type=="exec";Json value=type=="float"?Json(0.):type=="bool"?Json(false):type=="string"?Json(""):Json::array({.5,.5,.5});
            types["route."+type]={{"title","Reroute"},{"domain",exec?"logic":"both"},{"inputs",{{exec?"exec":"value",type}}},{"outputs",{{exec?"then":"value",type}}},{"defaults",exec?Json::object():Json{{"value",value}}},{"x-editor-reroute",true}};
        }
        types["material.texture"]={{"title","Texture sample"},{"domain","material"},{"inputs",{{"path","string"},{"u","float"},{"v","float"}}},{"outputs",{{"color","color"},{"alpha","float"}}},{"defaults",{{"path","Examples/Example_Image.png"},{"u",.5},{"v",.5}}}};
        types["material.uv"]={{"title","Texture coordinates"},{"domain","material"},{"inputs",Json::object()},{"outputs",{{"u","float"},{"v","float"}}},{"defaults",Json::object()}};
        types["uv.transform"]={{"title","UV transform"},{"domain","material"},{"inputs",{{"u","float"},{"v","float"},{"tileU","float"},{"tileV","float"},{"offsetU","float"},{"offsetV","float"}}},{"outputs",{{"u","float"},{"v","float"}}},{"defaults",{{"u",.5},{"v",.5},{"tileU",1.},{"tileV",1.},{"offsetU",0.},{"offsetV",0.}}}};
        types["pattern.checker"]={{"title","Checker pattern"},{"domain","material"},{"inputs",{{"u","float"},{"v","float"},{"scale","float"},{"a","color"},{"b","color"}}},{"outputs",{{"value","color"}}},{"defaults",{{"u",.5},{"v",.5},{"scale",8.},{"a",{.05,.1,.2}},{"b",{.8,.5,.1}}}}};
        types["pattern.gradient"]={{"title","UV gradient"},{"domain","material"},{"inputs",{{"u","float"},{"a","color"},{"b","color"}}},{"outputs",{{"value","color"}}},{"defaults",{{"u",.5},{"a",{.05,.1,.2}},{"b",{.8,.5,.1}}}}};
        types["color.compose"]={{"title","Compose color"},{"domain","both"},{"inputs",{{"r","float"},{"g","float"},{"b","float"}}},{"outputs",{{"value","color"}}},{"defaults",{{"r",.5},{"g",.5},{"b",.5}}}};
        for(const auto* kind:{"sine","fract","clamp"}){const std::string op=kind;types["math."+op]={{"title",op},{"domain","both"},{"inputs",{{"value","float"}}},{"outputs",{{"value","float"}}},{"defaults",{{"value",.5}}}};}
        for(const auto* type:{"function.input","function.output","function.call"})types[type]={{"title",type},{"domain","both"},{"inputs",Json::object()},{"outputs",Json::object()},{"defaults",Json::object()}};
        extendBlueprintNodes(types);return true;
    }();(void)enriched;return types;
}
const Json& nodeTypes(){return mutableNodeTypes();}
void registerNodeType(const std::string& id,const Json& descriptor,NodeEvaluator evaluator) {
    if(id.empty()||mutableNodeTypes().contains(id)||!evaluator)throw std::runtime_error("Invalid or duplicate node registration");
    for(const auto& key:{"title","domain","inputs","outputs","defaults"})if(!descriptor.contains(key))throw std::runtime_error("Incomplete node descriptor");
    if(!descriptor["title"].is_string()||!descriptor["defaults"].is_object()||!descriptor["inputs"].is_object()||!descriptor["outputs"].is_object())throw std::runtime_error("Invalid node descriptor");
    const auto supported=blueprintTypes();const std::set<std::string> allowed(supported.begin(),supported.end());
    for(const auto& direction:{"inputs","outputs"})for(const auto& pin:descriptor[direction])
        if(!pin.is_string()||!allowed.count(pin.get<std::string>()))throw std::runtime_error("Custom node uses an unsupported value type");
    if(descriptor["domain"]!="logic"&&descriptor["domain"]!="material"&&descriptor["domain"]!="both")throw std::runtime_error("Unknown node domain");
    mutableNodeTypes()[id]=descriptor;customEvaluators.emplace(id,std::move(evaluator));
}
namespace {
Json node(const std::string& id,const std::string& type,float x,float y) {
    return {{"id",id},{"type",type},{"position",{x,y}},{"values",nodeTypes().at(type).at("defaults")}};
}
Json& findNode(Json& g,const std::string& id) {
    for (auto& n:g["nodes"]) if(n.at("id")==id) return n;
    throw std::runtime_error("Node not found: "+id);
}
const Json& findNode(const Json& g,const std::string& id) {
    for (const auto& n:g.at("nodes")) if(n.at("id")==id) return n;
    throw std::runtime_error("Node not found: "+id);
}
Json valueSchema(const std::string& type){return graphValueSchema(type);}
std::string pinType(const Json& g,const Json& n,const std::string& dir,const std::string& pin) {
    const auto spec=nodeDefinition(g,n);const auto& pins=spec.at(dir);
    if(!pins.contains(pin)) throw std::runtime_error("Pin not found: "+pin);
    return pins.at(pin).get<std::string>();
}
}
Json initialGraphs() {
    Json logic={{"domain","logic"},{"variables",Json::object()},{"nodes",Json::array({node("begin","event.begin",40,90),node("print","debug.print",360,90)})},{"links",Json::array({{{"id","start"},{"fromNode","begin"},{"fromPin","then"},{"toNode","print"},{"toPin","exec"}}})}};
    Json material={{"domain","material"},{"variables",Json::object()},{"nodes",Json::array({node("color","constant.color",40,90),node("surface","material.output",360,90)})},{"links",Json::array({{{"id","color_link"},{"fromNode","color"},{"fromPin","value"},{"toNode","surface"},{"toPin","baseColor"}}})}};
    return {{"logic",logic},{"material",material}};
}
Json validateGraphBody(const Json& g) {
    const auto domain=g.at("domain").get<std::string>();
    if(domain!="logic"&&domain!="material"&&domain!="function") throw std::runtime_error("Invalid graph domain");
    if(!g.at("nodes").is_array() || !g.at("links").is_array() || g["nodes"].size()>500 || g["links"].size()>2000) throw std::runtime_error("Graph size limit");
    const auto variables=g.value("variables",Json::object());
    if(!variables.is_object()||variables.size()>256)throw std::runtime_error("Invalid graph variable collection");
    for(const auto& [name,variable]:variables.items()){
        if(name.empty()||name.size()>64||(!std::isalpha(static_cast<unsigned char>(name[0]))&&name[0]!='_'))throw std::runtime_error("Invalid variable name");
        for(char c:name)if(!std::isalnum(static_cast<unsigned char>(c))&&c!='_')throw std::runtime_error("Invalid variable name");
        validateValue(variable.at("default"),graphValueSchema(variable.at("type"),g.value("types",Json::object())),name);
    }
    const auto bookmarks=g.value("bookmarks",Json::array());if(!bookmarks.is_array()||bookmarks.size()>64)throw std::runtime_error("At most 64 graph bookmarks are supported");std::set<std::string> bookmarkIds;
    for(const auto& m:bookmarks){const std::string id=m.at("id");if(id.empty()||!bookmarkIds.insert(id).second)throw std::runtime_error("Duplicate bookmark ID");validateValue(m.at("name"),{{"type","string"},{"minLength",1},{"maxLength",128}});validateValue(m.at("pan"),{{"type","array"},{"minItems",2},{"maxItems",2},{"items",{{"type","number"},{"minimum",-100000},{"maximum",100000}}}});validateValue(m.at("zoom"),{{"type","number"},{"minimum",.4},{"maximum",1.8}});}
    const auto comments=g.value("comments",Json::array());if(!comments.is_array()||comments.size()>128)throw std::runtime_error("Invalid comment groups");std::set<std::string> commentIds;
    for(const auto& c:comments){const std::string id=c.at("id");if(id.empty()||!commentIds.insert(id).second)throw std::runtime_error("Duplicate comment ID");validateValue(c.at("text"),{{"type","string"},{"maxLength",2048}});validateValue(c.at("position"),{{"type","array"},{"minItems",2},{"maxItems",2},{"items",{{"type","number"},{"minimum",-100000},{"maximum",100000}}}});validateValue(c.at("size"),{{"type","array"},{"minItems",2},{"maxItems",2},{"items",{{"type","number"},{"minimum",40},{"maximum",10000}}}});}
    std::set<std::string> ids,linkIds,inputs;
    for(const auto& n:g["nodes"]) {
        std::string id=n.at("id"),type=n.at("type");
        if(id.empty()||!ids.insert(id).second||!nodeTypes().contains(type)) throw std::runtime_error("Unknown/duplicate node");
        const auto d=nodeDefinition(g,n);if(type=="object.new")blueprintClass(g.value("classes",Json::object()),n.at("values").at("class"),g.value("types",Json::object()));
        if(d.contains("variableType")){
            const std::string name=n.value("variable","");
            if(!variables.contains(name)||variables[name]["type"]!=d["variableType"])throw std::runtime_error("Node variable binding is missing or has the wrong type");
        }
        if(n.contains("breakpoint")&&!n["breakpoint"].is_boolean())throw std::runtime_error("Invalid breakpoint");
        if(n.contains("comment"))validateValue(n["comment"],{{"type","string"},{"maxLength",2048}},"comment");
        if(domain=="function"&&g.value("mode","pure")=="pure"){
            if(type.rfind("variable.",0)==0||type=="typed.get"||type=="typed.set"||type=="material.output")throw std::runtime_error("Functions are pure; pass variables through typed inputs");
            for(const auto* direction:{"inputs","outputs"})for(const auto& pin:d[direction])if(pin=="exec")throw std::runtime_error("Execution nodes are unavailable in pure functions");
        }
        if(domain=="function"&&g.value("mode","pure")=="execution"&&(type=="event.begin"||type=="material.output"))throw std::runtime_error("Execution functions use their input and return boundaries");
        if(domain!="function" && d["domain"]!="both" && d["domain"]!=domain) throw std::runtime_error("Node unavailable in this domain");
        validateValue(n.at("position"),{{"type","array"},{"minItems",2},{"maxItems",2},{"items",{{"type","number"},{"minimum",-100000},{"maximum",100000}}}},"position");
        if(!n.at("values").is_object()||n["values"].size()!=d["defaults"].size()) throw std::runtime_error("Invalid node values");
        for(const auto& [key,v]:n["values"].items()) {
            if(!d["defaults"].contains(key)) throw std::runtime_error("Unknown node value");
            std::string valueType=d["inputs"].contains(key)?d["inputs"][key].get<std::string>():d["outputs"].at(key).get<std::string>();
            validateValue(v,graphValueSchema(valueType,g.value("types",Json::object())),key);
        }
    }
    std::map<std::string,std::vector<std::string>> edges;
    for(const auto& l:g["links"]) {
        std::string id=l.at("id"),from=l.at("fromNode"),to=l.at("toNode"),fp=l.at("fromPin"),tp=l.at("toPin");
        if(id.empty()||!linkIds.insert(id).second||!inputs.insert(to+"/"+tp).second) throw std::runtime_error("Duplicate link or occupied input");
        if(pinType(g,findNode(g,from),"outputs",fp)!=pinType(g,findNode(g,to),"inputs",tp)) throw std::runtime_error("Incompatible pin types");
        edges[from].push_back(to);
    }
    std::set<std::string> visiting,done;
    std::function<void(const std::string&)> visit=[&](const std::string& id) {
        if(visiting.count(id)) throw std::runtime_error("Cycles are not supported by the sandbox graph runtime");
        if(done.count(id)) return; visiting.insert(id);
        for(const auto& next:edges[id]) visit(next);
        visiting.erase(id);done.insert(id);
    };
    for(const auto& id:ids) visit(id);
    return {{"valid",true},{"nodeCount",g["nodes"].size()},{"linkCount",g["links"].size()}};
}
Json graphBodyCommand(Json& g,const std::string& op,const Json& p) {
    if(op=="graph_link_reroute"||op=="graph_comment_edit"||op.rfind("graph_bookmark_",0)==0)return graphNavigationCommand(g,op,p);
    if(op=="graph_nodes_move"||op=="graph_nodes_remove"||op=="graph_fragment_copy"||op=="graph_fragment_paste"||op.rfind("graph_comment_",0)==0)return graphBatchCommand(g,op,p);
    if(op.rfind("graph_variable_",0)==0){
        if(!g.contains("variables"))g["variables"]=Json::object();auto& variables=g["variables"];const std::string name=p.at("name");
        if(op=="graph_variable_add"){
            if(variables.contains(name))throw std::runtime_error("Variable already exists");
            variables[name]={{"type",p.at("type")},{"default",p.at("default")}};
        }else{
            if(!variables.contains(name))throw std::runtime_error("Variable not found");
            if(op=="graph_variable_default")variables[name]["default"]=p.at("value");
            else if(op=="graph_variable_remove"){
                for(const auto& n:g["nodes"])if(n.value("variable","")==name)throw std::runtime_error("Remove variable nodes before deleting their declaration");
                variables.erase(name);
            }else if(op=="graph_variable_rename"){
                std::string next=p.at("newName");if(variables.contains(next))throw std::runtime_error("Variable already exists");
                auto definition=variables.at(name);variables.erase(name);variables[next]=definition;
                for(auto& n:g["nodes"])if(n.value("variable","")==name)n["variable"]=next;
            }else throw std::runtime_error("Unknown variable operation");
        }validateGraph(g);return {{"variables",variables}};
    }
    if(op=="graph_breakpoint_set"||op=="graph_node_comment"){
        auto& n=findNode(g,p.at("nodeId"));
        if(op=="graph_breakpoint_set"){
            const auto spec=nodeDefinition(g,n);
            bool execution=false;for(const auto* side:{"inputs","outputs"})for(const auto& pin:spec[side])execution|=pin=="exec";
            if(!execution)throw std::runtime_error("Breakpoints require an execution node");
            n["breakpoint"]=p.at("enabled");
        }else n["comment"]=p.at("text");
        validateGraph(g);return {{"node",n}};
    }
    if(op=="graph_nodes_duplicate"){
        const auto ids=p.at("nodeIds").get<std::vector<std::string>>();std::map<std::string,std::string> remap;Json copies=Json::array(),links=Json::array();auto offset=p.value("offset",Json::array({40.,40.}));
        for(const auto& key:ids){if(remap.count(key))throw std::runtime_error("Duplicate node ID");auto n=findNode(g,key);const auto next=uniqueId();remap[key]=next;n["id"]=next;for(int i=0;i<2;++i)n["position"][i]=n["position"][i].get<double>()+offset[i].get<double>();copies.push_back(n);}
        for(const auto& original:g["links"])if(remap.count(original["fromNode"])&&remap.count(original["toNode"])){
            auto link=original;link["id"]=uniqueId();link["fromNode"]=remap[original["fromNode"]];link["toNode"]=remap[original["toNode"]];links.push_back(link);
        }
        for(const auto& n:copies)g["nodes"].push_back(n);for(const auto& link:links)g["links"].push_back(link);validateGraph(g);return {{"nodes",copies}};
    }
    if(op=="graph_get") return {{"graph",g},{"nodeTypes",nodeTypes()}};
    if(op=="graph_validate") return validateGraph(g);
    if(op=="graph_node_add") {
        std::string type=p.at("type");
        if(!nodeTypes().contains(type)) throw std::runtime_error("Unknown node type");
        auto n=node(uniqueId(),type,p.at("position")[0],p.at("position")[1]);
        if(type=="function.input"||type=="function.output")throw std::runtime_error("Function boundary nodes are created with the function");
        if(type=="function.call"||type=="macro.call"||type=="flow.foreach")n["function"]=p.at("functionRef");
        if(type=="object.new"){const auto classes=g.value("classes",Json::object());if(classes.empty())throw std::runtime_error("Define a class before constructing objects");n["values"]["class"]=p.value("classRef",classes.begin().key());}
        if(type=="object.call"){n["class"]=p.at("classRef");n["method"]=p.at("methodRef");n["function"]=blueprintClass(g.at("classes"),n.at("class"),g.value("types",Json::object()))["methods"].at(n.at("method").get<std::string>());}
        if(type=="interface.call"||type=="interface.supports"){n["interface"]=p.at("interfaceRef");if(type=="interface.call")n["method"]=p.at("methodRef");}
        if(p.contains("typeRef"))n["typeRef"]=p.at("typeRef");if(p.contains("fieldRef"))n["field"]=p.at("fieldRef");
        const auto spec=nodeDefinition(g,n);n["values"]=spec.at("defaults");if(type=="object.new"){const auto classes=g.at("classes");n["values"]["class"]=p.value("classRef",classes.begin().key());}
        if(spec.contains("variableType")){
            std::string name=p.value("variableName","");const auto variables=g.value("variables",Json::object());
            if(name.empty())for(const auto& [key,v]:variables.items())if(v.at("type")==spec["variableType"]){name=key;break;}
            if(!variables.contains(name)||variables[name]["type"]!=spec["variableType"])throw std::runtime_error("Choose a declared variable of the matching type");
            n["variable"]=name;if(n["values"].contains("value"))n["values"]["value"]=variables[name]["default"];
        }
        g["nodes"].push_back(n); validateGraph(g); return {{"node",n}};
    }
    if(op=="graph_node_move") { findNode(g,p.at("nodeId"))["position"]=p.at("position"); validateGraph(g);return {{"moved",true}}; }
    if(op=="graph_node_remove") {
        std::string id=p.at("nodeId"); findNode(g,id);auto& a=g["nodes"];
        a.erase(std::remove_if(a.begin(),a.end(),[&](const Json& n){return n["id"]==id;}),a.end());
        auto& links=g["links"];links.erase(std::remove_if(links.begin(),links.end(),[&](const Json& l){return l["fromNode"]==id||l["toNode"]==id;}),links.end());
        return {{"removed",id}};
    }
    if(op=="graph_node_value") {
        auto& n=findNode(g,p.at("nodeId")); std::string key=p.at("key");
        if(!n["values"].contains(key)) throw std::runtime_error("Unknown node value");
        n["values"][key]=p.at("value");validateGraph(g);return {{"node",n}};
    }
    if(op=="graph_link_add") {
        Json l={{"id",uniqueId()},{"fromNode",p.at("fromNode")},{"fromPin",p.at("fromPin")},{"toNode",p.at("toNode")},{"toPin",p.at("toPin")}};
        g["links"].push_back(l);validateGraph(g);return {{"link",l}};
    }
    if(op=="graph_link_remove") {
        std::string id=p.at("linkId");auto& a=g["links"];auto n=a.size();
        a.erase(std::remove_if(a.begin(),a.end(),[&](const Json& l){return l.at("id")==id;}),a.end());
        if(n==a.size()) throw std::runtime_error("Link not found");return {{"removed",id}};
    }
    throw std::runtime_error("Unknown graph command");
}
Json graphCommand(Json& graphs,const std::string& op,const Json& p){
    const std::string key=p.at("graphId");auto& owner=graphs.at(key);
    if(op=="graph_expand_plan"||op=="graph_expand_apply")return graphExpandCommand(owner,op,p);
    if(op=="graph_collapse_plan"||op=="graph_collapse_apply")return graphCollapseCommand(owner,op,p);
    if(op=="graph_signature_plan"||op=="graph_signature_apply")return graphSignatureCommand(owner,op,p);
    if(op=="graph_type_migration_plan"||op=="graph_type_migration_apply")return graphTypeMigrationCommand(owner,op,p);
    if(op=="graph_types_get"||op.rfind("graph_type_",0)==0)return graphTypeCommand(owner,op,p);
    if(op=="graph_interfaces_get"||op.rfind("graph_interface_",0)==0)return graphInterfaceCommand(owner,op,p);
    if(op.rfind("graph_class_",0)==0)return blueprintCommand(owner,op,p);
    if(op.rfind("graph_function_",0)==0)return functionLibraryCommand(owner,op,p);
    if(owner.at("domain")=="material-instance"&&op!="graph_get"&&op!="graph_validate")throw std::runtime_error("Edit material topology in the parent graph");
    const std::string function=p.value("functionName","");auto view=graphView(owner,function);
    auto result=graphBodyCommand(view,op,p);
    if(op=="graph_get"){
        Json definitions=Json::object();for(const auto& n:view["nodes"])definitions[n.at("id").get<std::string>()]=nodeDefinition(view,n);result["nodeDefinitions"]=definitions;
    }
    if(op!="graph_get"&&op!="graph_validate"&&op!="graph_fragment_copy"){
        if(function.empty())owner=view;else{if(view.contains("functions"))owner["functions"]=view.at("functions");if(view.contains("classes"))owner["classes"]=view.at("classes");view.erase("functions");view.erase("classes");if(view.contains("types"))owner["types"]=view.at("types");view.erase("types");if(view.contains("interfaces"))owner["interfaces"]=view.at("interfaces");view.erase("interfaces");owner["functions"][function]=view;}
        validateGraph(owner);
    }return result;
}
Json evaluateCustomNode(const std::string& type,const Json& values,const std::string& pin){
    auto found=customEvaluators.find(type);const auto result=found==customEvaluators.end()?evaluateBlueprintNode(type,values):found->second(values);
    if(!result.contains(pin))throw std::runtime_error("Custom node did not return the requested output");
    validateValue(result.at(pin),valueSchema(nodeTypes().at(type).at("outputs").at(pin)),pin);return result.at(pin);
}
}
