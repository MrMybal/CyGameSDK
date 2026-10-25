#include "Core.h"
#include <fstream>
#include "Graph.h"
#include "Ipc.h"
#include <windows.h>
#include <iostream>
using namespace cygame;
void expect(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class F>void rejects(F fn){bool failed=false;try{fn();}catch(...){failed=true;}expect(failed,"Expected rejection");}
// A source adapter can opt out, or declare its own targets without ImGui.
class ContractAdapter final:public IGameAdapter {
public:
    bool advertise=false,sceneValid=true;
    Json descriptor()const override{return {{"id","contract.adapter"}};}
    Json propertySchema(const Json&)const override{return {{"type","object"}};}
    Json createEntity(const std::string&,const std::string&)const override{return Json::object();}
    Json initialScene()const override{return Json::array();}
    Json validateScene(const Json&)const override{return {{"valid",sceneValid}};}
    Json buildTargets()const override{
        if(!advertise)return IGameAdapter::buildTargets();
        return Json::array({{{"id","custom.export"},{"name","Custom target"},{"providerId","custom.provider"},{"configurations",{"Test"}},
            {"optionsSchema",{{"type","object"},{"properties",{{"quality",{{"type","integer"},{"minimum",1},{"maximum",3},{"default",2}}}}},{"additionalProperties",false}}}}});
    }
};
int main(int argc,char** argv){
    try{
        if(argc==4&&std::string(argv[1])=="--serve"){
            EditorCore core(fs::u8path(argv[2]));EditorIpc ipc(core);
            const auto start=GetTickCount64(),duration=static_cast<ULONGLONG>(std::stoi(argv[3]))*1000;
            while(GetTickCount64()-start<duration){ipc.poll();Sleep(2);}return 0;
        }
        if(argc!=2)return 2;
        EditorCore c(fs::u8path(argv[1]));
        auto original=c.entities();
        c.execute("property_set",{{"entityId","hero"},{"propertyId","position"},{"value",{2.,3.,4.}}});
        expect(c.entity("hero")["properties"]["position"][0]==2.,"Property not applied");
        c.execute("edit_undo");expect(c.entities()==original,"Undo failed");
        c.execute("edit_redo");expect(c.entity("hero")["properties"]["position"][0]==2.,"Redo failed");
        auto before=c.entities();
        rejects([&]{c.execute("property_set",{{"entityId","hero"},{"propertyId","scale"},{"value",{-1.,1.,1.}}});});
        expect(c.entities()==before,"Failed operation mutated state");
        rejects([&]{c.execute("scene_create",{{"name",""}});});
        rejects([&]{c.execute("scene_select",{{"entityId","unknown"}});});
        rejects([&]{c.execute("editor_status",{{"unexpected",true}});});
        auto graphs=initialGraphs();expect(evaluateGraph(graphs["logic"])["messages"][0]=="Hello from the graph","Logic runtime failed");
        expect(evaluateGraph(graphs["material"])["baseColor"][1]==0.45,"Material evaluation failed");
        const auto beforeGraph=c.graphs();
        rejects([&]{c.execute("graph_link_add",{{"graphId","logic"},{"fromNode","begin"},{"fromPin","then"},{"toNode","print"},{"toPin","text"}});});
        expect(c.graphs()==beforeGraph,"Failed graph link mutated document");
        c.execute("graph_node_add",{{"graphId","logic"},{"type","math.add"},{"position",{100,200}}});
        expect(c.graphs()["logic"]["nodes"].size()==3,"Node creation failed");
        c.execute("edit_undo");expect(c.graphs()==beforeGraph,"Graph undo failed");
        c.execute("scene_delete",{{"entityId","hero"}});expect(c.selection().empty(),"Deleted selection survived");
        expect(!c.search("position").empty(),"Search missing properties");
        rejects([&]{c.execute("asset_open",{{"path","../settings/ui-preferences.json"}});});
        registerNodeType("custom.double",{{"title","Double"},{"domain","both"},{"inputs",{{"a","float"}}},{"outputs",{{"value","float"}}},{"defaults",{{"a",0.25}}}},[](const Json& values){return Json{{"value",values.at("a").get<double>()*2}};});
        auto custom=c.execute("graph_node_add",{{"graphId","material"},{"type","custom.double"},{"position",{50,300}}});
        c.execute("graph_link_add",{{"graphId","material"},{"fromNode",custom["node"]["id"]},{"fromPin","value"},{"toNode","surface"},{"toPin","roughness"}});
        expect(c.execute("graph_run",{{"graphId","material"}})["roughness"]==0.5,"Custom node evaluator not used");
        auto math=c.execute("graph_node_add",{{"graphId","logic"},{"type","math.add"},{"position",{50,300}}});
        const auto validGraphs=c.graphs();
        rejects([&]{c.execute("graph_link_add",{{"graphId","logic"},{"fromNode",math["node"]["id"]},{"fromPin","value"},{"toNode",math["node"]["id"]},{"toPin","a"}});});
        expect(c.graphs()==validGraphs,"Cyclic graph mutated the document");
        auto adapter=std::make_unique<ContractAdapter>();auto* settings=adapter.get();EditorCore adapted(fs::u8path(argv[1]),std::move(adapter));
        expect(adapted.execute("build_describe")["targets"].empty(),"Unimplemented target unexpectedly offered");
        rejects([&]{adapted.execute("build_prepare",{{"targetId","custom.export"}});});
        settings->advertise=true;
        auto plan=adapted.execute("build_prepare",{{"targetId","custom.export"}});
        expect(plan["configuration"]=="Test","First adapter configuration not selected by default");
        expect(plan["valid"]==true&&plan["options"]["quality"]==2,"Adapter build defaults not respected");
        expect(plan["snapshot"]["adapterId"]=="contract.adapter","Build hard-coded Sandbox adapter");
        rejects([&]{adapted.execute("build_prepare",{{"targetId","custom.export"},{"configuration","Test"},{"options",{{"quality",4}}}});});
        settings->sceneValid=false;
        expect(adapted.execute("build_prepare",{{"targetId","custom.export"},{"configuration","Test"}})["valid"]==false,"Adapter validation failure ignored");
        bool inspected=false;
        ImportDefinition importer={{{"id","test.raw"},{"name","Contract file"},{"extensions",{".contract"}},{"outputExtensions",{".asset"}},{"maxBytes",256}},[&](const fs::path&){inspected=true;return Json{{"kind","test"}};}};
        c.registerImporter(importer);rejects([&]{c.registerImporter(importer);});
        const auto source=fs::absolute(fs::u8path(argv[1])/"data/reports"/(uniqueId()+".contract"));fs::create_directories(source.parent_path());{std::ofstream file(source);file<<"contract";}
        auto importPlan=c.execute("asset_import_prepare",{{"sourcePath",source.u8string()},{"destination","Custom/Probe.asset"},{"importerId","test.raw"}});fs::remove(source);
        expect(inspected&&importPlan["info"]["kind"]=="test","Source importer inspector was not called");
        std::cout<<"Native contracts: passed\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
