#include "Graph.h"
#include <algorithm>
#include <cctype>
#include <set>
namespace cygame {
namespace {
void need(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void identifier(const std::string& name){need(!name.empty()&&name.size()<=64&&(std::isalpha(static_cast<unsigned char>(name[0]))||name[0]=='_'),"Invalid type/member identifier");for(unsigned char c:name)need(std::isalnum(c)||c=='_',"Invalid type/member identifier");}
Json schema(const std::string& type,const Json& types,std::set<std::string>& visiting,int depth,int& count){
    need(++count<=512&&depth<=8,"Type expansion exceeds 512 fields or eight levels");
    if(!isNamedGraphType(type))return graphValueSchema(type);
    need(types.is_object()&&types.contains(type),"Named graph type is not declared");need(visiting.insert(type).second,"Recursive structures are unsupported");identifier(type.substr(type.find('.')+1));const auto& d=types.at(type);need(d.is_object(),"Invalid graph type declaration");Json result;
    if(type.rfind("enum.",0)==0){need(d.at("kind")=="enum","Type ID and declaration kind disagree");const auto& members=d.at("members");need(members.is_array()&&!members.empty()&&members.size()<=64,"Enums require 1..64 members");std::set<std::string> names;for(const auto& member:members){need(member.is_string(),"Enum members must be identifiers");const std::string name=member;identifier(name);need(names.insert(name).second,"Enum members must be unique");}need(d.at("default").is_string()&&names.count(d.at("default").get<std::string>()),"Enum default must be a declared member");result={{"type","string"},{"enum",members}};
    }else{need(d.at("kind")=="struct","Type ID and declaration kind disagree");const auto& fields=d.at("fields");need(fields.is_object()&&!fields.empty()&&fields.size()<=32,"Structures require 1..32 fields");Json properties=Json::object(),required=Json::array();for(const auto& [name,field]:fields.items()){identifier(name);need(field.is_object()&&field.at("type").is_string()&&field.contains("default"),"Fields require a type and default");properties[name]=schema(field.at("type"),types,visiting,depth+1,count);validateValue(field.at("default"),properties[name],name);required.push_back(name);}result={{"type","object"},{"properties",properties},{"required",required},{"additionalProperties",false},{"maxProperties",32},{"x-editor-max-bytes",65536},{"x-editor-widget","record"}};}
    visiting.erase(type);result["x-editor-nominalType"]=type;return result;
}
Json definition(const char* title,Json inputs,Json outputs,Json defaults){return {{"title",title},{"domain","logic"},{"inputs",inputs},{"outputs",outputs},{"defaults",defaults}};}
}
bool isNamedGraphType(const std::string& type){return type.rfind("enum.",0)==0||type.rfind("struct.",0)==0;}
std::vector<std::string> blueprintTypes(const Json& types){auto result=blueprintTypes();for(const auto& [id,d]:types.items())result.push_back(id);return result;}
Json graphValueSchema(const std::string& type,const Json& types){std::set<std::string> visiting;int count=0;auto result=schema(type,types,visiting,0,count);need(result.dump().size()<=128*1024,"Expanded type schema exceeds 128 KiB");return result;}
Json graphDefaultValue(const std::string& type,const Json& types){if(!isNamedGraphType(type))return graphDefaultValue(type);graphValueSchema(type,types);const auto& d=types.at(type);if(d.at("kind")=="enum")return d.at("default");Json result=Json::object();for(const auto& [name,field]:d.at("fields").items())result[name]=field.at("default");return result;}
Json graphWidget(const std::string& type,const Json& types){if(!isNamedGraphType(type))return graphWidget(type);auto spec=graphValueSchema(type,types);if(type.rfind("struct.",0)==0)for(const auto& [name,field]:types.at(type).at("fields").items())spec["properties"][name]=graphWidget(field.at("type"),types);return spec;}
void validateGraphTypes(const Json& types){need(types.is_object()&&types.size()<=64,"Graph type library limit is 64");size_t bytes=0;for(const auto& [id,d]:types.items()){need(isNamedGraphType(id),"Type IDs must begin with enum. or struct.");auto spec=graphValueSchema(id,types);bytes+=spec.dump().size();need(bytes<=2*1024*1024,"Expanded type library schemas exceed 2 MiB");validateValue(graphDefaultValue(id,types),spec,id);}}
void extendTypeNodes(Json& types){for(const auto* id:{"typed.literal","typed.get","typed.set","route.typed","struct.make","struct.break","struct.set","enum.equal","enum.to_string","enum.switch","object.get.typed","object.set.typed"})types[id]=definition("Named type node",Json::object(),Json::object(),Json::object());}
Json typedNodeDefinition(const Json& graph,const Json& n){
    const std::string op=n.at("type");if(op!="typed.literal"&&op!="typed.get"&&op!="typed.set"&&op!="route.typed"&&op.rfind("struct.",0)!=0&&op.rfind("enum.",0)!=0&&op!="object.get.typed"&&op!="object.set.typed")return Json();
    const std::string id=n.at("typeRef");need(isNamedGraphType(id),"This node requires a named enum or structure");const auto types=graph.value("types",Json::object());const auto value=graphDefaultValue(id,types);Json d;
    if(op=="typed.literal")d=definition("Named literal",Json::object(),{{"value",id}},{{"value",value}});
    else if(op=="route.typed")d=definition("Named reroute",{{"value",id}},{{"value",id}},{{"value",value}});
    else if(op=="typed.get"||op=="typed.set"){bool set=op=="typed.set";d=definition(set?"Set named variable":"Get named variable",set?Json{{"exec","exec"},{"value",id}}:Json::object(),set?Json{{"then","exec"},{"value",id}}:Json{{"value",id}},set?Json{{"value",value}}:Json::object());d["variableType"]=id;}
    else if(op=="object.get.typed"||op=="object.set.typed"){bool set=op=="object.set.typed";Json inputs={{"object","object"},{"field","string"}},defaults={{"object",""},{"field","data"}},outputs={{"value",id}};if(set){inputs["exec"]="exec";inputs["value"]=id;defaults["value"]=value;outputs["then"]="exec";}d=definition(set?"Set named field":"Get named field",inputs,outputs,defaults);}
    else if(op.rfind("struct.",0)==0){need(id.rfind("struct.",0)==0,"Structure node requires a structure type");const auto& fields=types.at(id).at("fields");Json pins=Json::object();for(const auto& [name,field]:fields.items())pins[name]=field.at("type");if(op=="struct.make")d=definition("Make structure",pins,{{"value",id}},value);
        else if(op=="struct.break")d=definition("Break structure",{{"value",id}},pins,{{"value",value}});
        else{const std::string field=n.at("field");need(fields.contains(field),"Structure field is not declared");d=definition("Set structure field",{{"value",id},{"item",fields.at(field).at("type")}},{{"value",id}},{{"value",value},{"item",fields.at(field).at("default")}});}
    }else{need(id.rfind("enum.",0)==0,"Enum node requires an enum type");if(op=="enum.equal")d=definition("Equal enum",{{"a",id},{"b",id}},{{"value","bool"}},{{"a",value},{"b",value}});else if(op=="enum.to_string")d=definition("Enum to string",{{"value",id}},{{"value","string"}},{{"value",value}});else{Json outputs=Json::object();for(const auto& member:types.at(id).at("members"))outputs[member.get<std::string>()]="exec";d=definition("Switch on enum",{{"exec","exec"},{"value",id}},outputs,{{"value",value}});}}
    d["dataType"]=id;return d;
}
Json evaluateTypedNode(const Json& n,const Json& values){const std::string op=n.at("type");if(op=="typed.literal"||op=="route.typed"||op=="enum.to_string")return {{"value",values.at("value")}};if(op=="enum.equal")return {{"value",values.at("a")==values.at("b")}};if(op=="struct.make")return {{"value",values}};if(op=="struct.break")return values.at("value");if(op=="struct.set"){auto result=values.at("value");result[n.at("field").get<std::string>()]=values.at("item");return {{"value",result}};}throw std::runtime_error("Named node needs a runtime binding");}
Json graphTypeCommand(Json& graph,const std::string& op,const Json& p){
    auto describe=[&](){const auto types=graph.value("types",Json::object());Json schemas=Json::object(),defaults=Json::object();for(const auto& [id,d]:types.items()){schemas[id]=graphValueSchema(id,types);defaults[id]=graphDefaultValue(id,types);}return Json{{"types",types},{"schemas",schemas},{"defaults",defaults},{"supportedTypes",blueprintTypes(types)}};};
    if(op=="graph_types_get")return describe();need(graph.at("domain")=="logic","Named data types belong to logic assets");const std::string id=p.at("typeId");if(!graph.contains("types"))graph["types"]=Json::object();if(op=="graph_type_define")graph["types"][id]=p.at("definition");else{need(graph["types"].contains(id),"Type is not declared");graph["types"].erase(id);}validateGraph(graph);return describe();
}
}
