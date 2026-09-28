#include "audio/AudioEngine.hpp"
#include "vendor/miniaudio/miniaudio.h"

#include <iostream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

namespace Djusov {

static ma_engine s_engine;
static bool s_initialized = false;
static float s_masterVolume = 1.0f;

bool AudioEngine::init() {
    if (s_initialized) return true;

    ma_engine_config config = ma_engine_config_init();
    ma_result result = ma_engine_init(&config, &s_engine);
    if (result != MA_SUCCESS) {
        std::cerr << "[AudioEngine] Failed to initialize miniaudio engine (error: " << result << ")" << std::endl;
        s_initialized = false;
        return false;
    }

    s_initialized = true;
    ma_engine_set_volume(&s_engine, s_masterVolume);
    std::cout << "[AudioEngine] Initialized audio system with Source Engine sound library." << std::endl;
    return true;
}

void AudioEngine::shutdown() {
    if (s_initialized) {
        ma_engine_uninit(&s_engine);
        s_initialized = false;
    }
}

std::string AudioEngine::resolveSoundPath(const std::string& name) {
    std::string path = name;
    // If not ending in .wav, append
    if (path.find('.') == std::string::npos) {
        path += ".wav";
    }

    if (fs::exists(path)) return path;
    if (fs::exists("assets/sounds/" + path)) return "assets/sounds/" + path;
    if (fs::exists("de/sounds/" + path)) return "de/sounds/" + path;
    if (fs::exists("../assets/sounds/" + path)) return "../assets/sounds/" + path;

    return path;
}

void AudioEngine::play(const std::string& name, float volume, float pitch) {
    if (!s_initialized) return;

    std::string path = resolveSoundPath(name);
    if (!fs::exists(path)) return;

    ma_sound* sound = new ma_sound();
    ma_result res = ma_sound_init_from_file(&s_engine, path.c_str(), MA_SOUND_FLAG_DECODE, nullptr, nullptr, sound);
    if (res == MA_SUCCESS) {
        ma_sound_set_volume(sound, volume * s_masterVolume);
        ma_sound_set_pitch(sound, pitch);
        ma_sound_start(sound);
        // Note: For fire-and-forget in miniaudio, or simple sound:
        // ma_engine_play_sound exists for basic sounds:
    } else {
        delete sound;
        ma_engine_play_sound(&s_engine, path.c_str(), nullptr);
    }
}

void AudioEngine::play3D(const std::string& name, const glm::vec3& position, float volume, float radius) {
    if (!s_initialized) return;

    std::string path = resolveSoundPath(name);
    if (!fs::exists(path)) return;

    ma_sound* sound = new ma_sound();
    ma_result res = ma_sound_init_from_file(&s_engine, path.c_str(), 0, nullptr, nullptr, sound);
    if (res == MA_SUCCESS) {
        ma_sound_set_spatialization_enabled(sound, MA_TRUE);
        ma_sound_set_position(sound, position.x, position.y, position.z);
        ma_sound_set_min_distance(sound, 1.0f);
        ma_sound_set_max_distance(sound, radius);
        ma_sound_set_volume(sound, volume * s_masterVolume);
        ma_sound_start(sound);
    } else {
        delete sound;
        ma_engine_play_sound(&s_engine, path.c_str(), nullptr);
    }
}

void AudioEngine::updateListener(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up) {
    if (!s_initialized) return;

    ma_engine_listener_set_position(&s_engine, 0, position.x, position.y, position.z);
    ma_engine_listener_set_direction(&s_engine, 0, forward.x, forward.y, forward.z);
    ma_engine_listener_set_world_up(&s_engine, 0, up.x, up.y, up.z);
}

void AudioEngine::setMasterVolume(float volume) {
    s_masterVolume = std::clamp(volume, 0.0f, 1.0f);
    if (s_initialized) {
        ma_engine_set_volume(&s_engine, s_masterVolume);
    }
}

float AudioEngine::getMasterVolume() {
    return s_masterVolume;
}

std::vector<std::string> AudioEngine::getAvailableSounds() {
    return {
        "weapon_shoot",
        "weapon_reload",
        "weapon_empty",
        "hitmarker",
        "footstep",
        "lamp_hum",
        "switch_click"
    };
}

bool AudioEngine::isInitialized() {
    return s_initialized;
}

} // namespace Djusov
