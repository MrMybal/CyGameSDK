#include "Graph.h"
#include <set>
#include <algorithm>
namespace cygame {
Json findGraphNodes(const Json& graphs,const Json& p){
    Json results=Json::array();const std::string query=p.at("query"),scope=p.value("graphId","");bool truncated=false;
    auto scan=[&](const std::string& id,const Json& graph,const std::string& function){
        for(const auto& n:graph.at("nodes")){const auto spec=nodeDefinition(graph,n);const auto title=spec.at("title").get<std::string>();const std::string searchable=n.at("id").get<std::string>()+" "+n.at("type").get<std::string>()+" "+title+" "+n.value("variable","")+" "+n.value("comment","")+" "+n.at("values").dump();
            if(matches(searchable,query)){if(results.size()>=2000){truncated=true;return;}results.push_back({{"graphId",id},{"functionName",function},{"nodeId",n.at("id")},{"type",n.at("type")},{"label",title},{"comment",n.value("comment","")}});}
        }
    };
    if(!scope.empty()&&!graphs.contains(scope))throw std::runtime_error("Graph not found");
    for(const auto& [id,g]:graphs.items())if(scope.empty()||scope==id){scan(id,g,"");const auto functions=g.value("functions",Json::object());for(const auto& [name,f]:functions.items())scan(id,graphView(g,name),name);}
    return {{"results",results},{"truncated",truncated}};
}
Json graphNavigationCommand(Json& graph,const std::string& op,const Json& p){
    if(op=="graph_link_reroute"){
        Json original;for(const auto& l:graph.at("links"))if(l["id"]==p.at("linkId"))original=l;if(original.is_null())throw std::runtime_error("Link not found");
        Json source;for(const auto& n:graph["nodes"])if(n["id"]==original.at("fromNode"))source=n;const auto spec=nodeDefinition(graph,source);const std::string valueType=spec.at("outputs").at(original.at("fromPin").get<std::string>()),type=isNamedGraphType(valueType)?"route.typed":"route."+valueType,id=uniqueId();
        Json node={{"id",id},{"type",type},{"position",p.at("position")},{"values",nodeTypes().at(type).at("defaults")}};if(isNamedGraphType(valueType)){node["typeRef"]=valueType;node["values"]=typedNodeDefinition(graph,node).at("defaults");}graph["nodes"].push_back(node);
        for(auto& l:graph["links"])if(l["id"]==original.at("id")){l["toNode"]=id;l["toPin"]=valueType=="exec"?"exec":"value";}
        graph["links"].push_back({{"id",uniqueId()},{"fromNode",id},{"fromPin",valueType=="exec"?"then":"value"},{"toNode",original.at("toNode")},{"toPin",original.at("toPin")}});
        validateGraph(graph);return {{"node",node}};
    }
    if(op=="graph_comment_edit"){
        bool found=false;for(auto& c:graph["comments"])if(c.at("id")==p.at("commentId")){for(const auto* key:{"text","position","size"})if(p.contains(key))c[key]=p.at(key);found=true;}
        if(!found)throw std::runtime_error("Comment not found");validateGraph(graph);return {{"updated",true}};
    }
    if(!graph.contains("bookmarks"))graph["bookmarks"]=Json::array();auto& marks=graph["bookmarks"];
    if(op=="graph_bookmark_add"){
        Json mark={{"id",uniqueId()},{"name",p.at("name")},{"pan",p.at("pan")},{"zoom",p.at("zoom")}};marks.push_back(mark);validateGraph(graph);return {{"bookmark",mark}};
    }
    if(op=="graph_bookmark_remove"){
        auto count=marks.size();marks.erase(std::remove_if(marks.begin(),marks.end(),[&](const Json& m){return m["id"]==p.at("bookmarkId");}),marks.end());if(marks.size()==count)throw std::runtime_error("Bookmark not found");return {{"removed",true}};
    }
    throw std::runtime_error("Unknown graph navigation operation");
}
}
