#include "script/ScriptEngine.hpp"
#include "world/World.hpp"
#include "world/SaveManager.hpp"
#include "player/Player.hpp"

#include <iostream>
#include <fstream>
#include <sstream>

#include "lua.h"
#include "lualib.h"
#include "Luau/Compiler.h"

namespace Djusov {

lua_State* ScriptEngine::s_L = nullptr;
World* ScriptEngine::s_world = nullptr;
Player* ScriptEngine::s_localPlayer = nullptr;
std::vector<ScriptInstance> ScriptEngine::s_activeScripts;

// ================= Module Implementations =================

// @de/savemanager bindings
static int lua_SaveManager_save(lua_State* L) {
    const char* slot = luaL_optstring(L, 1, "auto_save");
    World* world = ScriptEngine::getWorld();
    Player* player = ScriptEngine::getLocalPlayer();
    if (world && player && Djusov::SaveManager::saveGame(slot, *world, *player)) {
        lua_pushboolean(L, 1);
    } else {
        lua_pushboolean(L, 0);
    }
    return 1;
}

static int lua_SaveManager_load(lua_State* L) {
    const char* slot = luaL_checkstring(L, 1);
    World* world = ScriptEngine::getWorld();
    Player* player = ScriptEngine::getLocalPlayer();
    if (world && player && Djusov::SaveManager::loadGame(slot, *world, *player)) {
        lua_pushboolean(L, 1);
    } else {
        lua_pushboolean(L, 0);
    }
    return 1;
}

// @de/world bindings
static int lua_World_getObject(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    World* world = ScriptEngine::getWorld();
    if (!world) {
        lua_pushnil(L);
        return 1;
    }
    Entity* entity = world->getEntityByName(name);
    if (!entity) {
        lua_pushnil(L);
        return 1;
    }

    // Push entity wrapper table
    lua_newtable(L);
    lua_pushinteger(L, entity->getId());
    lua_setfield(L, -2, "id");
    lua_pushstring(L, entity->getName().c_str());
    lua_setfield(L, -2, "name");

    // Position table
    glm::vec3 pos = entity->getPosition();
    lua_newtable(L);
    lua_pushnumber(L, pos.x); lua_setfield(L, -2, "x");
    lua_pushnumber(L, pos.y); lua_setfield(L, -2, "y");
    lua_pushnumber(L, pos.z); lua_setfield(L, -2, "z");
    lua_setfield(L, -2, "position");

    return 1;
}

static int lua_World_getTime(lua_State* L) {
    static float s_time = 0.0f;
    lua_pushnumber(L, s_time);
    return 1;
}

// @de/players bindings
static int lua_Players_getLocalPlayer(lua_State* L) {
    Player* player = ScriptEngine::getLocalPlayer();
    if (!player) {
        lua_pushnil(L);
        return 1;
    }
    lua_newtable(L);
    lua_pushstring(L, player->getName().c_str());
    lua_setfield(L, -2, "name");
    lua_pushnumber(L, player->getHealth());
    lua_setfield(L, -2, "health");
    lua_pushnumber(L, player->getStamina());
    lua_setfield(L, -2, "stamina");

    glm::vec3 pos = player->getPosition();
    lua_newtable(L);
    lua_pushnumber(L, pos.x); lua_setfield(L, -2, "x");
    lua_pushnumber(L, pos.y); lua_setfield(L, -2, "y");
    lua_pushnumber(L, pos.z); lua_setfield(L, -2, "z");
    lua_setfield(L, -2, "position");

    return 1;
}

// @de/lighting bindings
static int lua_Lighting_setSunIntensity(lua_State* L) {
    float intensity = static_cast<float>(luaL_checknumber(L, 1));
    World* world = ScriptEngine::getWorld();
    if (world) {
        world->setSunIntensity(intensity);
    }
    return 0;
}

static int lua_Lighting_setSunColor(lua_State* L) {
    float r = static_cast<float>(luaL_checknumber(L, 1));
    float g = static_cast<float>(luaL_checknumber(L, 2));
    float b = static_cast<float>(luaL_checknumber(L, 3));
    World* world = ScriptEngine::getWorld();
    if (world) {
        world->setSunColor(glm::vec3(r, g, b));
    }
    return 0;
}

int ScriptEngine::customRequire(lua_State* L) {
    const char* moduleName = luaL_checkstring(L, 1);
    std::string modStr = moduleName;

    // Built-in @de/... service modules
    if (modStr == "@de/savemanager") {
        lua_newtable(L);
        lua_pushcfunction(L, lua_SaveManager_save, "save");
        lua_setfield(L, -2, "save");
        lua_pushcfunction(L, lua_SaveManager_load, "load");
        lua_setfield(L, -2, "load");
        return 1;
    } else if (modStr == "@de/world") {
        lua_newtable(L);
        lua_pushcfunction(L, lua_World_getObject, "getObject");
        lua_setfield(L, -2, "getObject");
        lua_pushcfunction(L, lua_World_getTime, "getTime");
        lua_setfield(L, -2, "getTime");
        return 1;
    } else if (modStr == "@de/players") {
        lua_newtable(L);
        lua_pushcfunction(L, lua_Players_getLocalPlayer, "getLocalPlayer");
        lua_setfield(L, -2, "getLocalPlayer");
        return 1;
    } else if (modStr == "@de/lighting") {
        lua_newtable(L);
        lua_pushcfunction(L, lua_Lighting_setSunIntensity, "setSunIntensity");
        lua_setfield(L, -2, "setSunIntensity");
        lua_pushcfunction(L, lua_Lighting_setSunColor, "setSunColor");
        lua_setfield(L, -2, "setSunColor");
        return 1;
    }

    // Try loading custom file module
    std::string path = modStr;
    if (path.rfind(".luau") == std::string::npos && path.rfind(".lua") == std::string::npos) {
        path += ".luau";
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        file.open("de/scripts/" + path);
    }

    if (file.is_open()) {
        std::stringstream ss;
        ss << file.rdbuf();
        std::string code = ss.str();
        std::string err;
        std::string bytecode = compileCode(code, &err);
        if (bytecode.empty()) {
            luaL_error(L, "Module compilation error in '%s': %s", path.c_str(), err.c_str());
            return 0;
        }

        if (luau_load(L, path.c_str(), bytecode.data(), bytecode.size(), 0) == 0) {
            if (lua_pcall(L, 0, 1, 0) == 0) {
                return 1; // Return the module's returned table
            } else {
                const char* errMsg = lua_tostring(L, -1);
                luaL_error(L, "Module runtime error in '%s': %s", path.c_str(), errMsg);
                return 0;
            }
        }
    }

    luaL_error(L, "Could not find module '%s'", moduleName);
    return 0;
}

void ScriptEngine::init(World* world, Player* localPlayer) {
    s_world = world;
    s_localPlayer = localPlayer;

    if (s_L) {
        lua_close(s_L);
    }

    s_L = luaL_newstate();
    luaL_openlibs(s_L);

    // Override require with our modular customRequire
    lua_pushcfunction(s_L, customRequire, "require");
    lua_setglobal(s_L, "require");

    std::cout << "[ScriptEngine] Initialized Luau Virtual Machine with @de/... modules." << std::endl;
}

void ScriptEngine::shutdown() {
    if (s_L) {
        lua_close(s_L);
        s_L = nullptr;
    }
}

std::string ScriptEngine::compileCode(const std::string& source, std::string* outError) {
    Luau::CompileOptions opts;
    opts.optimizationLevel = 1;
    opts.debugLevel = 1;

    std::string bytecode = Luau::compile(source, opts);
    if (bytecode.empty() || bytecode[0] == 0) {
        if (outError) *outError = "Compilation failed";
    }
    return bytecode;
}

bool ScriptEngine::executeString(const std::string& code, const std::string& chunkName, std::string* outError) {
    if (!s_L) return false;

    std::string err;
    std::string bytecode = compileCode(code, &err);
    if (bytecode.empty()) {
        if (outError) *outError = err;
        return false;
    }

    if (luau_load(s_L, chunkName.c_str(), bytecode.data(), bytecode.size(), 0) != 0) {
        const char* msg = lua_tostring(s_L, -1);
        if (outError && msg) *outError = msg;
        lua_pop(s_L, 1);
        return false;
    }

    if (lua_pcall(s_L, 0, 0, 0) != 0) {
        const char* msg = lua_tostring(s_L, -1);
        if (outError && msg) *outError = msg;
        lua_pop(s_L, 1);
        return false;
    }

    return true;
}

bool ScriptEngine::executeFile(const std::string& filePath, std::string* outError) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        if (outError) *outError = "Could not open file: " + filePath;
        return false;
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return executeString(ss.str(), filePath, outError);
}

void ScriptEngine::update(float dt) {
    if (!s_L) return;

    // Call global onUpdate(dt) if defined
    lua_getglobal(s_L, "onUpdate");
    if (lua_isfunction(s_L, -1)) {
        lua_pushnumber(s_L, dt);
        if (lua_pcall(s_L, 1, 0, 0) != 0) {
            std::cerr << "[ScriptEngine] Error in onUpdate: " << lua_tostring(s_L, -1) << std::endl;
            lua_pop(s_L, 1);
        }
    } else {
        lua_pop(s_L, 1);
    }
}

void ScriptEngine::triggerHit(uint32_t entityId, float damage, const std::string& attacker) {
    if (!s_L) return;
    lua_getglobal(s_L, "onHit");
    if (lua_isfunction(s_L, -1)) {
        lua_pushinteger(s_L, entityId);
        lua_pushnumber(s_L, damage);
        lua_pushstring(s_L, attacker.c_str());
        if (lua_pcall(s_L, 3, 0, 0) != 0) {
            std::cerr << "[ScriptEngine] Error in onHit: " << lua_tostring(s_L, -1) << std::endl;
            lua_pop(s_L, 1);
        }
    } else {
        lua_pop(s_L, 1);
    }
}

void ScriptEngine::triggerTouch(uint32_t entityId, Player* player) {
    if (!s_L || !player) return;
    lua_getglobal(s_L, "onTouch");
    if (lua_isfunction(s_L, -1)) {
        lua_pushinteger(s_L, entityId);
        lua_pushstring(s_L, player->getName().c_str());
        if (lua_pcall(s_L, 2, 0, 0) != 0) {
            std::cerr << "[ScriptEngine] Error in onTouch: " << lua_tostring(s_L, -1) << std::endl;
            lua_pop(s_L, 1);
        }
    } else {
        lua_pop(s_L, 1);
    }
}

} // namespace Djusov
