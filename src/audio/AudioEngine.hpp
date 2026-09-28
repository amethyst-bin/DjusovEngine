#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace Djusov {

class AudioEngine {
public:
    static bool init();
    static void shutdown();

    static void play(const std::string& name, float volume = 1.0f, float pitch = 1.0f);
    static void play3D(const std::string& name, const glm::vec3& position, float volume = 1.0f, float radius = 25.0f);
    static void updateListener(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up);

    static void setMasterVolume(float volume);
    static float getMasterVolume();

    static std::vector<std::string> getAvailableSounds();
    static bool isInitialized();

private:
    static std::string resolveSoundPath(const std::string& name);
};

} // namespace Djusov
