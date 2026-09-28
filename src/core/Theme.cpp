#include "core/Theme.hpp"
#include <imgui.h>
#include <cstdio>
#include <memory>
#include <array>

namespace Djusov {

ThemePreset Theme::s_preset = ThemePreset::Amoled;
bool Theme::s_followSystemAccent = true;
glm::vec4 Theme::s_accentColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f); // Default white
glm::vec4 Theme::s_customAccentColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
float Theme::s_fontSize = 15.0f;

static std::string execCommand(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) return "";
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    // Trim newlines and quotes
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r' || result.back() == '\'')) {
        result.pop_back();
    }
    if (!result.empty() && result.front() == '\'') {
        result.erase(0, 1);
    }
    return result;
}

std::string Theme::detectSystemAccentColor() {
    static std::string cachedColor = "";
    static bool checked = false;
    if (checked) return cachedColor;
    checked = true;
#ifdef __linux__
    cachedColor = execCommand("gsettings get org.gnome.desktop.interface accent-color 2>/dev/null");
    if (!cachedColor.empty()) return cachedColor;
    cachedColor = execCommand("gsettings get org.freedesktop.appearance accent-color 2>/dev/null");
    if (!cachedColor.empty()) return cachedColor;
#endif
    return "";
}

static glm::vec4 parseAccentColorName(const std::string& name) {
    if (name == "yellow") return glm::vec4(0.96f, 0.77f, 0.19f, 1.0f);
    if (name == "blue")   return glm::vec4(0.23f, 0.51f, 0.96f, 1.0f);
    if (name == "teal")   return glm::vec4(0.18f, 0.80f, 0.77f, 1.0f);
    if (name == "green")  return glm::vec4(0.34f, 0.78f, 0.44f, 1.0f);
    if (name == "orange") return glm::vec4(0.98f, 0.55f, 0.23f, 1.0f);
    if (name == "red")    return glm::vec4(0.94f, 0.33f, 0.31f, 1.0f);
    if (name == "pink")   return glm::vec4(0.91f, 0.38f, 0.67f, 1.0f);
    if (name == "purple") return glm::vec4(0.69f, 0.45f, 0.95f, 1.0f);
    if (name == "slate")  return glm::vec4(0.55f, 0.61f, 0.71f, 1.0f);
    return glm::vec4(1.0f, 1.0f, 1.0f, 1.0f); // Default clean white
}

void Theme::init() {
    if (s_followSystemAccent) {
        std::string detected = detectSystemAccentColor();
        if (!detected.empty()) {
            s_accentColor = parseAccentColorName(detected);
        } else {
            s_accentColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        }
    } else {
        s_accentColor = s_customAccentColor;
    }
}

void Theme::setPreset(ThemePreset preset) {
    s_preset = preset;
    apply();
}

void Theme::setFollowSystemAccent(bool follow) {
    s_followSystemAccent = follow;
    init();
    apply();
}

void Theme::setAccentColor(const glm::vec4& color) {
    s_customAccentColor = color;
    if (!s_followSystemAccent) {
        s_accentColor = color;
        apply();
    }
}

void Theme::setFontSize(float size) {
    s_fontSize = size;
}

void Theme::apply() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    ImVec4 accent(s_accentColor.r, s_accentColor.g, s_accentColor.b, 1.0f);
    ImVec4 accentDim(s_accentColor.r * 0.7f, s_accentColor.g * 0.7f, s_accentColor.b * 0.7f, 0.85f);
    ImVec4 accentActive(s_accentColor.r * 1.15f, s_accentColor.g * 1.15f, s_accentColor.b * 1.15f, 1.0f);

    style.WindowRounding = 4.0f;
    style.ChildRounding = 3.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 3.0f;
    style.TabRounding = 4.0f;

    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.TabBorderSize = 0.0f;

    style.WindowPadding = ImVec2(10.0f, 10.0f);
    style.FramePadding = ImVec2(8.0f, 5.0f);
    style.ItemSpacing = ImVec2(8.0f, 6.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
    style.IndentSpacing = 16.0f;
    style.ScrollbarSize = 12.0f;
    style.GrabMinSize = 10.0f;

    if (s_preset == ThemePreset::Amoled) {
        // True deep Amoled black palette
        colors[ImGuiCol_Text]                  = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
        colors[ImGuiCol_TextDisabled]          = ImVec4(0.45f, 0.45f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg]              = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_ChildBg]               = ImVec4(0.03f, 0.03f, 0.04f, 1.00f);
        colors[ImGuiCol_PopupBg]               = ImVec4(0.04f, 0.04f, 0.05f, 0.98f);
        colors[ImGuiCol_Border]                = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
        colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg]               = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
        colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
        colors[ImGuiCol_FrameBgActive]         = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
        colors[ImGuiCol_TitleBg]               = ImVec4(0.02f, 0.02f, 0.03f, 1.00f);
        colors[ImGuiCol_TitleBgActive]         = ImVec4(0.05f, 0.05f, 0.07f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.00f, 0.00f, 0.00f, 0.60f);
        colors[ImGuiCol_MenuBarBg]             = ImVec4(0.03f, 0.03f, 0.04f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.02f, 0.02f, 0.03f, 0.60f);
        colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.20f, 0.20f, 0.24f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.28f, 0.28f, 0.32f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive]   = accentDim;
        colors[ImGuiCol_CheckMark]             = accent;
        colors[ImGuiCol_SliderGrab]            = accent;
        colors[ImGuiCol_SliderGrabActive]      = accentActive;
        colors[ImGuiCol_Button]                = ImVec4(0.10f, 0.10f, 0.13f, 1.00f);
        colors[ImGuiCol_ButtonHovered]         = ImVec4(0.17f, 0.17f, 0.22f, 1.00f);
        colors[ImGuiCol_ButtonActive]          = accentDim;
        colors[ImGuiCol_Header]                = ImVec4(0.10f, 0.10f, 0.13f, 1.00f);
        colors[ImGuiCol_HeaderHovered]         = ImVec4(0.16f, 0.16f, 0.20f, 1.00f);
        colors[ImGuiCol_HeaderActive]          = accentDim;
        colors[ImGuiCol_Separator]             = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
        colors[ImGuiCol_SeparatorHovered]      = accent;
        colors[ImGuiCol_SeparatorActive]       = accentActive;
        colors[ImGuiCol_ResizeGrip]            = ImVec4(0.15f, 0.15f, 0.18f, 0.50f);
        colors[ImGuiCol_ResizeGripHovered]     = accentDim;
        colors[ImGuiCol_ResizeGripActive]      = accent;
        colors[ImGuiCol_Tab]                   = ImVec4(0.04f, 0.04f, 0.06f, 1.00f);
        colors[ImGuiCol_TabHovered]            = ImVec4(0.14f, 0.14f, 0.18f, 1.00f);
        colors[ImGuiCol_TabActive]             = ImVec4(0.10f, 0.10f, 0.14f, 1.00f);
        colors[ImGuiCol_TabUnfocused]          = ImVec4(0.02f, 0.02f, 0.03f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.06f, 0.06f, 0.08f, 1.00f);
        colors[ImGuiCol_DockingPreview]        = ImVec4(accent.x, accent.y, accent.z, 0.35f);
        colors[ImGuiCol_DockingEmptyBg]        = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotLines]             = accent;
        colors[ImGuiCol_PlotLinesHovered]      = accentActive;
        colors[ImGuiCol_PlotHistogram]         = accent;
        colors[ImGuiCol_PlotHistogramHovered]  = accentActive;
        colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
        colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
        colors[ImGuiCol_TableBorderLight]      = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
        colors[ImGuiCol_TableRowBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt]         = ImVec4(0.04f, 0.04f, 0.06f, 0.50f);
        colors[ImGuiCol_TextSelectedBg]        = ImVec4(accent.x, accent.y, accent.z, 0.30f);
        colors[ImGuiCol_NavHighlight]          = accent;
    } else if (s_preset == ThemePreset::DarkGraphite) {
        // Godot 4 style dark charcoal
        colors[ImGuiCol_Text]                  = ImVec4(0.92f, 0.92f, 0.94f, 1.00f);
        colors[ImGuiCol_TextDisabled]          = ImVec4(0.50f, 0.50f, 0.55f, 1.00f);
        colors[ImGuiCol_WindowBg]              = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
        colors[ImGuiCol_ChildBg]               = ImVec4(0.14f, 0.14f, 0.17f, 1.00f);
        colors[ImGuiCol_PopupBg]               = ImVec4(0.13f, 0.13f, 0.16f, 0.98f);
        colors[ImGuiCol_Border]                = ImVec4(0.22f, 0.22f, 0.26f, 1.00f);
        colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg]               = ImVec4(0.17f, 0.17f, 0.20f, 1.00f);
        colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.22f, 0.22f, 0.27f, 1.00f);
        colors[ImGuiCol_FrameBgActive]         = ImVec4(0.25f, 0.25f, 0.30f, 1.00f);
        colors[ImGuiCol_TitleBg]               = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
        colors[ImGuiCol_TitleBgActive]         = ImVec4(0.14f, 0.14f, 0.17f, 1.00f);
        colors[ImGuiCol_Button]                = ImVec4(0.19f, 0.19f, 0.24f, 1.00f);
        colors[ImGuiCol_ButtonHovered]         = ImVec4(0.25f, 0.25f, 0.32f, 1.00f);
        colors[ImGuiCol_ButtonActive]          = accentDim;
        colors[ImGuiCol_Header]                = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
        colors[ImGuiCol_HeaderHovered]         = ImVec4(0.24f, 0.24f, 0.30f, 1.00f);
        colors[ImGuiCol_HeaderActive]          = accentDim;
        colors[ImGuiCol_CheckMark]             = accent;
        colors[ImGuiCol_SliderGrab]            = accent;
        colors[ImGuiCol_SliderGrabActive]      = accentActive;
        colors[ImGuiCol_Tab]                   = ImVec4(0.14f, 0.14f, 0.17f, 1.00f);
        colors[ImGuiCol_TabHovered]            = ImVec4(0.22f, 0.22f, 0.28f, 1.00f);
        colors[ImGuiCol_TabActive]             = ImVec4(0.19f, 0.19f, 0.24f, 1.00f);
    }
}

} // namespace Djusov
