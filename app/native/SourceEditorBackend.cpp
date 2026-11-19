#include "SourceEditorBackend.h"
#include <cyui/modules/CodeEditor.h>
namespace cygame {
struct SourceEditor::Impl {cyui::CodeEditor editor;};
SourceEditor::SourceEditor():impl_(std::make_unique<Impl>()){}
SourceEditor::~SourceEditor()=default;
void SourceEditor::setText(const std::string& text){impl_->editor.setText(text);}
std::string SourceEditor::text()const{return impl_->editor.text();}
void SourceEditor::setLanguage(cyui::CodeLanguage language){if(impl_->editor.language()!=language)impl_->editor.setLanguage(language);}
void SourceEditor::setReadOnly(bool value){impl_->editor.setReadOnly(value);}
bool SourceEditor::draw(const char* id,ImVec2 size){return impl_->editor.draw(id,size);}
}
