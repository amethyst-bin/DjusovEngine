#include "ui/GameHUD.hpp"
#include "core/Theme.hpp"
#include <imgui.h>
#include <algorithm>

namespace Djusov {

bool GameHUD::s_chatOpen = false;
char GameHUD::s_chatInputBuffer[128] = "";

void GameHUD::init() {
    s_chatOpen = false;
    s_chatInputBuffer[0] = '\0';
}

void GameHUD::render(const Player& player, const Weapon& weapon, const NetClient& netClient,
                     int screenWidth, int screenHeight, float adsAmount) {

    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    float w = static_cast<float>(screenWidth);
    float h = static_cast<float>(screenHeight);
    glm::vec2 center(w * 0.5f, h * 0.5f);

    // 1. Damage Screen Vignette (flashes red when hit)
    float damageVignette = player.getDamageVignette();
    if (damageVignette > 0.01f) {
        ImU32 redVignette = ImColor(0.85f, 0.05f, 0.05f, damageVignette * 0.45f);
        drawList->AddRectFilled(ImVec2(0, 0), ImVec2(w, h), redVignette);
    }

    // 2. Dynamic Crosshair (only visible when not in ADS)
    float crosshairAlpha = (1.0f - adsAmount);
    if (crosshairAlpha > 0.05f) {
        float gap = player.isSprinting() ? 14.0f : 8.0f;
        float len = 8.0f;
        float thickness = 1.8f;
        ImU32 crossColor = ImColor(1.0f, 1.0f, 1.0f, 0.85f * crosshairAlpha);

        // Top
        drawList->AddLine(ImVec2(center.x, center.y - gap), ImVec2(center.x, center.y - gap - len), crossColor, thickness);
        // Bottom
        drawList->AddLine(ImVec2(center.x, center.y + gap), ImVec2(center.x, center.y + gap + len), crossColor, thickness);
        // Left
        drawList->AddLine(ImVec2(center.x - gap, center.y), ImVec2(center.x - gap - len, center.y), crossColor, thickness);
        // Right
        drawList->AddLine(ImVec2(center.x + gap, center.y), ImVec2(center.x + gap + len, center.y), crossColor, thickness);

        // Center dot
        drawList->AddCircleFilled(ImVec2(center.x, center.y), 1.5f, crossColor);
    }

    // 3. Hitmarker (diagonal cross + damage number)
    const auto& hm = weapon.getHitmarker();
    if (hm.timeRemaining > 0.01f) {
        float alpha = hm.timeRemaining / 0.35f;
        ImU32 hitColor = hm.isHeadshot ? ImColor(1.0f, 0.2f, 0.2f, alpha) : ImColor(1.0f, 1.0f, 1.0f, alpha);
        float size = hm.isHeadshot ? 12.0f : 8.0f;

        drawList->AddLine(ImVec2(center.x - size, center.y - size), ImVec2(center.x - 3, center.y - 3), hitColor, 2.0f);
        drawList->AddLine(ImVec2(center.x + size, center.y - size), ImVec2(center.x + 3, center.y - 3), hitColor, 2.0f);
        drawList->AddLine(ImVec2(center.x - size, center.y + size), ImVec2(center.x - 3, center.y + 3), hitColor, 2.0f);
        drawList->AddLine(ImVec2(center.x + size, center.y + size), ImVec2(center.x + 3, center.y + 3), hitColor, 2.0f);
    }

    // 4. Bottom Left: Health & Stamina Bars
    float barWidth = 220.0f;
    float barHeight = 12.0f;
    float posX = 30.0f;
    float posY = h - 60.0f;

    // Background panel
    drawList->AddRectFilled(ImVec2(posX - 8, posY - 28), ImVec2(posX + barWidth + 8, posY + barHeight * 2 + 16),
                            ImColor(0.04f, 0.04f, 0.05f, 0.75f), 6.0f);

    // HP Bar
    float hpFraction = std::clamp(player.getHealth() / player.getMaxHealth(), 0.0f, 1.0f);
    drawList->AddRectFilled(ImVec2(posX, posY), ImVec2(posX + barWidth, posY + barHeight), ImColor(0.12f, 0.12f, 0.15f, 1.0f), 3.0f);
    ImU32 hpColor = (hpFraction > 0.3f) ? ImColor(0.2f, 0.85f, 0.35f, 1.0f) : ImColor(0.9f, 0.25f, 0.2f, 1.0f);
    drawList->AddRectFilled(ImVec2(posX, posY), ImVec2(posX + barWidth * hpFraction, posY + barHeight), hpColor, 3.0f);

    // HP Text
    char hpBuf[32];
    std::snprintf(hpBuf, sizeof(hpBuf), "HP %d / %d", (int)player.getHealth(), (int)player.getMaxHealth());
    drawList->AddText(ImVec2(posX, posY - 20), ImColor(0.95f, 0.95f, 0.95f, 1.0f), hpBuf);

    // Stamina Bar
    float staFraction = std::clamp(player.getStamina() / player.getMaxStamina(), 0.0f, 1.0f);
    float staY = posY + barHeight + 6.0f;
    drawList->AddRectFilled(ImVec2(posX, staY), ImVec2(posX + barWidth, staY + 6.0f), ImColor(0.12f, 0.12f, 0.15f, 1.0f), 2.0f);
    drawList->AddRectFilled(ImVec2(posX, staY), ImVec2(posX + barWidth * staFraction, staY + 6.0f), ImColor(0.25f, 0.65f, 0.95f, 1.0f), 2.0f);

    // 5. Bottom Right: Tactical Weapon Ammo
    float ammoX = w - 180.0f;
    float ammoY = h - 65.0f;
    drawList->AddRectFilled(ImVec2(ammoX - 10, ammoY - 10), ImVec2(w - 20.0f, h - 20.0f), ImColor(0.04f, 0.04f, 0.05f, 0.75f), 6.0f);

    char ammoBuf[32];
    if (weapon.isReloading()) {
        std::snprintf(ammoBuf, sizeof(ammoBuf), "RELOADING...");
        drawList->AddText(ImVec2(ammoX, ammoY + 8), ImColor(0.95f, 0.75f, 0.2f, 1.0f), ammoBuf);
    } else {
        std::snprintf(ammoBuf, sizeof(ammoBuf), "%d / %d", weapon.getAmmo(), weapon.getReserveAmmo());
        drawList->AddText(ImVec2(ammoX, ammoY - 2), ImColor(0.95f, 0.95f, 0.95f, 1.0f), "RIFLE 5.56");
        drawList->AddText(ImVec2(ammoX, ammoY + 16), ImColor(1.0f, 1.0f, 1.0f, 1.0f), ammoBuf);
    }

    // 6. Top Left: Multiplayer Chat Log
    const auto& chat = netClient.getChatLog();
    if (!chat.empty() || s_chatOpen) {
        float chatY = 30.0f;
        for (const auto& msg : chat) {
            char lineBuf[180];
            std::snprintf(lineBuf, sizeof(lineBuf), "[%s]: %s", msg.sender, msg.text);
            drawList->AddText(ImVec2(30.0f, chatY), ImColor(0.9f, 0.92f, 0.95f, 0.9f), lineBuf);
            chatY += 18.0f;
        }

        if (s_chatOpen) {
            ImGui::SetNextWindowPos(ImVec2(30.0f, chatY + 5.0f));
            ImGui::SetNextWindowSize(ImVec2(350.0f, 32.0f));
            ImGui::Begin("##ChatWindow", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground);
            ImGui::SetKeyboardFocusHere();
            if (ImGui::InputText("##ChatInput", s_chatInputBuffer, sizeof(s_chatInputBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
                if (std::strlen(s_chatInputBuffer) > 0) {
                    const_cast<NetClient&>(netClient).sendChatMessage(s_chatInputBuffer);
                    s_chatInputBuffer[0] = '\0';
                }
                s_chatOpen = false;
            }
            ImGui::End();
        }
    }
}

void GameHUD::renderPauseMenu(float& inOutSensitivity, float& inOutVolume, bool& inOutResume, bool& requestExit) {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f - 180.0f, io.DisplaySize.y * 0.5f - 140.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(360.0f, 280.0f));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove;
    if (ImGui::Begin("Game Paused", nullptr, flags)) {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.2f, 1.0f), "  SETTINGS & CONTROLS");
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::SliderFloat("Mouse Sensitivity", &inOutSensitivity, 0.2f, 3.5f, "%.2fx");
        ImGui::SliderFloat("Master Volume", &inOutVolume, 0.0f, 1.0f, "%.0f%%");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Resume (Esc)", ImVec2(-1, 36))) {
            inOutResume = true;
        }

        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button("Quit / Return", ImVec2(-1, 32))) {
            requestExit = true;
        }
        ImGui::PopStyleColor();
    }
    ImGui::End();
}

} // namespace Djusov
