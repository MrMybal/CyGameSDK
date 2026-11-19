#pragma once
#include "Core.h"
#include <cyui/modules/Inspector.h>
namespace cygame {
using PropertyCaption = std::function<std::string(const std::string&, const std::string&)>;
cyui::PropertyValue uiPropertyValue(const Json&, const Json&, const std::string&, const PropertyCaption&);
Json jsonPropertyValue(const Json&, const cyui::PropertyValue&, const Json&);
void uiPropertyMetadata(cyui::PropertyMetadata&, const Json&);
}
