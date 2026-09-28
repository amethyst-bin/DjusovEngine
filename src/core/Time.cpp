#include "core/Time.hpp"
#include <GLFW/glfw3.h>

namespace Djusov {

float Time::s_lastFrameTime = 0.0f;
float Time::s_deltaTime = 0.016f;
float Time::s_unscaledDeltaTime = 0.016f;
float Time::s_totalTime = 0.0f;
float Time::s_timeScale = 1.0f;
int Time::s_fps = 60;
int Time::s_frameCount = 0;
float Time::s_fpsTimer = 0.0f;

void Time::init() {
    s_lastFrameTime = static_cast<float>(glfwGetTime());
    s_deltaTime = 0.016f;
    s_unscaledDeltaTime = 0.016f;
    s_totalTime = 0.0f;
    s_timeScale = 1.0f;
    s_fps = 60;
    s_frameCount = 0;
    s_fpsTimer = 0.0f;
}

void Time::update() {
    float current = static_cast<float>(glfwGetTime());
    s_unscaledDeltaTime = current - s_lastFrameTime;
    if (s_unscaledDeltaTime > 0.2f) s_unscaledDeltaTime = 0.2f; // clamp against extreme lag spikes
    s_deltaTime = s_unscaledDeltaTime * s_timeScale;
    s_totalTime = current;
    s_lastFrameTime = current;

    s_frameCount++;
    s_fpsTimer += s_unscaledDeltaTime;
    if (s_fpsTimer >= 1.0f) {
        s_fps = s_frameCount;
        s_frameCount = 0;
        s_fpsTimer = 0.0f;
    }
}

} // namespace Djusov
