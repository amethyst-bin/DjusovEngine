#pragma once

#include "world/World.hpp"
#include "player/Player.hpp"
#include <string>
#include <vector>

namespace Djusov {

struct SaveSlotInfo {
    std::string slotName;
    std::string timestamp;
    std::string mapName;
    glm::vec3 playerPosition;
    float playerHealth;
};

class SaveManager {
public:
    static void init();

    // Map file management
    static bool saveMap(const std::string& filePath, const World& world);
    static bool loadMap(const std::string& filePath, World& world);

    // Full Instance Snapshot game save/load
    static bool saveGame(const std::string& slotName, const World& world, const Player& player, const std::string& mapName = "default");
    static bool loadGame(const std::string& slotName, World& world, Player& player);

    // Query available saves for launcher & in-game menu
    static std::vector<SaveSlotInfo> getSaveSlots();
    static bool deleteSave(const std::string& slotName);

    static std::string getSavesDirectory();
    static std::string getMapsDirectory();
};

} // namespace Djusov
