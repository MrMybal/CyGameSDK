#include "Editor.h"
#include "Graph.h"
#include "BuildClient.h"
#include <fstream>
namespace cygame {
namespace {
bool activeJob(const Json& status){const auto state=status.value("state","Idle");return state!="Idle"&&state!="Completed"&&state!="Failed"&&state!="Cancelled"&&state!="Interrupted";}
fs::path ownedFile(const fs::path& base,const fs::path& input){const auto root=fs::weakly_canonical(base),path=fs::weakly_canonical(input),relative=path.lexically_relative(root);if(relative.empty()||relative.is_absolute()||*relative.begin()==".."||!fs::is_regular_file(path))throw std::runtime_error("Shader files must belong to the editor workspace");return path;}
std::string sourceText(const fs::path& path){if(fs::file_size(path)>1024*1024)throw std::runtime_error("Shader source exceeds 1 MiB");std::ifstream file(path,std::ios::binary);return std::string(std::istreambuf_iterator<char>(file),{});}
}
Json EditorApp::shaderAction(const std::string& operation,const Json& p){
    const std::string id=p.at("graphId");const auto domain=core.graphs().at(id).at("domain");if(domain!="material"&&domain!="material-instance")throw std::runtime_error("Preview requires a material");auto& view=shaderViews_[id];
    if(operation=="material_preview_settings"){if(core.graphs().at(id).at("domain")!="material"&&core.graphs().at(id).at("domain")!="material-instance")throw std::runtime_error("Preview requires a material");view.plane=p.at("shape")=="Plane";return {{"shape",p.at("shape")}};}
    if(operation=="material_compiler_view"){compilerGraph_=id;return open("compiler");}
    if(operation=="material_preview_apply"){
        auto prepared=core.execute("material_prepare",{{"graphId",id}});const auto path=ownedFile(core.root(),fs::u8path(p.at("reportPath").get<std::string>()));auto report=readJson(path);
        if(report.value("format","")!="cygame-preview-shader"||report.value("schemaVersion",0)!=1||!report.value("success",false)||report.at("graphId")!=id)throw std::runtime_error("Invalid compiled preview report");
        if(report.at("signature")!=prepared.at("signature")||report.at("parameterNames")!=prepared.at("parameterNames"))throw std::runtime_error("Compiled preview is stale; compile the current graph");
        report["shaderPath"]=ownedFile(path.parent_path(),fs::u8path(report.at("shaderPath").get<std::string>())).u8string();
        auto source=sourceText(ownedFile(path.parent_path(),fs::u8path(report.at("sourcePath").get<std::string>())));
        if(sha256(source)!=report.at("sourceSha256"))throw std::runtime_error("Shader source checksum mismatch");
        const auto textures=report.value("textures",Json::array()),expected=prepared.value("textures",Json::array());if(textures.size()!=expected.size())throw std::runtime_error("Compiled texture binding count mismatch");
        for(size_t i=0;i<textures.size();++i){if(textures[i].at("assetPath")!=expected[i].at("assetPath")||textures[i].at("sha256")!=expected[i].at("sha256"))throw std::runtime_error("Compiled textures are stale");auto file=ownedFile(path.parent_path(),fs::u8path(textures[i].at("filePath").get<std::string>()));if(fs::file_size(file)>32*1024*1024)throw std::runtime_error("Texture exceeds preview limits");std::ifstream input(file,std::ios::binary);if(sha256(std::string(std::istreambuf_iterator<char>(input),{}))!=textures[i].at("sha256"))throw std::runtime_error("Texture snapshot checksum mismatch");report["textures"][i]["filePath"]=file.u8string();}
        if(!materialShaderInstaller)throw std::runtime_error("DX11 preview renderer unavailable");materialShaderInstaller(id,report);
        view.prepared=prepared;view.revision=core.revision();view.installed=report;view.source=source;view.error.clear();return {{"applied",true},{"signature",prepared.at("signature")}};
    }
    if(operation=="material_compile_request"){
        auto prepared=core.execute("material_prepare",p);size_t active=0;for(auto& [key,v]:shaderViews_)if(v.client&&activeJob(v.client->status()))++active;
        if(active>=2)throw std::runtime_error("Two desktop preview jobs are already active");
        if(!view.client)view.client=std::make_shared<BuildClient>(core.root());view.error.clear();view.prepared=prepared;view.revision=core.revision();return view.client->start(p,"material_compile");
    }
    if(operation=="material_compile_cancel"){if(view.client)return view.client->cancel();return {{"state","Idle"}};}
    return shaderStatus(id);
}
Json EditorApp::shaderStatus(const std::string& id){
    auto& view=shaderViews_[id];
    if(view.prepared.is_null()||view.revision!=core.revision()){
        view.revision=core.revision();try{view.prepared=core.execute("material_prepare",{{"graphId",id}});view.error.clear();}catch(const std::exception& e){view.prepared=Json::object();view.error=e.what();}
    }
    Json status=view.client?view.client->status():Json{{"state","Idle"}};
    if(status.value("state","")=="Completed"&&status.contains("job")){
        const auto job=status.at("job");const std::string key=job.at("jobId");if(view.attemptedJob!=key){view.attemptedJob=key;try{shaderAction("material_preview_apply",{{"graphId",id},{"reportPath",job.at("outputs").at("reportPath")}});}catch(const std::exception& e){view.error=e.what();}}
    }
    status["current"]=view.prepared.contains("signature")&&view.installed.contains("signature")&&view.prepared.at("signature")==view.installed.at("signature");
    if(!view.error.empty())status["previewError"]=view.error;if(view.prepared.contains("signature"))status["signature"]=view.prepared.at("signature");return status;
}
void EditorApp::materialPreviewUI(const std::string& id){
    auto status=shaderStatus(id);auto& view=shaderViews_[id];
    cyui::sectionTitle(tr("Material preview"));
    {cyui::ScopedDisabled running(activeJob(status));if(cyui::ghostButton(tr("Compile preview")))invoke("material_compile_request",{{"graphId",id},{"expectedRevision",core.revision()}});}
    ImGui::SameLine();if(cyui::iconButton(cyui::icon::code,tr("Shader source and diagnostics")))invoke("material_compiler_view",{{"graphId",id}});
    cyui::badge(tr(status.value("current",false)?"GPU graph preview":"UV center sample"));if(activeJob(status))cyui::textDim("%s",tr(status.value("state","Idle").c_str()));
    const char* shapes[]={tr("Sphere"),tr("Plane")};int shape=view.plane?1:0;if(cyui::beginProperties("##previewProperties",cyui::dp(65))){cyui::property(tr("Surface"));if(ImGui::Combo("##shape",&shape,shapes,2))view.plane=shape==1;cyui::endProperties();}
    Json preview=view.prepared.value("preview",Json{{"baseColor",{.2,.45,.8}},{"roughness",.5},{"metallic",0.}});preview["_previewPlane"]=view.plane;
    if(status.value("current",false)){preview["_shaderSignature"]=view.prepared.at("signature");preview["_shaderParameters"]=view.prepared.at("parameters");}
    if(materialRenderer){const float width=std::max(cyui::dp(96),ImGui::GetContentRegionAvail().x);try{ImGui::Image(materialRenderer(id,preview,{width,width}),{width,width});}catch(const std::exception& e){cyui::textDim("%s",e.what());}}
    if(!status.value("current",false)&&view.prepared.contains("sampleError"))cyui::textDim("%s",view.prepared.at("sampleError").get_ref<const std::string&>().c_str());
    if(status.contains("previewError"))ImGui::TextWrapped("%s",status.at("previewError").get_ref<const std::string&>().c_str());
    else if(!status.value("current",false))ImGui::TextWrapped("%s",tr("Compile to preview the graph across the surface. Material parameters then update live."));
}
void EditorApp::compilerPanel(){
    cyui::heading(tr("Material Compiler"),2);
    if(compilerGraph_.empty()||!core.graphs().contains(compilerGraph_)){compilerGraph_="material";}
    ImGui::SetNextItemWidth(cyui::dp(380));if(ImGui::BeginCombo("##compilerGraph",compilerGraph_.c_str())){for(const auto& [key,g]:core.graphs().items())if((g.at("domain")=="material"||g.at("domain")=="material-instance")&&ImGui::Selectable(key.c_str(),key==compilerGraph_))compilerGraph_=key;ImGui::EndCombo();}
    auto status=shaderStatus(compilerGraph_);auto& view=shaderViews_[compilerGraph_];
    {cyui::ScopedDisabled running(activeJob(status));if(cyui::primaryButton(tr("Compile preview")))invoke("material_compile_request",{{"graphId",compilerGraph_},{"expectedRevision",core.revision()}});}
    ImGui::SameLine();if(cyui::ghostButton(tr("Refresh material"))){view.revision=~uint64_t(0);view.error.clear();}
    if(activeJob(status)){ImGui::SameLine();if(cyui::ghostButton(tr("Cancel job")))invoke("material_compile_cancel",{{"graphId",compilerGraph_}});}
    cyui::badge(tr(status.value("state","Idle").c_str()));ImGui::SameLine();cyui::badge(tr(status.value("current",false)?"Current shader":"Compile required"));
    if(status.contains("previewError"))cyui::callout(cyui::Tone::error,status.at("previewError").get_ref<const std::string&>().c_str());
    if(status.contains("error")&&status.at("error").is_string())cyui::callout(cyui::Tone::error,status.at("error").get_ref<const std::string&>().c_str());
    Json diagnostics=Json::array();if(status.contains("job")){
        const auto job=status.at("job");if(job.contains("progress")&&job.at("progress").is_object())cyui::progressBar(job.at("progress").value("percent",0.f)/100.f);
        Json report;if(job.contains("outputs"))report=job.at("outputs");if(job.contains("error")&&job.at("error").is_object()){
            const auto error=job.at("error");ImGui::TextWrapped("%s",error.value("message","").c_str());if(error.contains("technicalDetails")&&error.at("technicalDetails").is_object())report=error.at("technicalDetails");
        }
        if(report.is_object())diagnostics=report.value("diagnostics",Json::array());
    }
    for(size_t i=0;i<diagnostics.size();++i){const auto& d=diagnostics[i];if(ImGui::Selectable(label(d.at("message").get<std::string>(),"diagnostic"+std::to_string(i)).c_str())&&!d.value("nodeId","").empty())focusGraphNode(compilerGraph_,d.value("functionName",""),d.at("nodeId"));}
    cyui::sectionTitle(tr("Generated HLSL"));if(view.source.empty())cyui::textDim("%s",tr("Compile a supported material to inspect its shader source."));else{std::vector<char> text(view.source.begin(),view.source.end());text.push_back(0);ImGui::InputTextMultiline("##shaderSource",text.data(),text.size(),{-1,-1},ImGuiInputTextFlags_ReadOnly);}
}
}
