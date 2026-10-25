#include "Graph.h"
#include <set>
namespace cygame {
namespace {
Json clean(Json graph,bool presentation){
    graph.erase("assetPath");graph.erase("assetMissing");
    if(presentation){graph.erase("comments");graph.erase("bookmarks");for(auto& n:graph["nodes"]){n.erase("position");n.erase("comment");n.erase("breakpoint");}}
    if(graph.contains("functions"))for(auto& f:graph["functions"])f=clean(f,presentation);return graph;
}
Json indexed(const Json& array){Json result=Json::object();for(const auto& item:array)result[item.at("id").get<std::string>()]=item;return result;}
std::string preview(const Json& value){auto text=value.dump(2);if(text.size()>8000){size_t end=8000;while(end&&(static_cast<unsigned char>(text[end])&0xc0)==0x80)--end;text=text.substr(0,end)+"\n...";}return text;}
}
Json compareGraphDocuments(const Json& saved,const Json& live,bool ignorePresentation){
    const auto before=clean(saved,ignorePresentation),after=clean(live,ignorePresentation);
    Json entries=Json::array(),summary={{"Added",0},{"Removed",0},{"Changed",0}};size_t bytes=0;bool truncated=false;
    auto entry=[&](const std::string& kind,const std::string& key,const std::string& function,const Json& a,const Json& b,bool had,bool has){
        if(had&&has&&a==b)return;const std::string change=!had?"Added":!has?"Removed":"Changed";summary[change]=summary[change].get<int>()+1;
        Json row={{"kind",kind},{"key",key},{"functionName",function},{"change",change},{"beforePreview",had?preview(a):""},{"afterPreview",has?preview(b):""}};
        if(had&&a.dump().size()<16384)row["before"]=a;if(has&&b.dump().size()<16384)row["after"]=b;
        auto size=row.dump().size();if(entries.size()>=1024||bytes+size>2*1024*1024){truncated=true;return;}bytes+=size;entries.push_back(row);
    };
    auto collection=[&](const Json& a,const Json& b,const std::string& kind,const std::string& function){
        std::set<std::string> keys;for(const auto& it:a.items())keys.insert(it.key());for(const auto& it:b.items())keys.insert(it.key());
        for(const auto& key:keys)entry(kind,key,function,a.value(key,Json()),b.value(key,Json()),a.contains(key),b.contains(key));
    };
    auto body=[&](Json a,Json b,const std::string& function){
        for(const auto* key:{"nodes","links","comments","bookmarks"}){
            const auto aa=a.value(key,Json::array()),bb=b.value(key,Json::array());Json orderA=Json::array(),orderB=Json::array();std::set<std::string> setA,setB;
            for(const auto& item:aa){orderA.push_back(item.at("id"));setA.insert(item.at("id"));}for(const auto& item:bb){orderB.push_back(item.at("id"));setB.insert(item.at("id"));}
            if(setA==setB&&orderA!=orderB)entry("ordering",key,function,orderA,orderB,true,true);
            collection(indexed(a.value(key,Json::array())),indexed(b.value(key,Json::array())),key,function);a.erase(key);b.erase(key);}
        collection(a.value("variables",Json::object()),b.value("variables",Json::object()),"variables",function);a.erase("variables");b.erase("variables");a.erase("functions");b.erase("functions");collection(a,b,"properties",function);
    };
    body(before,after,"");const auto fa=before.value("functions",Json::object()),fb=after.value("functions",Json::object());std::set<std::string> functions;
    for(const auto& it:fa.items())functions.insert(it.key());for(const auto& it:fb.items())functions.insert(it.key());
    for(const auto& name:functions){if(!fa.contains(name)||!fb.contains(name))entry("functions",name,name,fa.value(name,Json()),fb.value(name,Json()),fa.contains(name),fb.contains(name));else body(fa.at(name),fb.at(name),name);}
    return {{"changed",summary["Added"].get<int>()+summary["Removed"].get<int>()+summary["Changed"].get<int>()>0},{"summary",summary},{"entries",entries},{"truncated",truncated},{"ignorePresentation",ignorePresentation}};
}
}
