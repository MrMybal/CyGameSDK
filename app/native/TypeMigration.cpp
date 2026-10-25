#include "Graph.h"
#include <algorithm>
#include <map>
#include <set>
namespace cygame {
namespace {
using Names=std::map<std::string,std::string>;
std::string mapped(const Names& names,const std::string& old){const auto it=names.find(old);return it==names.end()?old:it->second;}
class TypeMigration {
    const Json& source_;Json next_,types_,shape_,report_,summary_;std::string target_;Names names_;std::map<std::string,Json> neutral_,defaults_,fieldDefaults_,schemas_;std::map<std::string,bool> dependencies_;std::set<std::string> visiting_;size_t detailBytes_=0,changeCount_=0;
    void count(const char* key){summary_[key]=summary_[key].get<int>()+1;}
    void record(Json detail){++changeCount_;const auto size=detail.dump().size();if(report_["changes"].size()<512&&detailBytes_+size<=256*1024){report_["changes"].push_back(detail);detailBytes_+=size;}}
    void loss(const char* kind,const char* key,const std::string& path,const Json& old){count(key);Json detail={{"kind",kind},{"declaration",path},{"oldValueBytes",old.dump().size()}};auto bytes=old.dump();if(bytes.size()<=1024)detail["oldValue"]=old;else{size_t end=256;while(end>0&&(static_cast<unsigned char>(bytes[end])&0xc0)==0x80)--end;detail["oldValuePreview"]=bytes.substr(0,end)+"...";}record(detail);}
    const Json& schema(const std::string& type){if(!schemas_.count(type))schemas_[type]=graphValueSchema(type,shape_);return schemas_.at(type);}
    void bounded(const Json& value,const std::string& type){validateValue(value,schema(type),type);}
    Json neutral(const std::string& type,int depth=0){
        if(!isNamedGraphType(type))return graphDefaultValue(type);if(neutral_.count(type))return neutral_.at(type);
        if(depth>8||!shape_.contains(type)||!visiting_.insert(type).second)throw std::runtime_error("Invalid, cyclic or too-deep migrated type");
        const auto definition=shape_.at(type);Json value;
        if(type.rfind("enum.",0)==0)value=definition.at("default");else{value=Json::object();for(const auto& [field,spec]:definition.at("fields").items()){auto d=neutral(spec.at("type"),depth+1);shape_[type]["fields"][field]["default"]=d;value[field]=d;}}
        visiting_.erase(type);neutral_[type]=value;return value;
    }
    bool depends(const std::string& type){
        if(type==target_)return true;if(!isNamedGraphType(type)||type.rfind("enum.",0)==0)return false;if(dependencies_.count(type))return dependencies_.at(type);bool found=false;
        for(const auto& [key,field]:types_.at(type).at("fields").items())found|=depends(field.at("type"));dependencies_[type]=found;return found;
    }
    Json fieldDefault(const std::string& owner,const std::string& field){
        const auto key=owner+"/"+field;if(fieldDefaults_.count(key))return fieldDefaults_.at(key);const auto spec=types_.at(owner).at("fields").at(field);Json value=spec.at("default");
        if(owner!=target_&&depends(spec.at("type")))value=convert(spec.at("type"),spec.at("type"),value,Json(),"types/"+owner+"/fields/"+field+"/default");
        bounded(value,spec.at("type"));fieldDefaults_[key]=value;return value;
    }
    Json defaultValue(const std::string& type){
        if(!isNamedGraphType(type))return graphDefaultValue(type);if(defaults_.count(type))return defaults_.at(type);const auto definition=types_.at(type);Json value;
        if(type.rfind("enum.",0)==0)value=definition.at("default");else{value=Json::object();for(const auto& [key,field]:definition.at("fields").items()){value[key]=fieldDefault(type,key);if(value.dump().size()>65536)throw std::runtime_error("Migrated record default exceeds 64 KiB");}}
        bounded(value,type);defaults_[type]=value;return value;
    }
    Json convert(const std::string& oldType,const std::string& newType,const Json& old,const Json& fallback,const std::string& path){
        if(oldType!=newType){auto value=fallback.is_null()?defaultValue(newType):fallback;bounded(value,newType);loss("Value reset","valuesReset",path,old);return value;}
        if(!depends(newType)){bounded(old,newType);count("valuesPreserved");return old;}
        Json value;
        if(newType.rfind("enum.",0)==0){const auto next=mapped(names_,old.get<std::string>());const auto& members=types_.at(newType).at("members");if(!next.empty()&&std::find(members.begin(),members.end(),Json(next))!=members.end())value=next;else{value=defaultValue(newType);loss("Value reset","valuesReset",path,old);}}
        else{
            const auto before=source_.at("types").at(oldType).at("fields"),after=types_.at(newType).at("fields");value=Json::object();std::set<std::string> kept;
            for(const auto& [name,field]:after.items()){
                std::string origin=name;if(newType==target_){origin.clear();for(const auto& [from,to]:names_)if(to==name)origin=from;}
                if(!origin.empty()&&before.contains(origin)){kept.insert(origin);value[name]=convert(before.at(origin).at("type"),field.at("type"),old.at(origin),fieldDefault(newType,name),path+"/"+name);}
                else{value[name]=fieldDefault(newType,name);count("valuesAdded");record({{"kind","Value added"},{"declaration",path+"/"+name}});}
                if(value.dump().size()>65536)throw std::runtime_error("Migrated record value exceeds 64 KiB");
            }
            for(const auto& [name,field]:before.items())if(!kept.count(name))loss("Value removed","valuesRemoved",path+"/"+name,old.at(name));
        }
        bounded(value,newType);if(value!=old){count("valuesMigrated");record({{"kind","Value migrated"},{"declaration",path}});}else count("valuesPreserved");return value;
    }
    void declaration(Json& spec,const std::string& path){
        const auto type=spec.at("type").get<std::string>();if(!depends(type))return;const auto before=spec;spec["default"]=convert(type,type,spec.at("default"),Json(),path+"/default");
        if(spec!=before){count("declarations");record({{"kind","Declaration updated"},{"declaration",path}});}
    }
    void body(const std::string& scope){
        const auto oldView=graphView(source_,scope);auto& body=scope.empty()?next_:next_["functions"][scope];const auto before=body;
        if(body.contains("variables"))for(auto& [name,spec]:body["variables"].items())declaration(spec,scope+"/variables/"+name);

        std::map<std::string,Names> inputMaps,outputMaps;
        for(auto& node:body["nodes"]){const std::string type=node.at("type"),id=node.at("id");if(node.value("typeRef","")!=target_)continue;
            if(type=="struct.make")inputMaps[id]=names_;if(type=="struct.break"||type=="enum.switch")outputMaps[id]=names_;
            if(type=="struct.set"){const auto field=mapped(names_,node.at("field"));if(field.empty())throw std::runtime_error("A removed field still has a Set structure field node; remove or rebind that node first");node["field"]=field;}
        }
        const auto newView=graphView(next_,scope);std::map<std::string,Json> specifications;
        for(auto& node:body["nodes"]){const std::string id=node.at("id");const auto oldNode=std::find_if(oldView.at("nodes").begin(),oldView.at("nodes").end(),[&](const Json& n){return n.at("id")==id;});const auto oldSpec=nodeDefinition(oldView,*oldNode),newSpec=nodeDefinition(newView,node);specifications[id]=newSpec;Json values=newSpec.at("defaults");std::set<std::string> retained;bool affected=depends(node.value("typeRef",std::string()));
            for(const auto& [key,old]:oldNode->at("values").items()){
                const auto name=inputMaps.count(id)?mapped(inputMaps[id],key):key;if(name.empty()||!values.contains(name)){loss("Value removed","valuesRemoved",scope+"/nodes/"+id+"/"+key,old);affected=true;continue;}
                const std::string oldType=oldSpec.at("inputs").contains(key)?oldSpec.at("inputs").at(key):oldSpec.at("outputs").at(key),newType=newSpec.at("inputs").contains(name)?newSpec.at("inputs").at(name):newSpec.at("outputs").at(name);
                if(depends(oldType)||depends(newType)||oldType!=newType){values[name]=convert(oldType,newType,old,values.at(name),scope+"/nodes/"+id+"/"+name);affected=true;}else values[name]=old;
                retained.insert(name);
            }
            for(const auto& [key,v]:values.items())if(!retained.count(key)){count("valuesAdded");affected=true;}node["values"]=values;if(affected)count("nodes");
        }
        Json links=Json::array();for(auto link:body.at("links")){
            const std::string a=link.at("fromNode"),b=link.at("toNode"),fp=link.at("fromPin"),tp=link.at("toPin");const auto from=outputMaps.count(a)?mapped(outputMaps[a],fp):fp,to=inputMaps.count(b)?mapped(inputMaps[b],tp):tp;
            const bool valid=!from.empty()&&!to.empty()&&specifications.at(a).at("outputs").contains(from)&&specifications.at(b).at("inputs").contains(to)&&specifications.at(a).at("outputs").at(from)==specifications.at(b).at("inputs").at(to);
            if(!valid){count("linksRemoved");record({{"kind","Link removed"},{"scope",scope},{"link",link}});continue;}
            if(from!=fp||to!=tp){count("linksRenamed");record({{"kind","Link renamed"},{"scope",scope},{"linkId",link.at("id")},{"fromPin",fp},{"toPin",tp},{"newFromPin",from},{"newToPin",to}});}link["fromPin"]=from;link["toPin"]=to;links.push_back(link);
        }body["links"]=links;if(!scope.empty()&&before!=body&&std::find(report_["targets"]["functions"].begin(),report_["targets"]["functions"].end(),Json(scope))==report_["targets"]["functions"].end())report_["targets"]["functions"].push_back(scope);
    }
public:
    TypeMigration(const Json& source,const Json& p):source_(source),next_(source),types_(source.at("types")),target_(p.at("typeId")){
        validateGraph(source);if(source.at("domain")!="logic")throw std::runtime_error("Type migrations belong to a logic owner");const auto old=types_.at(target_),request=p.at("definition");if(request.at("kind")!=old.at("kind"))throw std::runtime_error("Type migration preserves the nominal identity and kind");if(request.dump().size()>1024*1024)throw std::runtime_error("Type definition exceeds 1 MiB");
        for(const auto& [key,value]:request.items())types_[target_][key]=value;
        const bool recordType=old.at("kind")=="struct";Json oldNames=Json::object(),newNames=Json::object();if(recordType){oldNames=old.at("fields");newNames=request.at("fields");}else{for(const auto& member:old.at("members"))oldNames[member.get<std::string>()]=true;for(const auto& member:request.at("members"))newNames[member.get<std::string>()]=true;}
        const auto renames=p.value("renames",Json::object());if(!renames.is_object()||renames.size()>64)throw std::runtime_error("Invalid type member rename map");for(const auto& [from,to]:renames.items())if(!to.is_string()||!oldNames.contains(from)||!newNames.contains(to.get<std::string>()))throw std::runtime_error("Renames require existing members and declared destinations");
        std::set<std::string> destinations;for(const auto& [from,value]:oldNames.items()){const auto to=renames.contains(from)?renames.at(from).get<std::string>():newNames.contains(from)?from:"";if(!to.empty()&&!destinations.insert(to).second)throw std::runtime_error("Type member renames must be one-to-one");names_[from]=to;}
        if(recordType)for(const auto& [from,to]:names_)if(!to.empty()){const auto incoming=types_[target_]["fields"].at(to);auto merged=old["fields"].at(from);for(const auto& [key,value]:incoming.items())merged[key]=value;types_[target_]["fields"][to]=merged;}
        shape_=types_;for(const auto& [id,d]:types_.items())neutral(id);validateGraphTypes(shape_);
        report_={{"targets",{{"functions",Json::array()},{"interfaces",Json::array()},{"types",Json::array()},{"classes",Json::array()}}},{"changes",Json::array()}};
        summary_={{"declarations",0},{"nodes",0},{"linksRenamed",0},{"linksRemoved",0},{"valuesPreserved",0},{"valuesAdded",0},{"valuesRemoved",0},{"valuesReset",0},{"valuesMigrated",0}};
    }
    Json prepare(const Json& p){
        try{
            for(auto& [id,d]:types_.items())if(d.at("kind")=="struct")for(auto& [key,field]:d["fields"].items())field["default"]=fieldDefault(id,key);
            validateGraphTypes(types_);for(const auto& [id,d]:types_.items())if(d!=source_.at("types").at(id)){count("declarations");report_["targets"]["types"].push_back(id);record({{"kind","Declaration updated"},{"declaration","types/"+id},{"before",source_.at("types").at(id)},{"after",d}});}next_["types"]=types_;
            if(next_.contains("classes"))for(auto& [name,c]:next_["classes"].items()){const auto before=c;for(auto& [field,spec]:c["fields"].items())declaration(spec,"classes/"+name+"/fields/"+field);if(c!=before)report_["targets"]["classes"].push_back(name);}
            if(next_.contains("interfaces"))for(auto& [name,d]:next_["interfaces"].items())for(auto& [method,definition]:d["methods"].items()){const auto before=definition;for(const auto* side:{"inputs","outputs"})for(auto& [pin,spec]:definition[side].items())declaration(spec,"interfaces/"+name+"/"+method+"/"+side+"/"+pin);if(before!=definition)report_["targets"]["interfaces"].push_back({{"name",name},{"method",method}});}
            if(next_.contains("functions"))for(auto& [name,function]:next_["functions"].items()){const auto before=function;for(const auto* side:{"inputs","outputs"})for(auto& [pin,spec]:function[side].items())declaration(spec,"functions/"+name+"/"+side+"/"+pin);if(before!=function)report_["targets"]["functions"].push_back(name);}
            body("");const auto functions=source_.value("functions",Json::object());for(const auto& [name,f]:functions.items())body(name);validateGraph(next_);report_["valid"]=true;
        }catch(const std::exception& e){report_["valid"]=false;report_["diagnostic"]=e.what();}
        report_["summary"]=summary_;report_["requiresLossAcceptance"]=summary_["linksRemoved"].get<int>()+summary_["valuesRemoved"].get<int>()+summary_["valuesReset"].get<int>()>0;report_["changeCount"]=changeCount_;report_["truncated"]=changeCount_>report_["changes"].size();
        const auto request=Json{{"typeId",target_},{"definition",p.at("definition")},{"renames",p.value("renames",Json::object())}};report_["sourceHash"]=sha256(source_.dump());report_["planHash"]=sha256(Json{{"sourceHash",report_["sourceHash"]},{"request",request},{"operation","type-migration-v1"}}.dump());return report_;
    }
    Json take(){return std::move(next_);}
};
}
Json graphTypeMigrationCommand(Json& graph,const std::string& operation,const Json& p){
    TypeMigration migration(graph,p);auto report=migration.prepare(p);if(operation=="graph_type_migration_plan")return report;
    if(operation!="graph_type_migration_apply")throw std::runtime_error("Unknown type migration operation");if(report.at("planHash")!=p.at("planHash"))throw std::runtime_error("Type migration preview is stale or the request changed; preview again");
    if(!report.at("valid").get<bool>())throw std::runtime_error(report.value("diagnostic","Invalid type migration"));if(report.at("requiresLossAcceptance").get<bool>()&&!p.value("acceptLoss",false))throw std::runtime_error("Type migration removes or resets values/connections; explicitly accept the listed losses");
    graph=migration.take();report["applied"]=true;return report;
}
}
