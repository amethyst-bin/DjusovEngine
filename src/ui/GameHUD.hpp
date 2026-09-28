#pragma once

#include "player/Player.hpp"
#include "player/Weapon.hpp"
#include "network/NetClient.hpp"
#include <vector>
#include <string>

struct GLFWwindow;

namespace Djusov {

class GameHUD {
public:
    static void init();
    static void render(GLFWwindow* window, float dt, const Player& player, Weapon& weapon, const NetClient& netClient,
                       int screenWidth, int screenHeight, float adsAmount);

    static void renderPauseMenu(float& inOutSensitivity, float& inOutVolume, bool& inOutResume, bool& requestExit);

    static bool isChatOpen() { return s_chatOpen; }
    static void toggleChat() { s_chatOpen = !s_chatOpen; }
    static void setChatOpen(bool open) { s_chatOpen = open; }

    static void onMouseScroll(double yoffset);

private:
    static bool s_chatOpen;
    static char s_chatInputBuffer[128];

    // Half-Life Weapon Selector State
    static float s_weaponSelectTimer;
    static int s_selectedSlot;
    static int s_hoverSlot;
    static double s_scrollDelta;

    // Cry of Fear ECG State
    static float s_ecgTime;
};

} // namespace Djusov
