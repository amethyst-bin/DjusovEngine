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

using namespace Djusov;

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

    // Initialize subsystems
    Time::init();
    MaterialManager::init();

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
    if (!saveSlot.empty()) {
        SaveManager::loadGame(saveSlot, world, localPlayer);
    } else {
        SaveManager::loadMap(mapPath, world);
        localPlayer.setPosition(world.getSpawnPoint() + glm::vec3(0, 1.0f, 0));
    }

    // Avatar mesh for player reflection in mirrors
    auto avatarMesh = Primitives::createCharacterAvatarMesh();
    Material avatarMat = MaterialManager::createSmoothPlastic();
    avatarMat.albedo = glm::vec3(0.2f, 0.5f, 0.85f);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        Time::update();
        float dt = Time::deltaTime();

        // Cursor unlock toggle (Escape)
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            s_cursorLocked = !s_cursorLocked;
            glfwSetInputMode(window, GLFW_CURSOR, s_cursorLocked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
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

        // 5. Post-Process Pass (ACES Tonemapping + ADS Scope Peripheral Blur)
        renderer.renderPostProcess(controller.getADSProgress(), 1.0f, true);

        // 6. Game HUD Overlay (Health, Stamina, Ammo, Reticle, Hitmarkers)
        GameHUD::render(localPlayer, weapon, netClient, fboW, fboH, controller.getADSProgress());

        glfwSwapBuffers(window);
    }

    ScriptEngine::shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
