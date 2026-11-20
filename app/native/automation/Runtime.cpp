#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include "Runtime.h"
#include <windows.h>
#include <wincrypt.h>
#include <fstream>
#include <regex>
#include <set>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <cctype>
namespace cygame::automation {
Json readJson(const fs::path& p){
 HANDLE h=INVALID_HANDLE_VALUE;for(int attempt=0;attempt<30;++attempt){h=CreateFileW(p.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);if(h!=INVALID_HANDLE_VALUE)break;const auto error=GetLastError();if(error!=ERROR_FILE_NOT_FOUND&&error!=ERROR_SHARING_VIOLATION&&error!=ERROR_ACCESS_DENIED&&error!=ERROR_DELETE_PENDING)break;Sleep(2);}
 if(h==INVALID_HANDLE_VALUE)throw cymcp::Error("FileError","Cannot read "+cymcp::path_utf8(p));
 struct Close{HANDLE h;~Close(){CloseHandle(h);}} close{h};LARGE_INTEGER size{};if(!GetFileSizeEx(h,&size)||size.QuadPart<0||size.QuadPart>64*1024*1024)throw cymcp::Error("FileError","JSON file exceeds 64 MiB");
 std::string s(size_t(size.QuadPart),char(0));size_t at=0;while(at<s.size()){DWORD n=0;if(!ReadFile(h,s.data()+at,DWORD(s.size()-at),&n,nullptr)||!n)throw cymcp::Error("FileError","Incomplete JSON file");at+=n;}
 if(s.size()>=3&&s.substr(0,3)=="\xef\xbb\xbf")s.erase(0,3);return cymcp::parse_json(s,64*1024*1024);
}
void writeJson(const fs::path& p,const Json& j){
    fs::create_directories(p.parent_path());
    const auto name=p.parent_path()/fs::u8path(cymcp::path_utf8(p.filename())+"."+cymcp::random_token(16)+".tmp");
    {auto f=cymcp::open_private_output_file(name);auto s=j.dump(2);if(std::fwrite(s.data(),1,s.size(),f.get())!=s.size()||std::fflush(f.get())!=0)throw cymcp::Error("FileError","Cannot write private state");}
    bool moved=false;for(int attempt=0;attempt<30&&!moved;++attempt){moved=MoveFileExW(name.c_str(),p.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=FALSE;if(!moved)Sleep(2);}if(!moved){fs::remove(name);throw cymcp::Error("FileError","Cannot publish private state");}
}
namespace {
thread_local Runtime* scheduledRuntime=nullptr;
thread_local std::string scheduledJob;
class JobFailure:public cymcp::Error{public:Json details;JobFailure(std::string code,std::string message,Json value):cymcp::Error(std::move(code),std::move(message)),details(std::move(value)){}};
bool readOnly(const std::string& n){
 if(n.rfind("op_",0)==0)return readOnly(n.substr(3));
 static const std::set<std::string> names={"editor_status","scene_list","scene_validate","editor_search","assets_list","asset_trash_list","edit_history","graph_get","graph_validate","graph_diagnose","graph_debug_status","graph_function_evaluate","graph_fragment_copy","asset_dependencies","graph_find","material_get","build_describe","build_prepare","graph_diff","material_prepare","material_sample","texture_info","asset_importers","asset_import_prepare","asset_import_info","asset_import_check","model_get","model_export_prepare","model_sample","json_get","model_list","model_process_prepare","game_codec_describe","game_codec_list","graph_types_get","graph_interfaces_get","graph_signature_plan","graph_type_migration_plan","graph_collapse_plan","graph_expand_plan","build_status","material_compile_status","asset_import_status"};
 return names.count(n)!=0||n=="cy_get_job"||n=="cy_list_operations"||n=="cy_describe_operation"||n=="cy_finish_task";
}
Json schema(Json s){
 if(!s.is_object())return s;
 if(s.contains("format")){if(s["format"]!="uuid")throw cymcp::Error("UnsupportedSchema","Unsupported host format");s["description"]=s.value("description",std::string())+" Must be a UUID.";s.erase("format");}
 if(s.contains("pattern")){s["description"]=s.value("description",std::string())+" Required pattern: "+s["pattern"].get<std::string>()+".";s.erase("pattern");}
 if(s.contains("properties"))for(auto& v:s["properties"])v=schema(v);
 for(const auto* k:{"items","additionalProperties","not","if","then","else"})if(s.contains(k))s[k]=schema(s[k]);
 for(const auto* k:{"oneOf","anyOf","allOf"})if(s.contains(k))for(auto& v:s[k])v=schema(v);
 return s;
}
void patterns(const Json& v,const Json& s){
 if(!s.is_object())return;
 if(v.is_string()&&s.value("format","")=="uuid"&&!std::regex_match(v.get<std::string>(),std::regex("[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}")))throw cymcp::Error("InvalidParameters","Value must be a UUID");
 if(v.is_string()&&s.contains("pattern")&&!std::regex_search(v.get<std::string>(),std::regex(s["pattern"].get<std::string>())))throw cymcp::Error("InvalidParameters","Value does not match the operation's required pattern");
 if(v.is_object()&&s.contains("properties"))for(const auto& [k,c]:s["properties"].items())if(v.contains(k))patterns(v[k],c);
 if(v.is_array()&&s.contains("items"))for(const auto& c:v)patterns(c,s["items"]);
 for(const auto* k:{"allOf","anyOf","oneOf"})if(s.contains(k)){
  size_t matches=0;for(const auto& child:s[k])try{cymcp::validate(v,schema(child));patterns(v,child);++matches;}catch(const cymcp::Error&){}
  if((std::string(k)=="allOf"&&matches!=s[k].size())||(std::string(k)=="oneOf"&&matches!=1)||(std::string(k)=="anyOf"&&matches==0))throw cymcp::Error("InvalidParameters","Operation schema alternatives do not match");
 }
}
uint64_t processBirth(HANDLE h){FILETIME birth,exit,kernel,user;if(!GetProcessTimes(h,&birth,&exit,&kernel,&user))return 0;return (uint64_t(birth.dwHighDateTime)<<32)|birth.dwLowDateTime;}
bool alive(DWORD pid,uint64_t birth=0){HANDLE h=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid);if(!h)return GetLastError()==ERROR_ACCESS_DENIED;DWORD exit=0;const bool ok=GetExitCodeProcess(h,&exit)&&exit==STILL_ACTIVE&&(!birth||processBirth(h)==birth);CloseHandle(h);return ok;}
std::string id(const std::string& value){if(value.empty()||value.size()>128||!std::all_of(value.begin(),value.end(),[](unsigned char c){return std::isalnum(c)||c=='-'||c=='_';}))throw cymcp::Error("InvalidParameters","Invalid job ID");return value;}
struct JobLock{HANDLE h{};JobLock(const fs::path& root,const CallContext& c){const auto text="Local\\CyGameSDK-Jobs-"+cymcp::sha256(cymcp::path_utf8(root));h=CreateMutexA(nullptr,FALSE,text.c_str());if(!h)throw cymcp::Error("Busy","Cannot open project worker lock");for(;;){c.cancellation.check();auto r=WaitForSingleObject(h,50);if(r==WAIT_OBJECT_0||r==WAIT_ABANDONED)break;if(r==WAIT_FAILED)throw cymcp::Error("Busy","Cannot acquire project worker lock");}}~JobLock(){if(h){ReleaseMutex(h);CloseHandle(h);}}};
}
cymcp::LocalAccessProfile parseAccessProfile(const std::string& id){
 for(const auto& option:accessProfiles)if(id==option.id)return option.profile;
 if(id=="protected_access")return cymcp::LocalAccessProfile::protected_access;
 throw cymcp::Error("InvalidConfiguration","Unknown local MCP access profile: "+id);
}
const char* accessProfileId(cymcp::LocalAccessProfile value){for(const auto& o:accessProfiles)if(value==o.profile)return o.id;throw cymcp::Error("InvalidConfiguration","Unknown access profile");}
Runtime::Runtime(fs::path root,fs::path install,bool allow,fs::path store,fs::path manifest,std::string profile):root_(fs::weakly_canonical(root)),install_(fs::weakly_canonical(install)),jobStore_(store.empty()?root_/"data/automation/jobs":fs::absolute(store)),permitChanges_(allow){
 cymcp::private_directory(jobStore_);
 const auto settingsFile=root_/"data/settings/automation.json";
 config_=fs::exists(settingsFile)?readJson(settingsFile):Json::object();
 if(!config_.is_object())throw cymcp::Error("InvalidConfiguration","Automation settings must be an object");
 std::string configuredProfile;bool explicitProfile=false;
 for(const auto* key:{"access_profile","accessProfile","localAccessProfile"})if(config_.contains(key)){
  const auto value=config_.at(key).get<std::string>();const auto parsed=parseAccessProfile(value);
  if(explicitProfile&&parseAccessProfile(configuredProfile)!=parsed)throw cymcp::Error("InvalidConfiguration","Conflicting local MCP access profiles");
  configuredProfile=value;explicitProfile=true;
 }
 if(!explicitProfile)configuredProfile="free";
 accessProfile_=parseAccessProfile(profile.empty()?configuredProfile:profile);access_=cymcp::local_access_settings(accessProfile_);
 if(!config_.contains("access_profile")){config_["access_profile"]=accessProfileId(parseAccessProfile(configuredProfile));writeJson(settingsFile,config_);}
 configuredChanges_=config_.value("allowChanges",false);
 manifest_=readJson(manifest.empty()?install_/"app/CyTool.json":manifest);
 changes=std::make_shared<cymcp::ChangeFeed>();
 if(config_.value("performance",false)){cymcp::PerformanceOptions p;p.jsonl_file=root_/"data/automation/metrics"/fs::u8path(cymcp::random_token(16)+".jsonl");performance=std::make_shared<cymcp::PerformanceMonitor>(p);}
 cymcp::Policy policy=access_.control_operations?cymcp::Policy([this](const CallContext&,const std::string& name,const Json&){return permitted(name)?cymcp::Decision::allow:cymcp::Decision::deny;}):cymcp::Policy::allow_all();
 operations=std::make_shared<cymcp::Registry>(policy);operations->set_change_feed(changes);operations->set_performance_monitor(performance);
 cymcp::ResourceOptions ro;ro.chunk_bytes=32768;ro.result_ttl=std::chrono::milliseconds(config_.value("resultTtlMs",300000));ro.max_stored_bytes=config_.value("resultMemoryBytes",64ULL*1024*1024);
 if(config_.value("diskSpooling",true)){ro.disk_directory=root_/"data/automation/spool";ro.max_disk_bytes=config_.value("spoolQuotaBytes",1024ULL*1024*1024);ro.spool_threshold=config_.value("spoolThresholdBytes",1024ULL*1024);}
 resources=std::make_shared<cymcp::ResourceRegistry>(cymcp::Policy::allow_all(),ro);resources->set_change_feed(changes);
 auto catalogue=std::make_shared<const std::string>(manifest_.dump(2));
 resources->add({"cygame://catalogue","Operation catalogue","Application schemas and descriptions","application/json",catalogue->size(),false,std::chrono::milliseconds(30000),[catalogue](size_t o,size_t n,const CallContext&){return catalogue->substr(std::min(o,catalogue->size()),n);}});
 for(const auto& d:manifest_["operations"]){
  const std::string name=d.at("id");cymcp::Operation op;op.name=name;op.description=d.value("description",d.at("name").get<std::string>());op.input_schema=schema(d.at("inputSchema"));op.output_schema=schema(d.value("outputSchema",Json{{"type","object"}}));if(!op.output_schema.contains("type"))op.output_schema["type"]="object";op.read_only=readOnly(name);
  op.handler=[this,name,original=d.at("inputSchema")](const Json& a,const CallContext& c){
   patterns(a,original);
   static const std::set<std::string> jobs={"build_execute","material_compile","asset_import","asset_reimport","model_export","model_process","game_codec_list","game_codec_export","portable_pack","portable_inspect","portable_export_glb"};
   return jobs.count(name)?runJob(name,a,c):editor(name,a,c);
  };
  operations->add(std::move(op));
 }
 // Compatibility aliases keep existing op_* MCP clients on native SDK jobs.
 for(const auto& d:manifest_["operations"]){
  const std::string name=d.at("id");Json input={{"type","object"},{"properties",{{"parameters",schema(d["inputSchema"])}}},{"required",{"parameters"}},{"additionalProperties",false}};
  operations->add({"op_"+name,"Compatibility job submission for "+name+". New clients may call "+name+" directly.",input,{{"type","object"}},readOnly(name),[this,name](const Json& a,const CallContext& c){return Json{{"result",submit(name,a.at("parameters"),c)}};}});
 }
 registerControls();
 cymcp::ToolGatewayOptions go;go.inline_bytes=config_.value("inlineResultBytes",16384);go.resources=ro;
 gateway_=std::make_shared<cymcp::ToolGateway>(operations,go,resources);
}
Json Runtime::editor(const std::string& op,const Json& args,const CallContext& ctx){
 std::unique_lock<std::timed_mutex> guard(pipe_,std::defer_lock);while(!guard.try_lock_for(std::chrono::milliseconds(20)))ctx.cancellation.check();ctx.cancellation.check();
 auto ep=readJson(root_/"data/ipc/editor.json");const std::string pipe=ep.at("pipe");
 if(pipe.rfind("\\\\.\\pipe\\CyGameSDK-",0)!=0)throw cymcp::Error("InvalidConfiguration","Invalid editor endpoint");
 auto wide=fs::u8path(pipe).wstring();HANDLE h=INVALID_HANDLE_VALUE;auto until=std::min(cymcp::Clock::now()+std::chrono::seconds(12),ctx.cancellation.deadline);
 while(h==INVALID_HANDLE_VALUE){ctx.cancellation.check();if(cymcp::Clock::now()>until)throw cymcp::Error("EditorUnavailable","Start the editor for this --root before invoking live operations");if(WaitNamedPipeW(wide.c_str(),50))h=CreateFileW(wide.c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_EXISTING,0,nullptr);}
 struct Close{HANDLE h;~Close(){CloseHandle(h);}} close{h};
 const auto payload=Json{{"protocolVersion",1},{"operation",op},{"parameters",args}}.dump();if(payload.size()>1024*1024)throw cymcp::Error("InvalidParameters","Editor request exceeds 1 MiB");
 std::string wire(4,'\0');auto count=uint32_t(payload.size());std::memcpy(wire.data(),&count,4);wire+=payload;DWORD bytes{};
 if(!WriteFile(h,wire.data(),DWORD(wire.size()),&bytes,nullptr)||bytes!=wire.size())throw cymcp::Error("EditorUnavailable","Editor request was not fully written; inspect state before retrying");
 auto read=[&](size_t size){std::string data;while(data.size()<size){ctx.cancellation.check();if(cymcp::Clock::now()>until)throw cymcp::Error("EditorTimeout","Inspect the live state before retrying a mutation");DWORD n=0;if(!PeekNamedPipe(h,nullptr,0,nullptr,&n,nullptr))throw cymcp::Error("EditorUnavailable","Editor disconnected");if(!n){std::this_thread::sleep_for(std::chrono::milliseconds(2));continue;}char b[65536];if(!ReadFile(h,b,DWORD(std::min({size-data.size(),size_t(n),sizeof(b)})),&bytes,nullptr))throw cymcp::Error("EditorUnavailable","Editor disconnected");data.append(b,bytes);}return data;};
 auto header=read(4);std::memcpy(&count,header.data(),4);if(count>4*1024*1024)throw cymcp::Error("InvalidOutput","Editor response exceeds 4 MiB");auto response=cymcp::parse_json(read(count),4*1024*1024);
 if(response.contains("error"))throw cymcp::Error(response["error"].value("code","EditorCommandFailed"),response["error"].value("message","Editor call failed"));
 return response.at("result");
}
Json Runtime::job(const std::string& name,const CallContext& c)const{
 auto j=readJson(jobStore_/id(name)/"job.json");if(j.value("owner","")!=c.principal)throw cymcp::Error("PermissionDenied","Job belongs to another authenticated identity");
 if((j["state"]=="Running"||j["state"]=="Created")&&!alive(j.value("pid",0UL),j.value("ownerStartTime",uint64_t(0)))){j["state"]="Interrupted";j["error"]={{"code","Interrupted"},{"message","The owning process ended. Inspect outputs; this action is never replayed automatically."}};writeJson(jobStore_/name/"job.json",j);}
 return j;
}
Json Runtime::jobPage(const CallContext& c,size_t offset,size_t limit,const std::string& state,const std::string& operation)const{
 std::vector<std::string> ids;Json out=Json::array();size_t skipped=0,unreadable=0;
 if(fs::exists(jobStore_))for(const auto& entry:fs::directory_iterator(jobStore_)){c.cancellation.check();if(entry.is_directory())ids.push_back(cymcp::path_utf8(entry.path().filename()));if(ids.size()>100000)throw cymcp::Error("BudgetExceeded","Job index exceeds 100000 entries; archive old jobs explicitly");}
 std::sort(ids.begin(),ids.end());bool more=false;
 for(const auto& name:ids){c.cancellation.check();Json record;try{record=job(name,c);}catch(const cymcp::Error& e){if(e.code!="PermissionDenied")++unreadable;continue;}
  if(!state.empty()&&record.value("state","")!=state)continue;if(!operation.empty()&&record.value("operation",record.value("operationId",""))!=operation)continue;
  if(skipped++<offset)continue;if(out.size()==limit){more=true;break;}out.push_back(std::move(record));
 }
 Json page={{"jobs",out},{"unreadableRecords",unreadable}};if(more)page["nextOffset"]=offset+out.size();return page;
}
Json Runtime::jobs(const CallContext& c)const{
 Json all=Json::array();size_t offset=0;for(;;){auto page=jobPage(c,offset,200);for(auto& record:page["jobs"])all.push_back(std::move(record));if(!page.contains("nextOffset"))break;offset=page["nextOffset"];}return all;
}
Json Runtime::cancelJob(const std::string& name,const CallContext& c){auto j=job(name,c);{std::lock_guard lock(jobsMutex_);auto i=activeJobs_.find(name);if(i!=activeJobs_.end())i->second.cancel();}writeJson(jobStore_/id(name)/"cancel.request",{{"requested",true}});return j;}
Json Runtime::runJob(const std::string& name,const Json& args,const CallContext& c){
 const auto jobId=scheduledRuntime==this?scheduledJob:cymcp::random_token(16);const auto work=jobStore_/jobId;cymcp::private_directory(work);
 Json state={{"jobId",jobId},{"operation",name},{"state","Created"},{"owner",c.principal},{"pid",GetCurrentProcessId()},{"ownerStartTime",processBirth(GetCurrentProcess())},{"progress",{{"percent",0}}},{"workspace",cymcp::path_utf8(work)}};
 const auto publish=[&](){writeJson(work/"job.json",state);if(!desktop_.empty())writeJson(fs::u8path(desktop_)/"state.json",{{"runId",fs::path(fs::u8path(desktop_)).filename().string()},{"state",state["state"]=="Created"?"Created":"Running"},{"job",state},{"clientClosed",false}});};
 publish();
 try{
  JobLock lock(root_,c);c.cancellation.check();state["state"]="Running";publish();
  cymcp::ProcessOptions p;p.executable=install_/"runtime/python/Scripts/python.exe";p.arguments={cymcp::path_utf8(install_/"app/job_runner.py")};p.cwd=install_;p.environment["CYGAME_EDITOR_ROOT"]=cymcp::path_utf8(root_);p.environment["PYTHONUTF8"]="1";p.max_line_bytes=16*1024*1024;
  auto child=std::make_unique<cymcp::Process>(p);child->write(Json{{"workspace",cymcp::path_utf8(work)},{"operation",name},{"parameters",args},{"workerConfig",workerConfig_}}.dump()+"\n");child->close_input();
  std::atomic_bool finished=false;std::thread cancellation([&](){try{while(!finished){bool cancel=c.cancellation.cancelled()||cymcp::Clock::now()>c.cancellation.deadline||fs::exists(work/"cancel.request")||(!desktop_.empty()&&fs::exists(fs::u8path(desktop_)/"cancel.request"));if(cancel){writeJson(work/"cancel.request",{{"requested",true}});for(int i=0;i<100&&!finished;++i)std::this_thread::sleep_for(std::chrono::milliseconds(10));if(!finished)child->terminate();return;}std::this_thread::sleep_for(std::chrono::milliseconds(20));}}catch(...){child->terminate();}});
  Json result;std::string error;try{cymcp::ProcessLine line;while(child->next(line)){if(line.stderr_stream){if(error.size()<4096)error+=line.text.substr(0,4096-error.size());continue;}auto message=cymcp::parse_json(line.text,16*1024*1024);if(message.contains("workerPid")){state["workerPid"]=message["workerPid"];if(!state.contains("workerPids"))state["workerPids"]=Json::array();if(state["workerPids"].size()<16)state["workerPids"].push_back(message["workerPid"]);publish();}else if(message.contains("progress")){state["progress"]={{"percent",message["progress"]}};state["currentStep"]=message.value("currentStep","");publish();c.emit("Progress",{{"progress",state["progress"]}});}else if(message.contains("result"))result=message["result"];else if(message.contains("error")){state["error"]=message["error"];}}auto code=child->wait();finished=true;cancellation.join();child.reset();
   if(code!=0||result.is_null()){if(fs::exists(work/"cancel.request"))throw cymcp::Error("Cancelled","Job cancelled; inspect persistent destinations before retrying");if(state.contains("error"))throw JobFailure(state["error"].value("code","WorkerFailed"),state["error"].value("message","Worker failed"),state["error"].value("details",Json()));throw cymcp::Error("WorkerFailed",cymcp::redact_error_message(error.empty()?"Worker produced no completed result":error));}
  }catch(...){finished=true;if(cancellation.joinable())cancellation.join();throw;}
  // Preserve a successful result even when cancellation arrived after its persistent commit.
  state["state"]="Completed";state["progress"]={{"percent",100}};state["outputs"]=result;writeJson(work/"result.json",result);publish();return result;
 }catch(const cymcp::Error& e){state["state"]=e.code=="Cancelled"?"Cancelled":"Failed";state["error"]={{"code",e.code},{"message",e.what()}};if(const auto* jobError=dynamic_cast<const JobFailure*>(&e);jobError&&!jobError->details.is_null())state["error"]["technicalDetails"]=jobError->details;publish();throw;}
 catch(const std::exception& e){state["state"]="Failed";state["error"]={{"code","WorkerFailed"},{"message",cymcp::redact_error_message(e.what())}};publish();throw;}
}

bool Runtime::permitted(const std::string& name)const{
 if(!access_.control_operations)return true;
 if(readOnly(name)||name=="job_cancel"||name=="job_get"||name=="job_list"||name=="automation_info"||name=="job_result_read"||name=="job_output_read"||name=="job_output_open")return true;
 bool configured=false;try{const auto path=root_/"data/settings/automation.json";if(fs::exists(path))configured=readJson(path).value("allowChanges",false);}catch(...){}configuredChanges_=configured;
 return permitChanges_||configured;
}
Runtime::~Runtime(){{std::lock_guard lock(jobsMutex_);for(auto& [id,c]:activeJobs_)c.cancel();}executor_.reset();}
Json Runtime::submit(const std::string& name,const Json& args,const CallContext& c){
 if(!permitted(name))throw cymcp::Error("PermissionDenied","Editor changes require host authorization");const auto d=operations->describe(name);cymcp::validate(args,d.at("inputSchema"));
 for(const auto& op:manifest_["operations"])if(op["id"]==name)patterns(args,op["inputSchema"]);
 const auto jobId=cymcp::random_token(16);const auto path=jobStore_/jobId;cymcp::private_directory(path);
 auto child=c;child.cancellation=c.cancellation.child();child.request_id=jobId;
 Json initial={{"jobId",jobId},{"operation",name},{"owner",c.principal},{"state","Created"},{"pid",GetCurrentProcessId()},{"ownerStartTime",processBirth(GetCurrentProcess())},{"workspace",cymcp::path_utf8(path)},{"progress",{{"percent",0}}}};
 writeJson(path/"job.json",initial);
 {std::lock_guard lock(jobsMutex_);activeJobs_[jobId]=child.cancellation;}
 executor_->post([this,name,args,child,initial,path,jobId]()mutable{
  auto j=initial;scheduledRuntime=this;scheduledJob=jobId;struct ClearScheduled{~ClearScheduled(){scheduledRuntime=nullptr;scheduledJob.clear();}} clear;
  try{child.cancellation.check();j["state"]="Running";writeJson(path/"job.json",j);j["outputs"]=operations->invoke(name,args,child);j["state"]="Completed";j["progress"]={{"percent",100}};writeJson(path/"result.json",j["outputs"]);}
  catch(const JobFailure& e){j["state"]=e.code=="Cancelled"?"Cancelled":"Failed";j["error"]={{"code",e.code},{"message",e.what()},{"technicalDetails",e.details}};}
  catch(const cymcp::Error& e){bool completed=false;try{auto saved=readJson(path/"job.json");if((e.code=="Cancelled"||e.code=="Timeout")&&saved.value("state","")=="Completed"){j=std::move(saved);completed=true;}}catch(...){}if(!completed){j["state"]=e.code=="Cancelled"?"Cancelled":"Failed";j["error"]={{"code",e.code},{"message",e.what()}};}}
  catch(const std::exception& e){j["state"]="Failed";j["error"]={{"code","InternalError"},{"message",cymcp::redact_error_message(e.what())}};}
  try{auto saved=readJson(path/"job.json");saved.update(j);j=std::move(saved);}catch(...){}writeJson(path/"job.json",j);std::lock_guard lock(jobsMutex_);activeJobs_.erase(jobId);
 });
 return initial;
}
void Runtime::registerControls(){
 auto add=[&](std::string n,std::string d,Json input,bool ro,auto fn){operations->add({std::move(n),std::move(d),std::move(input),{{"type","object"}},ro,fn});};
 const Json none={{"type","object"},{"additionalProperties",false}},jobInput={{"type","object"},{"properties",{{"jobId",{{"type","string"},{"minLength",1},{"maxLength",128}}}}},{"required",{"jobId"}},{"additionalProperties",false}};
 add("automation_info","Inspect the actual SDK revision, application configuration and available modes.",none,true,[this](const Json&,const CallContext&){return Json{{"sdk","CyMCPSDK"},{"sdkVersion",cymcp::version},{"sdkRevision","34d4bcae126bc83d1a2925b5047dac36e3d09d92"},{"editorVersion",manifest_["version"]},{"operations",manifest_["operations"].size()},{"accessProfile",accessProfileId(accessProfile_)},{"requireToken",access_.require_token},{"controlOperations",access_.control_operations},{"root",cymcp::path_utf8(root_)}};});
 add("job_get","Recover a durable job state and outputs. Interrupted jobs are never replayed.",jobInput,true,[this](const Json& a,const CallContext& c){return job(a.at("jobId"),c);});
 Json listInput=none;listInput["properties"]={{"offset",{{"type","integer"},{"minimum",0}}},{"limit",{{"type","integer"},{"minimum",1},{"maximum",200}}},{"state",{{"type","string"}}},{"operation",{{"type","string"}}}};
 add("job_list","List owned durable jobs with filtering and pagination. Follow nextOffset; no automatic replay or retention-based deletion.",listInput,true,[this](const Json& a,const CallContext& c){return jobPage(c,a.value("offset",size_t(0)),a.value("limit",size_t(100)),a.value("state",""),a.value("operation",""));});
 add("job_cancel","Request cooperative cancellation of an owned job. A completed commit is preserved.",jobInput,false,[this](const Json& a,const CallContext& c){return cancelJob(a.at("jobId"),c);});
 add("cy_get_job","Compatibility durable job polling.",jobInput,true,[this](const Json& a,const CallContext& c){return Json{{"result",job(a.at("jobId"),c)}};});
 add("cy_list_operations","Compatibility operation discovery.",none,true,[this](const Json&,const CallContext&){return Json{{"result",manifest_["operations"]}};});
 add("cy_finish_task","Finish a client task without deleting durable results.",none,true,[](const Json&,const CallContext&){return Json{{"result",{{"finished",true},{"resultsRetained",true}}}};});
 Json fileInput=jobInput;fileInput["properties"]["path"]={{"type","string"},{"minLength",1},{"maxLength",1024}};fileInput["required"].push_back("path");
 auto file=[this](const Json& a,const CallContext& c){auto record=job(a.at("jobId"),c);if(record.value("state","")=="Running"||record.value("state","")=="Created")throw cymcp::Error("Busy","Wait for the producer to stop before reading its output files");const auto base=fs::weakly_canonical(fs::u8path(record.at("workspace").get<std::string>()));auto relative=fs::u8path(a.at("path").get<std::string>());if(relative.is_absolute()||relative.has_root_name())throw cymcp::Error("InvalidPath","Use a relative path in the owned job workspace");auto path=fs::weakly_canonical(base/relative),inside=path.lexically_relative(base);if(inside.empty()||inside.is_absolute()||*inside.begin()==".."||!fs::is_regular_file(path))throw cymcp::Error("InvalidPath","Output is not a regular file inside this owned job");return path;};
 add("job_output_open","Stream an owned output file once into a private, quota-limited SDK resource. Follow its chunk URIs; the original durable output is retained.",fileInput,true,[this,file](const Json& a,const CallContext& c){auto path=file(a,c);return resources->store_stream([path](std::ostream& out,const CallContext& ctx){std::ifstream f(path,std::ios::binary);char bytes[65536];while(f){ctx.cancellation.check();f.read(bytes,sizeof(bytes));out.write(bytes,f.gcount());}if(!f.eof())throw cymcp::Error("FileError","Output read failed");},"application/octet-stream",c,true);});
 fileInput["properties"]["offset"]={{"type","integer"},{"minimum",0}};fileInput["properties"]["length"]={{"type","integer"},{"minimum",1},{"maximum",65536}};
 add("job_output_read","Read an owned durable output in base64 chunks. Works across CLI invocations and after transient resource expiration; never re-executes the producing operation.",fileInput,true,[file](const Json& a,const CallContext& c){auto path=file(a,c);auto total=fs::file_size(path),offset=a.value("offset",uint64_t(0));auto size=std::min(uint64_t(a.value("length",65536)),total-std::min(total,offset));if(offset>total)throw cymcp::Error("InvalidParameters","Offset exceeds output size");std::ifstream f(path,std::ios::binary);f.seekg(offset);std::string bytes(size,'\0');f.read(bytes.data(),size);if(size_t(f.gcount())!=size)throw cymcp::Error("FileError","Output changed during read");DWORD length=0;CryptBinaryToStringA(reinterpret_cast<const BYTE*>(bytes.data()),DWORD(size),CRYPT_STRING_BASE64|CRYPT_STRING_NOCRLF,nullptr,&length);std::string encoded(length,'\0');if(!CryptBinaryToStringA(reinterpret_cast<const BYTE*>(bytes.data()),DWORD(size),CRYPT_STRING_BASE64|CRYPT_STRING_NOCRLF,encoded.data(),&length))throw cymcp::Error("FileError","Cannot encode output");encoded.resize(length);while(!encoded.empty()&&encoded.back()==0)encoded.pop_back();return Json{{"base64",encoded},{"offset",offset},{"nextOffset",offset+size},{"totalBytes",total},{"complete",offset+size==total}};});
 Json part=jobInput;part["properties"]["offset"]={{"type","integer"},{"minimum",0}};part["properties"]["length"]={{"type","integer"},{"minimum",1},{"maximum",65536}};
 add("job_result_read","Read the complete durable JSON result in bounded chunks, including after SDK resource TTL expiry.",part,true,[this](const Json& a,const CallContext& c){auto j=job(a.at("jobId"),c);const auto bytes=j.value("outputs",Json::object()).dump();size_t offset=a.value("offset",size_t(0)),length=a.value("length",size_t(32768));if(offset>bytes.size())throw cymcp::Error("InvalidParameters","Offset exceeds result");if(offset<bytes.size()&&(static_cast<unsigned char>(bytes[offset])&0xc0)==0x80)throw cymcp::Error("InvalidParameters","Offset splits UTF-8");size_t end=std::min(offset+length,bytes.size());while(end>offset&&end<bytes.size()&&(static_cast<unsigned char>(bytes[end])&0xc0)==0x80)--end;if(end==offset&&offset<bytes.size())throw cymcp::Error("InvalidParameters","Increase chunk length to include a UTF-8 character");return Json{{"text",bytes.substr(offset,end-offset)},{"totalBytes",bytes.size()},{"nextOffset",end},{"complete",end==bytes.size()}};});
}
std::shared_ptr<const cymcp::Registry> Runtime::tools(bool compact)const{return compact?gateway_->tools():operations;}
cymcp::McpServerOptions Runtime::serverOptions()const{cymcp::McpServerOptions o;o.resources=resources;o.changes=config_.value("notifications",true)?changes:nullptr;o.performance=performance;o.cache_ttl=std::chrono::milliseconds(config_.value("cacheTtlMs",30000));return o;}
void Runtime::poll(){
 try{permitted("_host_policy_refresh");if(!fs::exists(root_/"data/ipc/editor.json")||!alive(readJson(root_/"data/ipc/editor.json").value("pid",0UL))){if(!lastRevision_.empty()){resources->remove("cygame://scene");lastRevision_.clear();}return;}const auto rev=readJson(root_/"data/ipc/revision.json").dump();if(rev==lastRevision_)return;auto text=std::make_shared<const std::string>([&](){CallContext c;c.cancellation.deadline=cymcp::Clock::now()+std::chrono::milliseconds(400);return editor("scene_list",Json::object(),c).dump();}());cymcp::Resource snapshot{"cygame://scene","Scene snapshot","Current native editor scene, refreshed after document revisions","application/json",text->size(),false,std::chrono::milliseconds(config_.value("cacheTtlMs",30000)),[text](size_t o,size_t n,const CallContext&){return text->substr(std::min(o,text->size()),n);}};if(lastRevision_.empty())resources->add(std::move(snapshot));else resources->replace(std::move(snapshot));lastRevision_=rev;}catch(...){}
}
}
