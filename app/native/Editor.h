#pragma once
#include "Core.h"
#include "MeshPreview.h"
#include <cyui/CyUI.h>
#include <imgui.h>
#include <set>
namespace cygame {
struct GizmoDraft { bool active=false; uint64_t revision=0; std::string primary;Json original=Json::array(),transforms=Json::array();float matrix[16]{}; };
struct Camera { float yaw=-0.85f,pitch=0.55f,distance=11.0f,target[3]{0,0,0.4f}; int mode=0,drawMode=0;bool grid=true; };
struct GraphView {
    ImVec2 pan{40,30};
    float zoom=1;
    std::string selected,linkSelected,dragging,wireNode,wirePin;
    bool wireOutput=false;
    std::set<std::string> selection;bool marquee=false;ImVec2 marqueeStart{};Json dragPositions=Json::array();uint64_t dragRevision=0;
    ImVec2 dragOrigin{},nodeOrigin{},newNodeAt{100,100};
    char filter[128]{};
    Json preview={{"baseColor",{0.2,0.45,0.8}},{"roughness",0.5},{"metallic",0.0}};
    std::string result,previewSignature;
    Json debug={{"state","Idle"}},diagnostics=Json::array();
};
struct MaterialViewState {Json report;uint64_t revision=0;};
struct ViewportContext { ImVec2 origin,size; Camera& camera; EditorCore& core; };
struct AssetEditorDefinition {
    std::string id;
    std::vector<std::string> extensions;
    std::function<void(const Json&)> open;
};
class BuildClient;
class EditorApp {
public:
    explicit EditorApp(EditorCore& core);
    ~EditorApp();
    void frame();
    void configureStyle(float dpi);
    std::string startupPanel()const{return workspacePreset_=="Scripting"?"logic":workspacePreset_=="Materials"?"material":"viewport";}
    Json open(const std::string& id,bool instance=false);
    Json previewTypeMigration(const Json& request);
    Json previewSignatureMigration(const Json& request);
    Json openFunction(const std::string& graphId,const std::string& name);
    Json invoke(const std::string& id,const Json& p=Json::object());
    const char* tr(const char* text) const;
    std::string label(const std::string& text,const std::string& id) const;
    // Source-plugin UI hooks; these do not load code from asset files.
    void registerAssetEditor(AssetEditorDefinition definition);
    void openAsset(const Json& descriptor);
    Json openJson(const std::string& path);
    Json openModel(const std::string& path);
    Json openLoadedModel(const std::string& id);
    std::function<ImTextureID(const std::string&,const ModelFrame&,const ModelCamera&,ImVec2)> meshRenderer;
    std::function<void(ViewportContext&)> viewportRenderer;
    // Optional QA probe; coordinates are in ImGui screen space.
    std::function<void(const std::string&,ImVec2)> viewportSelectionProbe;
    std::function<void(const std::string&,const Json&,const Json&)> graphProbe;
    std::function<void(const std::string&,ImVec2,ImVec2)> interfaceProbe;
    std::function<void(const std::string&,const Json&)> debuggerProbe;
    std::function<ImTextureID(const std::string&,const Json&,ImVec2)> materialRenderer;
    std::function<void(const std::string&,const Json&)> materialShaderInstaller;
    std::function<ImTextureID(const std::string&,int,bool)> textureRenderer;
    std::function<ImTextureID(const std::string&)> thumbnailRenderer;
    std::function<void()> clearThumbnails;
    EditorCore& core;
    bool shouldClose=false,resetLayout=false,persistSettings=true;
    void requestClose(){closePrompt_=true;}
    void previewSettings(const std::string& lang,const std::string& theme){if(!lang.empty())language_=lang;if(!theme.empty())theme_=theme;persistSettings=false;}
private:
    std::vector<AssetEditorDefinition> assetEditors_;
    Json interfaceFeatures_,translations_,preferences_,assets_=Json::array(),lastResult_=Json::object();
    std::string language_="en",theme_="unreal-dark",operation_="editor_status",assetFolder_,assetPath_,activeGraph_="logic",activeField_;
    std::map<std::string,Json> drafts_;
    std::map<std::string,Camera> cameras_;
    std::map<std::string,GizmoDraft> gizmos_;
    int gizmoOperation_=0;
    std::string debugGraph_="logic",commentNode_,activeFunction_,signatureKey_;
    void graphExpandUI(const std::string& graphId,const std::string& functionName,const std::string& viewKey);Json expandRequest_,expandReport_;uint64_t expandRevision_=0;bool expandStale_=false;
    void graphCollapseUI(const std::string& graphId,const std::string& functionName,const std::string& viewKey);Json collapseRequest_,collapseReport_;char collapseName_[65]="CollapsedGraph";int collapseKind_=0;uint64_t collapseRevision_=0;bool collapseStale_=false;
    void signatureMigrationPanel(bool typePanel=false);void signatureRenameControls(Json& draft,Json& renames,const std::string& context);Json migrationRequest_,migrationReport_,signatureRenames_=Json::object(),interfaceRenames_=Json::object();bool migrationAcceptLoss_=false,migrationStale_=false;uint64_t migrationRevision_=~uint64_t(0);char migrationPort_[65]="RenamedPort";int migrationInputIndex_=0,migrationOutputIndex_=0;
    Json signatureDraft_=Json::object();char functionName_[65]="NewFunction",functionRename_[65]{},portName_[65]="Input",groupComment_[2049]="Comment";int portType_=0;bool outputPort_=false,newExecutionFunction_=false;
    int debugFrame_=-1;
    char myGraphSearch_[128]{};
    char variableRename_[65]{},variableName_[65]="NewVariable",nodeComment_[2049]{};
    int variableType_=0;
    Json variableDraft_=0.0;
    bool localTransform_=false,snapping_=false;
    float translationSnap_=0.5f,rotationSnap_=15.f,scaleSnap_=0.1f;
    char actorFolder_[256]{};
    std::map<std::string,GraphView> graphViews_;
    std::unique_ptr<IEditorPlugin> diagnostics_;
    bool pluginEnabled_=true,layoutChecked_=false,searchOpen_=false,focusSearch_=false;
    bool closePrompt_=false,autoSave_=false,logPaused_=false;
    float textScale_=1,styleDpi_=1;
    bool stylePending_=false;
    char featureSearch_[128]{},operationSearch_[128]{};
    void interfaceOverview();
    void jsonEditor(const std::string& id);char jsonSearch_[128]{},jsonName_[129]="NewField",jsonValue_[4097]="null";std::string jsonParent_,jsonTarget_;bool jsonAdd_=false;
    void modelAuthoringUI(const std::string& id,const Json& model);
    void modelMotionUI(const std::string& id,const Json& model);
    void drawModelCollisions(const std::string& id,const Json& model,ImVec2 origin,ImVec2 size);
    void gameCodecPanel();std::unique_ptr<BuildClient> codecClient_;Json codecDescriptors_=Json::array();std::string codecModel_,codecId_;int codecVersion_=0,codecPlatform_=0;bool codecVerified_=true;char codecOptions_[4096]="{}",codecLosses_[4096]="[]";
    void graphTypesUI();bool typeEditing_=false,typePopup_=false;int typeKind_=0;char typeName_[65]="NewType",typeField_[65]="NewField";Json typeDraft_=Json::object(),typeRenames_=Json::object();void recordTypeRename(const std::string& from,const std::string& to);char typeRename_[65]="RenamedField";int typeRenameIndex_=0;
    void graphInterfacesUI();void interfaceDefinitionUI();bool interfaceEditing_=false;char interfaceName_[65]="NewInterface",interfaceMethod_[65]="NewMethod",interfacePort_[65]="NewPort";Json interfaceDraft_=Json::object(),classInterfaces_=Json::array();
    void blueprintLibraryUI();void classDefinitionUI();
    bool newMacro_=false;char className_[65]="NewClass",classParent_[65]="",classFields_[4096]="{\"health\":{\"type\":\"float\",\"default\":100}}",classMethods_[2048]="{}",debugEvent_[129]="CustomEvent";float debugDelta_=0.1f;
    void modelLibrary();void modelEditor(const std::string& id);
    struct ModelView {bool motionInitialized=false,ikUsePole=true,motionRotation=false,motionAxes[3]={true,false,true};int ikRoot=0,ikTip=0,motionRoot=0,ikIterations=64,motionSource=-2;float ikTarget[3]={1,1,0},ikPole[3]={0,0,1},ikTolerance=.001f,motionFps=30,trimStart=0,trimEnd=0;std::string motionTool,processedRun;ModelCamera camera;ModelFrame frame;uint64_t revision=~uint64_t(0);float time=0,sampledTime=-1,speed=1;int lod=0,sampledLod=-1,collisionKind=0,retargetAnimation=0,translationMode=0;float lodRatio=.5f,retargetFps=30,retargetScale=1;bool collisions=true;std::string retargetSource;Json mapping=Json::object();char mappingFilter[128]{};std::shared_ptr<BuildClient> processor;int sampledAnimation=-2,animation=-1,node=0,mesh=0,primitive=0,vertex=0,track=0,key=0;std::string requestedTab;char clipName[129]="NewClip";int trackChannel=0;bool playing=false,loop=true,skeleton=true;std::shared_ptr<BuildClient> exporter;};
    std::map<std::string,ModelView> modelViews_;
    int viewportCount_=1,assetType_=0;
    char globalSearch_[256]{},objectSearch_[128]{},propertySearch_[128]{},assetSearch_[256]{},menuFilters_[6][128]{};
    char operationParams_[16384]="{}",recipePath_[512]="",assetInspect_[16384]="";
    std::string resultText_,logSnapshot_;
    std::string workspacePreset_="Level",assetOperation_,contextAsset_;
    std::set<std::string> favorites_;
    Json assetFolders_=Json::array();
    bool assetGrid_=true,favoritesOnly_=false,drawerOpen_=false,drawerFocus_=false,assetDialog_=false;
    char assetDestination_[1024]{},newAssetName_[128]="NewGraph",newFolder_[256]="NewFolder",layoutName_[65]="MyLayout";
    void assetContext(const Json& asset);
    Json referenceReport_;char referencePath_[1025]="Examples/Example_ParameterMaterial.cyasset",newReference_[1025]{};
    void referenceViewer();
    void trashBrowser();
    void contentDrawer();
    void selectWorkspace(const std::string& preset);
    Json workspaceAction(const std::string& id,const Json& p);
    std::string panelTitle(const Panel& p)const;
    void savePreferences();
    void menuBar();
    void probeInterface(const std::string& id);
    void topBar();
    void layout(ImGuiID dockId);
    void outliner();
    void historyPanel();
    void myGraph();
    void functionLibraryUI();
    void debuggerPanel();
    void graphDiagnosticsPanel();
    uint64_t graphSearchRevision_=~uint64_t(0);std::string graphSearchScope_;
    Json graphSearchResults_;char graphSearch_[257]{},bookmarkName_[129]="Bookmark";bool findAllGraphs_=false,graphSearchTruncated_=false;
    void graphNavigator();
    void graphDiffPanel();std::string diffGraph_;Json diffReport_;uint64_t diffRevision_=~uint64_t(0);bool diffIgnorePresentation_=false;int diffSelection_=-1;
    std::unique_ptr<BuildClient> buildClient_;std::string buildTarget_,buildConfiguration_;Json buildOptions_=Json::object(),buildPlan_;
    void buildPanel();
    std::unique_ptr<BuildClient> importClient_;char importSource_[2049]{},importDestination_[1025]="Imported/Image.png";bool reimport_=false;std::string importProvider_;
    Json importReport_;std::string importCompleted_;
    Json importAction(const std::string& operation,const Json& parameters);
    void importPanel();
    std::map<std::string,MaterialViewState> materialViews_;std::string materialParentKey_;char materialParent_[1025]{};
    struct ShaderView {std::shared_ptr<BuildClient> client;Json prepared,installed;uint64_t revision=~uint64_t(0);std::string attemptedJob,error,source;bool plane=false;};
    std::map<std::string,ShaderView> shaderViews_;std::string compilerGraph_;
    Json shaderAction(const std::string& operation,const Json& parameters);
    Json shaderStatus(const std::string& graphId);
    void compilerPanel();
    void materialPreviewUI(const std::string& graphId);
    struct ImageView {Json info;float zoom=1;ImVec2 pan{};bool fit=true,refresh=false;int channel=0;};
    std::map<std::string,ImageView> imageViews_;
    Json openTexture(const std::string& path);
    void textureEditor(const std::string& path);
    void materialInstanceEditor(const std::string& graphId);
    void focusGraphNode(const std::string& graph,const std::string& function,const std::string& node);
    void graphNavigationTools(const std::string& graph,const std::string& function,const Json& view);
    void graphCommentProperties(const std::string& graph,const std::string& function,const Json& comments);
    void details();
    void contentBrowser();
    void assetsRefresh();
    void assetEditor();
    void modules();
    void options();
    void operations();
    void logPanel();
    void searchPopup();
    void viewport(const std::string& id);
    bool transformGizmo(const std::string& id,const ImVec2& origin,const ImVec2& size,const float* view,const float* projection);
    void graphEditor(const std::string& graphId,const std::string& functionName="");
    bool editValue(const std::string& id,const Json& schema,const Json& original,Json& committed);
    void saveRecipe();
    void loadRecipe();
};
}
