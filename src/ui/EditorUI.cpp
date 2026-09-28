#include "ui/EditorUI.hpp"
#include "core/Theme.hpp"
#include "core/Math.hpp"
#include "world/SaveManager.hpp"
#include "render/Primitives.hpp"

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cstring>

namespace Djusov {

EditorUI::EditorUI()
    : m_window(nullptr), m_world(nullptr), m_player(nullptr),
      m_selectedEntityId(0),
      m_showScriptEditor(true), m_showSettings(false), m_showBuildDialog(false), m_showDemoWindow(false),
      m_buildMultiplayer(false), m_buildWindows(false),
      m_buildMinFOV(10.0f), m_buildMaxFOV(40.0f), m_buildAllowQuickSave(true) {
    std::strncpy(m_buildGameName, "MyCityGame", sizeof(m_buildGameName));
}

void EditorUI::init(GLFWwindow* window, World* world, Player* player) {
    m_window = window;
    m_world = world;
    m_player = player;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Load JetBrains Mono font
    ImFont* font = io.Fonts->AddFontFromFileTTF("assets/fonts/ui_font.ttf", 15.0f);
    if (!font) {
        std::cerr << "[EditorUI] Warning: Could not load 'assets/fonts/ui_font.ttf', using fallback font." << std::endl;
        io.Fonts->AddFontDefault();
    }

    Theme::init();
    Theme::apply();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    m_scriptEditorDock.init();
    std::cout << "[EditorUI] Initialized Dear ImGui with Amoled theme and JetBrains Mono." << std::endl;
}

void EditorUI::openScriptInEditor(const std::string& scriptPath) {
    m_showScriptEditor = true;
    m_scriptEditorDock.openFile(scriptPath);
}

void EditorUI::renderMenuBar(bool& isPlayMode, float& gridSnap, bool& requestBuild) {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Level")) {
                if (m_world) m_world->clear();
                m_selectedEntityId = 0;
            }
            if (ImGui::MenuItem("Save Level", "Ctrl+S")) {
                if (m_world) SaveManager::saveMap("de/maps/default.djson", *m_world);
            }
            if (ImGui::MenuItem("Load Level")) {
                if (m_world) SaveManager::loadMap("de/maps/default.djson", *m_world);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Build Game")) {
                m_showBuildDialog = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
                glfwSetWindowShouldClose(m_window, GLFW_TRUE);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Delete", "Del")) {
                if (m_world && m_selectedEntityId > 0) {
                    m_world->removeEntity(m_selectedEntityId);
                    m_selectedEntityId = 0;
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Script Editor", nullptr, &m_showScriptEditor);
            ImGui::MenuItem("Settings", nullptr, &m_showSettings);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Run")) {
            if (ImGui::MenuItem(isPlayMode ? "Stop" : "Play", "F5")) {
                isPlayMode = !isPlayMode;
            }
            ImGui::EndMenu();
        }

        // Toolbar widgets embedded directly inside the top menu bar!
        ImGui::SameLine(320);
        ImGui::TextDisabled("|");
        ImGui::SameLine();

        if (isPlayMode) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.25f, 0.25f, 1.0f));
            if (ImGui::Button(" Stop ", ImVec2(60, 20))) {
                isPlayMode = false;
            }
            ImGui::PopStyleColor();
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.75f, 0.35f, 1.0f));
            if (ImGui::Button(" Play ", ImVec2(60, 20))) {
                isPlayMode = true;
            }
            ImGui::PopStyleColor();
        }

        ImGui::SameLine();
        ImGui::Text("Snap:");
        ImGui::SameLine();
        const char* snapOptions[] = { "Off", "0.25m", "0.5m", "1.0m", "2.0m", "5.0m" };
        static int currentSnapIdx = 3;
        ImGui::SetNextItemWidth(75);
        if (ImGui::Combo("##SnapCombo", &currentSnapIdx, snapOptions, IM_ARRAYSIZE(snapOptions))) {
            float values[] = { 0.0f, 0.25f, 0.5f, 1.0f, 2.0f, 5.0f };
            gridSnap = values[currentSnapIdx];
        }

        ImGui::SameLine();
        if (ImGui::Button("Build Game", ImVec2(80, 20))) {
            m_showBuildDialog = true;
        }

        ImGui::SameLine(ImGui::GetWindowWidth() - 320);
        ImGui::TextDisabled("[RMB + WASD to Fly | LMB to Select | F: Focus]");

        ImGui::EndMainMenuBar();
    }
}

void EditorUI::renderViewport(GLuint viewportTexture, int& outViewW, int& outViewH,
                            bool& outHovered, bool& outFocused, bool& isPlayMode) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    if (ImGui::Begin("Viewport", nullptr, flags)) {
        outHovered = ImGui::IsWindowHovered();
        outFocused = ImGui::IsWindowFocused();

        ImVec2 avail = ImGui::GetContentRegionAvail();
        if (avail.x > 16.0f && avail.y > 16.0f) {
            outViewW = static_cast<int>(avail.x);
            outViewH = static_cast<int>(avail.y);

            if (viewportTexture != 0) {
                // OpenGL textures are flipped vertically relative to ImGui UVs
                ImGui::Image((ImTextureID)(intptr_t)viewportTexture, avail, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
            }
        }

        // Viewport Overlay: Quick info in top-left corner
        ImVec2 windowPos = ImGui::GetWindowPos();
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(ImVec2(windowPos.x + 10, windowPos.y + 35),
                                ImVec2(windowPos.x + 240, windowPos.y + 85),
                                IM_COL32(0, 0, 0, 180), 4.0f);
        drawList->AddText(ImVec2(windowPos.x + 16, windowPos.y + 40),
                          IM_COL32(240, 240, 240, 255), "3D Scene Viewport");
        char statsBuf[64];
        std::snprintf(statsBuf, sizeof(statsBuf), "Entities: %zu | %dx%d",
                      m_world ? m_world->getEntities().size() : 0, outViewW, outViewH);
        drawList->AddText(ImVec2(windowPos.x + 16, windowPos.y + 60),
                          IM_COL32(180, 180, 180, 255), statsBuf);
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

void EditorUI::renderSceneHierarchy() {
    if (ImGui::Begin("Scene Hierarchy")) {
        if (!m_world) {
            ImGui::End();
            return;
        }

        if (ImGui::TreeNodeEx("World", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow)) {
            for (const auto& entity : m_world->getEntities()) {
                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                if (entity->getId() == m_selectedEntityId) {
                    flags |= ImGuiTreeNodeFlags_Selected;
                }

                std::string label = entity->getName() + " (" + entity->getPrimitiveType() + ")";
                ImGui::TreeNodeEx((void*)(intptr_t)entity->getId(), flags, "%s", label.c_str());

                if (ImGui::IsItemClicked()) {
                    m_selectedEntityId = entity->getId();
                }
            }
            ImGui::TreePop();
        }

        // Right click empty area to spawn
        if (ImGui::BeginPopupContextWindow("HierarchyContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
            if (ImGui::MenuItem("Create Cube")) m_world->createEntity("Cube", "Cube");
            if (ImGui::MenuItem("Create Sphere")) m_world->createEntity("Sphere", "Sphere");
            if (ImGui::MenuItem("Create Cylinder")) m_world->createEntity("Cylinder", "Cylinder");
            if (ImGui::MenuItem("Create Plane")) m_world->createEntity("Plane", "Plane");
            if (ImGui::MenuItem("Create Ramp")) m_world->createEntity("Ramp", "Ramp");
            ImGui::Separator();
            if (ImGui::MenuItem("Create Mirror")) {
                auto m = m_world->createEntity("Mirror", "Plane");
                m->setMaterial(MaterialManager::createMirror());
                m->setScale(glm::vec3(4.0f, 1.0f, 3.0f));
                m->setRotation(glm::vec3(90.0f, 0.0f, 0.0f));
            }
            if (ImGui::MenuItem("Create Glass Window")) {
                auto g = m_world->createEntity("GlassWindow", "Plane");
                g->setMaterial(MaterialManager::createGlass());
                g->setScale(glm::vec3(3.0f, 1.0f, 2.5f));
                g->setRotation(glm::vec3(90.0f, 0.0f, 0.0f));
            }
            ImGui::EndPopup();
        }
    }
    ImGui::End();
}

void EditorUI::renderInspector() {
    if (ImGui::Begin("Inspector")) {
        if (!m_world || m_selectedEntityId == 0) {
            ImGui::TextDisabled("No entity selected.");
            ImGui::End();
            return;
        }

        Entity* entity = m_world->getEntityById(m_selectedEntityId);
        if (!entity) {
            m_selectedEntityId = 0;
            ImGui::End();
            return;
        }

        // Name
        char nameBuf[64];
        std::strncpy(nameBuf, entity->getName().c_str(), sizeof(nameBuf));
        if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
            entity->setName(nameBuf);
        }

        bool active = entity->isActive();
        if (ImGui::Checkbox("Active", &active)) {
            entity->setActive(active);
        }

        ImGui::Separator();

        // 1. Transform
        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            glm::vec3 pos = entity->getPosition();
            if (ImGui::DragFloat3("Position", &pos.x, 0.1f)) {
                entity->setPosition(pos);
            }

            glm::vec3 rot = entity->getRotation();
            if (ImGui::DragFloat3("Rotation", &rot.x, 1.0f)) {
                entity->setRotation(rot);
            }

            glm::vec3 scale = entity->getScale();
            if (ImGui::DragFloat3("Scale", &scale.x, 0.05f, 0.01f, 100.0f)) {
                entity->setScale(scale);
            }

            if (ImGui::Button("Reset Transform")) {
                entity->setPosition(glm::vec3(0.0f));
                entity->setRotation(glm::vec3(0.0f));
                entity->setScale(glm::vec3(1.0f));
            }
        }

        // 2. Material
        if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
            Material& mat = entity->getMaterial();

            // Preset selector
            std::vector<std::string> presetNames = MaterialManager::getMaterialNames();
            if (ImGui::BeginCombo("Preset", mat.name.c_str())) {
                for (const auto& pName : presetNames) {
                    bool isSelected = (mat.name == pName);
                    if (ImGui::Selectable(pName.c_str(), isSelected)) {
                        Material* pMat = MaterialManager::getMaterial(pName);
                        if (pMat) mat = *pMat;
                    }
                    if (isSelected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            // PBR Sliders
            ImGui::ColorEdit3("Base Color", &mat.albedo.x);
            ImGui::SliderFloat("Metallic", &mat.metallic, 0.0f, 1.0f);
            ImGui::SliderFloat("Roughness", &mat.roughness, 0.01f, 1.0f);

            bool isMirror = mat.isMirror;
            if (ImGui::Checkbox("Mirror (3D Reflection)", &isMirror)) {
                mat.isMirror = isMirror;
                if (isMirror) {
                    mat.metallic = 1.0f;
                    mat.roughness = 0.01f;
                }
            }

            bool isGlass = mat.isGlass;
            if (ImGui::Checkbox("Glass (Fresnel Refraction)", &isGlass)) {
                mat.isGlass = isGlass;
                if (isGlass) {
                    mat.transmission = 0.95f;
                    mat.ior = 1.52f;
                    mat.alpha = 0.25f;
                }
            }

            if (mat.isGlass) {
                ImGui::SliderFloat("Transmission", &mat.transmission, 0.0f, 1.0f);
                ImGui::SliderFloat("IOR", &mat.ior, 1.0f, 2.5f);
                ImGui::SliderFloat("Alpha", &mat.alpha, 0.0f, 1.0f);
            }

            ImGui::ColorEdit3("Emissive Color", &mat.emissive.x);
            ImGui::SliderFloat("Emissive Intensity", &mat.emissiveIntensity, 0.0f, 15.0f);

            ImGui::DragFloat2("UV Tiling", &mat.uvTiling.x, 0.1f, 0.1f, 50.0f);
            ImGui::DragFloat2("UV Offset", &mat.uvOffset.x, 0.05f);
        }

        // 3. Collider
        if (ImGui::CollapsingHeader("Collider", ImGuiTreeNodeFlags_DefaultOpen)) {
            bool hasCol = entity->hasCollider();
            if (ImGui::Checkbox("Has Collider", &hasCol)) {
                entity->setHasCollider(hasCol);
            }

            bool isTrig = entity->isTrigger();
            if (ImGui::Checkbox("Is Trigger", &isTrig)) {
                entity->setTrigger(isTrig);
            }
        }

        // 4. Light Component
        if (ImGui::CollapsingHeader("Light Component")) {
            bool hasLight = entity->hasLight();
            if (ImGui::Checkbox("Enable Light", &hasLight)) {
                entity->setHasLight(hasLight);
            }

            if (entity->hasLight()) {
                PointLightData& light = entity->getLight();
                ImGui::ColorEdit3("Light Color", &light.color.x);
                ImGui::SliderFloat("Intensity", &light.intensity, 0.5f, 25.0f);
                ImGui::SliderFloat("Radius", &light.radius, 1.0f, 50.0f);
            }
        }

        // 5. Script
        if (ImGui::CollapsingHeader("Script")) {
            char scriptBuf[128];
            std::strncpy(scriptBuf, entity->getScriptPath().c_str(), sizeof(scriptBuf));
            if (ImGui::InputText("Script Path", scriptBuf, sizeof(scriptBuf))) {
                entity->setScriptPath(scriptBuf);
            }

            if (ImGui::Button("Open in Script Editor")) {
                openScriptInEditor(entity->getScriptPath().empty() ? "de/scripts/main.luau" : entity->getScriptPath());
            }
        }

        ImGui::Separator();
        if (ImGui::Button("Delete Entity", ImVec2(-1, 26))) {
            m_world->removeEntity(m_selectedEntityId);
            m_selectedEntityId = 0;
        }
    }
    ImGui::End();
}

void EditorUI::renderPalette(float gridSnap) {
    if (ImGui::Begin("Object Palette")) {
        if (!m_world) {
            ImGui::End();
            return;
        }

        ImGui::TextDisabled("1-Click Object Spawner");
        ImGui::Separator();

        glm::vec3 spawnPos(0, 0.5f, 0);
        if (m_player) {
            spawnPos = m_player->getPosition() + glm::vec3(0, 0.5f, 3.0f);
            spawnPos = Math::snapToGrid(spawnPos, gridSnap);
        }

        if (ImGui::Button("Cube", ImVec2(100, 28))) {
            auto e = m_world->createEntity("Cube", "Cube");
            e->setPosition(spawnPos);
            m_selectedEntityId = e->getId();
        }
        ImGui::SameLine();
        if (ImGui::Button("Sphere", ImVec2(100, 28))) {
            auto e = m_world->createEntity("Sphere", "Sphere");
            e->setPosition(spawnPos);
            m_selectedEntityId = e->getId();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cylinder", ImVec2(100, 28))) {
            auto e = m_world->createEntity("Cylinder", "Cylinder");
            e->setPosition(spawnPos);
            m_selectedEntityId = e->getId();
        }

        if (ImGui::Button("Plane (Ground)", ImVec2(100, 28))) {
            auto e = m_world->createEntity("Plane", "Plane");
            e->setPosition(glm::vec3(0, 0, 0));
            e->setScale(glm::vec3(50, 1, 50));
            e->setMaterial(MaterialManager::createAsphalt());
            m_selectedEntityId = e->getId();
        }
        ImGui::SameLine();
        if (ImGui::Button("Ramp (Slope)", ImVec2(100, 28))) {
            auto e = m_world->createEntity("Ramp", "Ramp");
            e->setPosition(spawnPos);
            e->setScale(glm::vec3(3, 2, 6));
            m_selectedEntityId = e->getId();
        }
        ImGui::SameLine();
        if (ImGui::Button("Street Lamp", ImVec2(100, 28))) {
            auto e = m_world->createEntity("StreetLamp", "Cylinder");
            e->setPosition(spawnPos);
            e->setScale(glm::vec3(0.2f, 4.0f, 0.2f));
            e->setMaterial(MaterialManager::createMetal());
            e->setHasLight(true);
            e->getLight().color = glm::vec3(1.0f, 0.85f, 0.6f);
            e->getLight().intensity = 8.0f;
            e->getLight().radius = 20.0f;
            m_selectedEntityId = e->getId();
        }

        if (ImGui::Button("Mirror Wall", ImVec2(100, 28))) {
            auto e = m_world->createEntity("MirrorWall", "Plane");
            e->setPosition(spawnPos);
            e->setScale(glm::vec3(4.0f, 1.0f, 3.0f));
            e->setRotation(glm::vec3(90.0f, 0.0f, 0.0f));
            e->setMaterial(MaterialManager::createMirror());
            m_selectedEntityId = e->getId();
        }
        ImGui::SameLine();
        if (ImGui::Button("Glass Window", ImVec2(100, 28))) {
            auto e = m_world->createEntity("GlassWindow", "Plane");
            e->setPosition(spawnPos);
            e->setScale(glm::vec3(3.0f, 1.0f, 2.5f));
            e->setRotation(glm::vec3(90.0f, 0.0f, 0.0f));
            e->setMaterial(MaterialManager::createGlass());
            m_selectedEntityId = e->getId();
        }
    }
    ImGui::End();
}

void EditorUI::renderSettings() {
    if (ImGui::Begin("Settings", &m_showSettings)) {
        ImGui::Text("Appearance");
        ImGui::Separator();

        // Theme Preset
        const char* themes[] = { "Amoled", "Dark Graphite", "Light" };
        int currentTheme = static_cast<int>(Theme::getPreset());
        if (ImGui::Combo("Theme", &currentTheme, themes, IM_ARRAYSIZE(themes))) {
            Theme::setPreset(static_cast<ThemePreset>(currentTheme));
        }

        // Follow System Accent Color toggle
        bool followSys = Theme::getFollowSystemAccent();
        if (ImGui::Checkbox("Follow System Accent Color (GTK / Qt / DWM)", &followSys)) {
            Theme::setFollowSystemAccent(followSys);
        }

        if (!followSys) {
            glm::vec4 accent = Theme::getAccentColor();
            if (ImGui::ColorEdit3("Custom Accent Color", &accent.x)) {
                Theme::setAccentColor(accent);
            }
        } else {
            glm::vec4 accent = Theme::getAccentColor();
            ImGui::ColorEdit3("Detected Accent Color", &accent.x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoPicker);
        }
    }
    ImGui::End();
}

void EditorUI::renderBuildDialog(bool& requestBuild) {
    ImGui::SetNextWindowSize(ImVec2(450, 360), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Build Game", &m_showBuildDialog)) {
        ImGui::Text("Export Standalone Game Package");
        ImGui::Separator();

        ImGui::InputText("Game Name", m_buildGameName, sizeof(m_buildGameName));

        const char* targetModes[] = { "Singleplayer", "Multiplayer" };
        int modeIdx = m_buildMultiplayer ? 1 : 0;
        if (ImGui::Combo("Game Mode", &modeIdx, targetModes, IM_ARRAYSIZE(targetModes))) {
            m_buildMultiplayer = (modeIdx == 1);
        }

        const char* targetPlatforms[] = { "Linux (Native Executable)", "Windows (.exe via MinGW)" };
        int platformIdx = m_buildWindows ? 1 : 0;
        if (ImGui::Combo("Target Platform", &platformIdx, targetPlatforms, IM_ARRAYSIZE(targetPlatforms))) {
            m_buildWindows = (platformIdx == 1);
        }

        ImGui::Separator();
        ImGui::Text("Launcher Configuration");
        ImGui::SliderFloat("Min FOV", &m_buildMinFOV, 10.0f, 60.0f);
        ImGui::SliderFloat("Max FOV", &m_buildMaxFOV, 40.0f, 120.0f);
        ImGui::Checkbox("Allow Quick Save (Ctrl+S)", &m_buildAllowQuickSave);

        ImGui::Separator();
        ImGui::TextDisabled("Output: dist/ folder with Game launcher, bin/, and de/");

        if (ImGui::Button("Build Game -> dist/", ImVec2(-1, 32))) {
            requestBuild = true;
            m_showBuildDialog = false;
        }
    }
    ImGui::End();
}

void EditorUI::render(bool& isPlayMode, float& gridSnap, bool& requestBuild,
                      GLuint viewportTexture, int& outViewW, int& outViewH,
                      bool& outViewportHovered, bool& outViewportFocused) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags hostFlags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
                                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                 ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_MenuBar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpaceHost", nullptr, hostFlags);
    ImGui::PopStyleVar(3);

    ImGuiID dockspaceId = ImGui::GetID("DjusovDockSpace");

    // Initial default layout setup (Godot / Roblox Studio layout)
    static bool s_dockLayoutBuilt = false;
    if (!s_dockLayoutBuilt || ImGui::DockBuilderGetNode(dockspaceId) == nullptr) {
        s_dockLayoutBuilt = true;

        ImGui::DockBuilderRemoveNode(dockspaceId);
        ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->WorkSize);

        ImGuiID dockMain = dockspaceId;
        ImGuiID dockLeft = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Left, 0.22f, nullptr, &dockMain);
        ImGuiID dockRight = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Right, 0.26f, nullptr, &dockMain);
        ImGuiID dockBottom = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Down, 0.28f, nullptr, &dockMain);
        ImGuiID dockLeftDown = ImGui::DockBuilderSplitNode(dockLeft, ImGuiDir_Down, 0.48f, nullptr, &dockLeft);

        ImGui::DockBuilderDockWindow("Scene Hierarchy", dockLeft);
        ImGui::DockBuilderDockWindow("Object Palette", dockLeftDown);
        ImGui::DockBuilderDockWindow("Inspector", dockRight);
        ImGui::DockBuilderDockWindow("Script Editor", dockBottom);
        ImGui::DockBuilderDockWindow("Viewport", dockMain);

        ImGui::DockBuilderFinish(dockspaceId);
    }

    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    renderMenuBar(isPlayMode, gridSnap, requestBuild);
    ImGui::End();

    renderViewport(viewportTexture, outViewW, outViewH, outViewportHovered, outViewportFocused, isPlayMode);
    renderSceneHierarchy();
    renderInspector();
    renderPalette(gridSnap);

    if (m_showScriptEditor) {
        m_scriptEditorDock.render(&m_showScriptEditor);
    }

    if (m_showSettings) {
        renderSettings();
    }

    if (m_showBuildDialog) {
        renderBuildDialog(requestBuild);
    }
}

} // namespace Djusov
