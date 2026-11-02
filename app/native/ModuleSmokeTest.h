#pragma once
#include "Editor.h"
#include <imgui_internal.h>
namespace cygame {
// Only this isolated host receives input. Domain assertions read the real EditorCore.
class ModuleSmokeTest {
    EditorCore& core_;EditorApp& app_;fs::path report_;std::string entity_,name_;size_t undo_=0;double zoom_=1;Json localSpec_={{"type","number"}},localValue_=0.;int localCommits_=0;uint64_t localRevision_=0;
    struct Target{ImVec2 a,b;int frame;ImGuiID viewport;};std::map<std::string,Target> targets_;
    Json checks_=Json::object(),observed_=Json::object();bool passed_=true,pointed_=false;int frame_=0;ImVec2 pointer_{};ImGuiID viewport_=0;
    void check(const std::string& key,bool ok){checks_[key]=ok;passed_&=ok;}
    void hover(const std::string& key,bool valueStart=false){const auto it=targets_.find(key);if(it==targets_.end()||frame_-it->second.frame>4){check("target/"+key,false);return;}const auto& t=it->second;pointer_={valueStart?t.a.x+cyui::dp(10):(t.a.x+t.b.x)*.5f,(t.a.y+t.b.y)*.5f};viewport_=t.viewport;pointed_=true;}
    std::string name()const{return core_.entity(entity_).at("properties").at("name");}
    size_t undo()const{return core_.execute("edit_history").at("undo").size();}
public:
    ModuleSmokeTest(EditorCore& core,EditorApp& app,fs::path report):core_(core),app_(app),report_(std::move(report)){if(enabled()){entity_=core_.selection();name_=name();}}
    bool enabled()const{return !report_.empty();}
    void probe(const std::string& key,ImVec2 a,ImVec2 b){targets_[key]={a,b,frame_,ImGui::GetWindowViewport()->ID};}
    void input(ImGuiIO& io,int frame){if(!enabled())return;frame_=frame;io.ConfigInputTrickleEventQueue=false;io.AddFocusEvent(true);
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
        if(pointed_){io.AddMouseViewportEvent(viewport_);io.AddMousePosEvent(pointer_.x,pointer_.y);}
    }
    void draw(){
        if(!enabled()||frame_<300)return;
        ImGui::SetNextWindowPos({ImGui::GetMainViewport()->Pos.x+200,ImGui::GetMainViewport()->Pos.y+200},ImGuiCond_FirstUseEver);ImGui::SetNextWindowSize({430,140},ImGuiCond_FirstUseEver);
        if(ImGui::Begin("Local schema fixture###moduleFixture")){Json committed;if(app_.editValue("module/local",localSpec_,localValue_,committed)){localValue_=committed;++localCommits_;}}ImGui::End();
    }
    int finish(int frame){if(!enabled()||frame!=356)return -1;writeJson(report_,{{"passed",passed_},{"input","isolated own-host ImGui events: Inspector, Commands, Assets, Images"},{"checks",checks_},{"observed",observed_}});return passed_?0:3;}
};
}
