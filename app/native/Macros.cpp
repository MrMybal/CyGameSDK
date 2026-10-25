#include "Graph.h"
#include <cctype>
namespace cygame {
namespace {
Json expandBody(Json body,const Json& library,int depth){
    if(depth>16)throw std::runtime_error("Macro expansion exceeds 16 levels");
    for(size_t cursor=0;cursor<body["nodes"].size();){
        const auto call=body["nodes"][cursor];if(call.at("type")!="macro.call"){++cursor;continue;}
        const std::string name=call.at("function"),callId=call.at("id");auto macro=expandBody(library.at(name),library,depth+1);if(macro.value("kind","")!="macro")throw std::runtime_error("Macro call needs a macro definition");
        std::string in,out;for(const auto& n:macro["nodes"]){if(n.at("type")=="function.input")in=n.at("id");if(n.at("type")=="function.output")out=n.at("id");}
        auto incoming=body["links"];Json nodes=Json::array(),links=Json::array();for(const auto& n:body["nodes"])if(n.at("id")!=callId)nodes.push_back(n);for(const auto& l:body["links"])if(l.at("fromNode")!=callId&&l.at("toNode")!=callId)links.push_back(l);
        std::map<std::string,std::pair<std::string,std::string>> bound;std::string scope="m_"+sha256(callId).substr(0,16)+"_";
        auto constant=[&](const std::string& pin,const std::string& type,const Json& value){std::string id=callId+"/default/"+pin;bool exists=false;for(const auto& n:nodes)exists|=n.at("id")==id;if(!exists){Json n={{"id",id},{"type",isNamedGraphType(type)?"typed.literal":"constant."+type},{"values",{{"value",value}}},{"position",call.at("position")},{"macroCall",callId}};if(isNamedGraphType(type))n["typeRef"]=type;nodes.push_back(n);}return std::make_pair(id,std::string("value"));};
        for(const auto& [pin,spec]:macro["inputs"].items()){bool found=false;for(const auto& l:incoming)if(l.at("toNode")==callId&&l.at("toPin")==pin){bound[pin]={l.at("fromNode"),l.at("fromPin")};found=true;break;}if(!found)bound[pin]=constant("in_"+pin,spec.at("type"),call["values"].at(pin));}
        for(const auto& l:incoming)if(l.at("toNode")==callId&&l.at("toPin")=="exec")bound["then"]={l.at("fromNode"),l.at("fromPin")};
        const auto variables=macro.value("variables",Json::object());for(const auto& [variable,spec]:variables.items())body["variables"][scope+variable.substr(0,40)]=spec;
        for(auto n:macro["nodes"])if(n.at("id")!=in&&n.at("id")!=out){n["id"]=callId+"/"+n.at("id").get<std::string>();n["macroCall"]=callId;n["macroName"]=name;if(n.contains("variable"))n["variable"]=scope+n["variable"].get<std::string>().substr(0,40);for(const auto& l:macro["links"])if(l.at("fromNode")==in&&l.at("fromPin")=="then"&&callId+"/"+l.at("toNode").get<std::string>()==n.at("id")){n["macroEntry"]=true;n["breakpoint"]=call.value("breakpoint",false);}nodes.push_back(n);}
        auto source=[&](const Json& l)->std::pair<std::string,std::string>{const std::string node=l.at("fromNode"),pin=l.at("fromPin");if(node==in)return bound.count(pin)?bound.at(pin):std::make_pair(std::string(),pin);return {callId+"/"+node,pin};};
        auto link=[&](const std::pair<std::string,std::string>& from,const std::string& to,const std::string& pin){if(!from.first.empty())links.push_back({{"id",uniqueId()},{"fromNode",from.first},{"fromPin",from.second},{"toNode",to},{"toPin",pin}});};
        for(const auto& l:macro["links"])if(l.at("toNode")!=out)link(source(l),callId+"/"+l.at("toNode").get<std::string>(),l.at("toPin"));
        for(const auto& outer:incoming)if(outer.at("fromNode")==callId){const std::string pin=outer.at("fromPin"),inner=pin=="then"?"exec":pin;bool found=false;for(const auto& l:macro["links"])if(l.at("toNode")==out&&l.at("toPin")==inner){link(source(l),outer.at("toNode"),outer.at("toPin"));found=true;}if(!found&&pin!="then")link(constant("out_"+pin,macro["outputs"].at(pin).at("type"),macro["outputs"].at(pin).at("default")),outer.at("toNode"),outer.at("toPin"));}
        body["nodes"]=nodes;body["links"]=links;if(nodes.size()>500||links.size()>2000)throw std::runtime_error("Expanded macro body exceeds graph limits");cursor=0;
    }return body;
}
}
Json expandGraphMacros(const Json& graph){
    auto result=graph;const auto library=graph.value("functions",Json::object());for(const auto& [name,function]:library.items())result["functions"][name]=expandBody(function,library,0);result=expandBody(result,library,0);return result;
}
}
