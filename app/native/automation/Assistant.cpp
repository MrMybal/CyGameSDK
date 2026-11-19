#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include "../AutomationBridge.h"
#include "../SourceEditorBackend.h"
#include "Runtime.h"
#include <cymcp/chat_ui.hpp>
#include <cyui/Core.h>
#include <imgui.h>
#include <windows.h>
#include <future>
#include <array>
#include <algorithm>
#include <cstdio>
namespace cygame {
using automation::Runtime;using automation::Json;namespace fs=std::filesystem;
namespace {
template<size_t N>void assign(std::array<char,N>& b,const std::string& s){std::snprintf(b.data(),b.size(),"%s",s.c_str());}
const char* tr(const char* s){static thread_local std::map<std::string,std::string> cache;auto& text=cache[s];text=cyui::tr(s);return text.c_str();}
const char* label(const char* s){static thread_local std::map<std::string,std::string> cache;auto& text=cache[s];text=cyui::trLabel(s);return text.c_str();}
}
struct AutomationBridge::Impl{
 std::unique_ptr<Runtime> runtime;
 std::shared_ptr<cymcp::ProviderRegistry> providers=std::make_shared<cymcp::ProviderRegistry>();
 std::unique_ptr<cymcp::ChatController> controller;
 std::unique_ptr<cymcp::ChatPanel> panel;
 std::unique_ptr<cymcp::McpServer> mcp;
 std::unique_ptr<cymcp::LocalHttpServer> server;
 Json catalogue,settings=Json::object();std::string provider="codex",error;bool allow=false,saveHistory=true,compact=true,contextWindow=false,showSettings=true;
 std::array<char,256> model{},endpoint{},keyEnv{},historyId{},executable{};
 std::array<char,16384> providerSettings{};std::array<char,2048> apiKey{};SourceEditor settingsEditor;std::string settingsEditorText;int contextKiB=256;
 std::future<std::pair<std::string,Json>> modelDiscovery;cymcp::CallContext modelContext;Json availableModels=Json::array();
 std::future<std::vector<std::string>> historyDiscovery;std::vector<std::string> historyIds;
 cymcp::ProviderConfig discoveredConfig;
 std::future<std::vector<cymcp::DiscoveredConnection>> discovery;std::vector<cymcp::DiscoveredConnection> connections;
 cymcp::CallContext discoveryContext;
 std::atomic_bool closed=false;std::thread refresh;
 fs::path endpointFile;
 Impl(std::string root,std::string install,std::function<void()> picker){
  runtime=std::make_unique<Runtime>(fs::u8path(root),fs::u8path(install));
  settings=runtime->config();allow=settings.value("allowChanges",false);saveHistory=settings.value("saveHistory",true);compact=settings.value("compactDiscovery",true);provider=settings.value("provider","codex");
  catalogue=providers->describe();select(provider);if(settings.contains("providerOptions"))assign(providerSettings,settings["providerOptions"].dump(2));assign(executable,settings.value("executable",""));assign(model,settings.value("model",""));assign(endpoint,settings.value("baseUrl",std::string(endpoint.data())));assign(keyEnv,settings.value("keyEnv",std::string(keyEnv.data())));assign(historyId,settings.value("historyId","editor-assistant"));contextWindow=settings.value("contextWindow",false);contextKiB=settings.value("contextKiB",256);
  controller=std::make_unique<cymcp::ChatController>(providers);
  cymcp::ChatPanelOptions opts;opts.pick_attachments=[picker](){picker();return std::vector<fs::path>{};};opts.open_settings=[this](){showSettings=true;};
  opts.edit_message=[this](std::uint64_t id,const std::string& text){controller->edit_message(id,text);};
  opts.delete_message=[this](std::uint64_t id){controller->delete_turn(id);};opts.require_editable_history=true;
  panel=std::make_unique<cymcp::ChatPanel>(*controller,std::move(opts));
  mcp=std::make_unique<cymcp::McpServer>(*runtime->tools(compact),"CyGameSDK",runtime->serverOptions());
  const auto token=cymcp::random_token();server=std::make_unique<cymcp::LocalHttpServer>(token,[this](const Json& r,const cymcp::CallContext& c){auto local=c;local.principal="local";return r.contains("jsonrpc")?mcp->handle(r,local):cymcp::dispatch_app(*runtime->operations,r,local);});
  const auto port=server->start();endpointFile=runtime->root()/"data/ipc/automation.json";
  automation::writeJson(endpointFile,{{"url","http://127.0.0.1:"+std::to_string(port)},{"token",token},{"pid",GetCurrentProcessId()},{"sdk","CyMCPSDK 0.7.0"}});
  refresh=std::thread([this](){while(!closed){runtime->poll();for(int i=0;i<10&&!closed;++i)std::this_thread::sleep_for(std::chrono::milliseconds(25));}});
 }
 ~Impl(){closed=true;discoveryContext.cancellation.cancel();modelContext.cancellation.cancel();if(controller)controller->cancel();if(server)server->stop();if(refresh.joinable())refresh.join();if(discovery.valid())try{discovery.get();}catch(...){}if(modelDiscovery.valid())try{modelDiscovery.get();}catch(...){}if(historyDiscovery.valid())try{historyDiscovery.get();}catch(...){}panel.reset();controller.reset();apiKey.fill(0);mcp.reset();server.reset();std::error_code ec;fs::remove(endpointFile,ec);runtime.reset();}
 void select(const std::string& id){
  provider=id;discoveredConfig={};apiKey.fill(0);availableModels=Json::array();
  for(const auto& d:catalogue)if(d["id"]==id){
   const auto defaults=d.value("defaults",Json::object());assign(endpoint,defaults.value("base_url",""));assign(keyEnv,defaults.value("credential_env",""));assign(providerSettings,defaults.value("settings",Json::object()).dump(2));return;
  }assign(providerSettings,"{}");
 }
 void persistPermissions(){
  settings["allowChanges"]=allow;automation::writeJson(runtime->root()/"data/settings/automation.json",settings);runtime->configuredChanges(allow);
 }
 void persist(){
  settings["provider"]=provider;
  settings["executable"]=executable.data();
  const auto options=cymcp::parse_json(providerSettings.data());Json safe=Json::object();
  for(const auto* key:{"persistent","deferred_tools","tool_discovery_threshold","max_tool_rounds","max_output_tokens","timeout_ms","turn_timeout_ms","approval_policy","sandbox","setting_sources"})if(options.contains(key))safe[key]=options[key];
  settings["providerOptions"]=safe;settings["model"]=model.data();settings["baseUrl"]=endpoint.data();settings["keyEnv"]=keyEnv.data();settings["historyId"]=historyId.data();settings["saveHistory"]=saveHistory;settings["allowChanges"]=allow;settings["compactDiscovery"]=compact;settings["contextWindow"]=contextWindow;settings["contextKiB"]=contextKiB;
  automation::writeJson(runtime->root()/"data/settings/automation.json",settings);runtime->configuredChanges(allow);
 }
 void connect(bool restore=false,const Json& override=Json::object()){
  cymcp::ChatConnection c;c.provider_id=override.value("provider",provider);c.model=override.value("model",std::string(model.data()));c.system_prompt="You are the CyGameSDK editor assistant. Use the app operation registry for editor actions. Inspect schemas before calling, respect permissions, preserve authored IDs and values, use Undo for reversible edits, never claim a game codec exists without verified output. Ask for missing game-specific facts. Do not run shell commands to bypass app tools.";
  c.provider=discoveredConfig;c.provider.settings=cymcp::parse_json(providerSettings.data());if(endpoint[0])c.provider.settings["base_url"]=endpoint.data();if(override.contains("settings"))c.provider.settings=override["settings"];
  if(executable[0])c.provider.process.executable=fs::u8path(executable.data());
  if(override.contains("executable"))c.provider.process.executable=fs::u8path(override["executable"].get<std::string>());
  c.provider.process.cwd=runtime->root();
  // Only transports whose implementation accepts the operation registry.
  std::string transport;
  for(const auto& d:catalogue)if(d["id"]==c.provider_id)transport=d.at("transport");
  if(transport=="app-server-stdio"||transport=="responses-sse"||transport=="messages-sse"||transport=="chat-completions-sse"||transport=="chat-ndjson")c.provider.tools=runtime->tools(compact);
  c.provider.approvals=cymcp::Policy::deny_all();c.provider.settings["allow_all_actions"]=false;c.provider.settings["deferred_tools"]=compact;
  const auto envName=std::string(keyEnv.data());if(apiKey[0])c.provider.api_key=apiKey.data();else if(!envName.empty())if(const auto* key=std::getenv(envName.c_str()))c.provider.api_key=key;
  const bool local=c.provider_id=="ollama"||c.provider_id=="llama.cpp"||c.provider_id=="lmstudio";
  c.automatic_model=c.model.empty()&&local;
  if(c.model.empty()&&!local&&(transport=="responses-sse"||transport=="messages-sse"||transport=="chat-completions-sse"))throw cymcp::Error("ModelRequired","Select a model from the actual provider catalogue or enter its exact ID.");c.performance=runtime->performance;
  if(contextWindow&&transport!="app-server-stdio"&&transport.find("stdio")==std::string::npos){c.context.max_bytes=size_t(std::clamp(contextKiB,16,65536))*1024;c.context.recent_turns=8;}
  if(saveHistory){c.history_directory=runtime->root()/"data/automation/conversations";c.history_id=historyId.data();c.restore=restore;
   if(!restore&&fs::exists(c.history_directory/fs::u8path(c.history_id+".json"))){c.history_id=c.history_id.substr(0,80)+"-"+cymcp::random_token(16).substr(0,8);assign(historyId,c.history_id);}
  }
  else if(restore)throw cymcp::Error("InvalidOptions","Enable history before restoring");
  controller->open(std::move(c));error.clear();showSettings=false;persist();
 }
 Json state()const{
  const auto& s=controller->state();Json rows=Json::array();
  for(const auto& m:s.messages)rows.push_back({{"role",m.role},{"text",m.markdown},{"interrupted",m.interrupted},{"error",m.error_code},{"attachments",m.attachments.size()}});
  return {{"phase",s.phase},{"busy",s.busy},{"connected",s.connected},{"provider",s.provider_id},{"model",s.model},{"messages",rows},{"errorCode",s.error_code},{"errorMessage",s.error_message},{"storageError",s.storage_error},{"diagnosticCount",s.diagnostics.size()}};
 }
 Json action(const Json& a){
  const auto op=a.at("action").get<std::string>();
  if(op=="connect")connect(a.value("restore",false),a);
  else if(op=="send")controller->send(a.at("text"));
  else if(op=="compose"){panel->set_draft(a.at("text"));showSettings=false;}
  else if(op=="cancel")controller->cancel();
  else if(op=="save")controller->save();
  else if(op=="disconnect")controller->disconnect();
  else if(op=="reconnect")controller->reconnect();
  else if(op=="rights"){allow=a.at("allowChanges");persistPermissions();}
  else if(op=="attach")panel->add_attachments({fs::u8path(a.at("path").get<std::string>())});
  else if(op=="poll")controller->poll();
  else throw cymcp::Error("UnknownOperation","Unknown assistant host action");
  return state();
 }
 void loadModels(){
  if(modelDiscovery.valid())return;
  auto registry=providers;auto id=provider;auto config=discoveredConfig;config.settings=cymcp::parse_json(providerSettings.data());if(endpoint[0])config.settings["base_url"]=endpoint.data();if(executable[0])config.process.executable=fs::u8path(executable.data());
  if(apiKey[0])config.api_key=apiKey.data();else if(keyEnv[0])if(const auto* key=std::getenv(keyEnv.data()))config.api_key=key;
  modelContext={};const auto ctx=modelContext;modelDiscovery=std::async(std::launch::async,[registry,id,config,ctx](){auto p=registry->create(id,config);p->connect(ctx);auto models=p->list_models(ctx);p->disconnect();return std::make_pair(id,models);});
 }
 void loadHistories(){
  if(historyDiscovery.valid())return;
  const auto folder=runtime->root()/"data/automation/conversations";historyDiscovery=std::async(std::launch::async,[folder](){std::vector<std::string> ids;if(fs::is_directory(folder))for(const auto& f:fs::directory_iterator(folder)){if(f.is_regular_file()&&f.path().extension()==".json")ids.push_back(cymcp::path_utf8(f.path().stem()));if(ids.size()>=1000)break;}std::sort(ids.begin(),ids.end());return ids;});
 }
 void draw(){
  controller->poll();allow=runtime->configuredChanges();
  if(modelDiscovery.valid()&&modelDiscovery.wait_for(std::chrono::seconds(0))==std::future_status::ready)try{auto value=modelDiscovery.get();if(value.first==provider)availableModels=std::move(value.second);}catch(const std::exception& e){error=cymcp::redact_error_message(e.what());}
  if(historyDiscovery.valid()&&historyDiscovery.wait_for(std::chrono::seconds(0))==std::future_status::ready)try{historyIds=historyDiscovery.get();}catch(const std::exception& e){error=cymcp::redact_error_message(e.what());}
  if(discovery.valid()&&discovery.wait_for(std::chrono::seconds(0))==std::future_status::ready)try{connections=discovery.get();}catch(const std::exception& e){error=cymcp::redact_error_message(e.what());}
  if(cyui::ghostButton(label(showSettings?"Conversation":"Connection settings")))showSettings=!showSettings;
  ImGui::SameLine();if(ImGui::Checkbox(label("Allow editor changes"),&allow))persistPermissions();
  if(controller->state().busy||discovery.valid()||modelDiscovery.valid()){ImGui::SameLine();if(cyui::ghostButton(label("Stop"))){controller->cancel();discoveryContext.cancellation.cancel();modelContext.cancellation.cancel();}}
  if(!error.empty())cyui::callout(cyui::Tone::error,error.c_str());
  if(showSettings){
   if(cyui::ghostButton(label("Detect providers"))&&!discovery.valid()){
    discoveryContext={};auto p=providers;auto ctx=discoveryContext;discovery=std::async(std::launch::async,[p,ctx](){return p->discover({},ctx);});
   }
   if(discovery.valid()){ImGui::SameLine();cyui::textDim("%s",tr("Detecting providers..."));}
   for(size_t i=0;i<connections.size();++i){auto& d=connections[i];ImGui::PushID(int(i));if(ImGui::Selectable((d.name+" — "+std::string(tr(d.state.c_str()))+"###connection").c_str(),false)&&d.available()){select(d.provider_id);discoveredConfig=d.config;assign(endpoint,d.config.settings.value("base_url",std::string(endpoint.data())));assign(executable,cymcp::path_utf8(d.config.process.executable));assign(providerSettings,d.config.settings.dump(2));}ImGui::PopID();}
   if(ImGui::BeginCombo(label("Provider"),provider.c_str())){for(const auto& d:catalogue){const auto id=d.at("id").get<std::string>();if(ImGui::Selectable(d.at("name").get_ref<const std::string&>().c_str(),id==provider))select(id);}ImGui::EndCombo();}
   ImGui::InputText(label("Endpoint"),endpoint.data(),endpoint.size());ImGui::InputText(label("Executable"),executable.data(),executable.size());ImGui::InputText(label("API key environment variable"),keyEnv.data(),keyEnv.size());
   ImGui::InputText(label("API key (memory only)"),apiKey.data(),apiKey.size(),ImGuiInputTextFlags_Password);
   ImGui::InputText(label("Model ID"),model.data(),model.size());
   const auto& models=availableModels.empty()?controller->state().models:availableModels;
   if(models.is_array()&&!models.empty()&&ImGui::BeginCombo(label("Available models"),model.data())){for(const auto& m:models){const auto id=m.is_string()?m.get<std::string>():m.value("id",m.value("name",""));if(!id.empty()&&ImGui::Selectable(id.c_str()))assign(model,id);}ImGui::EndCombo();}
   if(cyui::ghostButton(label("Refresh models")))loadModels();
   ImGui::Checkbox(label("Save conversation history"),&saveHistory);ImGui::InputText(label("History ID"),historyId.data(),historyId.size());
   if(cyui::ghostButton(label("Refresh saved chats")))loadHistories();
   if(!historyIds.empty()&&ImGui::BeginCombo(label("Saved conversations"),historyId.data())){for(const auto& id:historyIds)if(ImGui::Selectable(id.c_str()))assign(historyId,id);ImGui::EndCombo();}
   ImGui::Checkbox(label("Compact tool discovery"),&compact);ImGui::Checkbox(label("Limit API context window"),&contextWindow);if(contextWindow)ImGui::SliderInt(label("Context budget (KiB)"),&contextKiB,16,4096);
   if(ImGui::CollapsingHeader(label("Provider options (JSON)"))){
    const std::string text=providerSettings.data();if(settingsEditorText!=text){settingsEditor.setText(text);settingsEditorText=text;}settingsEditor.setLanguage(cyui::CodeLanguage::json);
    if(settingsEditor.draw("##assistantProviderJson",cyui::dp(0,160))){auto edited=settingsEditor.text();if(edited.size()<providerSettings.size()){assign(providerSettings,edited);settingsEditorText=edited;}else{settingsEditor.setText(settingsEditorText);cyui::notify(tr("Text exceeds the field limit."),cyui::Tone::error);}}
   }
   try{
    if(cyui::primaryButton(label("Connect / New chat")))connect();
    ImGui::SameLine();if(cyui::ghostButton(label("Restore saved chat")))connect(true);
    ImGui::SameLine();if(cyui::ghostButton(label("Save settings")))persist();
   }catch(const std::exception& e){error=cymcp::redact_error_message(e.what());}
   ImGui::TextWrapped("%s",tr("Direct app tools are available for Codex and compatible API providers. Other CLI agents use their own MCP configuration. Credentials stay in the environment or provider account."));
   cyui::divider();ImGui::TextWrapped("%s",tr("The local MCP endpoint and token are stored in a private automation.json file under data/ipc. The same permissions apply to chat, CLI and MCP."));
  }else panel->draw("CyGameAssistant");
 }
};
AutomationBridge::AutomationBridge(const std::string& root,const std::string& install,std::function<void()> picker):impl_(std::make_unique<Impl>(root,install,std::move(picker))){}
AutomationBridge::~AutomationBridge()=default;
void AutomationBridge::draw(){try{impl_->draw();}catch(const std::exception& e){impl_->error=cymcp::redact_error_message(e.what());cyui::callout(cyui::Tone::error,impl_->error.c_str());}}
void AutomationBridge::attach(const std::string& path){impl_->panel->add_attachments({fs::u8path(path)});}
std::string AutomationBridge::action(const std::string& input){return impl_->action(cymcp::parse_json(input)).dump();}
std::string AutomationBridge::state()const{return impl_->state().dump();}
}
