#pragma once
#include <cyui/modules/Code.h>
#include <memory>
namespace cygame {
// Keep the optional C++20 backend behind a C++17 host boundary.
class SourceEditor {
public:
    SourceEditor();~SourceEditor();
    void setText(const std::string&);std::string text() const;
    void setLanguage(cyui::CodeLanguage);void setReadOnly(bool);
    bool draw(const char*,ImVec2);
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
