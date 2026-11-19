#include "Editor.h"
#include "AutomationBridge.h"
#include <windows.h>
namespace cygame {
void EditorApp::prepareAssistant(){
 if(assistant_)return;
 wchar_t exe[32768]{};GetModuleFileNameW(nullptr,exe,32768);
 assistant_=std::make_unique<AutomationBridge>(core.root().u8string(),fs::path(exe).parent_path().u8string(),[this]{
  assistantAttachmentPicker_=true;importPickerOpen_=true;importPickerRoot_=core.root().root_path().generic_u8string();importPicker_.directory=importPicker_.pathText=core.root().generic_u8string();importPicker_.selection.clear();importListing_={};
 });
}
void EditorApp::drawAssistant(){try{prepareAssistant();assistant_->draw();}catch(const std::exception& e){cyui::callout(cyui::Tone::error,e.what());}}
std::string EditorApp::assistantAction(const std::string& input){prepareAssistant();return assistant_->action(input);}
std::string EditorApp::assistantState(){prepareAssistant();return assistant_->state();}
void EditorApp::shutdownAssistant(){assistant_.reset();}
}
