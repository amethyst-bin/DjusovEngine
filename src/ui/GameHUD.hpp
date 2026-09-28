#pragma once

#include "player/Player.hpp"
#include "player/Weapon.hpp"
#include "network/NetClient.hpp"
#include <vector>
#include <string>

namespace Djusov {

class GameHUD {
public:
    static void init();
    static void render(const Player& player, const Weapon& weapon, const NetClient& netClient,
                       int screenWidth, int screenHeight, float adsAmount);

    static bool isChatOpen() { return s_chatOpen; }
    static void toggleChat() { s_chatOpen = !s_chatOpen; }
    static void setChatOpen(bool open) { s_chatOpen = open; }

private:
    static bool s_chatOpen;
    static char s_chatInputBuffer[128];
};

} // namespace Djusov
