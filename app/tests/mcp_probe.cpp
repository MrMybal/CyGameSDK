#include <cymcp/mcp.hpp>
#include <iostream>
#include <filesystem>
namespace fs=std::filesystem;using cymcp::Json;
int main(int argc,char** argv){
 try{
  if(argc!=3)throw std::runtime_error("Pass CLI executable and isolated editor root");
  cymcp::ProcessOptions options;options.executable=fs::u8path(argv[1]);options.arguments={"--root",argv[2],"--allow-write","mcp","--full"};
  cymcp::McpClient client(options);client.connect();Json initial;
  for(int i=0;i<40;++i)try{initial=client.read_resource("cygame://scene");break;}catch(const cymcp::Error&){std::this_thread::sleep_for(std::chrono::milliseconds(50));}
  if(initial.is_null())throw std::runtime_error("Live scene resource missing");
  client.read_resource("cygame://scene");if(!client.cache_stats().hits)throw std::runtime_error("Compatible resource read was not cached");
  client.start_notifications({{"resourcesListChanged",true},{"resourceSubscriptions",{"cygame://scene"}}});
  auto result=client.call_tool("property_set",{{"entityId","hero"},{"propertyId","name"},{"value","Notification invalidation"}});
  if(result.value("isError",false))throw std::runtime_error("Actual scene mutation failed");
  bool updated=false;Json notifications=Json::array();
  for(int i=0;i<100;++i){
   auto batch=client.drain_notifications();for(const auto& event:batch)notifications.push_back(event);
   auto page=client.read_resource("cygame://scene");
   if(page.dump().find("Notification invalidation")!=std::string::npos){updated=true;break;}
   std::this_thread::sleep_for(std::chrono::milliseconds(25));
  }
  if(!updated)throw std::runtime_error("Resource cache was not invalidated before TTL");
  if(notifications.empty())throw std::runtime_error("Change notifications were not delivered");
  client.call_tool("edit_undo",Json::object());client.stop_notifications();client.clear_cache();
  auto first=client.call_tool("scene_create",{{"name","Uncached mutation"}}),second=client.call_tool("scene_create",{{"name","Uncached mutation"}});
  if(first.value("isError",false)||second.value("isError",false)||first==second)throw std::runtime_error("Mutations were cached or failed");
  client.call_tool("edit_undo",Json::object());client.call_tool("edit_undo",Json::object());
  auto stats=client.cache_stats();std::cout<<Json{{"passed",true},{"positiveTtlReadCache",true},{"nativeNotifications",true},{"invalidatedBeforeTtl",true},{"mutationsNeverCached",true},{"explicitClearCache",true},{"cacheHits",stats.hits}}.dump()<<'\n';return 0;
 }catch(const std::exception& e){std::cerr<<Json{{"error",cymcp::redact_error_message(e.what())}}.dump()<<'\n';return 1;}
}
