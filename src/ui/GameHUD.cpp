#include "ui/GameHUD.hpp"
#include "audio/AudioEngine.hpp"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <algorithm>
#include <cmath>

namespace Djusov {

bool GameHUD::s_chatOpen = false;
char GameHUD::s_chatInputBuffer[128] = "";
float GameHUD::s_weaponSelectTimer = 0.0f;
int GameHUD::s_selectedSlot = 1;
int GameHUD::s_hoverSlot = 1;
double GameHUD::s_scrollDelta = 0.0;
float GameHUD::s_ecgTime = 0.0f;

void GameHUD::init() {
    s_chatOpen = false;
    s_chatInputBuffer[0] = '\0';
    s_weaponSelectTimer = 0.0f;
    s_selectedSlot = 1;
    s_hoverSlot = 1;
    s_scrollDelta = 0.0;
    s_ecgTime = 0.0f;
}

void GameHUD::onMouseScroll(double yoffset) {
    s_scrollDelta += yoffset;
}

void GameHUD::render(GLFWwindow* window, float dt, const Player& player, Weapon& weapon, const NetClient& netClient,
                     int screenWidth, int screenHeight, float adsAmount) {

    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    float w = static_cast<float>(screenWidth);
    float h = static_cast<float>(screenHeight);
    glm::vec2 center(w * 0.5f, h * 0.5f);

    s_ecgTime += dt;

    // 1. Damage Screen Vignette (flashes red when hit)
    float damageVignette = player.getDamageVignette();
    float hpFraction = std::clamp(player.getHealth() / player.getMaxHealth(), 0.0f, 1.0f);

    // Cry of Fear low health pulse
    if (hpFraction < 0.35f && !player.isDead()) {
        float pulse = (std::sin(s_ecgTime * 5.0f) * 0.5f + 0.5f) * (1.0f - hpFraction / 0.35f);
        damageVignette = std::max(damageVignette, pulse * 0.55f);
    }

    if (damageVignette > 0.01f) {
        ImU32 redVignette = ImColor(0.75f, 0.04f, 0.04f, damageVignette * 0.55f);
        drawList->AddRectFilled(ImVec2(0, 0), ImVec2(w, h), redVignette);
    }

    // 2. Dynamic Crosshair (only if equipped and not ADS)
    if (weapon.hasWeapon()) {
        float crosshairAlpha = (1.0f - adsAmount);
        if (crosshairAlpha > 0.05f) {
            float gap = player.isSprinting() ? 14.0f : 8.0f;
            float len = 8.0f;
            float thickness = 1.8f;
            ImU32 crossColor = ImColor(1.0f, 1.0f, 1.0f, 0.85f * crosshairAlpha);

            drawList->AddLine(ImVec2(center.x, center.y - gap), ImVec2(center.x, center.y - gap - len), crossColor, thickness);
            drawList->AddLine(ImVec2(center.x, center.y + gap), ImVec2(center.x, center.y + gap + len), crossColor, thickness);
            drawList->AddLine(ImVec2(center.x - gap, center.y), ImVec2(center.x - gap - len, center.y), crossColor, thickness);
            drawList->AddLine(ImVec2(center.x + gap, center.y), ImVec2(center.x + gap + len, center.y), crossColor, thickness);
            drawList->AddCircleFilled(ImVec2(center.x, center.y), 1.5f, crossColor);
        }
    }

    // 3. Hitmarker (diagonal cross)
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

    // =========================================================================
    // 4. Cry of Fear Style Health & Stamina HUD (Bottom Left)
    // =========================================================================
    float hudX = 35.0f;
    float hudY = h - 110.0f;
    float hudW = 240.0f;
    float hudH = 80.0f;

    // Dark textured weathered background
    drawList->AddRectFilled(ImVec2(hudX - 10, hudY - 10), ImVec2(hudX + hudW + 10, hudY + hudH + 10),
                            IM_COL32(12, 12, 14, 210), 4.0f);
    drawList->AddRect(ImVec2(hudX - 10, hudY - 10), ImVec2(hudX + hudW + 10, hudY + hudH + 10),
                      IM_COL32(45, 45, 52, 230), 4.0f, 0, 1.5f);

    // Heartbeat ECG Pulse Waveform
    float ecgW = 100.0f;
    float ecgH = 26.0f;
    float ecgX = hudX + 130.0f;
    float ecgY = hudY + 4.0f;

    drawList->AddRectFilled(ImVec2(ecgX, ecgY), ImVec2(ecgX + ecgW, ecgY + ecgH), IM_COL32(8, 8, 10, 255), 2.0f);
    drawList->AddRect(ImVec2(ecgX, ecgY), ImVec2(ecgX + ecgW, ecgY + ecgH), IM_COL32(35, 35, 40, 255), 2.0f);

    // Animated ECG Line
    int bpm = player.isDead() ? 0 : (hpFraction < 0.35f ? 140 : (player.isSprinting() ? 120 : 72));
    float ecgSpeed = (bpm / 60.0f) * 2.5f;
    ImU32 ecgColor = player.isDead() ? IM_COL32(160, 30, 30, 200) : (hpFraction < 0.35f ? IM_COL32(230, 45, 45, 255) : IM_COL32(40, 210, 120, 255));

    for (int px = 0; px < (int)ecgW - 1; px += 2) {
        auto getEcgY = [&](float t) -> float {
            if (player.isDead()) return ecgY + ecgH * 0.5f;
            float phase = std::fmod(t, 1.0f);
            float offset = 0.0f;
            if (phase > 0.30f && phase < 0.36f) offset = -ecgH * 0.25f;
            else if (phase >= 0.36f && phase < 0.44f) offset = ecgH * 0.45f;
            else if (phase >= 0.44f && phase < 0.50f) offset = -ecgH * 0.35f;
            else if (phase >= 0.50f && phase < 0.60f) offset = ecgH * 0.12f;
            return (ecgY + ecgH * 0.5f) - offset;
        };

        float t1 = s_ecgTime * ecgSpeed + (float)px / ecgW;
        float t2 = s_ecgTime * ecgSpeed + (float)(px + 2) / ecgW;
        drawList->AddLine(ImVec2(ecgX + px, getEcgY(t1)), ImVec2(ecgX + px + 2, getEcgY(t2)), ecgColor, 1.5f);
    }

    // Health text & percentage (Cry of Fear blood-red gritty typography)
    char hpText[32];
    std::snprintf(hpText, sizeof(hpText), "%d%%", (int)player.getHealth());
    drawList->AddText(ImVec2(hudX, hudY - 2), IM_COL32(220, 220, 220, 255), "HEALTH");
    drawList->AddText(ImVec2(hudX + 68, hudY - 4), IM_COL32(200, 45, 45, 255), hpText);

    // Segmented Blood-Red Health Gauge
    float hpBarY = hudY + 22.0f;
    float hpBarW = 120.0f;
    float hpBarH = 10.0f;
    drawList->AddRectFilled(ImVec2(hudX, hpBarY), ImVec2(hudX + hpBarW, hpBarY + hpBarH), IM_COL32(25, 25, 28, 255), 2.0f);
    drawList->AddRectFilled(ImVec2(hudX, hpBarY), ImVec2(hudX + hpBarW * hpFraction, hpBarY + hpBarH), IM_COL32(185, 30, 30, 255), 2.0f);
    // Gauge notches
    for (int n = 1; n < 5; ++n) {
        drawList->AddLine(ImVec2(hudX + hpBarW * (n * 0.2f), hpBarY), ImVec2(hudX + hpBarW * (n * 0.2f), hpBarY + hpBarH), IM_COL32(12, 12, 14, 255), 1.5f);
    }

    // Stamina Gauge (slim, distressed cyan / amber)
    float staFraction = std::clamp(player.getStamina() / player.getMaxStamina(), 0.0f, 1.0f);
    float staY = hpBarY + hpBarH + 12.0f;
    drawList->AddText(ImVec2(hudX, staY - 4), IM_COL32(170, 175, 185, 255), "STAMINA");

    float staBarW = 230.0f;
    float staBarH = 6.0f;
    float staBarY = staY + 16.0f;

    drawList->AddRectFilled(ImVec2(hudX, staBarY), ImVec2(hudX + staBarW, staBarY + staBarH), IM_COL32(25, 25, 28, 255), 2.0f);
    ImU32 staColor = (staFraction < 0.20f) ? IM_COL32(240, 180, 40, 255) : IM_COL32(60, 140, 190, 255);
    drawList->AddRectFilled(ImVec2(hudX, staBarY), ImVec2(hudX + staBarW * staFraction, staBarY + staBarH), staColor, 2.0f);

    // =========================================================================
    // 5. Valve Half-Life Style Weapon Selection HUD (Top Screen)
    // =========================================================================
    if (window && !s_chatOpen) {
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) { s_hoverSlot = 1; s_weaponSelectTimer = 3.5f; }
        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) { s_hoverSlot = 2; s_weaponSelectTimer = 3.5f; }
        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) { s_hoverSlot = 3; s_weaponSelectTimer = 3.5f; }
        if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) { s_hoverSlot = 4; s_weaponSelectTimer = 3.5f; }

        if (std::abs(s_scrollDelta) > 0.1) {
            s_weaponSelectTimer = 3.5f;
            if (s_scrollDelta > 0) {
                s_hoverSlot = (s_hoverSlot <= 1) ? 4 : s_hoverSlot - 1;
            } else {
                s_hoverSlot = (s_hoverSlot >= 4) ? 1 : s_hoverSlot + 1;
            }
            s_scrollDelta = 0.0;
        }

        // Selection confirmation on Enter or click while weapon selector is visible
        if (s_weaponSelectTimer > 0.0f) {
            if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
                s_selectedSlot = s_hoverSlot;
                s_weaponSelectTimer = 0.0f;
                AudioEngine::play("switch_click");

                // Equip selected weapon
                switch (s_selectedSlot) {
                    case 1: weapon.setType(WeaponType::None); break;
                    case 2: weapon.setType(WeaponType::Glock); break;
                    case 3: weapon.setType(WeaponType::M4); break;
                    case 4: weapon.setType(WeaponType::Shotgun); break;
                }
            }
        }
    }

    if (s_weaponSelectTimer > 0.0f) {
        s_weaponSelectTimer -= dt;
        float alpha = std::min(1.0f, s_weaponSelectTimer / 0.5f);

        float hlWidth = 140.0f;
        float hlHeight = 44.0f;
        float startX = (w - (hlWidth * 4 + 30.0f)) * 0.5f;
        float startY = 25.0f;

        const char* slotNames[] = { "1: UNARMED", "2: GLOCK 17", "3: M4A1 RIFLE", "4: SHOTGUN" };

        for (int i = 1; i <= 4; ++i) {
            float boxX = startX + (i - 1) * (hlWidth + 10.0f);
            bool isHovered = (s_hoverSlot == i);

            ImU32 bgCol = isHovered ? IM_COL32(50, 35, 10, (int)(220 * alpha)) : IM_COL32(20, 20, 22, (int)(180 * alpha));
            ImU32 borderCol = isHovered ? IM_COL32(255, 180, 20, (int)(255 * alpha)) : IM_COL32(70, 70, 75, (int)(160 * alpha));
            ImU32 textCol = isHovered ? IM_COL32(255, 195, 30, (int)(255 * alpha)) : IM_COL32(180, 180, 185, (int)(200 * alpha));

            // Valve HL style golden brackets
            drawList->AddRectFilled(ImVec2(boxX, startY), ImVec2(boxX + hlWidth, startY + hlHeight), bgCol, 2.0f);
            drawList->AddRect(ImVec2(boxX, startY), ImVec2(boxX + hlWidth, startY + hlHeight), borderCol, 2.0f, 0, isHovered ? 2.0f : 1.0f);

            drawList->AddText(ImVec2(boxX + 12, startY + 6), textCol, slotNames[i - 1]);

            char statusSub[32] = "";
            if (i == 1) std::snprintf(statusSub, sizeof(statusSub), "[ READY ]");
            else std::snprintf(statusSub, sizeof(statusSub), "AMMO: READY");
            drawList->AddText(ImVec2(boxX + 12, startY + 24), IM_COL32(140, 140, 140, (int)(190 * alpha)), statusSub);
        }
    }

    // =========================================================================
    // 6. Bottom Right: Tactical Weapon Ammo Display
    // =========================================================================
    float ammoBoxW = 180.0f;
    float ammoBoxH = 65.0f;
    float ammoX = w - ammoBoxW - 35.0f;
    float ammoY = h - ammoBoxH - 35.0f;

    drawList->AddRectFilled(ImVec2(ammoX, ammoY), ImVec2(ammoX + ammoBoxW, ammoY + ammoBoxH), IM_COL32(12, 12, 15, 210), 4.0f);
    drawList->AddRect(ImVec2(ammoX, ammoY), ImVec2(ammoX + ammoBoxW, ammoY + ammoBoxH), IM_COL32(45, 45, 52, 230), 4.0f, 0, 1.5f);

    if (!weapon.hasWeapon()) {
        drawList->AddText(ImVec2(ammoX + 15, ammoY + 12), IM_COL32(160, 165, 175, 255), "EQUIPPED:");
        drawList->AddText(ImVec2(ammoX + 15, ammoY + 32), IM_COL32(230, 190, 40, 255), "UNARMED / FISTS");
    } else {
        std::string wName = weapon.getWeaponName();
        drawList->AddText(ImVec2(ammoX + 15, ammoY + 10), IM_COL32(245, 195, 30, 255), wName.c_str());

        char ammoCount[32];
        if (weapon.isReloading()) {
            std::snprintf(ammoCount, sizeof(ammoCount), "RELOADING...");
            drawList->AddText(ImVec2(ammoX + 15, ammoY + 34), IM_COL32(220, 60, 60, 255), ammoCount);
        } else {
            std::snprintf(ammoCount, sizeof(ammoCount), "%d  /  %d", weapon.getAmmo(), weapon.getReserveAmmo());
            drawList->AddText(ImVec2(ammoX + 15, ammoY + 34), IM_COL32(240, 240, 240, 255), ammoCount);
        }
    }

    // 7. Multiplayer Chat Box
    const auto& chat = netClient.getChatLog();
    if (!chat.empty() || s_chatOpen) {
        float chatY = 80.0f;
        for (const auto& msg : chat) {
            char lineBuf[180];
            std::snprintf(lineBuf, sizeof(lineBuf), "[%s]: %s", msg.sender, msg.text);
            drawList->AddText(ImVec2(35.0f, chatY), IM_COL32(220, 225, 235, 230), lineBuf);
            chatY += 18.0f;
        }

        if (s_chatOpen) {
            ImGui::SetNextWindowPos(ImVec2(35.0f, chatY + 5.0f));
            ImGui::SetNextWindowSize(ImVec2(380.0f, 32.0f));
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
