#include "world/SaveManager.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

namespace Djusov {

std::string SaveManager::getSavesDirectory() {
    return "de/saves";
}

std::string SaveManager::getMapsDirectory() {
    return "de/maps";
}

void SaveManager::init() {
    try {
        fs::create_directories(getSavesDirectory());
        fs::create_directories(getMapsDirectory());
    } catch (const std::exception& e) {
        std::cerr << "[SaveManager] Failed to create directories: " << e.what() << std::endl;
    }
}

bool SaveManager::saveMap(const std::string& filePath, const World& world) {
    try {
        fs::path p(filePath);
        if (p.has_parent_path()) {
            fs::create_directories(p.parent_path());
        }
        std::ofstream file(filePath);
        if (!file.is_open()) return false;

        nlohmann::json j = world.toJson();
        file << j.dump(4);
        std::cout << "[SaveManager] Saved map to " << filePath << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[SaveManager] Error saving map: " << e.what() << std::endl;
        return false;
    }
}

bool SaveManager::loadMap(const std::string& filePath, World& world) {
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "[SaveManager] Could not open map: " << filePath << std::endl;
            return false;
        }
        nlohmann::json j;
        file >> j;
        world.fromJson(j);
        std::cout << "[SaveManager] Loaded map from " << filePath << " (" << world.getEntities().size() << " entities)" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[SaveManager] Error loading map: " << e.what() << std::endl;
        return false;
    }
}

static std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

bool SaveManager::saveGame(const std::string& slotName, const World& world, const Player& player, const std::string& mapName) {
    init();
    std::string safeName = slotName.empty() ? "quick_save" : slotName;
    std::string filePath = getSavesDirectory() + "/" + safeName + ".djson";

    try {
        nlohmann::json j;
        j["slotName"] = safeName;
        j["timestamp"] = getCurrentTimestamp();
        j["mapName"] = mapName;
        j["player"] = player.toJson();
        j["world"] = world.toJson();

        std::ofstream file(filePath);
        if (!file.is_open()) return false;
        file << j.dump(4);
        std::cout << "[SaveManager] Saved full game snapshot to: " << filePath << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[SaveManager] Error saving game: " << e.what() << std::endl;
        return false;
    }
}

bool SaveManager::loadGame(const std::string& slotName, World& world, Player& player) {
    std::string filePath = getSavesDirectory() + "/" + slotName + ".djson";
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "[SaveManager] Save file not found: " << filePath << std::endl;
            return false;
        }
        nlohmann::json j;
        file >> j;

        if (j.contains("world")) {
            world.fromJson(j["world"]);
        }
        if (j.contains("player")) {
            player.fromJson(j["player"]);
        }
        std::cout << "[SaveManager] Successfully restored game snapshot from: " << filePath << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[SaveManager] Error loading game: " << e.what() << std::endl;
        return false;
    }
}

std::vector<SaveSlotInfo> SaveManager::getSaveSlots() {
    init();
    std::vector<SaveSlotInfo> slots;

    try {
        for (const auto& entry : fs::directory_iterator(getSavesDirectory())) {
            if (entry.is_regular_file() && entry.path().extension() == ".djson") {
                try {
                    std::ifstream file(entry.path());
                    if (file.is_open()) {
                        nlohmann::json j;
                        file >> j;

                        SaveSlotInfo info;
                        info.slotName = j.value("slotName", entry.path().stem().string());
                        info.timestamp = j.value("timestamp", "Unknown");
                        info.mapName = j.value("mapName", "default");

                        if (j.contains("player") && j["player"].contains("position")) {
                            auto pos = j["player"]["position"];
                            info.playerPosition = glm::vec3(pos[0], pos[1], pos[2]);
                            info.playerHealth = j["player"].value("health", 100.0f);
                        } else {
                            info.playerPosition = glm::vec3(0.0f);
                            info.playerHealth = 100.0f;
                        }

                        slots.push_back(info);
                    }
                } catch (...) {
                    // Ignore corrupted individual files
                }
            }
        }
    } catch (...) {}

    return slots;
}

bool SaveManager::deleteSave(const std::string& slotName) {
    std::string filePath = getSavesDirectory() + "/" + slotName + ".djson";
    try {
        return fs::remove(filePath);
    } catch (...) {
        return false;
    }
}

} // namespace Djusov
