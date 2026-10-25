#pragma once
#include <cyui/CyUI.h>
#include <algorithm>
#include <string>

namespace cygame {
// Application-owned theme: CyUI remains the only colour/semantic widget backend.
// Inspired by Unreal Editor's neutral charcoal surfaces and restrained blue focus.
inline bool editorThemeKnown(const std::string& key) {
    if(key=="unreal-dark")return true;
    for(const auto& theme:cyui::builtInThemes())if(theme.key==key)return true;
    return false;
}
inline const char* editorThemeName(const std::string& key) {
    if(key=="unreal-dark")return "Unreal-inspired Dark";
    for(const auto& theme:cyui::builtInThemes())if(theme.key==key)return theme.name.c_str();
    return "Unreal-inspired Dark";
}
inline void applyEditorTheme(const std::string& key,float dpi,float textScale) {
    cyui::Config config;
    config.themeKey=key=="unreal-dark"?"navy":key.c_str();
    config.density=cyui::Density::compact;config.dpiScale=dpi;config.squareWindows=true;
    cyui::apply(config);
    if(key=="unreal-dark") {
        using cyui::rgb;
        const cyui::Palette charcoal {
            rgb(0x181818),rgb(0x262626),rgb(0x303030),rgb(0x1c1c1c),rgb(0x3c3c3c),rgb(0x505050),
            rgb(0x363636),rgb(0x111111),rgb(0x242424),rgb(0x363636),
            rgb(0xe0e0e0),rgb(0xb5b5b5),rgb(0x929292),
            rgb(0x0070e0),rgb(0x1686ec),rgb(0x244e73),rgb(0xffffff),rgb(0x0070e0,0.35f),
            rgb(0x73c991),rgb(0xe8b567),rgb(0xe57878),rgb(0x7aafe0)
        };
        cyui::setPalette(charcoal);
        auto& style=ImGui::GetStyle();
        style.FrameRounding=cyui::dp(2);style.TabRounding=cyui::dp(2);
        style.ChildRounding=cyui::dp(2);style.PopupRounding=cyui::dp(2);
        style.GrabRounding=cyui::dp(1);style.ScrollbarRounding=cyui::dp(2);
        style.DockingSeparatorSize=cyui::dp(4);
    }
    ImGui::GetStyle().FontScaleMain=textScale;
}
}
