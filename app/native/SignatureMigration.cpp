#include "Graph.h"
#include <algorithm>
#include <map>
#include <set>
namespace cygame {
namespace {
using Names=std::map<std::string,std::string>;
struct PinMap{Names inputs,outputs;};
std::string ikey(const std::string& name,const std::string& method){return "i:"+name+"/"+method;}
Json fullInterface(const Json& spec){auto result=spec;result["inputs"]["self"]={{"type","object"},{"default",""}};return result;}
Names mapping(const Json& before,const Json& after,const Json& requested){
    if(!requested.is_object()||requested.size()>16)throw std::runtime_error("Invalid port rename map");
    for(const auto& [old,next]:requested.items())if(!next.is_string()||!before.contains(old)||!after.contains(next.get<std::string>()))throw std::runtime_error("A rename must map an existing port to a declared destination");
    Names result;std::set<std::string> destinations;
    for(const auto& [name,port]:before.items()){
        const auto next=requested.contains(name)?requested.at(name).get<std::string>():after.contains(name)?name:"";
        if(!next.empty()&&!destinations.insert(next).second)throw std::runtime_error("Two old ports cannot map to one destination");result[name]=next;
    }return result;
}
std::string mapped(const Names& map,const std::string& pin){auto found=map.find(pin);return found==map.end()?pin:found->second;}
Json definitions(const Json& old,const Json& desired,const Names& names,bool primary){
    Json result=Json::object();for(const auto& [name,spec]:desired.items()){
        std::string origin;for(const auto& [from,to]:names)if(to==name)origin=from;
        Json port=origin.empty()?Json::object():old.at(origin);
        if(primary||origin.empty()||port.at("type")!=spec.at("type"))for(const auto& [key,value]:spec.items())port[key]=value;
        result[name]=port;
    }return result;
}
void oldValue(Json& detail,const Json& value){const auto bytes=value.dump();detail["oldValueBytes"]=bytes.size();if(bytes.size()<=1024)detail["oldValue"]=value;else{size_t end=256;while(end>0&&(static_cast<unsigned char>(bytes[end])&0xc0)==0x80)--end;detail["oldValuePreview"]=bytes.substr(0,end)+"...";}}
struct Migration {Json graph,report;};
Migration prepare(const Json& source,const Json& p){
    validateGraph(source);const std::string target=p.at("target"),name=p.at("name"),method=p.value("method","");
    if(target!="function"&&target!="interface")throw std::runtime_error("Migration target must be function or interface");
    if(target=="function"&&!method.empty())throw std::runtime_error("Only interface targets have a method selector");
    const auto functions=source.value("functions",Json::object()),interfaces=source.value("interfaces",Json::object()),classes=source.value("classes",Json::object()),types=source.value("types",Json::object());
    const std::string seed=target=="function"?"f:"+name:ikey(name,method);
    const Json original=target=="function"?functions.at(name):fullInterface(interfaces.at(name).at("methods").at(method));
    Json desired=original;desired["inputs"]=p.at("inputs");desired["outputs"]=p.at("outputs");
    if(target=="interface"){if(desired["inputs"].contains("self"))throw std::runtime_error("Interface self is implicit");desired["inputs"]["self"]={{"type","object"},{"default",""}};}
    functionCallDefinition(desired,name,types);
    const auto renames=p.value("renames",Json::object());if(!renames.is_object())throw std::runtime_error("Invalid rename directions");for(const auto& [side,v]:renames.items())if(side!="inputs"&&side!="outputs")throw std::runtime_error("Unknown rename direction");
    PinMap pins{mapping(original.at("inputs"),desired.at("inputs"),renames.value("inputs",Json::object())),mapping(original.at("outputs"),desired.at("outputs"),renames.value("outputs",Json::object()))};
    std::map<std::string,std::set<std::string>> edges;std::set<std::string> methodFunctions;
    auto pair=[&](const std::string& a,const std::string& b){edges[a].insert(b);edges[b].insert(a);};
    for(const auto& [className,c]:classes.items()){
        const auto resolved=blueprintClass(classes,className,types);for(const auto& f:resolved.at("methods"))methodFunctions.insert("f:"+f.get<std::string>());
        if(!c.value("parent","").empty()){
            const auto parent=blueprintClass(classes,c.at("parent"),types);for(const auto& [key,f]:resolved.at("methods").items())if(parent.at("methods").contains(key))pair("f:"+f.get<std::string>(),"f:"+parent.at("methods").at(key).get<std::string>());
        }
        for(const auto& id:resolved.at("interfaces"))for(const auto& [key,spec]:interfaces.at(id.get<std::string>()).at("methods").items())pair(ikey(id,key),"f:"+resolved.at("methods").at(key).get<std::string>());
    }
    std::set<std::string> selected;std::vector<std::string> pending={seed};while(!pending.empty()){auto key=pending.back();pending.pop_back();if(selected.insert(key).second)for(const auto& next:edges[key])pending.push_back(next);}
    bool bound=target=="interface";for(const auto& key:selected)bound|=methodFunctions.count(key)!=0;
    if(bound&&(!desired.at("inputs").contains("self")||desired["inputs"]["self"]["type"]!="object"||mapped(pins.inputs,"self")!="self"))throw std::runtime_error("Bound method self must remain an object input named self");
    Migration result{source,Json::object()};auto& out=result.report;out["targets"]={{"functions",Json::array()},{"interfaces",Json::array()}};
    out["changes"]=Json::array();Json summary={{"declarations",0},{"nodes",0},{"linksRenamed",0},{"linksRemoved",0},{"valuesPreserved",0},{"valuesAdded",0},{"valuesRemoved",0},{"valuesReset",0}};size_t detailBytes=0,changeCount=0;
    auto record=[&](Json detail){++changeCount;const size_t bytes=detail.dump().size();if(out["changes"].size()<512&&detailBytes+bytes<=256*1024){out["changes"].push_back(detail);detailBytes+=bytes;}};
    for(const auto& key:selected){
        Json* declaration=nullptr;bool contract=key.rfind("i:",0)==0;
        if(contract){const auto slash=key.find('/');auto id=key.substr(2,slash-2),member=key.substr(slash+1);declaration=&result.graph["interfaces"][id]["methods"][member];out["targets"]["interfaces"].push_back({{"name",id},{"method",member}});}
        else{declaration=&result.graph["functions"][key.substr(2)];out["targets"]["functions"].push_back(key.substr(2));}
        const auto before=*declaration;auto full=contract?fullInterface(before):before;
        (*declaration)["inputs"]=definitions(full.at("inputs"),desired.at("inputs"),pins.inputs,key==seed);if(contract)(*declaration)["inputs"].erase("self");
        (*declaration)["outputs"]=definitions(full.at("outputs"),desired.at("outputs"),pins.outputs,key==seed);
        if(*declaration!=before){summary["declarations"]=summary["declarations"].get<int>()+1;record({{"kind","Declaration updated"},{"declaration",key},{"before",{{"inputs",before.at("inputs")},{"outputs",before.at("outputs")}}},{"after",{{"inputs",declaration->at("inputs")},{"outputs",declaration->at("outputs")}}}});}
    }
    auto body=[&](const std::string& scope){
        const auto oldView=graphView(source,scope),newView=graphView(result.graph,scope);auto& current=scope.empty()?result.graph:result.graph["functions"][scope];
        std::map<std::string,PinMap> nodePins;std::map<std::string,Json> specs;
        for(auto& n:current["nodes"]){const std::string id=n.at("id"),type=n.at("type");const auto oldNode=std::find_if(oldView.at("nodes").begin(),oldView.at("nodes").end(),[&](const Json& v){return v.at("id")==id;});const auto before=nodeDefinition(oldView,*oldNode);PinMap maps;bool affected=false;
            if((type=="function.call"||type=="object.call"||type=="macro.call")&&selected.count("f:"+n.at("function").get<std::string>())){maps=pins;affected=true;}
            if(type=="interface.call"&&selected.count(ikey(n.at("interface"),n.at("method")))){maps=pins;affected=true;}
            if(selected.count("f:"+scope)){if(type=="function.input"){maps.outputs=pins.inputs;affected=true;}if(type=="function.output"){maps.inputs=pins.outputs;affected=true;}}
            if(type=="flow.foreach"&&selected.count("f:"+n.at("function").get<std::string>())){
                const auto fn=n.at("function").get<std::string>();for(const auto* fixed:{"item","index"})if(mapped(pins.inputs,fixed)!=fixed||!result.graph["functions"][fn]["inputs"].contains(fixed)||result.graph["functions"][fn]["inputs"][fixed]["type"]!=source.at("functions").at(fn).at("inputs").at(fixed).at("type"))throw std::runtime_error("For-each item/index names and types must remain unchanged");
                if(mapped(pins.outputs,"break")!="break")throw std::runtime_error("For-each break output must keep its name");maps.inputs=pins.inputs;maps.inputs.erase("item");maps.inputs.erase("index");affected=true;
            }
            const auto after=nodeDefinition(newView,n);specs[id]=after;
            if(affected){
                nodePins[id]=maps;summary["nodes"]=summary["nodes"].get<int>()+1;Json values=after.at("defaults");std::set<std::string> retained;
                for(const auto& [old,value]:n.at("values").items()){
                    const std::string next=mapped(maps.inputs,old);const bool exists=!next.empty()&&values.contains(next);bool compatible=exists&&before.at("inputs").contains(old)&&after.at("inputs").at(next)==before.at("inputs").at(old);
                    if(compatible)try{validateValue(value,graphValueSchema(after.at("inputs").at(next),types));}catch(...){compatible=false;}
                    if(compatible){values[next]=value;retained.insert(next);summary["valuesPreserved"]=summary["valuesPreserved"].get<int>()+1;}
                    else{const auto count=exists?"valuesReset":"valuesRemoved";summary[count]=summary[count].get<int>()+1;Json detail={{"kind",exists?"Value reset":"Value removed"},{"scope",scope},{"nodeId",id},{"fromPin",old},{"toPin",next}};oldValue(detail,value);record(detail);if(exists)retained.insert(next);}
                    if(exists&&old!=next)record({{"kind","Port renamed"},{"scope",scope},{"nodeId",id},{"fromPin",old},{"toPin",next}});
                }
                for(const auto& [key,value]:values.items())if(!retained.count(key))summary["valuesAdded"]=summary["valuesAdded"].get<int>()+1;n["values"]=values;
            }
        }
        Json links=Json::array();for(auto link:current.at("links")){
            const auto from=link.at("fromNode").get<std::string>(),to=link.at("toNode").get<std::string>(),fp=link.at("fromPin").get<std::string>(),tp=link.at("toPin").get<std::string>();const auto nextFrom=nodePins.count(from)?mapped(nodePins[from].outputs,fp):fp,nextTo=nodePins.count(to)?mapped(nodePins[to].inputs,tp):tp;
            const bool valid=!nextFrom.empty()&&!nextTo.empty()&&specs.at(from).at("outputs").contains(nextFrom)&&specs.at(to).at("inputs").contains(nextTo)&&specs.at(from).at("outputs").at(nextFrom)==specs.at(to).at("inputs").at(nextTo);
            if(!valid){summary["linksRemoved"]=summary["linksRemoved"].get<int>()+1;record({{"kind","Link removed"},{"scope",scope},{"link",link}});continue;}
            if(fp!=nextFrom||tp!=nextTo){summary["linksRenamed"]=summary["linksRenamed"].get<int>()+1;record({{"kind","Link renamed"},{"scope",scope},{"linkId",link.at("id")},{"fromPin",fp},{"toPin",tp},{"newFromPin",nextFrom},{"newToPin",nextTo}});}
            link["fromPin"]=nextFrom;link["toPin"]=nextTo;links.push_back(link);
        }current["links"]=links;
    };
    try{body("");for(const auto& [key,f]:functions.items())body(key);validateGraph(result.graph);out["valid"]=true;}catch(const std::exception& e){out["valid"]=false;out["diagnostic"]=e.what();}
    out["summary"]=summary;out["requiresLossAcceptance"]=summary["linksRemoved"].get<int>()+summary["valuesRemoved"].get<int>()+summary["valuesReset"].get<int>()>0;out["changeCount"]=changeCount;out["truncated"]=changeCount>out["changes"].size();
    Json request={{"target",target},{"name",name},{"method",method},{"inputs",p.at("inputs")},{"outputs",p.at("outputs")},{"renames",renames}};out["sourceHash"]=sha256(source.dump());out["planHash"]=sha256(Json{{"sourceHash",out["sourceHash"]},{"request",request}}.dump());return result;
}
}
Json graphSignatureCommand(Json& graph,const std::string& operation,const Json& p){
    auto prepared=prepare(graph,p);if(operation=="graph_signature_plan")return prepared.report;
    if(operation!="graph_signature_apply")throw std::runtime_error("Unknown signature migration operation");
    if(prepared.report.at("planHash")!=p.at("planHash"))throw std::runtime_error("Migration preview is stale or the request changed; preview again");
    if(!prepared.report.at("valid").get<bool>())throw std::runtime_error(prepared.report.value("diagnostic","Invalid signature migration"));
    if(prepared.report.at("requiresLossAcceptance").get<bool>()&&!p.value("acceptLoss",false))throw std::runtime_error("Migration removes links or values; explicitly accept the previewed losses");
    graph=std::move(prepared.graph);prepared.report["applied"]=true;return prepared.report;
}
}
