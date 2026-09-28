#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <vector>

#include "core/Time.hpp"
#include "render/PBRRenderer.hpp"
#include "world/World.hpp"
#include "world/SaveManager.hpp"
#include "player/Player.hpp"
#include "player/FPSController.hpp"
#include "player/ViewModel.hpp"
#include "player/Weapon.hpp"
#include "script/ScriptEngine.hpp"
#include "network/NetServer.hpp"
#include "network/NetClient.hpp"
#include "ui/GameHUD.hpp"
#include "audio/AudioEngine.hpp"
#include "render/Primitives.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <filesystem>

namespace fs = std::filesystem;
using namespace Djusov;

static void setupDefaultScene(World& world) {
    auto ground = world.createEntity("Ground_Asphalt", "Plane");
    ground->setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
    ground->setScale(glm::vec3(80.0f, 1.0f, 80.0f));
    ground->setMaterial(MaterialManager::createAsphalt());

    auto b1 = world.createEntity("Building_Block_01", "Cube");
    b1->setPosition(glm::vec3(-12.0f, 6.0f, -15.0f));
    b1->setScale(glm::vec3(12.0f, 12.0f, 10.0f));
    b1->setMaterial(MaterialManager::createConcrete());

    auto b2 = world.createEntity("Building_Block_02", "Cube");
    b2->setPosition(glm::vec3(12.0f, 8.0f, -15.0f));
    b2->setScale(glm::vec3(14.0f, 16.0f, 10.0f));
    b2->setMaterial(MaterialManager::createBrick());

    auto mirror = world.createEntity("Mirror_Wall", "Plane");
    mirror->setPosition(glm::vec3(0.0f, 3.0f, -10.0f));
    mirror->setScale(glm::vec3(6.0f, 1.0f, 4.5f));
    mirror->setRotation(glm::vec3(90.0f, 0.0f, 0.0f));
    mirror->setMaterial(MaterialManager::createMirror());

    auto glass = world.createEntity("Glass_Storefront", "Plane");
    glass->setPosition(glm::vec3(-6.0f, 2.5f, -8.0f));
    glass->setScale(glm::vec3(4.0f, 1.0f, 3.0f));
    glass->setRotation(glm::vec3(90.0f, 0.0f, 0.0f));
    glass->setMaterial(MaterialManager::createGlass());

    auto lamp = world.createEntity("Street_Lamp", "Cylinder");
    lamp->setPosition(glm::vec3(4.0f, 2.5f, 2.0f));
    lamp->setScale(glm::vec3(0.2f, 5.0f, 0.2f));
    lamp->setMaterial(MaterialManager::createMetal());
    lamp->setHasLight(true);
    lamp->getLight().color = glm::vec3(1.0f, 0.85f, 0.55f);
    lamp->getLight().intensity = 12.0f;
    lamp->getLight().radius = 25.0f;

    auto ramp = world.createEntity("Metal_Ramp", "Ramp");
    ramp->setPosition(glm::vec3(6.0f, 0.0f, -4.0f));
    ramp->setScale(glm::vec3(3.0f, 2.5f, 6.0f));
    ramp->setMaterial(MaterialManager::createDiamondPlate());

    world.setSpawnPoint(glm::vec3(0.0f, 0.0f, 5.0f));
}

static double s_lastMouseX = 0.0, s_lastMouseY = 0.0;
static float s_mouseDeltaX = 0.0f, s_mouseDeltaY = 0.0f;
static bool s_firstMouse = true;
static bool s_cursorLocked = true;

static void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (s_firstMouse) {
        s_lastMouseX = xpos;
        s_lastMouseY = ypos;
        s_firstMouse = false;
    }
    if (s_cursorLocked) {
        s_mouseDeltaX = static_cast<float>(xpos - s_lastMouseX);
        s_mouseDeltaY = static_cast<float>(s_lastMouseY - ypos); // Inverted Y for pitch
    }
    s_lastMouseX = xpos;
    s_lastMouseY = ypos;
}

int main(int argc, char** argv) {
    try {
        if (argc > 0) {
            fs::path exeDir = fs::canonical(fs::path(argv[0])).parent_path();
            if (!fs::exists("de") && fs::exists(exeDir / "de")) {
                fs::current_path(exeDir);
            } else if (!fs::exists("de") && fs::exists(exeDir.parent_path() / "de")) {
                fs::current_path(exeDir.parent_path());
            }
        }
    } catch (...) {}

    std::string mapPath = "de/maps/default.djson";
    std::string saveSlot = "";
    std::string connectAddr = "";
    std::string playerName = "Player";
    int serverPort = 0;
    float customFov = 75.0f;
    float customSens = 0.12f;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--server" && i + 1 < argc) {
            serverPort = std::stoi(argv[++i]);
        } else if (arg == "--map" && i + 1 < argc) {
            mapPath = argv[++i];
        } else if (arg == "--save" && i + 1 < argc) {
            saveSlot = argv[++i];
        } else if (arg == "--connect" && i + 1 < argc) {
            connectAddr = argv[++i];
        } else if (arg == "--name" && i + 1 < argc) {
            playerName = argv[++i];
        } else if (arg == "--fov" && i + 1 < argc) {
            customFov = std::stof(argv[++i]);
        } else if (arg == "--sens" && i + 1 < argc) {
            customSens = std::stof(argv[++i]);
        }
    }

    // 1. Dedicated Headless Server mode
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

    // 2. Client Game Window
    if (!glfwInit()) {
        std::cerr << "[Runner] Failed to init GLFW!" << std::endl;
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    int winWidth = 1280;
    int winHeight = 720;
    GLFWwindow* window = glfwCreateWindow(winWidth, winHeight, "DjusovEngine - Standalone Game", nullptr, nullptr);
    if (!window) {
        std::cerr << "[Runner] Failed to create window!" << std::endl;
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // VSync
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glewExperimental = GL_TRUE;
    glewInit();
    glEnable(GL_DEPTH_TEST);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Initialize subsystems
    Time::init();
    MaterialManager::init();
    AudioEngine::init();

    PBRRenderer renderer;
    if (!renderer.init(winWidth, winHeight)) {
        std::cerr << "[Runner] Renderer initialization failed!" << std::endl;
        return 1;
    }

    World world;
    Player localPlayer(1, playerName);
    FPSController controller(&localPlayer);
    controller.setBaseFOV(customFov);
    controller.setSensitivity(customSens);

    ViewModel viewModel;
    viewModel.init();

    Weapon weapon;
    NetClient netClient;

    ScriptEngine::init(&world, &localPlayer);
    GameHUD::init();

    // Connect to multiplayer server if requested
    if (!connectAddr.empty()) {
        size_t colon = connectAddr.find(':');
        std::string ip = (colon != std::string::npos) ? connectAddr.substr(0, colon) : connectAddr;
        int port = (colon != std::string::npos) ? std::stoi(connectAddr.substr(colon + 1)) : NET_DEFAULT_PORT;
        netClient.connectToServer(ip, port, playerName);
    }

    // Load initial map or saved state
    bool mapLoaded = false;
    if (!saveSlot.empty()) {
        mapLoaded = SaveManager::loadGame(saveSlot, world, localPlayer);
    } else {
        mapLoaded = SaveManager::loadMap(mapPath, world);
    }
    if (!mapLoaded || world.getEntities().empty()) {
        std::cout << "[Runner] Generating default city scene..." << std::endl;
        setupDefaultScene(world);
    }
    localPlayer.setPosition(world.getSpawnPoint() + glm::vec3(0, 1.0f, 0));

    // Avatar mesh for player reflection in mirrors
    auto avatarMesh = Primitives::createCharacterAvatarMesh();
    Material avatarMat = MaterialManager::createSmoothPlastic();
    avatarMat.albedo = glm::vec3(0.2f, 0.5f, 0.85f);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        Time::update();
        float dt = Time::deltaTime();

        // Cursor unlock toggle (Escape)
        static bool s_escPressed = false;
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            if (!s_escPressed) {
                s_cursorLocked = !s_cursorLocked;
                glfwSetInputMode(window, GLFW_CURSOR, s_cursorLocked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
                s_escPressed = true;
            }
        } else {
            s_escPressed = false;
        }

        // Chat toggle (T key)
        if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS && !GameHUD::isChatOpen()) {
            GameHUD::setChatOpen(true);
        }

        // Quick-Save (Ctrl + S) - Full instance state snapshot
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            SaveManager::saveGame("quick_save", world, localPlayer);
        }

        // FPS Controller input & mouse look
        if (s_cursorLocked && !GameHUD::isChatOpen()) {
            controller.handleMouseMovement(s_mouseDeltaX, s_mouseDeltaY);
            controller.handleInput(window, dt);

            // Shooting (Left Mouse Button)
            if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
                glm::vec3 rayOrig = controller.getCameraPosition();
                glm::vec3 rayDir = controller.getCameraForward();
                if (weapon.fire(viewModel, controller, rayOrig, rayDir)) {
                    // Check raycast against world
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

            // Reload (R)
            if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
                weapon.reload();
            }
        }
        s_mouseDeltaX = 0.0f;
        s_mouseDeltaY = 0.0f;

        // Update systems
        viewModel.update(dt, controller, s_mouseDeltaX, s_mouseDeltaY);
        weapon.update(dt, viewModel, controller);
        ScriptEngine::update(dt);
        netClient.update(dt, localPlayer);

        // Rendering setup
        int fboW, fboH;
        glfwGetFramebufferSize(window, &fboW, &fboH);
        renderer.resize(fboW, fboH);
        float aspect = (fboH > 0) ? (float)fboW / (float)fboH : 1.777f;

        renderer.beginFrame();

        std::vector<RenderObject> renderObjects = world.getRenderObjects();

        // Append remote players from network
        auto remoteAvatars = netClient.getRemotePlayerRenderObjects();
        renderObjects.insert(renderObjects.end(), remoteAvatars.begin(), remoteAvatars.end());

        // 1. Shadow Pass
        renderer.renderShadowPass(renderObjects, world.getSunDirection(), controller.getCameraPosition());

        // 2. Planar Reflection Pass (Mirrors)
        const Entity* mirrorEntity = world.findClosestMirror(controller.getCameraPosition());
        if (mirrorEntity) {
            RenderObject mirrorObj;
            mirrorObj.mesh = mirrorEntity->getMesh();
            mirrorObj.transform = mirrorEntity->getTransformMatrix();
            mirrorObj.material = mirrorEntity->getMaterial();
            mirrorObj.mirrorNormal = glm::normalize(glm::vec3(mirrorObj.transform * glm::vec4(0, 0, 1, 0)));
            mirrorObj.mirrorPoint = glm::vec3(mirrorObj.transform[3]);

            // Render player avatar in mirror!
            RenderObject playerAvatar;
            playerAvatar.mesh = avatarMesh;
            playerAvatar.transform = glm::translate(glm::mat4(1.0f), localPlayer.getPosition());
            playerAvatar.material = avatarMat;

            renderer.renderReflectionPass(renderObjects, &playerAvatar,
                                         controller.getCameraPosition(), controller.getCameraForward(),
                                         controller.getProjectionMatrix(aspect), mirrorObj);
        }

        // 3. Main HDR Pass
        std::vector<PointLightData> pointLights = world.getPointLights();
        if (weapon.hasMuzzleFlash()) {
            PointLightData flashLight;
            flashLight.position = weapon.getMuzzleFlashPos();
            flashLight.color = glm::vec3(1.0f, 0.85f, 0.45f);
            flashLight.intensity = 15.0f;
            flashLight.radius = 12.0f;
            pointLights.push_back(flashLight);
        }

        renderer.renderMainPass(renderObjects, controller.getViewMatrix(), controller.getProjectionMatrix(aspect),
                                controller.getCameraPosition(), world.getSunDirection(), world.getSunColor(), world.getSunIntensity(),
                                pointLights);

        // 4. ViewModel Pass (1st Person hands & weapon with separate depth)
        renderer.renderViewModel(viewModel.getHandsMesh(), viewModel.getWeaponMesh(),
                                 viewModel.getHandsTransform(), viewModel.getWeaponTransform(),
                                 viewModel.getHandsMaterial(), viewModel.getWeaponMaterial(),
                                 controller.getViewMatrix(), controller.getViewModelProjectionMatrix(aspect),
                                 controller.getCameraPosition(), world.getSunDirection(), world.getSunColor());

        // 5. Post-Process Pass (ACES Tonemapping + ADS Scope Peripheral Blur to Screen)
        renderer.renderPostProcess(controller.getADSProgress(), 1.0f, 0);

        // 6. Game HUD Overlay (Health, Stamina, Ammo, Reticle, Hitmarkers)
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        GameHUD::render(localPlayer, weapon, netClient, fboW, fboH, controller.getADSProgress());

        if (!s_cursorLocked) {
            static float s_volVal = 1.0f;
            bool resume = false;
            bool exitGame = false;
            GameHUD::renderPauseMenu(customSens, s_volVal, resume, exitGame);
            controller.setSensitivity(customSens);
            AudioEngine::setMasterVolume(s_volVal);
            if (resume) {
                s_cursorLocked = true;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
            if (exitGame) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    AudioEngine::shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    ScriptEngine::shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
