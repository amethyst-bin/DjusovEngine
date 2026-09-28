#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <vector>

#include "core/Time.hpp"
#include "core/Theme.hpp"
#include "render/PBRRenderer.hpp"
#include "render/Primitives.hpp"
#include "render/Material.hpp"
#include "world/World.hpp"
#include "world/SaveManager.hpp"
#include "player/Player.hpp"
#include "player/FPSController.hpp"
#include "player/ViewModel.hpp"
#include "player/Weapon.hpp"
#include "script/ScriptEngine.hpp"
#include "network/NetServer.hpp"
#include "network/NetClient.hpp"
#include "ui/EditorUI.hpp"
#include "ui/GameHUD.hpp"
#include "build/GameBuilder.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

using namespace Djusov;

static double s_lastMouseX = 0.0, s_lastMouseY = 0.0;
static float s_mouseDeltaX = 0.0f, s_mouseDeltaY = 0.0f;
static bool s_firstMouse = true;
static bool s_freeCamRMB = false;

static void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (s_firstMouse) {
        s_lastMouseX = xpos;
        s_lastMouseY = ypos;
        s_firstMouse = false;
    }
    s_mouseDeltaX = static_cast<float>(xpos - s_lastMouseX);
    s_mouseDeltaY = static_cast<float>(s_lastMouseY - ypos);
    s_lastMouseX = xpos;
    s_lastMouseY = ypos;
}

// Populate an impressive starting scene with buildings, roads, glass, and mirrors
static void setupDefaultScene(World& world) {
    // 1. Asphalt Ground
    auto ground = world.createEntity("Ground_Asphalt", "Plane");
    ground->setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
    ground->setScale(glm::vec3(80.0f, 1.0f, 80.0f));
    ground->setMaterial(MaterialManager::createAsphalt());

    // 2. Concrete Building Block 1
    auto b1 = world.createEntity("Building_Block_01", "Cube");
    b1->setPosition(glm::vec3(-12.0f, 6.0f, -15.0f));
    b1->setScale(glm::vec3(12.0f, 12.0f, 10.0f));
    b1->setMaterial(MaterialManager::createConcrete());

    // 3. Concrete Building Block 2
    auto b2 = world.createEntity("Building_Block_02", "Cube");
    b2->setPosition(glm::vec3(12.0f, 8.0f, -15.0f));
    b2->setScale(glm::vec3(14.0f, 16.0f, 10.0f));
    b2->setMaterial(MaterialManager::createBrick());

    // 4. Mirror Wall (Planar 3D Reflection)
    auto mirror = world.createEntity("Mirror_Wall", "Plane");
    mirror->setPosition(glm::vec3(0.0f, 3.0f, -10.0f));
    mirror->setScale(glm::vec3(6.0f, 1.0f, 4.5f));
    mirror->setRotation(glm::vec3(90.0f, 0.0f, 0.0f));
    mirror->setMaterial(MaterialManager::createMirror());

    // 5. Glass Windows (Fresnel Refraction)
    auto glass = world.createEntity("Glass_Storefront", "Plane");
    glass->setPosition(glm::vec3(-6.0f, 2.5f, -8.0f));
    glass->setScale(glm::vec3(4.0f, 1.0f, 3.0f));
    glass->setRotation(glm::vec3(90.0f, 0.0f, 0.0f));
    glass->setMaterial(MaterialManager::createGlass());

    // 6. Street Lamp with Point Light
    auto lamp = world.createEntity("Street_Lamp", "Cylinder");
    lamp->setPosition(glm::vec3(4.0f, 2.5f, 2.0f));
    lamp->setScale(glm::vec3(0.2f, 5.0f, 0.2f));
    lamp->setMaterial(MaterialManager::createMetal());
    lamp->setHasLight(true);
    lamp->getLight().color = glm::vec3(1.0f, 0.85f, 0.55f);
    lamp->getLight().intensity = 12.0f;
    lamp->getLight().radius = 25.0f;

    // 7. Metal Ramp / Stairs
    auto ramp = world.createEntity("Metal_Ramp", "Ramp");
    ramp->setPosition(glm::vec3(6.0f, 0.0f, -4.0f));
    ramp->setScale(glm::vec3(3.0f, 2.5f, 6.0f));
    ramp->setMaterial(MaterialManager::createDiamondPlate());

    // 8. Glowing Neon Sign
    auto neon = world.createEntity("Neon_Sign", "Cube");
    neon->setPosition(glm::vec3(-12.0f, 8.5f, -9.8f));
    neon->setScale(glm::vec3(5.0f, 1.2f, 0.3f));
    neon->setMaterial(MaterialManager::createNeon());

    world.setSpawnPoint(glm::vec3(0.0f, 0.0f, 5.0f));
    SaveManager::saveMap("de/maps/default.djson", world);
}

int main(int argc, char** argv) {
    int serverPort = 0;
    bool doBuildTest = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--server" && i + 1 < argc) {
            serverPort = std::stoi(argv[++i]);
        } else if (arg == "--build-test") {
            doBuildTest = true;
        }
    }

    if (doBuildTest) {
        std::cout << "[Test] Running GameBuilder export test..." << std::endl;
        World testWorld;
        setupDefaultScene(testWorld);
        BuildConfig cfg;
        cfg.gameName = "TestGame";
        cfg.targetWindows = true; // Test Windows .exe build via MinGW
        bool ok = GameBuilder::buildProject(cfg, testWorld);
        std::cout << "[Test] GameBuilder result: " << (ok ? "SUCCESS" : "FAILED") << std::endl;
        return ok ? 0 : 1;
    }

    // 1. Headless Dedicated Server Mode
    if (serverPort > 0) {
        std::cout << "========================================\n";
        std::cout << "  DjusovEngine Dedicated Server v1.0.0  \n";
        std::cout << "  Port: " << serverPort << "\n";
        std::cout << "========================================\n";

        NetServer server;
        if (!server.start(serverPort)) return 1;

        Time::init();
        while (server.isRunning()) {
            Time::update();
            server.update(Time::deltaTime());
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return 0;
    }

    // 2. Full Engine & Editor GUI
    if (!glfwInit()) {
        std::cerr << "[Engine] Failed to init GLFW!" << std::endl;
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    int winWidth = 1600;
    int winHeight = 900;
    GLFWwindow* window = glfwCreateWindow(winWidth, winHeight, "DjusovEngine - 3D Game Engine & Studio", nullptr, nullptr);
    if (!window) {
        std::cerr << "[Engine] Failed to create window!" << std::endl;
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetCursorPosCallback(window, mouseCallback);

    glewExperimental = GL_TRUE;
    glewInit();
    glEnable(GL_DEPTH_TEST);

    // Subsystems
    Time::init();
    MaterialManager::init();
    SaveManager::init();

    PBRRenderer renderer;
    if (!renderer.init(winWidth, winHeight)) {
        std::cerr << "[Engine] PBR Renderer init failed!" << std::endl;
        return 1;
    }

    World world;
    setupDefaultScene(world);

    Player localPlayer(1, "Player");
    localPlayer.setPosition(world.getSpawnPoint() + glm::vec3(0, 1.0f, 0));

    FPSController controller(&localPlayer);
    ViewModel viewModel;
    viewModel.init();

    Weapon weapon;
    NetClient netClient;

    ScriptEngine::init(&world, &localPlayer);
    GameHUD::init();

    EditorUI editorUI;
    editorUI.init(window, &world, &localPlayer);

    auto avatarMesh = Primitives::createCharacterAvatarMesh();
    Material avatarMat = MaterialManager::createSmoothPlastic();
    avatarMat.albedo = glm::vec3(0.2f, 0.45f, 0.85f);

    bool isPlayMode = false;
    float gridSnap = 1.0f;
    bool requestBuild = false;

    // Viewport dimensions in Editor mode
    int viewW = 1280;
    int viewH = 720;
    bool viewHovered = false;
    bool viewFocused = false;

    // FreeCam position in editor mode
    glm::vec3 freeCamPos(0.0f, 4.0f, 10.0f);
    float freeCamYaw = -90.0f;
    float freeCamPitch = -15.0f;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        Time::update();
        float dt = Time::deltaTime();

        // Check if build was requested from Editor UI
        if (requestBuild) {
            requestBuild = false;
            BuildConfig cfg;
            cfg.gameName = editorUI.getBuildGameName();
            cfg.isMultiplayer = editorUI.isBuildMultiplayer();
            cfg.targetWindows = editorUI.isBuildWindows();
            cfg.minFOV = editorUI.getBuildMinFOV();
            cfg.maxFOV = editorUI.getBuildMaxFOV();
            cfg.allowQuickSave = editorUI.getBuildAllowQuickSave();
            GameBuilder::buildProject(cfg, world);
        }

        // F5 toggle play/stop
        static bool f5Pressed = false;
        if (glfwGetKey(window, GLFW_KEY_F5) == GLFW_PRESS) {
            if (!f5Pressed) {
                isPlayMode = !isPlayMode;
                f5Pressed = true;
            }
        } else {
            f5Pressed = false;
        }

        // Window & Viewport sizing
        int fboW, fboH;
        glfwGetFramebufferSize(window, &fboW, &fboH);

        int renderW = isPlayMode ? fboW : viewW;
        int renderH = isPlayMode ? fboH : viewH;
        if (renderW <= 16) renderW = 1280;
        if (renderH <= 16) renderH = 720;
        renderer.resize(renderW, renderH);

        float aspect = (float)renderW / (float)renderH;
        glm::mat4 projMatrix = glm::perspective(glm::radians(75.0f), aspect, 0.05f, 500.0f);
        glm::mat4 viewMatrix(1.0f);
        glm::vec3 cameraPos(0.0f);
        glm::vec3 cameraForward(0.0f, 0.0f, -1.0f);

        if (isPlayMode) {
            // PLAY MODE: lock cursor, possessed FPS character
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

            controller.handleMouseMovement(s_mouseDeltaX, s_mouseDeltaY);
            controller.handleInput(window, dt);

            // Shooting (LMB)
            if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
                glm::vec3 rayOrig = controller.getCameraPosition();
                glm::vec3 rayDir = controller.getCameraForward();
                if (weapon.fire(viewModel, controller, rayOrig, rayDir)) {
                    RaycastHit hit = world.raycast(rayOrig, rayDir);
                    if (hit.hit) {
                        weapon.registerHit(28.0f, false);
                        if (hit.entity) {
                            ScriptEngine::triggerHit(hit.entity->getId(), 28.0f, localPlayer.getName());
                        }
                    }
                    if (netClient.isConnected()) {
                        netClient.sendWeaponFire(rayOrig, rayDir);
                    }
                }
            }
            if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
                weapon.reload();
            }

            // Quick-save on Ctrl+S
            if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                SaveManager::saveGame("quick_save", world, localPlayer);
            }

            viewModel.update(dt, controller, s_mouseDeltaX, s_mouseDeltaY);
            weapon.update(dt, viewModel, controller);
            ScriptEngine::update(dt);
            netClient.update(dt, localPlayer);

            viewMatrix = controller.getViewMatrix();
            projMatrix = controller.getProjectionMatrix(aspect);
            cameraPos = controller.getCameraPosition();
            cameraForward = controller.getCameraForward();
        } else {
            // EDITOR MODE: FreeCam on RMB hold (only if viewport is hovered or already flying)
            if (viewHovered || s_freeCamRMB) {
                s_freeCamRMB = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
            } else {
                s_freeCamRMB = false;
            }
            glfwSetInputMode(window, GLFW_CURSOR, s_freeCamRMB ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

            if (s_freeCamRMB) {
                freeCamYaw += s_mouseDeltaX * 0.12f;
                freeCamPitch += s_mouseDeltaY * 0.12f;
                freeCamPitch = std::clamp(freeCamPitch, -89.0f, 89.0f);

                float yawR = glm::radians(freeCamYaw);
                float pitchR = glm::radians(freeCamPitch);
                cameraForward.x = std::cos(yawR) * std::cos(pitchR);
                cameraForward.y = std::sin(pitchR);
                cameraForward.z = std::sin(yawR) * std::cos(pitchR);
                cameraForward = glm::normalize(cameraForward);
                glm::vec3 right = glm::normalize(glm::cross(cameraForward, glm::vec3(0, 1, 0)));

                float camSpeed = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ? 24.0f : 10.0f;
                if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) freeCamPos += cameraForward * camSpeed * dt;
                if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) freeCamPos -= cameraForward * camSpeed * dt;
                if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) freeCamPos += right * camSpeed * dt;
                if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) freeCamPos -= right * camSpeed * dt;
                if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) freeCamPos.y += camSpeed * dt;
                if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) freeCamPos.y -= camSpeed * dt;
            } else {
                float yawR = glm::radians(freeCamYaw);
                float pitchR = glm::radians(freeCamPitch);
                cameraForward.x = std::cos(yawR) * std::cos(pitchR);
                cameraForward.y = std::sin(pitchR);
                cameraForward.z = std::sin(yawR) * std::cos(pitchR);
                cameraForward = glm::normalize(cameraForward);

                // Object picking on click inside Viewport
                static bool s_lmbClicked = false;
                if (viewHovered && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
                    if (!s_lmbClicked) {
                        s_lmbClicked = true;
                        RaycastHit hit = world.raycast(freeCamPos, cameraForward);
                        if (hit.hit && hit.entity) {
                            editorUI.setSelectedEntityId(hit.entity->getId());
                        }
                    }
                } else {
                    s_lmbClicked = false;
                }

                // F key: Focus on selected entity
                if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
                    auto ent = world.getEntityById(editorUI.getSelectedEntityId());
                    if (ent) {
                        freeCamPos = ent->getPosition() - cameraForward * 6.0f;
                    }
                }

                // Delete key: Remove entity
                if (glfwGetKey(window, GLFW_KEY_DELETE) == GLFW_PRESS) {
                    if (editorUI.getSelectedEntityId() > 0) {
                        world.removeEntity(editorUI.getSelectedEntityId());
                        editorUI.setSelectedEntityId(0);
                    }
                }
            }

            cameraPos = freeCamPos;
            viewMatrix = glm::lookAt(cameraPos, cameraPos + cameraForward, glm::vec3(0, 1, 0));
        }

        s_mouseDeltaX = 0.0f;
        s_mouseDeltaY = 0.0f;

        // Render passes
        renderer.beginFrame();
        std::vector<RenderObject> renderObjects = world.getRenderObjects();
        auto remoteAvatars = netClient.getRemotePlayerRenderObjects();
        renderObjects.insert(renderObjects.end(), remoteAvatars.begin(), remoteAvatars.end());

        // 1. Shadow Pass
        renderer.renderShadowPass(renderObjects, world.getSunDirection(), cameraPos);

        // 2. Planar Reflection Pass (Mirrors)
        const Entity* mirrorEntity = world.findClosestMirror(cameraPos);
        if (mirrorEntity) {
            RenderObject mirrorObj;
            mirrorObj.mesh = mirrorEntity->getMesh();
            mirrorObj.transform = mirrorEntity->getTransformMatrix();
            mirrorObj.material = mirrorEntity->getMaterial();
            mirrorObj.mirrorNormal = glm::normalize(glm::vec3(mirrorObj.transform * glm::vec4(0, 0, 1, 0)));
            mirrorObj.mirrorPoint = glm::vec3(mirrorObj.transform[3]);

            RenderObject playerAvatar;
            playerAvatar.mesh = avatarMesh;
            playerAvatar.transform = glm::translate(glm::mat4(1.0f), localPlayer.getPosition());
            playerAvatar.material = avatarMat;

            renderer.renderReflectionPass(renderObjects, &playerAvatar,
                                         cameraPos, cameraForward,
                                         projMatrix, mirrorObj);
        }

        // 3. Main HDR Pass
        std::vector<PointLightData> pointLights = world.getPointLights();
        if (isPlayMode && weapon.hasMuzzleFlash()) {
            PointLightData flashLight;
            flashLight.position = weapon.getMuzzleFlashPos();
            flashLight.color = glm::vec3(1.0f, 0.85f, 0.45f);
            flashLight.intensity = 15.0f;
            flashLight.radius = 12.0f;
            pointLights.push_back(flashLight);
        }

        renderer.renderMainPass(renderObjects, viewMatrix, projMatrix,
                                cameraPos, world.getSunDirection(), world.getSunColor(), world.getSunIntensity(),
                                pointLights);

        // 4. ViewModel Pass (only in Play Mode)
        if (isPlayMode) {
            renderer.renderViewModel(viewModel.getHandsMesh(), viewModel.getWeaponMesh(),
                                     viewModel.getHandsTransform(), viewModel.getWeaponTransform(),
                                     viewModel.getHandsMaterial(), viewModel.getWeaponMaterial(),
                                     viewMatrix, controller.getViewModelProjectionMatrix(aspect),
                                     cameraPos, world.getSunDirection(), world.getSunColor());
        }

        // 5. Post-Process Pass
        // In Play Mode -> render directly to window backbuffer (FBO 0)
        // In Editor Mode -> render to Viewport FBO so it displays inside the ImGui Viewport panel
        float ads = isPlayMode ? controller.getADSProgress() : 0.0f;
        GLuint targetFbo = isPlayMode ? 0 : renderer.getViewportFbo();
        renderer.renderPostProcess(ads, 1.0f, targetFbo);

        // Bind default framebuffer (the physical window on your monitor!)
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, fboW, fboH);
        if (!isPlayMode) {
            glClearColor(0.02f, 0.02f, 0.02f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }

        // 6. UI Render (Unified ImGui Lifecycle: NewFrame -> Components -> Render)
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (isPlayMode) {
            GameHUD::render(localPlayer, weapon, netClient, fboW, fboH, controller.getADSProgress());
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                isPlayMode = false;
            }
        } else {
            editorUI.render(isPlayMode, gridSnap, requestBuild,
                            renderer.getViewportTexture(), viewW, viewH,
                            viewHovered, viewFocused);
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ScriptEngine::shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
