#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <glm/glm.hpp>

struct lua_State;

namespace Djusov {

class World;
class Player;

enum class ScriptType {
    Script,       // Server / Authority script
    LocalScript,  // Client / Local player script
    ModuleScript  // Reusable module loaded with require()
};

struct ScriptInstance {
    std::string path;
    std::string sourceCode;
    ScriptType type = ScriptType::Script;
    uint32_t boundEntityId = 0;
    bool isRunning = false;
    std::string lastError = "";
};

class ScriptEngine {
public:
    static void init(World* world, Player* localPlayer);
    static void shutdown();

    static bool executeString(const std::string& code, const std::string& chunkName = "chunk", std::string* outError = nullptr);
    static bool executeFile(const std::string& filePath, std::string* outError = nullptr);

    static void update(float dt);
    static void triggerHit(uint32_t entityId, float damage, const std::string& attacker);
    static void triggerTouch(uint32_t entityId, Player* player);

    static lua_State* getState() { return s_L; }
    static World* getWorld() { return s_world; }
    static Player* getLocalPlayer() { return s_localPlayer; }
    static void setWorld(World* world) { s_world = world; }
    static void setLocalPlayer(Player* player) { s_localPlayer = player; }

    static std::string compileCode(const std::string& source, std::string* outError = nullptr);

private:
    static void registerModules();
    static int customRequire(lua_State* L);

    static lua_State* s_L;
    static World* s_world;
    static Player* s_localPlayer;
    static std::vector<ScriptInstance> s_activeScripts;
};

} // namespace Djusov
