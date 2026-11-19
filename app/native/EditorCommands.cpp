#include "Editor.h"
namespace cygame {
void EditorApp::prepareCommands(){
    struct Binding{const char* id;const char* label;const char* chord;bool typing;};
    const Binding bindings[]={{"project_save","Save project","Ctrl+S",false},{"project_load","Load project","Ctrl+O",false},
        {"edit_undo","Undo","Ctrl+Z",false},{"edit_redo","Redo","Ctrl+Y",false},
        {"search","Search everything","Ctrl+P",true},{"drawer","Content Drawer","Ctrl+Space",true},{"help","Context Help","F1",true}};
    if(!commandsInitialized_){
        for(const auto& binding:bindings){const auto id=uiId("editor/action/"+std::string(binding.id));cyui::Command command{id,binding.label,binding.chord,"Editor",true};command.allowWhileTyping=binding.typing;
            if(!shortcuts_.add(command))throw std::runtime_error("Editor command identity or binding collision");shortcutActions_[id]=binding.id;}
        if(preferences_.contains("keyBindings")&&preferences_["keyBindings"].is_string()){std::string error;if(!shortcuts_.loadBindingsJson(preferences_["keyBindings"],error))cyui::notify(error.c_str(),cyui::Tone::warning);}
        commandsInitialized_=true;
    }
    shortcuts_.find(uiId("editor/action/edit_undo"))->enabled=core.canUndo();
    shortcuts_.find(uiId("editor/action/edit_redo"))->enabled=core.canRedo();
}
void EditorApp::dispatchEditorCommand(cyui::ItemId id){
    if(!id||!shortcutActions_.count(id))return;const auto& action=shortcutActions_.at(id);
    if(action=="search")searchOpen_=true;
    else if(action=="drawer")invoke("content_drawer_toggle");
    else if(action=="help")openDocumentation();
    else invoke(action);
}
void EditorApp::pollEditorCommands(){
    // One host-wide pass: this shell has several independent docked root windows.
    // Registry match owns conflicts, contexts, enablement and typing policy.
    const auto& io=ImGui::GetIO();if(io.AppFocusLost)return;
    for(const auto& command:shortcuts_.commands()){
        const auto chord=cyui::parseShortcut(command.shortcut);const auto key=ImGuiKey(chord&~ImGuiMod_Mask_);
        if(chord&&int(io.KeyMods)==int(chord&ImGuiMod_Mask_)&&ImGui::IsKeyPressed(key,false)){
            const auto id=shortcuts_.match(chord,{},io.WantTextInput);if(id){dispatchEditorCommand(id);break;}
        }
    }
}
}
