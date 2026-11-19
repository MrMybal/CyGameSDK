#pragma once
#include "Editor.h"
namespace cygame {
// Explicit QA plan for this isolated host. No desktop/global input injection.
class AssistantSmokeTest {
 EditorCore& core_;EditorApp& app_;Json plan_,results_=Json::array();fs::path report_;size_t index_=0;bool active_=false,streamed_=false,cancelled_=false,complete_=false;
 std::chrono::steady_clock::time_point started_;
 size_t baselineMessages_=0;int result_=-1;bool submitted_=false,released_=false;
 void finish(bool pass,const std::string& error={}){
  complete_=true;result_=pass?0:3;Json data={{"passed",pass},{"steps",results_},{"error",error},{"source","own-host ChatController and actual shared operation registry"}};
  try{data["messageCount"]=Json::parse(app_.assistantState()).at("messages").size();data["entityName"]=core_.entity("hero").at("properties").at("name");}catch(...){}
  writeJson(report_,data);
 }
public:
 AssistantSmokeTest(EditorCore& c,EditorApp& a,const fs::path& path):core_(c),app_(a){
  if(path.empty())return;plan_=readJson(path);report_=fs::u8path(plan_.at("report").get<std::string>());app_.persistSettings=false;app_.open("assistant");
 }
 void input(ImGuiIO& io){
  if(!enabled()||complete_||!active_||plan_["steps"][index_].value("action","")!="compose")return;
  io.AddFocusEvent(true);
  if(!submitted_&&std::chrono::steady_clock::now()-started_>std::chrono::milliseconds(200)){io.AddKeyEvent(ImGuiKey_Enter,true);submitted_=true;}
  else if(submitted_&&!released_){io.AddKeyEvent(ImGuiKey_Enter,false);released_=true;}
 }
 bool enabled()const{return !report_.empty();}bool complete()const{return complete_;}
 int tick(){
  if(!enabled()||complete_)return result_;
  try{
   if(index_>=plan_["steps"].size()){finish(true);return result_;}
   const auto& step=plan_["steps"][index_];const std::string action=step.at("action");
   if(!active_){baselineMessages_=Json::parse(app_.assistantState())["messages"].size();started_=std::chrono::steady_clock::now();streamed_=cancelled_=submitted_=released_=false;active_=true;if(action!="wait")app_.assistantAction(step.dump());}
   const auto state=Json::parse(app_.assistantState());const auto elapsed=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started_).count();
   std::string last;for(const auto& m:state["messages"])if(m["role"]=="assistant")last=m.value("text","");
   const bool busy=state.value("busy",false);if(busy&&state["messages"].size()>baselineMessages_&&!last.empty())streamed_=true;
   if(step.contains("cancelAfterMs")&&!cancelled_&&elapsed>=step["cancelAfterMs"].get<double>()){app_.assistantAction(R"({"action":"cancel"})");cancelled_=true;}
   bool ready=!busy&&elapsed>50;
   if(action=="connect")ready&=state.value("connected",false);
   if(action=="wait"){ready=elapsed>=step.value("milliseconds",100);}
   if(action=="compose")ready&=submitted_&&state["messages"].size()>baselineMessages_;
   if((action=="send"||action=="compose")&&step.contains("contains"))ready&=last.find(step["contains"].get<std::string>())!=std::string::npos;
   if(ready){
    if(step.value("streaming",false)&&!streamed_)throw std::runtime_error("No partial response was observed while the turn was active");
    if(step.contains("entityName")&&core_.entity("hero").at("properties").at("name")!=step["entityName"])throw std::runtime_error("Chat tool did not change the real entity");
    if(step.value("cancelled",false)&&!cancelled_)throw std::runtime_error("Cancellation was not requested");
    if(step.value("attachment",false)){bool found=false;for(const auto& m:state["messages"])found|=m.value("attachments",0)>0;if(!found)throw std::runtime_error("Attachment was not submitted by the common composer");}
    if(step.contains("minimumMessages")&&state["messages"].size()<step["minimumMessages"].get<size_t>())throw std::runtime_error("Conversation history was not retained");
    results_.push_back({{"action",action},{"passed",true},{"streamed",streamed_},{"cancelled",cancelled_},{"elapsedMs",elapsed}});++index_;active_=false;
   }else if(elapsed>step.value("timeoutMs",15000))throw std::runtime_error("Assistant step timed out: "+action+" / "+state.value("phase","")+" / "+state.value("errorCode",""));
  }catch(const std::exception& e){finish(false,e.what());}
  return result_;
 }
};
}
