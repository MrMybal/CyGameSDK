#include "Documentation.h"
#include "../third_party/cydoc/cydoc_link.hpp"
#include <map>
namespace cygame {
HelpTopic documentationTopic(const std::string& panel){
    static const std::map<std::string,HelpTopic> topics={
        {"interface",{"overview","getting-started"}},{"viewport",{"workspace","layouts"}},
        {"outliner",{"workspace","layouts"}},{"details",{"quickstart","open"}},
        {"content",{"content-browser","browse"}},{"asset",{"content-browser","files"}},
        {"references",{"content-browser","references"}},{"trash",{"asset-import","recovery"}},
        {"imports",{"asset-import","import"}},{"logic",{"graphs","canvas"}},
        {"material",{"materials","nodes"}},{"mygraph",{"functions","definitions"}},
        {"functions",{"functions","definitions"}},{"types",{"named-types","declare"}},
        {"interfaces",{"interfaces","contracts"}},{"blueprints",{"runtime-objects","ownership"}},
        {"debugger",{"debugger","stepping"}},{"graphDiagnostics",{"graphs","connections"}},
        {"navigator",{"debugger","navigation"}},{"changes",{"debugger","compare"}},
        {"signatureMigration",{"signature-migration","preview"}},{"typeMigration",{"type-migration","preview"}},
        {"mesh",{"assets-3d","edit"}},{"compiler",{"materials","compile"}},
        {"codecs",{"game-codecs","registry"}},{"build",{"build-providers","targets"}},
        {"operations",{"operations","catalogue"}},{"modules",{"extensions","contracts"}},
        {"options",{"workspace","layouts"}},{"history",{"quickstart","open"}},
        {"log",{"troubleshooting","launch"}},{"diagnostics",{"troubleshooting","missing"}}
    };
    if(panel.rfind("texture:",0)==0)return {"images","view"};
    if(panel.rfind("model:",0)==0)return {"assets-3d","edit"};
    if(panel.rfind("json:",0)==0)return {"json","edit"};
    if(panel.rfind("fn:",0)==0)return {"functions","definitions"};
    if(panel.rfind("graph:",0)==0)return {"graphs","canvas"};
    if(panel.rfind("viewport.",0)==0)return {"workspace","layouts"};
    const auto it=topics.find(panel);return it==topics.end()?HelpTopic{"overview","getting-started"}:it->second;
}
std::string documentationUrl(const std::filesystem::path& root,const std::string& locale,const HelpTopic& topic){
    // std::filesystem resolves the host path. Encode UTF-8 path components exactly
    // once; a literal #, %, space or non-ASCII filename must never become a route.
    const auto path=std::filesystem::absolute(root/"Documentation/Documentation.html").lexically_normal().generic_u8string();
    std::string base=path.rfind("//",0)==0?"file:":"file:///";
    size_t start=0;while(start<path.size()){
        const auto end=path.find('/',start);const auto part=path.substr(start,end==std::string::npos?end:end-start);
        if(start==0&&part.size()==2&&part[1]==':')base+=part;
        else base+=cydoc_native::encode(part);
        if(end==std::string::npos)break;base+='/';start=end+1;
    }
    return cydoc_native::link(base,locale=="fr"?"fr":"en",topic.page,topic.section);
}
}
