#pragma once
#include "Core.h"
#include <set>
#include <stdexcept>

namespace cygame::examples {
// Source-only starter; not selected by the shipped editor and not a game decoder.
// It retains the current transform fields and the initial "hero" selection ID.
class ExampleGameAdapter final : public IGameAdapter {
public:
    Json descriptor() const override {
        return {{"id","example.game"},{"name","Example Game"},{"version","0.1.0"},
            {"capabilities",{{"scene",true},{"properties",true},{"preview",false},
                             {"build",false},{"package",false},{"play",false}}}};
    }
    Json propertySchema(const Json&) const override {
        const Json vector={{"type","array"},{"minItems",3},{"maxItems",3},
            {"items",{{"type","number"},{"minimum",-10000},{"maximum",10000}}},
            {"x-editor-widget","vector3"}};
        Json fields={
            {"name",{{"type","string"},{"minLength",1},{"maxLength",128},{"title","Name"}}},
            {"enabled",{{"type","boolean"},{"title","Enabled"}}},
            {"mobility",{{"type","string"},{"enum",{"Static","Movable"}},{"title","Mobility"}}},
            {"position",vector},{"rotation",vector},{"scale",vector},
            {"tint",{{"type","array"},{"minItems",3},{"maxItems",3},
                {"items",{{"type","number"},{"minimum",0},{"maximum",1}}},
                {"title","Tint"},{"x-editor-widget","color"}}},
            {"health",{{"type","integer"},{"minimum",0},{"maximum",100},{"title","Health"}}}
        };
        fields["position"]["title"]="Position"; fields["rotation"]["title"]="Rotation";
        fields["scale"]["title"]="Scale"; fields["scale"]["items"]["minimum"]=0.01;
        return {{"type","object"},{"properties",fields},
            {"required",{"name","enabled","mobility","position","rotation","scale","tint","health"}},
            {"additionalProperties",false}};
    }
    Json createEntity(const std::string& id,const std::string& name) const override {
        return {{"id",id},{"type","StaticMesh"},{"properties",{
            {"name",name},{"enabled",true},{"mobility","Static"},
            {"position",{0.,0.,0.5}},{"rotation",{0.,0.,0.}},{"scale",{1.,1.,1.}},
            {"tint",{0.47,0.52,0.6}},{"health",100}}}};
    }
    Json initialScene() const override { return Json::array({createEntity("hero","Example Actor")}); }
    Json validateScene(const Json& entities) const override {
        if(!entities.is_array()||entities.size()>2000)throw std::runtime_error("Invalid example scene size");
        std::set<std::string> ids;
        for(const auto& e:entities){
            if(!e.is_object()||!e.contains("id")||!e.at("id").is_string()
                ||e.value("type","")!="StaticMesh"||!e.contains("properties"))
                throw std::runtime_error("Invalid example entity");
            const auto id=e.at("id").get<std::string>();
            if(id.empty()||!ids.insert(id).second)throw std::runtime_error("Missing or duplicate example entity ID");
            validateValue(e.at("properties"),propertySchema(e),id);
        }
        return {{"valid",true},{"entityCount",entities.size()},{"adapterId","example.game"}};
    }
    // buildTargets() stays empty until a real game provider is implemented.
};
}
