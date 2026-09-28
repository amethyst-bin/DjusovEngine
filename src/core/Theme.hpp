#pragma once

#include <glm/glm.hpp>
#include <string>

struct ImGuiStyle;

namespace Djusov {

enum class ThemePreset {
    Amoled,
    DarkGraphite,
    Light
};

class Theme {
public:
    static void init();
    static void apply();

    static ThemePreset getPreset() { return s_preset; }
    static void setPreset(ThemePreset preset);

    static bool getFollowSystemAccent() { return s_followSystemAccent; }
    static void setFollowSystemAccent(bool follow);

    static glm::vec4 getAccentColor() { return s_accentColor; }
    static void setAccentColor(const glm::vec4& color);

    static float getFontSize() { return s_fontSize; }
    static void setFontSize(float size);

    static std::string detectSystemAccentColor();

private:
    static ThemePreset s_preset;
    static bool s_followSystemAccent;
    static glm::vec4 s_accentColor;
    static glm::vec4 s_customAccentColor;
    static float s_fontSize;
};

} // namespace Djusov
