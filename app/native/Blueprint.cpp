#include "Graph.h"
#include <algorithm>
#include <set>
#include <cmath>
namespace cygame {
std::vector<std::string> blueprintTypes(){return {"float","bool","string","color","int","vector3","object","array.float","array.int","array.string","array.bool","array.object","array.vector3","map.float","map.string","map.object","set.string","set.int"};}
Json graphValueSchema(const std::string& type){
    const auto allowed=blueprintTypes();if(std::find(allowed.begin(),allowed.end(),type)==allowed.end())throw std::runtime_error("Unsupported graph type: "+type);
    if(type=="float")return {{"type","number"},{"minimum",-1e9},{"maximum",1e9}};
    if(type=="int")return {{"type","integer"},{"minimum",-2147483648LL},{"maximum",2147483647}};
    if(type=="bool")return {{"type","boolean"}};
    if(type=="string"||type=="object")return {{"type","string"},{"maxLength",type=="object"?128:4096}};
    if(type=="color"||type=="vector3")return {{"type","array"},{"minItems",3},{"maxItems",3},{"items",{{"type","number"},{"minimum",type=="color"?0:-1e9},{"maximum",type=="color"?1:1e9}}}};
    if(type.rfind("array.",0)==0||type.rfind("set.",0)==0){auto element=type.substr(type.find('.')+1);if(element.find('.')!=std::string::npos)throw std::runtime_error("Nested containers require a declared adapter type");return {{"type","array"},{"maxItems",256},{"x-editor-max-bytes",65536},{"uniqueItems",type.rfind("set.",0)==0},{"items",graphValueSchema(element)}};}
    if(type.rfind("map.",0)==0){auto element=type.substr(4);if(element.find('.')!=std::string::npos)throw std::runtime_error("Nested maps are unsupported");return {{"type","object"},{"maxProperties",256},{"x-editor-max-bytes",65536},{"propertyNames",{{"type","string"},{"maxLength",128}}},{"additionalProperties",graphValueSchema(element)}};}
    throw std::runtime_error("Unsupported graph value type: "+type);
}
Json graphDefaultValue(const std::string& type){if(type=="float")return 0.;if(type=="int")return 0;if(type=="bool")return false;if(type=="string"||type=="object")return "";if(type=="color")return Json::array({.5,.5,.5});if(type=="vector3")return Json::array({0.,0.,0.});if(type.rfind("map.",0)==0)return Json::object();graphValueSchema(type);return Json::array();}
Json graphWidget(const std::string& type){auto spec=graphValueSchema(type);if(type=="color")spec["x-editor-widget"]="color";if(type.find('.')!=std::string::npos)spec["type"]="json";return spec;}
void extendBlueprintNodes(Json& types){
    extendTypeNodes(types);
    auto node=[&](const std::string& id,const std::string& title,Json inputs,Json outputs,Json defaults){types[id]={{"title",title},{"domain","logic"},{"inputs",inputs},{"outputs",outputs},{"defaults",defaults}};};
    const auto all=blueprintTypes();for(const auto& type:all){if(type=="float"||type=="bool"||type=="string"||type=="color")continue;const auto value=graphDefaultValue(type);node("route."+type,"Reroute",{{"value",type}},{{"value",type}},{{"value",value}});node("constant."+type,"Literal "+type,Json::object(),{{"value",type}},{{"value",value}});node("variable.get."+type,"Get "+type,Json::object(),{{"value",type}},Json::object());types["variable.get."+type]["variableType"]=type;node("variable.set."+type,"Set "+type,{{"exec","exec"},{"value",type}},{{"then","exec"},{"value",type}},{{"value",value}});types["variable.set."+type]["variableType"]=type;}
    for(const auto& type:all)if(type.rfind("array.",0)==0||type.rfind("set.",0)==0||type.rfind("map.",0)==0){const auto dot=type.find('.');const auto family=type.substr(0,dot),element=type.substr(dot+1);const auto empty=graphDefaultValue(type),value=graphDefaultValue(element);
        for(const auto* op:{"length","contains","get","set","add","remove"}){const std::string operation=op;if(family=="set"&&(operation=="get"||operation=="set"))continue;
            Json inputs={{"container",type}},outputs={{"value",type}},defaults={{"container",empty}};
            if(operation=="length")outputs["value"]="int";
            else if(operation=="contains")outputs["value"]="bool";
            if(operation!="length"){
                if(family=="map"){inputs["key"]="string";defaults["key"]="";}else if(operation=="get"||operation=="set"||operation=="remove"&&family=="array"){inputs["index"]="int";defaults["index"]=0;}
                if(operation=="add"||operation=="set"||operation=="contains"&&family!="map"||operation=="remove"&&family=="set"){inputs["item"]=element;defaults["item"]=value;}
                if(operation=="get")outputs["value"]=element;
            }
            node(family+"."+operation+"."+element,operation+" "+type,inputs,outputs,defaults);
        }
    }
    node("convert.int_string","Integer to string",{{"value","int"}},{{"value","string"}},{{"value",0}});
    node("vector.add","Add vectors",{{"a","vector3"},{"b","vector3"}},{{"value","vector3"}},{{"a",{0.,0.,0.}},{"b",{0.,0.,0.}}});
    node("vector.dot","Dot product",{{"a","vector3"},{"b","vector3"}},{{"value","float"}},{{"a",{0.,0.,0.}},{"b",{0.,0.,0.}}});
    node("flow.delay","Delay",{{"exec","exec"},{"seconds","float"}},{{"then","exec"}},{{"seconds",.1}});
    node("event.custom","Custom event",{{"name","string"}},{{"then","exec"}},{{"name","CustomEvent"}});
    node("flow.emit","Emit custom event",{{"exec","exec"},{"name","string"}},{{"then","exec"}},{{"name","CustomEvent"}});
    node("object.new","Construct object",{{"exec","exec"},{"class","string"}},{{"then","exec"},{"object","object"}},{{"class","NewClass"}});
    node("object.destroy","Destroy object",{{"exec","exec"},{"object","object"}},{{"then","exec"}},{{"object",""}});
    node("object.valid","Is valid object",{{"object","object"}},{{"value","bool"}},{{"object",""}});
    for(const auto& type:all){node("object.get."+type,"Get field "+type,{{"object","object"},{"field","string"}},{{"value",type}},{{"object",""},{"field","health"}});node("object.set."+type,"Set field "+type,{{"exec","exec"},{"object","object"},{"field","string"},{"value",type}},{{"then","exec"},{"value",type}},{{"object",""},{"field","health"},{"value",graphDefaultValue(type)}});}
    types["constant.color"]["domain"]="both";
    node("flow.foreach","For each (execution function)",{{"exec","exec"},{"values","array.float"}},{{"then","exec"},{"count","int"}},{{"values",Json::array()}});
    types["macro.call"]={{"title","Macro"},{"domain","logic"},{"inputs",Json::object()},{"outputs",Json::object()},{"defaults",Json::object()}};
    types["interface.call"]=types["macro.call"];types["interface.call"]["title"]="Interface call";types["interface.supports"]=types["macro.call"];types["interface.supports"]["title"]="Implements interface";
    types["object.call"]=types["macro.call"];types["object.call"]["title"]="Call method";
}
Json evaluateBlueprintNode(const std::string& type,const Json& values){
    if(type=="convert.int_string")return {{"value",std::to_string(values.at("value").get<int>())}};
    if(type=="vector.add"||type=="vector.dot"){Json v=Json::array();double sum=0;for(int i=0;i<3;++i){double a=values.at("a")[i],b=values.at("b")[i];v.push_back(a+b);sum+=a*b;}return {{"value",type=="vector.dot"?Json(sum):v}};}
    auto first=type.find('.'),last=type.rfind('.');if(first==last)throw std::runtime_error("Unsupported node evaluator: "+type);const auto family=type.substr(0,first),op=type.substr(first+1,last-first-1);if(family!="array"&&family!="set"&&family!="map")throw std::runtime_error("Unsupported node evaluator: "+type);
    Json container=values.at("container"),result=container;
    if(op=="length")result=container.size();
    else if(family=="map"){const std::string key=values.at("key");if(op=="contains")result=container.contains(key);else if(op=="get"){if(!container.contains(key))throw std::runtime_error("Map key is missing");result=container.at(key);}else if(op=="remove"){result.erase(key);}else result[key]=values.at("item");}
    else if(op=="contains")result=std::find(container.begin(),container.end(),values.at("item"))!=container.end();
    else if(op=="add"){if(family!="set"||std::find(result.begin(),result.end(),values.at("item"))==result.end())result.push_back(values.at("item"));}
    else if(op=="remove"&&family=="set"){result.erase(std::remove(result.begin(),result.end(),values.at("item")),result.end());}
    else{int index=values.at("index");if(index<0||index>=int(container.size()))throw std::runtime_error("Array index is outside bounds");if(op=="get")result=container[index];else if(op=="set")result[index]=values.at("item");else if(op=="remove")result.erase(result.begin()+index);}
    return {{"value",result}};
}
Json blueprintClass(const Json& classes,const std::string& name,const Json& types){
    std::set<std::string> seen;std::function<Json(const std::string&)> resolve=[&](const std::string& key){if(!classes.contains(key)||!seen.insert(key).second||seen.size()>16)throw std::runtime_error("Missing, cyclic or too-deep class inheritance");const auto& c=classes.at(key);const auto methods=c.value("methods",Json::object());if(!methods.is_object()||methods.size()>64)throw std::runtime_error("Class method limit is 64");auto result=c.value("parent","").empty()?Json{{"fields",Json::object()},{"methods",Json::object()}}:resolve(c.at("parent"));if(!result.contains("interfaces"))result["interfaces"]=Json::array();const auto implemented=c.value("interfaces",Json::array());if(!implemented.is_array()||implemented.size()>32)throw std::runtime_error("A class supports at most 32 interfaces");std::set<std::string> direct;for(const auto& v:implemented){const std::string id=v;if(!direct.insert(id).second)throw std::runtime_error("Duplicate implemented interface");if(std::find(result["interfaces"].begin(),result["interfaces"].end(),v)==result["interfaces"].end())result["interfaces"].push_back(v);}if(result["interfaces"].size()>32)throw std::runtime_error("Inherited interface limit is 32");for(const auto& [field,spec]:c.at("fields").items()){if(field.empty()||field.size()>64)throw std::runtime_error("Invalid class field name");if(result["fields"].contains(field)&&result["fields"][field]["type"]!=spec.at("type"))throw std::runtime_error("Inherited field type cannot change");validateValue(spec.at("default"),graphValueSchema(spec.at("type"),types),field);result["fields"][field]=spec;}for(const auto& [method,function]:methods.items())result["methods"][method]=function;if(result["fields"].size()>64)throw std::runtime_error("Class field limit is 64");return result;};return resolve(name);
}
void validateBlueprint(const Json& graph){
    const auto types=graph.value("types",Json::object());validateGraphTypes(types);if(graph.at("domain")!="logic"&&graph.at("domain")!="function"&&!types.empty())throw std::runtime_error("Named data types belong to logic assets");
    const auto classes=graph.value("classes",Json::object());if(!classes.is_object()||classes.size()>64)throw std::runtime_error("Class limit is 64");if(graph.at("domain")=="material"&&!classes.empty())throw std::runtime_error("Classes belong to logic assets");
    for(const auto& [name,c]:classes.items()){const auto methods=c.value("methods",Json::object());
        if(!c.value("parent","").empty()){auto parent=blueprintClass(classes,c.at("parent"),types);for(const auto& [method,function]:methods.items())if(parent["methods"].contains(method)){const auto& a=graph.at("functions").at(parent["methods"][method].get<std::string>());const auto& b=graph.at("functions").at(function.get<std::string>());if(a.value("mode","pure")!=b.value("mode","pure"))throw std::runtime_error("Method override mode mismatch");for(const auto* side:{"inputs","outputs"}){if(a.at(side).size()!=b.at(side).size())throw std::runtime_error("Method override signature mismatch");for(const auto& [pin,spec]:a.at(side).items())if(!b.at(side).contains(pin)||spec.at("type")!=b.at(side).at(pin).at("type"))throw std::runtime_error("Method override pin type mismatch");}}}
if(name.empty()||name.size()>64||!c.at("fields").is_object())throw std::runtime_error("Invalid class definition");auto definition=blueprintClass(classes,name,types);for(const auto& [method,function]:definition["methods"].items()){const auto& f=graph.at("functions").at(function.get<std::string>());if(f.value("kind","")=="macro"||!f.at("inputs").contains("self")||f["inputs"]["self"]["type"]!="object")throw std::runtime_error("A method needs a function with an object self input");}}
    validateGraphInterfaces(graph);
}
Json blueprintCommand(Json& graph,const std::string& id,const Json& p){
    if(id=="graph_class_define"){const std::string name=p.at("name");if(!graph.contains("classes"))graph["classes"]=Json::object();auto next=graph["classes"].value(name,Json::object());next["parent"]=p.value("parent","");next["fields"]=p.at("fields");next["methods"]=p.value("methods",Json::object());if(p.contains("interfaces"))next["interfaces"]=p.at("interfaces");graph["classes"][name]=next;}
    else if(id=="graph_class_remove")graph.at("classes").erase(p.at("name").get<std::string>());
    else throw std::runtime_error("Unknown Blueprint operation");
    validateGraph(graph);return {{"classes",graph.value("classes",Json::object())}};
}
}
