#include "Graph.h"
#include <algorithm>
#include <set>
namespace cygame {
namespace {
using Endpoint=std::pair<std::string,std::string>;
Json collapse(Json& candidate,const Json& source,const Json& p){
    validateGraph(source);
    const std::string context=p.value("functionName",""),name=p.at("name"),kind=p.value("kind","function");
    auto ids=p.at("nodeIds").get<std::vector<std::string>>();std::sort(ids.begin(),ids.end());
    const auto request=Json{{"functionName",context},{"name",name},{"kind",kind},{"nodeIds",ids}};
    const auto sourceHash=sha256(source.dump()),fingerprint=sha256(Json{{"operation","collapse-v1"},{"sourceHash",sourceHash},{"request",request}}.dump());
    Json report={{"valid",false},{"sourceHash",sourceHash},{"planHash",fingerprint},{"name",name},{"kind",kind},{"sourceFunction",context},{"movedNodeIds",ids},{"inputs",Json::object()},{"outputs",Json::object()},{"internalLinks",0},{"incomingLinks",0},{"outgoingLinks",0}};
    try{
        if(source.at("domain")!="logic"&&source.at("domain")!="material")throw std::runtime_error("Collapse needs a logic or material owner");
        if(kind!="function"&&kind!="macro")throw std::runtime_error("Collapse kind must be function or macro");
        if(kind=="macro"&&source.at("domain")!="logic")throw std::runtime_error("Macros belong to logic owners");
        if(source.value("functions",Json::object()).contains(name))throw std::runtime_error("Choose a new, unused function name");
        if(ids.empty()||ids.size()>128||std::adjacent_find(ids.begin(),ids.end())!=ids.end())throw std::runtime_error("Select 1..128 distinct nodes");
        const std::set<std::string> selected(ids.begin(),ids.end());const auto view=graphView(source,context);std::map<std::string,Json> nodes,specs;std::set<std::string> usedNodeIds,usedLinkIds;
        for(const auto& n:view.at("nodes")){const std::string id=n.at("id");nodes[id]=n;usedNodeIds.insert(id);}
        for(const auto& l:view.at("links"))usedLinkIds.insert(l.at("id"));
        Json moved=Json::array(),inner=Json::array();double minX=100000,minY=100000,maxX=-100000;
        for(const auto& id:ids){
            if(!nodes.count(id))throw std::runtime_error("Selected node is missing from this body");const auto n=nodes.at(id),spec=nodeDefinition(view,n);const std::string type=n.at("type");
            if(type=="function.input"||type=="function.output"||type=="material.output"||spec.contains("variableType"))throw std::runtime_error("Keep boundaries, variables and output nodes outside the selection; pass their data through wires");
            for(const auto* side:{"inputs","outputs"})for(const auto& t:spec.at(side))if(t=="exec")throw std::runtime_error("Collapse currently supports pure calculations only; execution nodes stay outside");
            specs[id]=spec;moved.push_back(n);minX=std::min(minX,n.at("position")[0].get<double>());minY=std::min(minY,n.at("position")[1].get<double>());maxX=std::max(maxX,n.at("position")[0].get<double>());
        }
        std::vector<Json> incoming;std::map<Endpoint,std::vector<Json>> outgoing;
        for(const auto& l:view.at("links")){
            const std::string a=l.at("fromNode"),b=l.at("toNode");const bool from=selected.count(a)!=0,to=selected.count(b)!=0;
            if(from&&to){inner.push_back(l);}
            else if(to)incoming.push_back(l);else if(from)outgoing[{a,l.at("fromPin")}].push_back(l);
        }
        if(incoming.size()>16||outgoing.size()>16||outgoing.empty())throw std::runtime_error("Selection needs 1..16 connected data outputs and at most 16 incoming data wires");
        // Calls evaluate all arguments eagerly. Check dependencies per output pin,
        // including the independent U/V branches of UV transform nodes.
        std::map<Endpoint,Json> inputLinks;for(const auto& l:view.at("links"))inputLinks[{l.at("toNode"),l.at("toPin")}]=l;
        for(const auto& [endpoint,links]:outgoing){
            std::set<std::string> ancestors,requiredInputs;std::set<Endpoint> visited;
            std::function<void(const Endpoint&)> visit=[&](const Endpoint& port){
                if(!visited.insert(port).second)return;ancestors.insert(port.first);const auto& node=nodes.at(port.first);const auto& spec=specs.at(port.first);
                for(const auto& [pin,type]:spec.at("inputs").items()){
                    if(node.at("type")=="uv.transform"){const bool u=port.second=="u";if(pin!=(u?"u":"v")&&pin!=(u?"tileU":"tileV")&&pin!=(u?"offsetU":"offsetV"))continue;}
                    const auto link=inputLinks.find({port.first,pin});if(link==inputLinks.end())continue;const std::string sourceNode=link->second.at("fromNode");
                    if(selected.count(sourceNode))visit({sourceNode,link->second.at("fromPin")});else requiredInputs.insert(link->second.at("id"));
                }
            };visit(endpoint);
            if(ancestors!=selected||requiredInputs.size()!=incoming.size())throw std::runtime_error("Each exported output must use every selected node and incoming wire; narrow the selection to preserve eager argument evaluation");
        }
        auto generated=[&](std::set<std::string>& used,const std::string& label){std::string id="collapse_"+fingerprint.substr(0,20)+"_"+label;for(int i=1;used.count(id);++i)id="collapse_"+fingerprint.substr(0,20)+"_"+label+"_"+std::to_string(i);used.insert(id);return id;};
        const auto callId=generated(usedNodeIds,"call"),entryId=generated(usedNodeIds,"inputs"),exitId=generated(usedNodeIds,"outputs");report["callNodeId"]=callId;
        Json inputs=Json::object(),outputs=Json::object(),defaults=Json::object(),returns=Json::object();std::map<std::string,std::string> inputPins;std::map<Endpoint,std::string> outputPins;int index=0;
        std::sort(incoming.begin(),incoming.end(),[](const Json& a,const Json& b){return a.at("id")<b.at("id");});
        for(const auto& l:incoming){
            const std::string target=l.at("toNode"),pin=l.at("toPin"),port="in_"+std::to_string(++index),type=specs.at(target).at("inputs").at(pin);const auto fallback=nodes.at(target).at("values").at(pin);
            inputs[port]={{"type",type},{"default",fallback}};defaults[port]=fallback;inputPins[l.at("id")]=port;
            report["inputs"][port]={{"type",type},{"sourceNode",l.at("fromNode")},{"sourcePin",l.at("fromPin")},{"targetNode",target},{"targetPin",pin},{"linkId",l.at("id")},{"defaultBytes",fallback.dump().size()}};
            inner.push_back({{"id",generated(usedLinkIds,"input"+std::to_string(index))},{"fromNode",entryId},{"fromPin",port},{"toNode",target},{"toPin",pin}});
        }
        index=0;size_t outgoingCount=0;
        for(const auto& [endpoint,links]:outgoing){
            const std::string port="out_"+std::to_string(++index),type=specs.at(endpoint.first).at("outputs").at(endpoint.second);const auto fallback=graphDefaultValue(type,source.value("types",Json::object()));
            outputs[port]={{"type",type},{"default",fallback}};returns[port]=fallback;outputPins[endpoint]=port;outgoingCount+=links.size();
            report["outputs"][port]={{"type",type},{"sourceNode",endpoint.first},{"sourcePin",endpoint.second},{"connections",links.size()}};
            inner.push_back({{"id",generated(usedLinkIds,"output"+std::to_string(index))},{"fromNode",endpoint.first},{"fromPin",endpoint.second},{"toNode",exitId},{"toPin",port}});
        }
        moved.push_back({{"id",entryId},{"type","function.input"},{"position",{std::max(-100000.,minX-280),minY}},{"values",Json::object()}});
        moved.push_back({{"id",exitId},{"type","function.output"},{"position",{std::min(100000.,maxX+280),minY}},{"values",returns}});
        Json function={{"domain","function"},{"mode","pure"},{"kind",kind},{"variables",Json::object()},{"inputs",inputs},{"outputs",outputs},{"nodes",moved},{"links",inner}};
        const Json call={{"id",callId},{"type",kind=="macro"?"macro.call":"function.call"},{"function",name},{"position",{minX,minY}},{"values",defaults}};
        candidate=source;if(!candidate.contains("functions"))candidate["functions"]=Json::object();candidate["functions"][name]=function;auto& body=context.empty()?candidate:candidate["functions"][context];
        Json outerNodes=Json::array(),outerLinks=Json::array();bool inserted=false;
        for(const auto& n:view.at("nodes")){if(selected.count(n.at("id"))){if(!inserted){outerNodes.push_back(call);inserted=true;}}else outerNodes.push_back(n);}
        for(auto l:view.at("links")){
            const bool from=selected.count(l.at("fromNode"))!=0,to=selected.count(l.at("toNode"))!=0;if(from&&to)continue;
            if(to){l["toNode"]=callId;l["toPin"]=inputPins.at(l.at("id"));}
            if(from){const Endpoint endpoint={l.at("fromNode"),l.at("fromPin")};l["fromNode"]=callId;l["fromPin"]=outputPins.at(endpoint);}outerLinks.push_back(l);
        }
        body["nodes"]=outerNodes;body["links"]=outerLinks;validateGraph(candidate);
        report["internalLinks"]=inner.size()-incoming.size()-outgoing.size();report["incomingLinks"]=incoming.size();report["outgoingLinks"]=outgoingCount;report["valid"]=true;
    }catch(const std::exception& e){report["valid"]=false;report["diagnostic"]=e.what();}
    if(report.dump().size()>256*1024-64){report["valid"]=false;report["inputs"]=Json::object();report["outputs"]=Json::object();report["diagnostic"]="Collapse preview exceeds 256 KiB";}
    return report;
}
}
Json graphCollapseCommand(Json& owner,const std::string& operation,const Json& p){
    Json candidate;auto report=collapse(candidate,owner,p);if(operation=="graph_collapse_plan")return report;
    if(operation!="graph_collapse_apply")throw std::runtime_error("Unknown graph collapse operation");
    if(report.at("planHash")!=p.at("planHash"))throw std::runtime_error("Collapse preview is stale or the request changed; preview again");
    if(!report.at("valid").get<bool>())throw std::runtime_error(report.value("diagnostic","Invalid collapse"));owner=std::move(candidate);report["applied"]=true;return report;
}
}
