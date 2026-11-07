#pragma once
#include "Editor.h"
#include "Documentation.h"
namespace cygame {
// Injects only this isolated host's ImGui context. The opener is intercepted.
class HelpSmokeTest {
    EditorCore& core_;EditorApp& app_;fs::path report_;Json checks_=Json::object(),urls_=Json::array();
    void check(const std::string& key,bool value){checks_[key]=value;}
public:
    HelpSmokeTest(EditorCore& core,EditorApp& app,fs::path report):core_(core),app_(app),report_(std::move(report)){
        if(!enabled())return;
        app_.documentationLauncher=[this](const std::string& url){urls_.push_back(url);return true;};
        const auto escaped=documentationUrl(fs::u8path("D:/Docs space/#é%20"),"fr",{"graphs","canvas"});
        check("encodedFilePath",escaped=="file:///D:/Docs%20space/%23%C3%A9%2520/Documentation/Documentation.html#/fr/graphs/canvas");
        check("unsupportedLocaleFallback",documentationUrl(core_.root(),"xx",{"overview","getting-started"}).find("#/en/overview/getting-started")!=std::string::npos);
        check("dynamicModelContext",documentationTopic("model:local").page=="assets-3d");
        check("dynamicFunctionContext",documentationTopic("fn:logic#test").page=="functions");
        check("unknownContextFallback",documentationTopic("extension.unknown").page=="overview");
        app_.previewSettings("en","");app_.openDocumentation(false);
        check("overviewEntry",urls_.size()==1&&urls_.back().get<std::string>().find("#/en/overview/getting-started")!=std::string::npos);
    }
    bool enabled()const{return !report_.empty();}
    void input(ImGuiIO& io,int frame){if(!enabled())return;io.AddFocusEvent(true);
        if(frame==8){app_.previewSettings("fr","");app_.open("logic");}
        if(frame==12)io.AddKeyEvent(ImGuiKey_F1,true);
        if(frame==14)io.AddKeyEvent(ImGuiKey_F1,false);
        if(frame==18)check("focusedPanelF1French",urls_.size()==2&&urls_.back().get<std::string>().find("#/fr/graphs/canvas")!=std::string::npos);
        if(frame==20){app_.previewSettings("en","");app_.open("content");}
        if(frame==24)io.AddKeyEvent(ImGuiKey_F1,true);
        if(frame==26)io.AddKeyEvent(ImGuiKey_F1,false);
        if(frame==30)check("focusedPanelF1English",urls_.size()==3&&urls_.back().get<std::string>().find("#/en/content-browser/browse")!=std::string::npos);
        if(frame==32){app_.helpContext_="graph:material";app_.openDocumentation();check("dynamicMaterialContext",urls_.size()==4&&urls_.back().get<std::string>().find("#/en/materials/nodes")!=std::string::npos);}
    }
    int finish(int frame){if(!enabled()||frame!=34)return -1;bool passed=true;for(const auto& v:checks_.items())passed&=v.value().get<bool>();writeJson(report_,{{"passed",passed},{"checks",checks_},{"urls",urls_},{"input","isolated own-host ImGui F1; intercepted browser opener"}});return passed?0:3;}
};
}
