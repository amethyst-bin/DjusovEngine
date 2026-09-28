#pragma once

namespace Djusov {

class Time {
public:
    static void init();
    static void update();

    static float deltaTime() { return s_deltaTime; }
    static float unscaledDeltaTime() { return s_unscaledDeltaTime; }
    static float totalTime() { return s_totalTime; }
    static float timeScale() { return s_timeScale; }
    static void setTimeScale(float scale) { s_timeScale = scale; }
    static int fps() { return s_fps; }

private:
    static float s_lastFrameTime;
    static float s_deltaTime;
    static float s_unscaledDeltaTime;
    static float s_totalTime;
    static float s_timeScale;
    static int s_fps;
    static int s_frameCount;
    static float s_fpsTimer;
};

} // namespace Djusov
