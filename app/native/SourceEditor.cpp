#include "Editor.h"
#include <cstdio>
namespace cygame {
bool EditorApp::editSource(const std::string& key,char* buffer,size_t capacity,ImVec2 size,cyui::CodeLanguage language){
    auto& view=sourceEditors_[key];if(!view.editor)view.editor=std::make_unique<SourceEditor>();
    const std::string incoming=buffer;
    if(view.text!=incoming){view.editor->setText(incoming);view.text=incoming;}
    view.editor->setLanguage(language);view.editor->setReadOnly(false);
    if(!view.editor->draw(key.c_str(),size))return false;
    const auto edited=view.editor->text();
    if(edited.size()>=capacity){view.editor->setText(view.text);cyui::notify(tr("Text exceeds the field limit."),cyui::Tone::error);return false;}
    std::snprintf(buffer,capacity,"%s",edited.c_str());view.text=edited;return true;
}
void EditorApp::sourceView(const std::string& key,const std::string& text,cyui::CodeLanguage language,ImVec2 size,bool selectTextOnly){
    ImGui::PushID(key.c_str());auto& view=sourceViews_[key];
    if(cyui::ghostButton(tr("Copy text")))ImGui::SetClipboardText(text.c_str());
    if(selectTextOnly)view.selectText=true;else{ImGui::SameLine();ImGui::Checkbox(tr("Text selection"),&view.selectText);}
    if(size.y>0)size.y=std::max(cyui::dp(32),size.y-ImGui::GetFrameHeightWithSpacing());
    if(view.selectText){
        auto& editor=sourceEditors_["read/"+key];if(!editor.editor)editor.editor=std::make_unique<SourceEditor>();
        if(editor.text!=text){editor.editor->setText(text);editor.text=text;}
        editor.editor->setReadOnly(true);editor.editor->setLanguage(language);editor.editor->draw("##sourceSelection",size);
    }else{view.document.setText(text,language);cyui::codeViewer("##source",view.state,view.document,size);}
    ImGui::PopID();
}
}
