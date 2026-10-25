#include "Graph.h"
#include <algorithm>
#include <cctype>
#include <set>
namespace cygame {
namespace {
void identifier(const std::string& name){
    if(name.empty()||name.size()>64||(!std::isalpha(static_cast<unsigned char>(name[0]))&&name[0]!='_'))throw std::runtime_error("Use an identifier beginning with a letter or underscore");
    for(char c:name)if(!std::isalnum(static_cast<unsigned char>(c))&&c!='_')throw std::runtime_error("Invalid identifier");
}
Json valueSpec(const std::string& type,const Json& types){return graphValueSchema(type,types);}
void signature(const Json& f,const Json& types){
    const auto mode=f.value("mode","pure");if(mode!="pure"&&mode!="execution")throw std::runtime_error("Invalid function mode");
    for(const auto* side:{"inputs","outputs"}){
        const auto& pins=f.at(side);if(!pins.is_object()||pins.size()>16)throw std::runtime_error("Function signatures support at most 16 ports per direction");
        if(mode=="pure"&&std::string(side)=="outputs"&&pins.empty())throw std::runtime_error("A pure function needs at least one output");
        for(const auto& [name,pin]:pins.items()){identifier(name);if(mode=="execution"&&(name=="exec"||name=="then"))throw std::runtime_error("Execution functions reserve exec and then pin names");validateValue(pin.at("default"),valueSpec(pin.at("type"),types),name);}
    }
}
Json ports(const Json& definitions){Json types=Json::object();for(const auto& [name,p]:definitions.items())types[name]=p.at("type");return types;}
Json defaults(const Json& definitions){Json values=Json::object();for(const auto& [name,p]:definitions.items())values[name]=p.at("default");return values;}
void refreshValues(Json& g){
    for(auto& n:g["nodes"]){const auto spec=nodeDefinition(g,n);Json values=spec.at("defaults");for(auto& [key,v]:values.items())if(n["values"].contains(key)){
        try{auto type=spec["inputs"].contains(key)?spec["inputs"].at(key):spec["outputs"].at(key);validateValue(n["values"][key],valueSpec(type,g.value("types",Json::object())));v=n["values"][key];}catch(...){}
    }n["values"]=values;}
}
}
Json graphView(const Json& graph,const std::string& name){
    if(name.empty())return graph;
    auto view=graph.at("functions").at(name);view["functions"]=graph.at("functions");if(graph.contains("classes"))view["classes"]=graph.at("classes");if(graph.contains("types"))view["types"]=graph.at("types");if(graph.contains("interfaces"))view["interfaces"]=graph.at("interfaces");return view;
}
Json functionCallDefinition(const Json& function,const std::string& name,const Json& types){
    signature(function,types);const bool execution=function.value("mode","pure")=="execution";
    auto inputs=ports(function.at("inputs")),outputs=ports(function.at("outputs"));if(execution){inputs["exec"]="exec";outputs["then"]="exec";}
    return {{"title",name},{"domain",execution?"logic":"both"},{"inputs",inputs},{"outputs",outputs},{"defaults",defaults(function.at("inputs"))},{"function",name},{"execution",execution}};
}
Json nodeDefinition(const Json& graph,const Json& node){
    const std::string type=node.at("type");const auto types=graph.value("types",Json::object());const auto contract=interfaceNodeDefinition(graph,node);if(!contract.is_null())return contract;const auto typed=typedNodeDefinition(graph,node);if(!typed.is_null())return typed;
    if(type=="function.input"||type=="function.output"){
        if(graph.at("domain")!="function")throw std::runtime_error("Function boundaries belong inside a function");signature(graph,graph.value("types",Json::object()));
        const bool input=type=="function.input";
        auto inputs=input?Json::object():ports(graph.at("outputs")),outputs=input?ports(graph.at("inputs")):Json::object();
        if(graph.value("mode","pure")=="execution"){if(input)outputs["then"]="exec";else inputs["exec"]="exec";}
        return {{"title",input?"Function inputs":"Function outputs"},{"domain","both"},{"inputs",inputs},{"outputs",outputs},{"defaults",input?Json::object():defaults(graph.at("outputs"))}};
    }
    if(type=="flow.foreach"){
        const auto& function=graph.at("functions").at(node.at("function").get<std::string>());if(function.value("mode","pure")!="execution"||function.value("kind","")=="macro"||!function.at("inputs").contains("item")||!function["inputs"].contains("index")||function["inputs"]["index"]["type"]!="int")throw std::runtime_error("For each needs an execution function with a supported item type and index:int");if(function["outputs"].contains("break")&&function["outputs"]["break"]["type"]!="bool")throw std::runtime_error("Loop break output must be bool");auto result=nodeTypes().at(type);const std::string arrayType="array."+function["inputs"]["item"]["type"].get<std::string>();graphValueSchema(arrayType);result["inputs"]["values"]=arrayType;result["title"]="For each";for(const auto& [key,port]:function.at("inputs").items())if(key!="item"&&key!="index"){if(key=="values")throw std::runtime_error("Loop functions reserve values");result["inputs"][key]=port.at("type");result["defaults"][key]=port.at("default");}return result;
    }
    if(type=="object.call"){
        const std::string className=node.at("class"),method=node.at("method"),name=blueprintClass(graph.at("classes"),className,types)["methods"].at(method);if(node.at("function")!=name)throw std::runtime_error("Method binding changed; recreate its call node");return functionCallDefinition(graph.at("functions").at(name),className+"."+method,types);
    }
    if(type=="function.call"||type=="macro.call"){
        const std::string name=node.at("function");const auto& function=graph.at("functions").at(name);signature(function,types);if((type=="macro.call")!=(function.value("kind","")=="macro"))throw std::runtime_error("Use the matching function or macro call node");
        return functionCallDefinition(function,name,types);
    }return nodeTypes().at(type);
}
Json validateGraph(const Json& graph){
    if(graph.dump().size()>1024*1024)throw std::runtime_error("Graph asset exceeds the 1 MiB budget");validateBlueprint(graph);
    validateAssetReferences(graph);if(graph.at("domain")=="material-instance"){validateMaterialInstance(graph);return {{"valid",true},{"nodeCount",0},{"linkCount",0}};}
    const auto functions=graph.value("functions",Json::object());
    if(!functions.is_object()||functions.size()>32)throw std::runtime_error("At most 32 local functions are supported per asset");
    size_t count=graph.at("nodes").size();std::map<std::string,std::vector<std::string>> calls;
    for(const auto& [name,f]:functions.items()){
        identifier(name);signature(f,graph.value("types",Json::object()));if(graph.at("domain")=="material"&&f.value("mode","pure")=="execution")throw std::runtime_error("Execution functions belong to logic assets");if(f.at("domain")!="function"||(f.contains("functions")||f.contains("classes")||f.contains("types")||f.contains("interfaces")))throw std::runtime_error("Invalid embedded function format");
        auto view=graphView(graph,name);validateGraphBody(view);count+=f.at("nodes").size();
        int inputs=0,outputs=0;
        for(const auto& n:f.at("nodes")){inputs+=n.at("type")=="function.input";outputs+=n.at("type")=="function.output";if(n.at("type")=="function.call"||n.at("type")=="macro.call"||n.at("type")=="object.call"||n.at("type")=="flow.foreach")calls[name].push_back(n.at("function"));if(n.at("type")=="interface.call")for(const auto& target:interfaceTargets(graph,n))calls[name].push_back(target);}
        if(inputs!=1||outputs!=1)throw std::runtime_error("A function must keep exactly one input and output boundary");
    }
    if(count>2000)throw std::runtime_error("Graph and local functions exceed 2000 total nodes");
    std::set<std::string> pending;std::map<std::string,int> depths;
    std::function<int(const std::string&)> visit=[&](const std::string& name){
        if(pending.count(name))throw std::runtime_error("Recursive function calls are not supported");
        if(depths.count(name))return depths.at(name);pending.insert(name);int depth=1;
        for(const auto& next:calls[name])depth=std::max(depth,1+visit(next));pending.erase(name);
        if(depth>16)throw std::runtime_error("Function nesting exceeds 16 calls");return depths[name]=depth;
    };
    for(const auto& [name,f]:functions.items())visit(name);
    return validateGraphBody(graph);
}
Json functionLibraryCommand(Json& graph,const std::string& op,const Json& p){
    if(!graph.contains("functions"))graph["functions"]=Json::object();auto& library=graph["functions"];const std::string name=p.at("name");identifier(name);
    if(op=="graph_function_create"){
        if(library.contains(name))throw std::runtime_error("Function already exists");
        library[name]={{"domain","function"},{"variables",Json::object()},{"inputs",{{"x",{{"type","float"},{"default",0.}}}}},{"outputs",{{"result",{{"type","float"},{"default",0.}}}}},
            {"nodes",Json::array({{{"id","arguments"},{"type","function.input"},{"position",{40,90}},{"values",Json::object()}},{{"id","return"},{"type","function.output"},{"position",{370,90}},{"values",{{"result",0.}}}}})},
            {"links",Json::array({{{"id",uniqueId()},{"fromNode","arguments"},{"fromPin","x"},{"toNode","return"},{"toPin","result"}}})}};
        const auto kind=p.value("kind","function");if(kind=="macro"){if(graph.at("domain")!="logic")throw std::runtime_error("Macros currently belong to logic assets");library[name]["kind"]="macro";}else if(kind!="function")throw std::runtime_error("Invalid function kind");
        const auto mode=p.value("mode","pure");if(mode!="pure"){
            library[name]["mode"]=mode;library[name]["links"].push_back({{"id",uniqueId()},{"fromNode","arguments"},{"fromPin","then"},{"toNode","return"},{"toPin","exec"}});
        }
    }else{
        if(!library.contains(name))throw std::runtime_error("Function not found");
        if(op=="graph_function_signature"){
            library[name]["inputs"]=p.at("inputs");library[name]["outputs"]=p.at("outputs");signature(library[name],graph.value("types",Json::object()));
            refreshValues(graph);for(auto& [key,f]:library.items()){auto view=graphView(graph,key);refreshValues(view);view.erase("functions");view.erase("classes");view.erase("types");view.erase("interfaces");f=view;}
        }else if(op=="graph_function_rename"){
            const std::string next=p.at("newName");identifier(next);if(library.contains(next))throw std::runtime_error("Function already exists");
            auto function=library.at(name);library.erase(name);library[next]=function;
            auto update=[&](Json& nodes){for(auto& n:nodes)if(n.value("function","")==name)n["function"]=next;};
            update(graph["nodes"]);for(auto& f:library)update(f["nodes"]);if(graph.contains("classes"))for(auto& c:graph["classes"])if(c.contains("methods"))for(auto& method:c["methods"])if(method==name)method=next;
        }else if(op=="graph_function_remove"){
            auto used=[&](const Json& nodes){for(const auto& n:nodes)if(n.value("function","")==name)return true;return false;};
            if(used(graph["nodes"]))throw std::runtime_error("Remove function calls before deleting their definition");
            for(const auto& [key,f]:library.items())if(key!=name&&used(f.at("nodes")))throw std::runtime_error("Another function still calls this definition");library.erase(name);
        }else throw std::runtime_error("Unknown function operation");
    }
    validateGraph(graph);return {{"functions",library}};
}
}
