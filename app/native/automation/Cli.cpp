#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include "Runtime.h"
#include <windows.h>
#include <iostream>
#include <csignal>
namespace {
std::atomic_bool stopped=false;void stop(int){stopped=true;}
std::string utf8(const wchar_t* p){return cymcp::path_utf8(std::filesystem::path(p));}
}
int wmain(int argc,wchar_t** argv){
 using namespace cygame::automation;
 try{
  wchar_t exe[32768]{};GetModuleFileNameW(nullptr,exe,32768);auto install=fs::path(exe).parent_path(),root=install,store=fs::path{},run=fs::path{},manifest=fs::path{};
  std::string command="describe",operation,params="{}",connect,token,workerConfig;bool allow=false,full=false,bridge=false,chosen=false;int port=0;double timeout=300;std::string clientName="Local client";
  for(int i=1;i<argc;++i){const auto arg=utf8(argv[i]);auto next=[&](){if(i+1>=argc)throw cymcp::Error("InvalidArguments","Missing value for "+arg);return utf8(argv[++i]);};
   if(arg=="--manifest")manifest=fs::u8path(next());else if(arg=="--root")root=fs::u8path(next());else if(arg=="--data-dir")store=fs::u8path(next());else if(arg=="--run")run=fs::u8path(next());else if(arg=="--allow-write")allow=true;
   else if(arg=="--full")full=true;else if(arg=="--compact")full=false;else if(arg=="--bridge")bridge=true;else if(arg=="--connect")connect=next();
   else if(arg=="--token-env"){const auto key=next();if(const auto* value=std::getenv(key.c_str()))token=value;else throw cymcp::Error("AuthenticationRequired","Token environment variable is not set");}
   else if(arg=="--params")params=next();else if(arg=="--params-file"){auto path=next();if(path=="-")params=std::string((std::istreambuf_iterator<char>(std::cin)),{});else params=readJson(fs::u8path(path)).dump();}
   else if(arg=="--worker-config")workerConfig=next();else if(arg=="--port")port=std::stoi(next());
   else if(arg=="--timeout")timeout=std::stod(next());else if(arg=="--name")clientName=next();else if(arg=="--backend"||arg=="--model"){if(!next().empty())throw cymcp::Error("InvalidArguments","This editor declares no compute backend or model IDs for business operations");}else if(arg=="--permission"){next();}
   else if(arg=="-h"||arg=="--help"){command="help";chosen=true;}
   else if(arg.starts_with("--"))throw cymcp::Error("InvalidArguments","Unknown option "+arg);
   else if(!chosen){command=arg;chosen=true;}else if(operation.empty())operation=arg;else params=arg;
  }
  if(command=="help"){
   std::cout<<"CyGameSDK automation / CyMCPSDK 0.7.0\n"
    "CyGameCLI [--root PROJECT] [--allow-write] COMMAND\n"
    "  list | list-operations             List the real operation catalogue\n"
    "  describe [NAME] | describe-operation NAME\n"
    "  invoke NAME --params JSON | --params-file UTF8.json\n"
    "  jsonl | stdio                      SDK JSON requests on stdin/stdout\n"
    "  mcp [--full | --compact]            MCP stdio (compact discovery by default)\n"
    "  serve [--port PORT]                 Authenticated loopback /mcp and /v1\n"
    "  diagnose | list-backends | list-models | estimate NAME | can-run NAME\n"
    "  create-client --name NAME           Create a private endpoint token\n"
    "  --connect URL --token-env ENV       Address an existing authenticated service\n"
    "Mutations require the app's Allow editor changes setting or explicit --allow-write.\n"
    "Live document operations require an editor opened with the same --root.\n"
    "Discovery, schemas, job recovery and diagnostics do not open a GUI.\n";
   return 0;
  }
  Runtime app(root,install,allow,store,manifest);if(!workerConfig.empty())app.workerConfig(readJson(fs::u8path(workerConfig)));
  const auto output=[](const Json& v){std::cout<<v.dump()<<'\n'<<std::flush;};
  if(command=="create-client"){const auto path=root/"data/automation/client-token.json";auto value=Json{{"token",cymcp::random_token()},{"name",clientName}};writeJson(path,value);output({{"tokenFile",cymcp::path_utf8(path)},{"authentication","Use a private token as CYGAME_MCP_TOKEN for serve; never pass it as JSON arguments."}});return 0;}
  if(command=="list"||command=="list-operations"){output(app.operations->describe());return 0;}
  if(command=="describe"||command=="describe-operation"){output(operation.empty()?app.manifest():app.operations->describe(operation));return 0;}
  if(command=="list-backends"||command=="list-models"){output(Json::array());return 0;}
  if(command=="diagnose"){output(app.operations->invoke("automation_info",Json::object()));return 0;}
  if(command=="estimate"||command=="can-run"){app.operations->describe(operation);output({{"operation",operation},{"requiresEditor",true},{"canRun",fs::exists(root/"data/ipc/editor.json")},{"reason","Schema discovery works offline; live editor operations require the matching instance and host permission."}});return 0;}
  auto invoke=[&](const std::string& name,const Json& args){
   if(connect.empty()){cymcp::CallContext c;if(timeout<=0)throw cymcp::Error("InvalidArguments","Timeout must be positive");c.cancellation.deadline=cymcp::Clock::now()+std::chrono::milliseconds(int64_t(timeout*1000));return app.operations->invoke(name,args,c);}
   if(token.empty())if(const auto* v=std::getenv("CYGAME_MCP_TOKEN"))token=v;
   if(token.empty())throw cymcp::Error("AuthenticationRequired","Supply --token-env or CYGAME_MCP_TOKEN");
   cymcp::HttpRequest req;req.method="POST";req.url=connect;req.timeout=std::chrono::milliseconds(int64_t(timeout*1000));req.headers={{"Authorization","Bearer "+token},{"Content-Type","application/json"}};req.body=Json{{"method","invoke"},{"name",name},{"arguments",args}}.dump();auto response=cymcp::HttpClient().perform(req);auto body=cymcp::parse_json(response.body,64*1024*1024);if(response.status!=200||body.contains("error"))throw cymcp::Error(body.value("error",Json::object()).value("code","TransportError"),body.value("error",Json::object()).value("message","Remote invocation failed"));return body;
  };
  if(command=="invoke"){if(operation.empty())throw cymcp::Error("InvalidArguments","Operation name required");output(invoke(operation,cymcp::parse_json(params)));return 0;}
  if(command=="desktop"){
   if(run.empty())throw cymcp::Error("InvalidArguments","--run is required");
   auto r=readJson(run/"request.json");const auto ownerRoot=fs::weakly_canonical(fs::u8path(r.at("editorRoot").get<std::string>()));
   const auto relative=fs::weakly_canonical(run).lexically_relative(ownerRoot/"data/build-runs");
   if(relative.empty()||relative.is_absolute()||*relative.begin()=="..")throw cymcp::Error("InvalidPath","Desktop run must belong to this project");
   // The desktop is an explicit user action, with the application's own native validation.
   Json snapshot;{Runtime desktop(ownerRoot,install,true);desktop.desktop(run);
   try{desktop.operations->invoke(r.value("operation","build_execute"),r.at("parameters"));snapshot=readJson(run/"state.json");}catch(const std::exception& e){try{snapshot=readJson(run/"state.json");}catch(...){snapshot={{"state","Failed"},{"error",cymcp::redact_error_message(e.what())}};}}}
   if(snapshot.contains("job"))snapshot["state"]=snapshot["job"]["state"];
   snapshot["clientClosed"]=true;writeJson(run/"state.json",snapshot);return snapshot.value("state","Failed")=="Completed"?0:1;
  }
  if(command=="jsonl"||command=="stdio"){
   std::string line;while(std::getline(std::cin,line))try{
    auto request=cymcp::parse_json(line);cymcp::CallContext context;
    if(bridge&&request.contains("owner"))context.principal=request.at("owner");
    Json result;
    if(bridge&&request.value("bridge","")=="submit")result=app.submit(request.at("operation"),request.value("parameters",Json::object()),context);
    else if(bridge&&request.value("bridge","")=="get")result=app.job(request.at("jobId"),context);
    else if(bridge&&request.value("bridge","")=="list")result=app.jobs(context);
    else if(bridge&&request.value("bridge","")=="cancel")result=app.cancelJob(request.at("jobId"),context);
    else if(bridge&&request.value("bridge","")=="configure"){app.workerConfig(request.at("config"));result={{"configured",true}};}
    else result=cymcp::dispatch_app(*app.operations,request,context);
    output({{"result",result}});
   }catch(const cymcp::Error& e){output({{"error",{{"code",e.code},{"message",e.what()}}}});}
   catch(const std::exception& e){output({{"error",{{"code","InvalidRequest"},{"message",cymcp::redact_error_message(e.what())}}}});}
   return 0;
  }
  cymcp::McpServer mcp(*app.tools(!full),"CyGameSDK",app.serverOptions());
  std::atomic_bool finished=false;std::thread refresh([&](){while(!finished){app.poll();for(int i=0;i<10&&!finished;++i)std::this_thread::sleep_for(std::chrono::milliseconds(25));}});
  try{
   if(command=="mcp")mcp.serve_stdio(std::cin,std::cout);
   else if(command=="serve"){
    if(token.empty())if(const auto* v=std::getenv("CYGAME_MCP_TOKEN"))token=v;
    if(token.empty())throw cymcp::Error("AuthenticationRequired","Set CYGAME_MCP_TOKEN or --token-env to a private random token (32+ characters)");
    cymcp::LocalHttpServer server(token,[&](const Json& request,const CallContext& c){auto local=c;local.principal="local";return request.contains("jsonrpc")?mcp.handle(request,local):cymcp::dispatch_app(*app.operations,request,local);});
    port=server.start(port);std::signal(SIGINT,stop);std::signal(SIGTERM,stop);std::cerr<<"CyGameSDK listening on http://127.0.0.1:"<<port<<" (/mcp, /v1)\n";while(!stopped)std::this_thread::sleep_for(std::chrono::milliseconds(25));server.stop();
   }else throw cymcp::Error("InvalidArguments","Unknown command; use --help");
  }catch(...){finished=true;refresh.join();throw;}
  finished=true;refresh.join();return 0;
 }catch(const cymcp::Error& e){Json error={{"error",{{"code",e.code},{"message",cymcp::redact_error_message(e.what())}}}};std::cerr<<error.dump()<<'\n';return (e.code=="PermissionDenied"||e.code=="AccessDenied")?3:e.code=="InvalidArguments"||e.code=="InvalidParameters"?2:1;}
 catch(const std::exception& e){std::cerr<<Json{{"error",{{"code","InvalidRequest"},{"message",cymcp::redact_error_message(e.what())}}}}.dump()<<'\n';return 2;}
}
