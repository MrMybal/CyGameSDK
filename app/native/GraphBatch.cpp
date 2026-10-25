#include "Graph.h"
#include <algorithm>
#include <set>
namespace cygame {
Json graphBatchCommand(Json& graph,const std::string& op,const Json& p){
    auto node=[&](const std::string& id)->Json&{for(auto& n:graph["nodes"])if(n["id"]==id)return n;throw std::runtime_error("Node not found: "+id);};
    auto protect=[&](const Json& n){if(n["type"]=="function.input"||n["type"]=="function.output")throw std::runtime_error("Function boundary nodes cannot be copied or removed");};
    if(op=="graph_nodes_move"){
        std::set<std::string> seen;for(const auto& move:p.at("positions")){const std::string id=move.at("nodeId");if(!seen.insert(id).second)throw std::runtime_error("Duplicate node in batch");node(id)["position"]=move.at("position");}
        validateGraph(graph);return {{"movedCount",seen.size()}};
    }
    if(op=="graph_comment_add"){
        if(!graph.contains("comments"))graph["comments"]=Json::array();Json c={{"id",uniqueId()},{"text",p.at("text")},{"position",p.at("position")},{"size",p.at("size")}};
        graph["comments"].push_back(c);validateGraph(graph);return {{"comment",c}};
    }
    if(op=="graph_comment_remove"){
        auto& comments=graph.at("comments");const auto size=comments.size();comments.erase(std::remove_if(comments.begin(),comments.end(),[&](const Json& c){return c["id"]==p.at("commentId");}),comments.end());
        if(comments.size()==size)throw std::runtime_error("Comment not found");return {{"removed",true}};
    }
    if(op=="graph_fragment_paste"){
        const auto& fragment=p.at("fragment");if(fragment.value("format","")!="cygame-graph-fragment"||fragment.value("schemaVersion",0)!=1)throw std::runtime_error("Unsupported graph clipboard format");
        if(fragment.dump().size()>512*1024)throw std::runtime_error("Graph clipboard exceeds 512 KiB");
        if(!fragment.at("nodes").is_array()||fragment["nodes"].empty()||fragment["nodes"].size()>500||!fragment.at("links").is_array()||fragment["links"].size()>2000)throw std::runtime_error("Invalid graph clipboard size");
        for(const auto* category:{"variables","functions","classes","types","interfaces"}){
            const auto incoming=fragment.value(category,Json::object());if(!incoming.is_object())throw std::runtime_error("Invalid clipboard declarations");if(incoming.empty())continue;
            if(!graph.contains(category))graph[category]=Json::object();for(const auto& [name,definition]:incoming.items()){
                if(graph[category].contains(name)&&graph[category][name]!=definition)throw std::runtime_error(std::string("Clipboard declaration conflict: ")+name);
                graph[category][name]=definition;
            }
        }
        const auto offset=p.value("offset",Json::array({40.,40.}));std::map<std::string,std::string> remap;Json copies=Json::array();
        for(auto n:fragment["nodes"]){protect(n);const std::string old=n.at("id");if(old.empty()||remap.count(old))throw std::runtime_error("Duplicate clipboard node");remap[old]=uniqueId();n["id"]=remap[old];for(int i=0;i<2;++i)n["position"][i]=n["position"][i].get<double>()+offset[i].get<double>();copies.push_back(n);graph["nodes"].push_back(n);}
        for(auto l:fragment["links"]){const std::string a=l.at("fromNode"),b=l.at("toNode");if(!remap.count(a)||!remap.count(b))throw std::runtime_error("Clipboard links must stay inside its nodes");l["id"]=uniqueId();l["fromNode"]=remap[a];l["toNode"]=remap[b];graph["links"].push_back(l);}
        validateGraph(graph);return {{"nodes",copies}};
    }
    const auto ids=p.at("nodeIds").get<std::vector<std::string>>();std::set<std::string> selected;
    for(const auto& id:ids){if(!selected.insert(id).second)throw std::runtime_error("Duplicate node ID");protect(node(id));}
    if(op=="graph_nodes_remove"){
        auto& nodes=graph["nodes"];nodes.erase(std::remove_if(nodes.begin(),nodes.end(),[&](const Json& n){return selected.count(n.at("id"));}),nodes.end());
        auto& links=graph["links"];links.erase(std::remove_if(links.begin(),links.end(),[&](const Json& l){return selected.count(l.at("fromNode"))||selected.count(l.at("toNode"));}),links.end());
        validateGraph(graph);return {{"removedCount",ids.size()}};
    }
    if(op=="graph_fragment_copy"){
        Json nodes=Json::array(),links=Json::array(),variables=Json::object();for(const auto& id:ids){const auto& n=node(id);nodes.push_back(n);if(n.contains("variable"))variables[n.at("variable").get<std::string>()]=graph.at("variables").at(n.at("variable"));}
        for(const auto& l:graph["links"])if(selected.count(l.at("fromNode"))&&selected.count(l.at("toNode")))links.push_back(l);
        return {{"fragment",{{"format","cygame-graph-fragment"},{"schemaVersion",1},{"nodes",nodes},{"links",links},{"variables",variables},{"functions",graph.value("functions",Json::object())},{"classes",graph.value("classes",Json::object())},{"types",graph.value("types",Json::object())},{"interfaces",graph.value("interfaces",Json::object())}}}};
    }
    throw std::runtime_error("Unknown graph batch operation");
}
}
