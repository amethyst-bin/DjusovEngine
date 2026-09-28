#include "world/World.hpp"
#include <algorithm>
#include <cmath>

namespace Djusov {

World::World()
    : m_nextEntityId(1),
      m_sunDirection(0.55f, 1.0f, 0.35f),
      m_sunColor(1.0f, 0.98f, 0.92f),
      m_sunIntensity(2.8f),
      m_spawnPoint(0.0f, 0.5f, 0.0f) {}

World::~World() {
    clear();
}

void World::clear() {
    m_entities.clear();
    m_nextEntityId = 1;
}

std::shared_ptr<Entity> World::createEntity(const std::string& name, const std::string& primitiveType) {
    auto entity = std::make_shared<Entity>(m_nextEntityId++, name);
    entity->setPrimitiveType(primitiveType);
    m_entities.push_back(entity);
    return entity;
}

void World::addEntity(std::shared_ptr<Entity> entity) {
    if (!entity) return;
    if (entity->getId() >= m_nextEntityId) {
        m_nextEntityId = entity->getId() + 1;
    }
    m_entities.push_back(entity);
}

void World::removeEntity(uint32_t id) {
    m_entities.erase(
        std::remove_if(m_entities.begin(), m_entities.end(), [id](const std::shared_ptr<Entity>& e) {
            return e->getId() == id;
        }),
        m_entities.end()
    );
}

Entity* World::getEntityById(uint32_t id) {
    for (auto& e : m_entities) {
        if (e->getId() == id) return e.get();
    }
    return nullptr;
}

Entity* World::getEntityByName(const std::string& name) {
    for (auto& e : m_entities) {
        if (e->getName() == name) return e.get();
    }
    return nullptr;
}

// Ray-AABB intersection using Kay-Kajiya slab method
static bool intersectRayAABB(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                            const glm::vec3& boxMin, const glm::vec3& boxMax,
                            float& outT, glm::vec3& outNormal) {
    float tmin = 0.0f;
    float tmax = 1e9f;
    glm::vec3 normal(0.0f);

    for (int i = 0; i < 3; ++i) {
        if (std::abs(rayDir[i]) < 1e-6f) {
            if (rayOrigin[i] < boxMin[i] || rayOrigin[i] > boxMax[i]) return false;
        } else {
            float invD = 1.0f / rayDir[i];
            float t1 = (boxMin[i] - rayOrigin[i]) * invD;
            float t2 = (boxMax[i] - rayOrigin[i]) * invD;
            glm::vec3 n(0.0f);
            n[i] = -1.0f;

            if (t1 > t2) {
                std::swap(t1, t2);
                n[i] = 1.0f;
            }

            if (t1 > tmin) {
                tmin = t1;
                normal = n;
            }
            tmax = std::min(tmax, t2);

            if (tmin > tmax) return false;
        }
    }

    outT = tmin;
    outNormal = normal;
    return true;
}

RaycastHit World::raycast(const glm::vec3& origin, const glm::vec3& dir, float maxDist) {
    RaycastHit closestHit;
    closestHit.distance = maxDist;
    glm::vec3 normalizedDir = glm::normalize(dir);

    for (const auto& entity : m_entities) {
        if (!entity->isActive() || !entity->hasCollider()) continue;

        glm::vec3 bMin, bMax;
        entity->getWorldAABB(bMin, bMax);

        float hitT = 0.0f;
        glm::vec3 hitNormal;
        if (intersectRayAABB(origin, normalizedDir, bMin, bMax, hitT, hitNormal)) {
            if (hitT >= 0.0f && hitT < closestHit.distance) {
                closestHit.hit = true;
                closestHit.distance = hitT;
                closestHit.point = origin + normalizedDir * hitT;
                closestHit.normal = hitNormal;
                closestHit.entity = entity.get();
            }
        }
    }

    return closestHit;
}

bool World::checkCollision(const glm::vec3& playerPos, float radius, float height, glm::vec3& outCorrection) {
    glm::vec3 playerMin = playerPos - glm::vec3(radius, 0.0f, radius);
    glm::vec3 playerMax = playerPos + glm::vec3(radius, height, radius);
    outCorrection = glm::vec3(0.0f);
    bool collided = false;

    for (const auto& entity : m_entities) {
        if (!entity->isActive() || !entity->hasCollider() || entity->isTrigger()) continue;

        glm::vec3 bMin, bMax;
        entity->getWorldAABB(bMin, bMax);

        // AABB overlap check
        if (playerMax.x > bMin.x && playerMin.x < bMax.x &&
            playerMax.y > bMin.y && playerMin.y < bMax.y &&
            playerMax.z > bMin.z && playerMin.z < bMax.z) {

            // Compute overlap penetration depth on all 3 axes
            float dx1 = bMax.x - playerMin.x;
            float dx2 = playerMax.x - bMin.x;
            float dy1 = bMax.y - playerMin.y;
            float dy2 = playerMax.y - bMin.y;
            float dz1 = bMax.z - playerMin.z;
            float dz2 = playerMax.z - bMin.z;

            float minX = (dx1 < dx2) ? dx1 : -dx2;
            float minY = (dy1 < dy2) ? dy1 : -dy2;
            float minZ = (dz1 < dz2) ? dz1 : -dz2;

            float absX = std::abs(minX);
            float absY = std::abs(minY);
            float absZ = std::abs(minZ);

            // Push out along axis of least penetration
            if (absX < absY && absX < absZ) {
                outCorrection.x += minX;
            } else if (absY < absX && absY < absZ) {
                outCorrection.y += minY;
            } else {
                outCorrection.z += minZ;
            }
            collided = true;
        }
    }

    return collided;
}

std::vector<RenderObject> World::getRenderObjects() const {
    std::vector<RenderObject> list;
    list.reserve(m_entities.size());

    for (const auto& entity : m_entities) {
        if (!entity->isActive() || !entity->getMesh()) continue;

        RenderObject obj;
        obj.mesh = entity->getMesh();
        obj.transform = entity->getTransformMatrix();
        obj.material = entity->getMaterial();
        obj.isMirror = entity->getMaterial().isMirror;

        if (obj.isMirror) {
            // Compute mirror plane normal and center point from transform
            glm::mat4 m = obj.transform;
            obj.mirrorNormal = glm::normalize(glm::vec3(m * glm::vec4(0, 0, 1, 0)));
            obj.mirrorPoint = glm::vec3(m[3]);
        }

        list.push_back(obj);
    }
    return list;
}

std::vector<PointLightData> World::getPointLights() const {
    std::vector<PointLightData> lights;
    for (const auto& entity : m_entities) {
        if (!entity->isActive() || !entity->hasLight()) continue;
        PointLightData p = entity->getLight();
        p.position = entity->getPosition();
        lights.push_back(p);
    }
    return lights;
}

const Entity* World::findClosestMirror(const glm::vec3& cameraPos) const {
    const Entity* closest = nullptr;
    float closestDist = 1e9f;

    for (const auto& entity : m_entities) {
        if (entity->isActive() && entity->getMaterial().isMirror) {
            float dist = glm::distance(cameraPos, entity->getPosition());
            if (dist < closestDist) {
                closestDist = dist;
                closest = entity.get();
            }
        }
    }
    return closest;
}

nlohmann::json World::toJson() const {
    nlohmann::json j;
    j["sunDirection"] = { m_sunDirection.x, m_sunDirection.y, m_sunDirection.z };
    j["sunColor"] = { m_sunColor.r, m_sunColor.g, m_sunColor.b };
    j["sunIntensity"] = m_sunIntensity;
    j["spawnPoint"] = { m_spawnPoint.x, m_spawnPoint.y, m_spawnPoint.z };

    nlohmann::json entitiesArr = nlohmann::json::array();
    for (const auto& entity : m_entities) {
        entitiesArr.push_back(entity->toJson());
    }
    j["entities"] = entitiesArr;
    return j;
}

void World::fromJson(const nlohmann::json& j) {
    clear();
    if (j.contains("sunDirection")) {
        m_sunDirection = glm::vec3(j["sunDirection"][0], j["sunDirection"][1], j["sunDirection"][2]);
    }
    if (j.contains("sunColor")) {
        m_sunColor = glm::vec3(j["sunColor"][0], j["sunColor"][1], j["sunColor"][2]);
    }
    if (j.contains("sunIntensity")) m_sunIntensity = j["sunIntensity"];
    if (j.contains("spawnPoint")) {
        m_spawnPoint = glm::vec3(j["spawnPoint"][0], j["spawnPoint"][1], j["spawnPoint"][2]);
    }

    if (j.contains("entities")) {
        for (const auto& item : j["entities"]) {
            auto entity = std::make_shared<Entity>(0);
            entity->fromJson(item);
            addEntity(entity);
        }
    }
}

} // namespace Djusov
