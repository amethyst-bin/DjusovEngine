#pragma once

#include "world/Entity.hpp"
#include "render/PBRRenderer.hpp"
#include "player/Player.hpp"
#include <vector>
#include <memory>
#include <string>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

namespace Djusov {

struct RaycastHit {
    bool hit = false;
    float distance = 0.0f;
    glm::vec3 point = glm::vec3(0.0f);
    glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);
    Entity* entity = nullptr;
    Player* player = nullptr;
};

class World {
public:
    World();
    ~World();

    void clear();

    std::shared_ptr<Entity> createEntity(const std::string& name = "Object", const std::string& primitiveType = "Cube");
    void addEntity(std::shared_ptr<Entity> entity);
    void removeEntity(uint32_t id);
    Entity* getEntityById(uint32_t id);
    Entity* getEntityByName(const std::string& name);
    const std::vector<std::shared_ptr<Entity>>& getEntities() const { return m_entities; }

    // Lighting
    glm::vec3 getSunDirection() const { return m_sunDirection; }
    void setSunDirection(const glm::vec3& dir) { m_sunDirection = dir; }

    glm::vec3 getSunColor() const { return m_sunColor; }
    void setSunColor(const glm::vec3& color) { m_sunColor = color; }

    float getSunIntensity() const { return m_sunIntensity; }
    void setSunIntensity(float intensity) { m_sunIntensity = intensity; }

    glm::vec3 getSpawnPoint() const { return m_spawnPoint; }
    void setSpawnPoint(const glm::vec3& pt) { m_spawnPoint = pt; }

    // Physics & Raycasting
    RaycastHit raycast(const glm::vec3& origin, const glm::vec3& dir, float maxDist = 200.0f);
    bool checkCollision(const glm::vec3& playerPos, float radius, float height, glm::vec3& outCorrection);

    // Collect render objects for PBRRenderer
    std::vector<RenderObject> getRenderObjects() const;
    std::vector<PointLightData> getPointLights() const;
    const Entity* findClosestMirror(const glm::vec3& cameraPos) const;

    // Starter Pack & Loadout
    struct StarterPack {
        bool giveWeapon = false;
        std::string weaponType = "None"; // "None", "Glock", "Revolver", "M4", "Shotgun"
        int ammo = 30;
        int reserveAmmo = 120;
    };

    StarterPack& getStarterPack() { return m_starterPack; }
    const StarterPack& getStarterPack() const { return m_starterPack; }
    void setStarterPack(const StarterPack& sp) { m_starterPack = sp; }

    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& j);

private:
    std::vector<std::shared_ptr<Entity>> m_entities;
    uint32_t m_nextEntityId;

    glm::vec3 m_sunDirection;
    glm::vec3 m_sunColor;
    float m_sunIntensity;
    glm::vec3 m_spawnPoint;

    StarterPack m_starterPack;
};

} // namespace Djusov
