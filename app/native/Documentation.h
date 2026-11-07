#pragma once
#include <filesystem>
#include <string>
namespace cygame {
struct HelpTopic { std::string page,section; };
HelpTopic documentationTopic(const std::string& panel);
std::string documentationUrl(const std::filesystem::path& root,const std::string& locale,const HelpTopic& topic);
}
