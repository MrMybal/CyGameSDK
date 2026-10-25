#pragma once
#include "Core.h"
#include <deque>
namespace cygame {
// Register pure, typed source-plugin nodes before creating the editor host.
// Evaluators receive resolved input/default values and return named outputs.
using NodeEvaluator = std::function<Json(const Json&)>;
void registerNodeType(const std::string& id,const Json& descriptor,NodeEvaluator evaluator);
const Json& nodeTypes();
Json initialGraphs();
std::vector<std::string> blueprintTypes();
Json graphValueSchema(const std::string& type);
Json graphDefaultValue(const std::string& type);
Json graphWidget(const std::string& type);
bool isNamedGraphType(const std::string& type);
std::vector<std::string> blueprintTypes(const Json& types);
Json graphValueSchema(const std::string& type,const Json& types);
Json graphDefaultValue(const std::string& type,const Json& types);
Json graphWidget(const std::string& type,const Json& types);
void validateGraphTypes(const Json& types);
void extendTypeNodes(Json& types);
Json typedNodeDefinition(const Json& graph,const Json& node);
Json evaluateTypedNode(const Json& node,const Json& values);
Json graphTypeMigrationCommand(Json& graph,const std::string& operation,const Json& parameters);
Json graphTypeCommand(Json& graph,const std::string& op,const Json& parameters);
void extendBlueprintNodes(Json& types);
Json evaluateBlueprintNode(const std::string& type,const Json& values);
Json blueprintClass(const Json& classes,const std::string& name,const Json& types=Json::object());
void validateBlueprint(const Json& graph);
void validateGraphInterfaces(const Json& graph);
Json interfaceNodeDefinition(const Json& graph,const Json& node);
Json graphInterfaceCommand(Json& graph,const std::string& operation,const Json& parameters);
std::vector<std::string> interfaceTargets(const Json& graph,const Json& node);

Json blueprintCommand(Json& graph,const std::string& id,const Json& parameters);
Json expandGraphMacros(const Json& graph);
Json validateGraph(const Json& graph);
Json validateGraphBody(const Json& graph);
Json functionCallDefinition(const Json& function,const std::string& name,const Json& types=Json::object());
Json nodeDefinition(const Json& graph,const Json& node);
Json graphView(const Json& graph,const std::string& functionName="");
Json graphExpandCommand(Json& graph,const std::string& operation,const Json& parameters);
Json graphCollapseCommand(Json& graph,const std::string& operation,const Json& parameters);
Json graphSignatureCommand(Json& graph,const std::string& operation,const Json& parameters);
Json functionLibraryCommand(Json& graph,const std::string& operation,const Json& p);
using TextureSampler=std::function<Json(const std::string&,double,double)>;
Json evaluateFunction(const Json& graph,const std::string& name,const Json& arguments,TextureSampler sampler={});
void validateMaterialInstance(const Json& graph);
Json resolveMaterial(const fs::path& root,const Json& graphs,const std::string& graphId,bool includePreview=true);
void validateAssetReferences(const Json& graph);
Json assetDependencyReport(const fs::path& root,const Json& graphs,const std::string& selected);
Json compareGraphDocuments(const Json& before,const Json& after,bool ignorePresentation);
Json findGraphNodes(const Json& graphs,const Json& parameters);
Json graphNavigationCommand(Json& graph,const std::string& operation,const Json& parameters);
Json graphBatchCommand(Json& graph,const std::string& operation,const Json& p);
Json graphCommand(Json& graphs, const std::string& operation, const Json& p);
Json evaluateGraph(const Json& graph);
TextureSampler textureSampler(const fs::path& root);
Json sampleMaterial(const Json& graph,double u,double v,TextureSampler sampler={});
std::string sha256(const std::string& bytes);
Json prepareMaterial(const fs::path& root,const Json& graphs,const std::string& graphId);
Json graphDiagnostics(const Json& graph);
Json evaluateCustomNode(const std::string& type,const Json& values,const std::string& pin);
class GraphExecution {
public:
    explicit GraphExecution(Json graph,bool breakpoints=true);
    GraphExecution(const GraphExecution&)=delete;GraphExecution& operator=(const GraphExecution&)=delete;
    Json advance(bool resume);
    Json advanceTime(double delta);
    Json emit(const std::string& event);
    Json editObject(const std::string& object,const std::string& field,const Json& value);
    Json advance(const std::string& mode);
    Json status()const;
    Json result();
    bool matchesProgram(const Json& graph)const;
    void refreshBreakpoints(const Json& graph);
private:
    struct RunState {double time=0;int nextObject=0;Json interfaces=Json::object(),types=Json::object(),classes=Json::object(),objects=Json::object();std::deque<std::string> events;int steps=0,sliceSteps=0;Json messages=Json::array(),trace=Json::array();};
    Json loops_=Json::object();
    Json sourceSignature_;double waitUntil_=0;
    Json graph_,variables_=Json::object(),watches_=Json::object(),callValues_=Json::object(),returns_=Json::object();
    std::deque<std::string> pending_;
    std::string state_="Paused",error_,breakpoint_,functionName_,callerNode_;
    bool breakpoints_=true,returned_=false;
    int depth_=0;
    std::shared_ptr<int> budget_=std::make_shared<int>(10000);
    std::shared_ptr<RunState> run_=std::make_shared<RunState>();
    Json arguments_=Json::object();const Json* library_=nullptr;
    std::unique_ptr<GraphExecution> child_;
    GraphExecution(Json graph,Json arguments,std::shared_ptr<int> budget,int depth,const Json* library,
        std::shared_ptr<RunState> run=nullptr,std::string functionName="",std::string callerNode="");
    double sampleU_=.5,sampleV_=.5;TextureSampler textureSampler_;
    friend Json sampleMaterial(const Json&,double,double,TextureSampler);
    friend Json evaluateFunction(const Json&,const std::string&,const Json&,TextureSampler);
    std::string callTarget(const Json& node);
    Json input(const Json& node,const std::string& pin);
    Json output(const Json& node,const std::string& pin);
    Json nodeSpec(const Json& node)const;
    void initialize();
    void executeOne();
    void tick();
    GraphExecution& active();
    const GraphExecution& active()const;
    bool finished()const{return !child_&&pending_.empty();}
    void stack(Json& frames)const;

};
}
