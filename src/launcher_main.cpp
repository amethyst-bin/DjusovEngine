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
};

int main(int argc, char** argv) {
    try {
        if (argc > 0) {
            fs::path exeDir = fs::canonical(fs::path(argv[0])).parent_path();
            if (!fs::exists("de") && fs::exists(exeDir / "de")) {
                fs::current_path(exeDir);
            }
        }
    } catch (...) {}

    if (!glfwInit()) {
        std::cerr << "[Launcher] Failed to initialize GLFW!" << std::endl;
        return 1;
    }

    std::string gameName = "DjusovEngine";
    std::string gameMode = "Singleplayer";
    float minFov = 60.0f;
    float maxFov = 110.0f;
    float sensVal = 1.2f;
    float volVal = 0.8f;
    float fovVal = 75.0f;
    ImVec4 accentColor = ImVec4(0.96f, 0.77f, 0.19f, 1.0f); // Tactical yellow

    std::ifstream cfgFile("de/config.json");
    if (cfgFile.is_open()) {
        try {
            nlohmann::json j;
            cfgFile >> j;
            if (j.contains("gameName")) gameName = j["gameName"];
            if (j.contains("mode")) gameMode = j["mode"];
            if (j.contains("minFOV")) minFov = j["minFOV"];
            if (j.contains("maxFOV")) maxFov = j["maxFOV"];
            if (j.contains("sensitivity")) sensVal = j["sensitivity"];
            if (j.contains("volume")) volVal = j["volume"];
            if (j.contains("fov")) fovVal = j["fov"];
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

    int winW = 860;
    int winH = 540;
    GLFWwindow* window = glfwCreateWindow(winW, winH, (gameName + " - Main Menu").c_str(), nullptr, nullptr);
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

    if (fs::exists("de/fonts/ui_font.ttf")) {
        io.Fonts->AddFontFromFileTTF("de/fonts/ui_font.ttf", 16.0f);
    } else if (fs::exists("assets/fonts/ui_font.ttf")) {
        io.Fonts->AddFontFromFileTTF("assets/fonts/ui_font.ttf", 16.0f);
    } else {
        io.Fonts->AddFontDefault();
    }

    // High quality game menu styling
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.FrameRounding = 3.0f;
    style.ItemSpacing = ImVec2(10.0f, 10.0f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.04f, 0.04f, 0.06f, 1.0f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.09f, 0.09f, 0.12f, 1.0f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.12f, 0.13f, 0.17f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.20f, 0.22f, 0.28f, 1.0f);
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
                saves.push_back({ entry.path().stem().string(), "Saved Game" });
            }
        }
    }

    char ipBuf[64] = "127.0.0.1";
    int port = 7777;
    char nameBuf[32] = "Player";
    int selectedSave = -1;
    int currentTab = 0; // 0=Play, 1=Multiplayer, 2=Settings, 3=Editor
    std::string errorMessage = "";

    auto saveSettingsToConfig = [&]() {
        nlohmann::json j;
        std::ifstream in("de/config.json");
        if (in.is_open()) {
            try { in >> j; } catch (...) {}
            in.close();
        }
        j["sensitivity"] = sensVal;
        j["volume"] = volVal;
        j["fov"] = fovVal;
        std::ofstream out("de/config.json");
        if (out.is_open()) {
            out << j.dump(4);
        }
    };

    auto launchGame = [&](const std::string& extraArgs) {
        saveSettingsToConfig();
        std::string runnerPath = "bin/djusov_runner";
        if (!fs::exists(runnerPath)) runnerPath = "build/djusov_runner";
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
        ImGui::SetNextWindowSize(ImVec2(static_cast<float>(winW), static_cast<float>(winH)));
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

        ImGui::Begin("GameMenuScreen", nullptr, flags);

        // Header Background Banner
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(ImVec2(0, 0), ImVec2(static_cast<float>(winW), 80), IM_COL32(10, 12, 16, 255));
        dl->AddLine(ImVec2(0, 80), ImVec2(static_cast<float>(winW), 80), IM_COL32(245, 196, 48, 255), 2.0f);

        // Game Title
        dl->AddText(ImVec2(30, 20), IM_COL32(245, 196, 48, 255), "DJUSOV ENGINE");
        dl->AddText(ImVec2(30, 48), IM_COL32(160, 165, 175, 255), "TACTICAL FIRST-PERSON SIMULATION");

        dl->AddText(ImVec2(static_cast<float>(winW) - 130, 32), IM_COL32(120, 125, 135, 255), "RELEASE v1.0.0");

        ImGui::SetCursorPos(ImVec2(20, 100));

        // Left Navigation Column (Tactical Game Buttons)
        ImGui::BeginChild("NavColumn", ImVec2(220, static_cast<float>(winH) - 120), true);
        {
            auto menuButton = [&](const char* label, int tabIdx) {
                bool active = (currentTab == tabIdx);
                if (active) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(accentColor.x * 0.7f, accentColor.y * 0.7f, accentColor.z * 0.7f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                }
                if (ImGui::Button(label, ImVec2(-1, 46))) {
                    currentTab = tabIdx;
                }
                if (active) {
                    ImGui::PopStyleColor(2);
                }
                ImGui::Spacing();
            };

            menuButton("▶  START GAME", 0);
            menuButton("⬡  MULTIPLAYER", 1);
            menuButton("⚙  SETTINGS", 2);
            menuButton("◈  STUDIO EDITOR", 3);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.12f, 0.12f, 1.0f));
            if (ImGui::Button("✕  QUIT", ImVec2(-1, 40))) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
            ImGui::PopStyleColor();
        }
        ImGui::EndChild();

        ImGui::SameLine();

        // Right Content View
        ImGui::BeginChild("ContentView", ImVec2(static_cast<float>(winW) - 270, static_cast<float>(winH) - 120), true);
        {
            if (currentTab == 0) {
                // START GAME TAB
                ImGui::TextColored(accentColor, "CAMPAIGN & SANDBOX");
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.65f, 0.32f, 1.0f));
                if (ImGui::Button("▶  LAUNCH LEVEL", ImVec2(-1, 52))) {
                    if (selectedSave >= 0 && selectedSave < static_cast<int>(saves.size())) {
                        launchGame("--save " + saves[selectedSave].name);
                    } else {
                        launchGame("--map de/maps/default.djson");
                    }
                }
                ImGui::PopStyleColor();

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Text("Save Slots (Full Instance Snapshots):");

                if (saves.empty()) {
                    ImGui::TextDisabled("No saves found. A new game will be initialized.");
                } else {
                    for (int i = 0; i < static_cast<int>(saves.size()); ++i) {
                        bool isSel = (selectedSave == i);
                        std::string label = "Slot: " + saves[i].name;
                        if (ImGui::Selectable(label.c_str(), isSel)) {
                            selectedSave = i;
                        }
                    }
                }
            } else if (currentTab == 1) {
                // MULTIPLAYER TAB
                ImGui::TextColored(accentColor, "DEDICATED MULTIPLAYER MATCHMAKING");
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::InputText("Player Nickname", nameBuf, sizeof(nameBuf));
                ImGui::InputText("Server Address", ipBuf, sizeof(ipBuf));
                ImGui::InputInt("Server Port", &port);

                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.55f, 0.95f, 1.0f));
                if (ImGui::Button("▶  CONNECT TO SERVER", ImVec2(-1, 46))) {
                    launchGame("--connect " + std::string(ipBuf) + ":" + std::to_string(port) + " --name " + std::string(nameBuf));
                }
                ImGui::PopStyleColor();

                ImGui::Spacing();
                if (ImGui::Button("Host Local Dedicated Server (:7777)", ImVec2(-1, 36))) {
                    std::string cmd = (fs::exists("bin/djusov_runner") ? "bin/djusov_runner" : "build/djusov_runner") + std::string(" --server 7777 &");
                    std::system(cmd.c_str());
                    launchGame("--connect 127.0.0.1:7777 --name Host");
                }
            } else if (currentTab == 2) {
                // SETTINGS TAB
                ImGui::TextColored(accentColor, "INPUT & AUDIO CONFIGURATION");
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::Text("Mouse Sensitivity:");
                ImGui::SliderFloat("##Sensitivity", &sensVal, 0.2f, 3.5f, "%.2fx");
                ImGui::TextDisabled("Configurable for ultra-fast or precision aim.");

                ImGui::Spacing();
                ImGui::Text("Field of View (FOV):");
                ImGui::SliderFloat("##FOV", &fovVal, minFov, maxFov, "%.0f deg");

                ImGui::Spacing();
                ImGui::Text("Master Sound Volume:");
                ImGui::SliderFloat("##Volume", &volVal, 0.0f, 1.0f, "%.0f%%");

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::Button("Save Settings", ImVec2(160, 34))) {
                    saveSettingsToConfig();
                }
            } else if (currentTab == 3) {
                // STUDIO EDITOR TAB
                ImGui::TextColored(accentColor, "DJUSOVENGINE LEVEL STUDIO");
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::TextWrapped("Open the full 3D Level Editor to build city streets, spawn buildings, place lights and sounds, write Luau scripts, and compile new game builds.");

                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.50f, 0.15f, 1.0f));
                if (ImGui::Button("LAUNCH DJUSOVENGINE STUDIO", ImVec2(-1, 50))) {
                    std::string studioPath = fs::exists("DjusovEngine") ? "./DjusovEngine" : "./build/DjusovEngine";
                    std::system((studioPath + " &").c_str());
                    glfwSetWindowShouldClose(window, GLFW_TRUE);
                }
                ImGui::PopStyleColor();
            }

            if (!errorMessage.empty()) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "[Error] %s", errorMessage.c_str());
            }
        }
        ImGui::EndChild();

        ImGui::End();

        ImGui::Render();
        glViewport(0, 0, winW, winH);
        glClearColor(0.04f, 0.04f, 0.06f, 1.0f);
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
