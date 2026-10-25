#include "Editor.h"
#include "BuildClient.h"
namespace cygame {
void EditorApp::gameCodecPanel(){
    cyui::heading(tr("Game Codecs"),2);if(cyui::ghostButton(tr("Refresh codec providers")))invoke("game_codec_list");
    const auto state=codecClient_->status();cyui::badge(tr(state.value("state","Idle").c_str()));
    if(state.contains("job")){const auto outputs=state.at("job").value("outputs",Json::object());if(outputs.is_object()&&outputs.contains("codecs"))codecDescriptors_=outputs.at("codecs");}
    if(state.value("state","")=="Failed")cyui::callout(cyui::Tone::error,state.value("error",state.value("job",Json::object()).value("error",Json::object()).value("message","")).c_str());
    if(state.value("state","")=="Running"&&cyui::ghostButton(tr("Cancel")))codecClient_->cancel();
    if(codecDescriptors_.empty()){cyui::emptyState(tr("No game codec registered"),tr("Choose a game, exact version and real fixtures, then register its trusted source provider."),cyui::icon::info);return;}
    const std::string modelLabel=core.models().contains(codecModel_)?core.models().at(codecModel_).value("name",core.models().at(codecModel_).value("assetPath",codecModel_)):tr("Choose a source");
    if(ImGui::BeginCombo(tr("3D asset"),modelLabel.c_str())){for(const auto& [id,model]:core.models().items())if(ImGui::Selectable(label(model.value("name",model.value("assetPath",id)),id).c_str(),id==codecModel_))codecModel_=id;ImGui::EndCombo();}
    for(const auto& descriptor:codecDescriptors_){const std::string id=descriptor.at("id");if(ImGui::Selectable(label(descriptor.value("gameId",id)+" / "+id,id).c_str(),codecId_==id)){codecId_=id;codecVersion_=codecPlatform_=0;}}
    for(const auto& descriptor:codecDescriptors_)if(descriptor.at("id")==codecId_){
        const auto versions=descriptor.at("versions"),platforms=descriptor.at("platforms");if(versions.empty()||platforms.empty())continue;codecVersion_=std::clamp(codecVersion_,0,int(versions.size())-1);codecPlatform_=std::clamp(codecPlatform_,0,int(platforms.size())-1);
        auto combo=[&](const char* title,const Json& values,int& index){if(ImGui::BeginCombo(tr(title),values[index].get_ref<const std::string&>().c_str())){for(size_t i=0;i<values.size();++i)if(ImGui::Selectable(values[i].get_ref<const std::string&>().c_str(),index==int(i)))index=int(i);ImGui::EndCombo();}};combo("Game version",versions,codecVersion_);combo("Platform",platforms,codecPlatform_);
        ImGui::TextUnformatted(tr("Codec options (JSON)"));ImGui::InputTextMultiline("##codecOptions",codecOptions_,sizeof(codecOptions_),{cyui::dp(420),cyui::dp(100)});ImGui::TextUnformatted(tr("Accepted conversion losses (JSON array)"));ImGui::InputTextMultiline("##codecLosses",codecLosses_,sizeof(codecLosses_),{cyui::dp(420),cyui::dp(70)});
        ImGui::Checkbox(tr("Require verified output"),&codecVerified_);if(cyui::primaryButton(tr("Export through game codec"))){try{invoke("game_codec_export",{{"modelId",codecModel_},{"codecId",codecId_},{"gameVersion",versions[codecVersion_]},{"platform",platforms[codecPlatform_]},{"options",Json::parse(codecOptions_)},{"acceptedLosses",Json::parse(codecLosses_)},{"requireVerified",codecVerified_}});}catch(const std::exception& e){cyui::notify(e.what(),cyui::Tone::error);}}
    }
    if(state.contains("job")&&state["job"].contains("outputs")&&state["job"]["outputs"].contains("outputPath"))ImGui::TextWrapped("%s",state["job"]["outputs"]["outputPath"].get_ref<const std::string&>().c_str());
}
}
