#include "Graph.h"
#include <algorithm>
#include <set>
namespace cygame {
namespace {
using Endpoint=std::pair<std::string,std::string>;
// This is an authored-document operation. Runtime macro expansion is used only
// for dependency analysis, never as a serialized replacement for authored data.
Json expand(Json& candidate,const Json& source,const Json& p){
    validateGraph(source);const std::string context=p.value("functionName",""),callId=p.at("nodeId");
    const auto sourceHash=sha256(source.dump()),fingerprint=sha256(Json{{"operation","expand-v1"},{"sourceHash",sourceHash},{"functionName",context},{"nodeId",callId}}.dump());
    Json report={{"valid",false},{"sourceHash",sourceHash},{"planHash",fingerprint},{"nodeId",callId},{"sourceFunction",context},{"name",""},{"kind",""},{"nodeMap",Json::object()},{"addedNodeIds",Json::array()},{"inputs",Json::object()},{"outputs",Json::object()},{"clonedNodes",0},{"internalLinks",0},{"incomingLinks",0},{"outgoingLinks",0}};
    try{
        if(source.at("domain")!="logic"&&source.at("domain")!="material")throw std::runtime_error("Expansion needs a logic or material owner");
        const auto caller=graphView(source,context);Json call;std::set<std::string> nodeIds,linkIds,commentIds;
        for(const auto& n:caller.at("nodes")){nodeIds.insert(n.at("id"));if(n.at("id")==callId)call=n;}
        for(const auto& l:caller.at("links"))linkIds.insert(l.at("id"));for(const auto& c:caller.value("comments",Json::array()))commentIds.insert(c.at("id"));
        if(call.is_null()||(call.at("type")!="function.call"&&call.at("type")!="macro.call"))throw std::runtime_error("Select one local function or macro call");
        const std::string name=call.at("function");const auto callee=graphView(source,name);const bool macro=call.at("type")=="macro.call";report["name"]=name;report["kind"]=macro?"macro":"function";
        if(callee.value("mode","pure")!="pure")throw std::runtime_error("Only pure calls can be expanded; execution calls retain their runtime scope");
        Json entry,exit;std::vector<Json> interior;double minX=100000,minY=100000,maxX=-100000;
        for(const auto& n:callee.at("nodes")){
            const std::string type=n.at("type");if(type=="function.input"){entry=n;continue;}if(type=="function.output"){exit=n;continue;}
            const auto spec=nodeDefinition(callee,n);if(spec.contains("variableType")||type=="material.output")throw std::runtime_error("Unsupported scoped node in pure call");
            for(const auto* side:{"inputs","outputs"})for(const auto& t:spec.at(side))if(t=="exec")throw std::runtime_error("Execution nodes cannot be expanded by this operation");
            interior.push_back(n);minX=std::min(minX,n.at("position")[0].get<double>());minY=std::min(minY,n.at("position")[1].get<double>());maxX=std::max(maxX,n.at("position")[0].get<double>());
        }
        if(interior.size()>128)throw std::runtime_error("Expansion supports at most 128 interior nodes");if(interior.empty())minX=minY=maxX=0;
        std::set<std::string> wiredInputs,usedOutputs;for(const auto& l:caller.at("links")){if(l.at("toNode")==callId){wiredInputs.insert(l.at("toPin"));report["incomingLinks"]=report.at("incomingLinks").get<int>()+1;}if(l.at("fromNode")==callId){usedOutputs.insert(l.at("fromPin"));report["outgoingLinks"]=report.at("outgoingLinks").get<int>()+1;}}
        if(!macro&&!usedOutputs.empty()&&!wiredInputs.empty()){
            // A pure function eagerly resolves all call arguments. Removing that
            // boundary must not make a wired argument (possibly failing) lazy.
            // Expand nested macros for analysis: their outputs may ignore inputs.
            const auto analysis=expandGraphMacros(callee);std::map<std::string,Json> nodes;std::map<Endpoint,Endpoint> wires;
            for(const auto& n:analysis.at("nodes"))nodes[n.at("id")]=n;for(const auto& l:analysis.at("links"))wires[{l.at("toNode"),l.at("toPin")} ]={l.at("fromNode"),l.at("fromPin")};
            for(const auto& output:usedOutputs){std::set<Endpoint> visited;std::set<std::string> required;
                std::function<void(const Endpoint&)> visit=[&](const Endpoint& port){
                    if(!visited.insert(port).second)return;const auto& n=nodes.at(port.first);if(n.at("type")=="function.input"){required.insert(port.second);return;}
                    const auto spec=nodeDefinition(analysis,n);for(const auto& [pin,t]:spec.at("inputs").items()){
                        if(n.at("type")=="uv.transform"){const bool u=port.second=="u";if(pin!=(u?"u":"v")&&pin!=(u?"tileU":"tileV")&&pin!=(u?"offsetU":"offsetV"))continue;}
                        const auto it=wires.find({port.first,pin});if(it!=wires.end())visit(it->second);
                    }
                };const auto it=wires.find({exit.at("id"),output});if(it!=wires.end())visit(it->second);
                for(const auto& input:wiredInputs)if(!required.count(input))throw std::runtime_error("Each connected output must read every wired argument to preserve eager function evaluation; keep this call intact");
            }
        }
        auto generated=[&](std::set<std::string>& used,const std::string& label){std::string id="expand_"+fingerprint.substr(0,20)+"_"+label;for(int i=1;used.count(id);++i)id="expand_"+fingerprint.substr(0,20)+"_"+label+"_"+std::to_string(i);used.insert(id);return id;};
        const double x=call.at("position")[0],y=call.at("position")[1];const double dx=x+240-minX,dy=y-minY;Json added=Json::array(),links=Json::array();std::map<std::string,std::string> remap,inRelays,outRelays;int index=0;
        auto relay=[&](const std::string& id,const std::string& type,const Json& value,double px,double py){Json n={{"id",id},{"type",isNamedGraphType(type)?"route.typed":"route."+type},{"position",{px,py}},{"values",{{"value",value}}}};if(isNamedGraphType(type))n["typeRef"]=type;added.push_back(n);};
        for(const auto& [pin,spec]:callee.at("inputs").items()){const auto id=generated(nodeIds,"input"+std::to_string(++index));inRelays[pin]=id;relay(id,spec.at("type"),call.at("values").at(pin),x,y+(index-1)*70.);report["inputs"][pin]={{"type",spec.at("type")},{"relayNodeId",id},{"connected",wiredInputs.count(pin)!=0}};}
        index=0;for(auto n:interior){const std::string old=n.at("id"),id=generated(nodeIds,"node"+std::to_string(++index));remap[old]=id;report["nodeMap"][old]=id;n["id"]=id;n["position"]={n.at("position")[0].get<double>()+dx,n.at("position")[1].get<double>()+dy};added.push_back(n);}
        index=0;for(const auto& [pin,spec]:callee.at("outputs").items()){
            const auto id=generated(nodeIds,"output"+std::to_string(++index));outRelays[pin]=id;
            // Runtime macro outputs use the signature default when unwired;
            // pure functions read the authored return-node fallback instead.
            const auto value=macro?spec.at("default"):exit.at("values").at(pin);relay(id,spec.at("type"),value,maxX+dx+260,y+(index-1)*70.);report["outputs"][pin]={{"type",spec.at("type")},{"relayNodeId",id},{"connected",usedOutputs.count(pin)!=0}};
        }
        index=0;for(auto l:callee.at("links")){l["id"]=generated(linkIds,"link"+std::to_string(++index));const std::string from=l.at("fromNode"),to=l.at("toNode");
            if(from==entry.at("id")){l["fromNode"]=inRelays.at(l.at("fromPin"));l["fromPin"]="value";}else l["fromNode"]=remap.at(from);
            if(to==exit.at("id")){l["toNode"]=outRelays.at(l.at("toPin"));l["toPin"]="value";}else l["toNode"]=remap.at(to);links.push_back(l);
        }
        Json comments=caller.value("comments",Json::array());index=0;for(auto c:callee.value("comments",Json::array())){c["id"]=generated(commentIds,"comment"+std::to_string(++index));c["position"]={c.at("position")[0].get<double>()+dx,c.at("position")[1].get<double>()+dy};comments.push_back(c);}
        double lowX=x-20,lowY=y-45,highX=x+260,highY=y+100;for(const auto& n:added){const auto spec=nodeDefinition(caller,n);lowX=std::min(lowX,n.at("position")[0].get<double>()-20);lowY=std::min(lowY,n.at("position")[1].get<double>()-45);highX=std::max(highX,n.at("position")[0].get<double>()+225);highY=std::max(highY,n.at("position")[1].get<double>()+81+27*std::max(spec.at("inputs").size(),spec.at("outputs").size()));report["addedNodeIds"].push_back(n.at("id"));}
        const auto provenance=generated(commentIds,"provenance");comments.push_back({{"id",provenance},{"text",name},{"position",{lowX,lowY}},{"size",{highX-lowX,highY-lowY}},{"expandedCall",{{"format","cygame-expanded-call-v1"},{"node",call},{"sourceHash",sourceHash},{"function",name}}}});report["provenanceCommentId"]=provenance;
        candidate=source;auto& body=context.empty()?candidate:candidate["functions"][context];Json outer=Json::array();for(const auto& n:caller.at("nodes")){if(n.at("id")==callId)for(const auto& a:added)outer.push_back(a);else outer.push_back(n);}
        for(auto l:caller.at("links")){if(l.at("toNode")==callId){l["toNode"]=inRelays.at(l.at("toPin"));l["toPin"]="value";}if(l.at("fromNode")==callId){l["fromNode"]=outRelays.at(l.at("fromPin"));l["fromPin"]="value";}links.push_back(l);}
        body["nodes"]=outer;body["links"]=links;body["comments"]=comments;validateGraph(candidate);validateGraph(expandGraphMacros(candidate));report["clonedNodes"]=interior.size();report["internalLinks"]=callee.at("links").size();report["valid"]=true;
    }catch(const std::exception& e){report["valid"]=false;report["diagnostic"]=e.what();}
    if(report.dump().size()>256*1024-64){report["valid"]=false;report["nodeMap"]=Json::object();report["addedNodeIds"]=Json::array();report["inputs"]=Json::object();report["outputs"]=Json::object();report["diagnostic"]="Expansion preview exceeds 256 KiB";}
    return report;
}
}
Json graphExpandCommand(Json& owner,const std::string& operation,const Json& p){
    Json candidate;auto report=expand(candidate,owner,p);if(operation=="graph_expand_plan")return report;if(operation!="graph_expand_apply")throw std::runtime_error("Unknown graph expansion operation");
    if(report.at("planHash")!=p.at("planHash"))throw std::runtime_error("Expansion preview is stale or the request changed; preview again");if(!report.at("valid").get<bool>())throw std::runtime_error(report.value("diagnostic","Invalid expansion"));owner=std::move(candidate);report["applied"]=true;return report;
}
}
