#pragma once
#include "Editor.h"
#include <imgui_internal.h>
namespace cygame {
// Only this isolated host receives input. Domain assertions read the real EditorCore.
class ModuleSmokeTest {
    EditorCore& core_;EditorApp& app_;fs::path report_;std::string entity_,name_;size_t undo_=0;double zoom_=1;Json localSpec_={{"type","number"}},localValue_=0.;int localCommits_=0;uint64_t localRevision_=0;
    char sourceBuffer_[512]="{\"value\":0}";size_t sourceUndo_=0,tableUndo_=0;std::string tableName_;
    Json groupBefore_;std::string groupId_,groupNode_;size_t groupUndo_=0;
    struct Target{ImVec2 a,b;int frame;ImGuiID viewport;};std::map<std::string,Target> targets_;
    Json checks_=Json::object(),observed_=Json::object();bool passed_=true,pointed_=false;int frame_=0,offset_=0;ImVec2 pointer_{};ImGuiID viewport_=0;
    void check(const std::string& key,bool ok){checks_[key]=ok;passed_&=ok;}
    void hover(const std::string& key,bool valueStart=false){const auto it=targets_.find(key);if(it==targets_.end()||frame_-it->second.frame>4){check("target/"+key,false);return;}const auto& t=it->second;pointer_={valueStart?t.a.x+cyui::dp(10):(t.a.x+t.b.x)*.5f,(t.a.y+t.b.y)*.5f};viewport_=t.viewport;pointed_=true;}
    std::string name()const{return core_.entity(entity_).at("properties").at("name");}
    size_t undo()const{return core_.execute("edit_history").at("undo").size();}
public:
    ModuleSmokeTest(EditorCore& core,EditorApp& app,fs::path report,int startFrame=0):core_(core),app_(app),report_(std::move(report)){offset_=startFrame;if(enabled()){entity_=core_.selection();name_=name();}}
    bool enabled()const{return !report_.empty();}
    void probe(const std::string& key,ImVec2 a,ImVec2 b){targets_[key]={a,b,frame_,ImGui::GetWindowViewport()->ID};}
    void input(ImGuiIO& io,int frame){if(!enabled())return;frame+=offset_;frame_=frame;io.ConfigInputTrickleEventQueue=false;io.AddFocusEvent(true);
        const auto field="types/value/"+entity_+"/name";
        if(frame==8)app_.open("details");if(frame==16)undo_=undo();
        if(frame==18||frame==50||frame==82)hover(field);
        if(frame==20||frame==52||frame==84)io.AddMouseButtonEvent(0,true);
        if(frame==22||frame==54||frame==86)io.AddMouseButtonEvent(0,false);
        if(frame==26||frame==58||frame==90){io.AddKeyEvent(ImGuiMod_Ctrl,true);io.AddKeyEvent(ImGuiKey_A,true);}
        if(frame==28||frame==60||frame==92){io.AddKeyEvent(ImGuiKey_A,false);io.AddKeyEvent(ImGuiMod_Ctrl,false);}
        if(frame==30)io.AddInputCharactersUTF8("ModularCube");if(frame==62)io.AddInputCharactersUTF8("CancelledName");if(frame==94)io.AddInputCharactersUTF8("StaleName");
        if(frame==34||frame==104)io.AddKeyEvent(ImGuiKey_Enter,true);if(frame==36||frame==106)io.AddKeyEvent(ImGuiKey_Enter,false);
        if(frame==42){check("inspectorCommit",name()=="ModularCube");check("singleUndo",undo()==undo_+1);core_.execute("edit_undo");check("undo",name()==name_);core_.execute("edit_redo");check("redo",name()=="ModularCube");core_.execute("edit_undo");}
        if(frame==66)io.AddKeyEvent(ImGuiKey_Escape,true);if(frame==68)io.AddKeyEvent(ImGuiKey_Escape,false);
        if(frame==74){check("escapeCancels",name()==name_);check("cancelNoUndo",undo()==undo_);}
        if(frame==100)core_.execute("property_set",{{"entityId",entity_},{"propertyId","name"},{"value","ExternalName"}});
        if(frame==112){observed_["staleName"]=name();observed_["staleUndo"]=undo();observed_["baselineUndo"]=undo_;check("staleDraftCancelled",name()=="ExternalName");check("staleNoExtraUndo",undo()==undo_+1);core_.execute("edit_undo");}
        if(frame==120){core_.execute("property_set",{{"entityId",entity_},{"propertyId","name"},{"value","ModuleNeedle"}});for(const auto& e:core_.entities())if(e.at("id")!=entity_){core_.execute("scene_select",{{"entityId",e.at("id")}});break;}}
        if(frame==124){io.AddKeyEvent(ImGuiMod_Ctrl,true);io.AddKeyEvent(ImGuiKey_P,true);}if(frame==126){io.AddKeyEvent(ImGuiKey_P,false);io.AddKeyEvent(ImGuiMod_Ctrl,false);}
        if(frame==130)io.AddInputCharactersUTF8("ModuleNeedle");if(frame==138)io.AddKeyEvent(ImGuiKey_Enter,true);if(frame==140)io.AddKeyEvent(ImGuiKey_Enter,false);
        if(frame==148){check("commandKeyboardDispatch",core_.selection()==entity_);app_.open("content");}
        if(frame==154)hover("assets/first");if(frame==156||frame==162)io.AddMouseButtonEvent(0,true);if(frame==158||frame==164)io.AddMouseButtonEvent(0,false);
        if(frame==174){bool found=false;for(const auto& p:core_.panels)found|=p.id=="texture:Module.bmp"&&p.open;check("assetDoubleClickOpensImage",found);core_.execute("texture_view_settings",{{"path","Module.bmp"},{"zoom",1.},{"fit",false}});}
        if(frame==180)hover("image/canvas");if(frame==184)io.AddMouseWheelEvent(0,2);
        if(frame==192){zoom_=core_.execute("texture_view_settings",{{"path","Module.bmp"}}).at("zoom");check("imageWheelZoom",zoom_>1.);}
        if(frame==198)hover("image/fit");if(frame==200)io.AddMouseButtonEvent(0,true);if(frame==202)io.AddMouseButtonEvent(0,false);
        if(frame==210){auto state=core_.execute("texture_view_settings",{{"path","Module.bmp"}});check("imageFit",state.at("zoom").get<double>()!=zoom_);check("imageFitMode",state.at("fit").get<bool>());check("imageZoomBounds",state.at("zoom").get<double>()<=8.);app_.open("content");}
        if(frame==218)hover("assets/actions");if(frame==220)io.AddMouseButtonEvent(0,true);if(frame==222)io.AddMouseButtonEvent(0,false);
        if(frame==228)hover("assets/favorite");if(frame==230)io.AddMouseButtonEvent(0,true);if(frame==232)io.AddMouseButtonEvent(0,false);
        if(frame==238)hover("assets/actions");if(frame==240)io.AddMouseButtonEvent(0,true);if(frame==242)io.AddMouseButtonEvent(0,false);
        if(frame==252)check("assetFavoriteAction",targets_.count("assets/favorite/remove")&&targets_.at("assets/favorite/remove").frame>=246);
        if(frame==254)io.AddKeyEvent(ImGuiKey_Escape,true);if(frame==256)io.AddKeyEvent(ImGuiKey_Escape,false);
        if(frame==260){io.AddKeyEvent(ImGuiMod_Ctrl,true);io.AddKeyEvent(ImGuiKey_P,true);}if(frame==262){io.AddKeyEvent(ImGuiKey_P,false);io.AddKeyEvent(ImGuiMod_Ctrl,false);}
        if(frame==268)io.AddInputCharactersUTF8("scene_select");if(frame==276)io.AddKeyEvent(ImGuiKey_Enter,true);if(frame==278)io.AddKeyEvent(ImGuiKey_Enter,false);
        if(frame==288){bool opened=false;for(const auto& p:core_.panels)opened|=p.id=="operations"&&p.open;check("commandProtocolIdSearch",opened);
            const auto moved=app_.invoke("asset_move",{{"path","Module.bmp"},{"destination","Renamed.bmp"}});bool selected=false;for(const auto& asset:app_.assets_)if(cyui::selected(app_.assetState_.selection,EditorApp::uiId("asset/"+asset.at("path").get<std::string>())))selected|=asset.at("path")=="Renamed.bmp";
            check("assetRenameKeepsSelection",!moved.contains("error")&&selected);app_.invoke("asset_move",{{"path","Renamed.bmp"},{"destination","Module.bmp"}});
        }
        if(frame==300)localRevision_=core_.revision();
        if(frame==308)hover("types/value/module/local");if(frame==310){io.AddKeyEvent(ImGuiMod_Ctrl,true);io.AddMouseButtonEvent(0,true);}if(frame==312){io.AddMouseButtonEvent(0,false);io.AddKeyEvent(ImGuiMod_Ctrl,false);}
        if(frame==316){io.AddKeyEvent(ImGuiMod_Ctrl,true);io.AddKeyEvent(ImGuiKey_A,true);}if(frame==318){io.AddKeyEvent(ImGuiKey_A,false);io.AddKeyEvent(ImGuiMod_Ctrl,false);io.AddInputCharactersUTF8("6");}
        if(frame==324){localSpec_={{"type","boolean"}};localValue_=false;}
        if(frame==334){check("localSchemaCancelsDraft",localValue_.is_boolean()&&!localValue_.get<bool>()&&localCommits_==0);hover("types/value/module/local",true);}
        if(frame==338)io.AddMouseButtonEvent(0,true);if(frame==340)io.AddMouseButtonEvent(0,false);
        if(frame==348){check("newScalarTypeEditable",localValue_==true&&localCommits_==1);check("formDraftIsNotDocumentUndo",core_.revision()==localRevision_);}
        if(frame==364){localSpec_={{"type","array"},{"maxItems",1000},{"items",{{"type","integer"}}},{"x-editor-container","array.int"}};localValue_=Json::array();for(int i=0;i<1000;++i)localValue_.push_back(i);}
        if(frame==390){size_t rendered=0;for(const auto& [id,list]:app_.inspectors_["module/local"].lists)rendered+=list.renderedRows;observed_["virtualArrayRenderedRows"]=rendered;check("virtualArrayClipped",rendered>0&&rendered<1000);check("virtualArrayUnchanged",localValue_.size()==1000&&localValue_[999]==999);}
        if(frame==400){app_.outlinerTable_=false;app_.open("outliner");}
        if(frame==408)hover("outliner/first");if(frame==410)io.AddMouseButtonEvent(0,true);if(frame==412)io.AddMouseButtonEvent(0,false);
        if(frame==416)io.AddKeyEvent(ImGuiKey_End,true);if(frame==418)io.AddKeyEvent(ImGuiKey_End,false);
        if(frame==424){check("hierarchyKeyboardSelection",core_.selection()==core_.entities().back().at("id"));undo_=undo();}
        if(frame==426)io.AddKeyEvent(ImGuiKey_F2,true);if(frame==428)io.AddKeyEvent(ImGuiKey_F2,false);
        if(frame==434){io.AddKeyEvent(ImGuiMod_Ctrl,true);io.AddKeyEvent(ImGuiKey_A,true);}
        if(frame==436){io.AddKeyEvent(ImGuiKey_A,false);io.AddKeyEvent(ImGuiMod_Ctrl,false);io.AddInputCharactersUTF8("HierarchyRenamed");}
        if(frame==442)io.AddKeyEvent(ImGuiKey_Enter,true);if(frame==444)io.AddKeyEvent(ImGuiKey_Enter,false);
        if(frame==452){check("hierarchyRename",core_.entities().back().at("properties").at("name")=="HierarchyRenamed");check("hierarchySingleUndo",undo()==undo_+1);core_.execute("edit_undo");check("hierarchyUndo",core_.entities().back().at("properties").at("name")!="HierarchyRenamed");}
        if(frame==474){app_.invoke("workspace_layout",{{"preset","Scripting"}});const auto first=core_.graphs().at("logic").at("nodes")[0];groupNode_=first.at("id");const auto x=first.at("position")[0].get<double>(),y=first.at("position")[1].get<double>();groupId_=core_.execute("graph_comment_add",{{"graphId","logic"},{"text","Gesture group"},{"position",{x-20,y-35}},{"size",{250,140}}}).at("comment").at("id");}
        if(frame==492){groupBefore_=core_.graphs().at("logic");groupUndo_=undo();hover("graph/group/"+groupId_);}
        if(frame==496)io.AddMouseButtonEvent(0,true);
        if(frame==504){pointer_.x+=cyui::dp(35);pointer_.y+=cyui::dp(25);}
        if(frame==508)io.AddMouseButtonEvent(0,false);
        if(frame==520){
            const auto& graph=core_.graphs().at("logic");Json beforeGroup,afterGroup;for(const auto& c:groupBefore_.at("comments"))if(c.at("id")==groupId_)beforeGroup=c;for(const auto& c:graph.at("comments"))if(c.at("id")==groupId_)afterGroup=c;
            const double dx=afterGroup.at("position")[0].get<double>()-beforeGroup.at("position")[0].get<double>(),dy=afterGroup.at("position")[1].get<double>()-beforeGroup.at("position")[1].get<double>();
            check("groupMovedThroughUI",std::abs(dx)>1||std::abs(dy)>1);bool carried=false;for(const auto& n:graph.at("nodes"))if(n.at("id")==groupNode_)for(const auto& old:groupBefore_.at("nodes"))if(old.at("id")==groupNode_)carried=std::abs(n.at("position")[0].get<double>()-old.at("position")[0].get<double>()-dx)<.001&&std::abs(n.at("position")[1].get<double>()-old.at("position")[1].get<double>()-dy)<.001;
            check("groupCarriesContainedNode",carried);check("groupSingleUndo",undo()==groupUndo_+1);if(undo()>groupUndo_)core_.execute("edit_undo");check("groupUndo",core_.graphs().at("logic")==groupBefore_);
        }
        if(frame==528)hover("graph/group/"+groupId_);if(frame==532)io.AddMouseButtonEvent(0,true);if(frame==538){pointer_.x+=cyui::dp(30);pointer_.y+=cyui::dp(20);}
        if(frame==542)io.AddKeyEvent(ImGuiKey_Escape,true);if(frame==544)io.AddKeyEvent(ImGuiKey_Escape,false);if(frame==546)io.AddMouseButtonEvent(0,false);
        if(frame==552)check("groupEscapeCancels",core_.graphs().at("logic")==groupBefore_&&undo()==groupUndo_);
        if(frame==558)hover("graph/group/"+groupId_);if(frame==562)io.AddMouseButtonEvent(0,true);if(frame==568){pointer_.x+=cyui::dp(30);pointer_.y+=cyui::dp(20);}
        if(frame==572)core_.execute("property_set",{{"entityId",entity_},{"propertyId","name"},{"value","Concurrent group edit"}});
        if(frame==578)io.AddMouseButtonEvent(0,false);
        if(frame==588){check("groupExternalRevisionCancels",core_.graphs().at("logic")==groupBefore_&&undo()==groupUndo_+1);if(undo()>groupUndo_)core_.execute("edit_undo");}
        if(frame==602){app_.reimport_=false;app_.importSource_[0]=0;app_.open("imports");}
        if(frame==610)hover("imports/browse");if(frame==612)io.AddMouseButtonEvent(0,true);if(frame==614)io.AddMouseButtonEvent(0,false);
        if(frame==624){check("sourcePickerOpened",app_.importPickerOpen_);app_.importPicker_.directory=app_.importPicker_.pathText=(core_.root()/"data/assets").generic_u8string();}
        if(frame==636){bool found=false;for(const auto& entry:app_.importListing_.entries)found|=entry.name=="Module.bmp";check("sourcePickerCachedListing",!app_.importListing_.loading&&found);app_.importPicker_.selection={"Module.bmp"};app_.importPicker_.filename="Module.bmp";hover("imports/picker/open");}
        if(frame==640)io.AddMouseButtonEvent(0,true);if(frame==642)io.AddMouseButtonEvent(0,false);
        if(frame==652){check("sourcePickerAccepted",!app_.importPickerOpen_&&std::string(app_.importSource_).find("Module.bmp")!=std::string::npos);check("sourcePickerDoesNotImport",!fs::exists(core_.root()/"data/build-runs"));app_.importPickerOpen_=false;}
        if(frame==664){app_.outlinerTable_=true;app_.open("outliner");}
        if(frame==668)hover("outliner/table");if(frame==670)io.AddMouseButtonEvent(0,true);if(frame==672)io.AddMouseButtonEvent(0,false);
        if(frame==674){const auto row=EditorApp::uiId("outliner/entity/"+entity_);app_.sceneTable_.selection={row};app_.sceneTable_.anchor=app_.sceneTable_.active=row;app_.sceneTable_.anchorColumn=app_.sceneTable_.activeColumn=0;tableName_=name();tableUndo_=undo();ImGui::SetClipboardText("TableRenamed");}
        if(frame==676){io.AddKeyEvent(ImGuiMod_Ctrl,true);io.AddKeyEvent(ImGuiKey_V,true);}if(frame==678){io.AddKeyEvent(ImGuiKey_V,false);io.AddKeyEvent(ImGuiMod_Ctrl,false);}
        if(frame==688){check("sceneTablePaste",name()=="TableRenamed");check("sceneTableSingleUndo",undo()==tableUndo_+1);if(undo()>tableUndo_)core_.execute("edit_undo");check("sceneTableUndo",name()==tableName_);app_.outlinerTable_=false;}
        if(frame==704)sourceUndo_=undo();
        if(frame==710)hover("source/local");if(frame==712)io.AddMouseButtonEvent(0,true);if(frame==714)io.AddMouseButtonEvent(0,false);
        if(frame==716){io.AddKeyEvent(ImGuiMod_Ctrl,true);io.AddKeyEvent(ImGuiKey_A,true);}
        if(frame==718){io.AddKeyEvent(ImGuiKey_A,false);ImGui::SetClipboardText("{\"value\":7}");io.AddKeyEvent(ImGuiKey_V,true);}
        if(frame==720){io.AddKeyEvent(ImGuiKey_V,false);io.AddKeyEvent(ImGuiMod_Ctrl,false);}
        if(frame==728){observed_["sourceTyped"]=std::string(sourceBuffer_);observed_["sourceBackend"]=app_.sourceEditors_.at("module/source").editor->text();observed_["sourceWantText"]=io.WantTextInput;try{check("codeEditorPastedJson",Json::parse(sourceBuffer_).at("value")==7);}catch(...){check("codeEditorPastedJson",false);}}
        if(frame==732){io.AddKeyEvent(ImGuiMod_Ctrl,true);io.AddKeyEvent(ImGuiKey_Z,true);}
        if(frame==734){io.AddKeyEvent(ImGuiKey_Z,false);io.AddKeyEvent(ImGuiMod_Ctrl,false);}
        if(frame==742){observed_["sourceAfterUndo"]=std::string(sourceBuffer_);try{check("codeEditorLocalUndo",Json::parse(sourceBuffer_).at("value")==0);}catch(...){check("codeEditorLocalUndo",false);}check("codeEditorDoesNotUndoDocument",undo()==sourceUndo_);std::snprintf(sourceBuffer_,sizeof(sourceBuffer_),"%s","{\"external\":true}");}
        if(frame==760){io.AddKeyEvent(ImGuiMod_Ctrl,true);io.AddKeyEvent(ImGuiKey_A,true);}
        if(frame==762){io.AddKeyEvent(ImGuiKey_A,false);io.AddKeyEvent(ImGuiMod_Ctrl,false);io.AddInputCharactersUTF8("7");}
        if(frame==772){try{check("codeEditorTyping",Json::parse(sourceBuffer_)==7);}catch(...){check("codeEditorTyping",false);}}
        if(frame==752){const auto found=app_.sourceEditors_.find("module/source");check("codeEditorExternalReplace",found!=app_.sourceEditors_.end()&&found->second.editor->text()==std::string(sourceBuffer_));}
        if(pointed_){io.AddMouseViewportEvent(viewport_);io.AddMousePosEvent(pointer_.x,pointer_.y);}
    }
    void draw(){
        if(enabled()&&frame_>=700&&frame_<780){
            ImGui::SetNextWindowPos({ImGui::GetMainViewport()->Pos.x+200,ImGui::GetMainViewport()->Pos.y+180},ImGuiCond_FirstUseEver);ImGui::SetNextWindowSize({500,240},ImGuiCond_FirstUseEver);
            if(ImGui::Begin("Source buffer fixture###moduleSourceFixture")){app_.editSource("module/source",sourceBuffer_,sizeof(sourceBuffer_),ImGui::GetContentRegionAvail());probe("source/local",ImGui::GetItemRectMin(),ImGui::GetItemRectMax());}ImGui::End();return;
        }
        if(!enabled()||frame_<300||frame_>=400)return;
        ImGui::SetNextWindowPos({ImGui::GetMainViewport()->Pos.x+200,ImGui::GetMainViewport()->Pos.y+200},ImGuiCond_FirstUseEver);ImGui::SetNextWindowSize({430,140},ImGuiCond_FirstUseEver);
        if(ImGui::Begin("Local schema fixture###moduleFixture")){Json committed;if(app_.editValue("module/local",localSpec_,localValue_,committed)){localValue_=committed;++localCommits_;}}ImGui::End();
    }
    int finish(int frame){frame+=offset_;if(!enabled()||frame!=780)return -1;writeJson(report_,{{"passed",passed_},{"input","isolated own-host ImGui events: Inspector, Commands, Assets, Images, Hierarchy, virtual collections and graph groups"},{"checks",checks_},{"observed",observed_}});return passed_?0:3;}
};
}
