#pragma once
#include <functional>
#include <memory>
#include <string>
namespace cygame {
// C++17 boundary: no SDK/JSON types or second ImGui context cross this bridge.
class AutomationBridge {
    struct Impl;std::unique_ptr<Impl> impl_;
public:
    AutomationBridge(const std::string& root,const std::string& install,std::function<void()> picker);
    ~AutomationBridge();
    void draw();
    void attach(const std::string&);
    std::string action(const std::string&); // Own-host QA / trusted host controller.
    std::string state()const;
};
}
