#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <nlohmann/json.hpp>

#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace fs = std::filesystem;

struct SaveInfo {
    std::string name;
    std::string time;
    std::string map;
};

int main(int argc, char** argv) {
    if (!glfwInit()) {
        std::cerr << "[Launcher] Failed to initialize GLFW!" << std::endl;
        return 1;
    }

    // Read de/config.json
    std::string gameName = "MyCityGame";
    std::string gameMode = "Singleplayer";
    float minFov = 10.0f;
    float maxFov = 40.0f;
    bool allowQuickSave = true;
    ImVec4 accentColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // Default white

    std::ifstream cfgFile("de/config.json");
    if (cfgFile.is_open()) {
        try {
            nlohmann::json j;
            cfgFile >> j;
            if (j.contains("gameName")) gameName = j["gameName"];
            if (j.contains("mode")) gameMode = j["mode"];
            if (j.contains("minFOV")) minFov = j["minFOV"];
            if (j.contains("maxFOV")) maxFov = j["maxFOV"];
            if (j.contains("allowQuickSave")) allowQuickSave = j["allowQuickSave"];
            if (j.contains("accentColor")) {
                auto a = j["accentColor"];
                accentColor = ImVec4(a[0], a[1], a[2], 1.0f);
            }
        } catch (...) {}
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(520, 480, (gameName + " - Launcher").c_str(), nullptr, nullptr);
    if (!window) {
        std::cerr << "[Launcher] Failed to create window!" << std::endl;
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    glewInit();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // Load bundled font
    if (fs::exists("de/fonts/ui_font.ttf")) {
        io.Fonts->AddFontFromFileTTF("de/fonts/ui_font.ttf", 16.0f);
    } else if (fs::exists("assets/fonts/ui_font.ttf")) {
        io.Fonts->AddFontFromFileTTF("assets/fonts/ui_font.ttf", 16.0f);
    } else {
        io.Fonts->AddFontDefault();
    }

    // Apply Amoled theme
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.FrameRounding = 4.0f;
    style.ItemSpacing = ImVec2(8.0f, 8.0f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.0f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.0f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.12f, 0.12f, 0.15f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.18f, 0.18f, 0.22f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive] = accentColor;
    style.Colors[ImGuiCol_CheckMark] = accentColor;
    style.Colors[ImGuiCol_SliderGrab] = accentColor;
    style.Colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.95f, 1.0f);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Scan saves
    std::vector<SaveInfo> saves;
    if (fs::exists("de/saves")) {
        for (const auto& entry : fs::directory_iterator("de/saves")) {
            if (entry.path().extension() == ".djson") {
                saves.push_back({ entry.path().stem().string(), "Saved Game", "default" });
            }
        }
    }

    char ipBuf[64] = "127.0.0.1";
    int port = 7777;
    char nameBuf[32] = "Player";
    float fovVal = (minFov + maxFov) * 0.5f;
    float sensVal = 0.12f;
    float volVal = 0.8f;
    int selectedSave = -1;
    std::string errorMessage = "";

    auto launchGame = [&](const std::string& extraArgs) {
        // Validate dependencies
        std::string runnerPath = "bin/djusov_runner";
        if (!fs::exists(runnerPath)) {
            runnerPath = "build/djusov_runner";
        }
        if (!fs::exists(runnerPath)) {
            errorMessage = "Missing executable: 'bin/djusov_runner' was not found!";
            return;
        }

        std::string cmd = runnerPath + " " + extraArgs + " --fov " + std::to_string(fovVal) + " --sens " + std::to_string(sensVal) + " &";
        std::system(cmd.c_str());
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    };

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(520, 480));
        ImGui::Begin("LauncherMain", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        // Header Title
        ImGui::Spacing();
        ImGui::SetCursorPosX(20);
        ImGui::TextColored(accentColor, "%s", gameName.c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("v1.0.0");
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::BeginTabBar("LauncherTabs")) {
            if (ImGui::BeginTabItem("Play")) {
                ImGui::Spacing();

                if (gameMode == "Singleplayer") {
                    // Singleplayer Launch
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.65f, 0.32f, 1.0f));
                    if (ImGui::Button("Play Game", ImVec2(-20, 42))) {
                        if (selectedSave >= 0 && selectedSave < static_cast<int>(saves.size())) {
                            launchGame("--save " + saves[selectedSave].name);
                        } else {
                            launchGame("--map de/maps/default.djson");
                        }
                    }
                    ImGui::PopStyleColor();

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Text("Saves");

                    if (saves.empty()) {
                        ImGui::TextDisabled("No saves found. Start a new game!");
                    } else {
                        for (int i = 0; i < static_cast<int>(saves.size()); ++i) {
                            bool isSelected = (selectedSave == i);
                            if (ImGui::Selectable(saves[i].name.c_str(), isSelected)) {
                                selectedSave = i;
                            }
                        }
                        if (selectedSave >= 0) {
                            ImGui::TextDisabled("Selected save: %s", saves[selectedSave].name.c_str());
                        }
                    }
                } else {
                    // Multiplayer Launch
                    ImGui::InputText("Server IP", ipBuf, sizeof(ipBuf));
                    ImGui::InputInt("Port", &port);
                    ImGui::InputText("Nickname", nameBuf, sizeof(nameBuf));

                    ImGui::Spacing();
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.55f, 0.95f, 1.0f));
                    if (ImGui::Button("Join Game", ImVec2(-20, 38))) {
                        launchGame("--connect " + std::string(ipBuf) + ":" + std::to_string(port) + " --name " + std::string(nameBuf));
                    }
                    ImGui::PopStyleColor();

                    ImGui::Spacing();
                    if (ImGui::Button("Host Dedicated Server", ImVec2(-20, 30))) {
                        std::string cmd = "bin/djusov_runner --server " + std::to_string(port) + " &";
                        std::system(cmd.c_str());
                    }
                }

                if (!errorMessage.empty()) {
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "[Error] %s", errorMessage.c_str());
                }

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Settings")) {
                ImGui::Spacing();
                ImGui::SliderFloat("FOV", &fovVal, minFov, maxFov, "%.0f deg");
                ImGui::SliderFloat("Mouse Sensitivity", &sensVal, 0.02f, 0.50f);
                ImGui::SliderFloat("Master Volume", &volVal, 0.0f, 1.0f);
                ImGui::Spacing();
                ImGui::TextDisabled("Config: min FOV %.0f, max FOV %.0f", minFov, maxFov);
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
