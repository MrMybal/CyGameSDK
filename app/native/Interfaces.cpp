#include "Graph.h"
#include <algorithm>
#include <set>
namespace cygame {
namespace {
void interfaceIdentifier(const std::string& name){
    auto letter=[](char c){return c>='a'&&c<='z'||c>='A'&&c<='Z'||c=='_';};
    if(name.empty()||name.size()>64||!letter(name[0]))throw std::runtime_error("Interface names use identifiers of at most 64 bytes");
    for(char c:name)if(!letter(c)&&!(c>='0'&&c<='9'))throw std::runtime_error("Invalid interface identifier");
}
Json methodSignature(const Json& method){
    if(!method.is_object()||method.contains("kind")||method.contains("nodes")||method.contains("links"))throw std::runtime_error("Interface methods declare signatures, not bodies");
    auto result=method;auto& inputs=result.at("inputs");if(!inputs.is_object()||inputs.size()>15||inputs.contains("self"))throw std::runtime_error("Interface methods reserve self and allow 15 arguments");
    if(result.at("outputs").contains("self"))throw std::runtime_error("Interface methods reserve self");
    inputs["self"]={{"type","object"},{"default",""}};return result;
}
bool containsInterface(const Json& definition,const std::string& name){const auto implemented=definition.value("interfaces",Json::array());return std::find(implemented.begin(),implemented.end(),Json(name))!=implemented.end();}
void checkImplementation(const Json& graph,const Json& method,const std::string& function){
    const auto& target=graph.at("functions").at(function);const auto signature=methodSignature(method);
    if(target.value("kind","function")=="macro"||target.value("mode","pure")!=signature.value("mode","pure"))throw std::runtime_error("Interface implementation mode mismatch");
    for(const auto* side:{"inputs","outputs"}){
        if(target.at(side).size()!=signature.at(side).size())throw std::runtime_error("Interface implementation signature mismatch");
        for(const auto& [name,port]:signature.at(side).items())if(!target.at(side).contains(name)||target.at(side).at(name).at("type")!=port.at("type"))throw std::runtime_error("Interface implementation pin type mismatch: "+name);
    }
}
}
void validateGraphInterfaces(const Json& graph){
    const auto interfaces=graph.value("interfaces",Json::object()),types=graph.value("types",Json::object());
    if(!interfaces.is_object()||interfaces.size()>32)throw std::runtime_error("Graph interface limit is 32");
    if(graph.at("domain")!="logic"&&graph.at("domain")!="function"&&!interfaces.empty())throw std::runtime_error("Interfaces belong to logic assets");
    size_t expandedBytes=0;
    for(const auto& [name,definition]:interfaces.items()){
        interfaceIdentifier(name);const auto& methods=definition.at("methods");
        if(!methods.is_object()||methods.empty()||methods.size()>16)throw std::runtime_error("An interface declares one to 16 methods");
        for(const auto& [key,method]:methods.items()){
            interfaceIdentifier(key);const auto signature=methodSignature(method);functionCallDefinition(signature,name+"."+key,types);
            for(const auto* side:{"inputs","outputs"})for(const auto& port:signature.at(side))expandedBytes+=graphValueSchema(port.at("type"),types).dump().size();
            if(expandedBytes>2*1024*1024)throw std::runtime_error("Expanded interface schemas exceed 2 MiB");
        }
    }
    const auto classes=graph.value("classes",Json::object());
    for(const auto& [name,c]:classes.items()){
        const auto resolved=blueprintClass(classes,name,types);
        for(const auto& id:resolved.at("interfaces")){
            const std::string contract=id;if(!interfaces.contains(contract))throw std::runtime_error("Implemented interface is missing: "+contract);
            for(const auto& [key,method]:interfaces.at(contract).at("methods").items()){
                if(!resolved.at("methods").contains(key))throw std::runtime_error("Class is missing interface method: "+key);
                checkImplementation(graph,method,resolved.at("methods").at(key));
            }
        }
    }
}
Json interfaceNodeDefinition(const Json& graph,const Json& node){
    const std::string type=node.at("type");if(type!="interface.call"&&type!="interface.supports")return nullptr;
    if(graph.at("domain")!="logic"&&graph.at("domain")!="function")throw std::runtime_error("Interface nodes belong to logic assets");
    const std::string name=node.at("interface");const auto& definition=graph.at("interfaces").at(name);
    if(type=="interface.supports")return {{"title","Implements interface"},{"domain","logic"},{"inputs",{{"self","object"}}},{"outputs",{{"value","bool"}}},{"defaults",{{"self",""}}}};
    const std::string key=node.at("method");auto result=functionCallDefinition(methodSignature(definition.at("methods").at(key)),name+"."+key,graph.value("types",Json::object()));result["domain"]="logic";result.erase("function");return result;
}
std::vector<std::string> interfaceTargets(const Json& graph,const Json& node){
    std::set<std::string> result;const auto classes=graph.value("classes",Json::object());const std::string name=node.at("interface"),method=node.at("method");
    for(const auto& [key,c]:classes.items()){const auto resolved=blueprintClass(classes,key,graph.value("types",Json::object()));if(containsInterface(resolved,name))result.insert(resolved.at("methods").at(method));}
    return {result.begin(),result.end()};
}
Json graphInterfaceCommand(Json& graph,const std::string& operation,const Json& p){
    if(operation=="graph_interfaces_get")return {{"interfaces",graph.value("interfaces",Json::object())}};
    if(graph.at("domain")!="logic")throw std::runtime_error("Interfaces belong to a logic owner graph");
    if(!graph.contains("interfaces"))graph["interfaces"]=Json::object();const std::string name=p.at("name");interfaceIdentifier(name);
    if(operation=="graph_interface_define")graph["interfaces"][name]=p.at("definition");
    else if(operation=="graph_interface_remove"){if(!graph["interfaces"].erase(name))throw std::runtime_error("Interface not found");}
    else throw std::runtime_error("Unknown interface operation");
    validateGraph(graph);return {{"interfaces",graph.at("interfaces")}};
}
}
