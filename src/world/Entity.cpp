#include "world/Entity.hpp"
#include "render/Primitives.hpp"
#include <glm/gtc/matrix_transform.hpp>

namespace Djusov {

Entity::Entity(uint32_t id, const std::string& name)
    : m_id(id), m_name(name), m_active(true),
      m_position(0.0f), m_rotation(0.0f), m_scale(1.0f),
      m_primitiveType("Cube"), m_modelPath(""),
      m_hasCollider(true), m_isTrigger(false),
      m_colliderMin(-0.5f), m_colliderMax(0.5f),
      m_hasLight(false), m_scriptPath(""), m_scriptType("Script") {

    m_mesh = Primitives::createCube();
    m_material = MaterialManager::createConcrete();
}

void Entity::setPrimitiveType(const std::string& type) {
    m_primitiveType = type;
    if (type == "Cube") {
        m_mesh = Primitives::createCube();
        m_colliderMin = glm::vec3(-0.5f);
        m_colliderMax = glm::vec3(0.5f);
    } else if (type == "Sphere") {
        m_mesh = Primitives::createSphere();
        m_colliderMin = glm::vec3(-0.5f);
        m_colliderMax = glm::vec3(0.5f);
    } else if (type == "Cylinder") {
        m_mesh = Primitives::createCylinder();
        m_colliderMin = glm::vec3(-0.5f);
        m_colliderMax = glm::vec3(0.5f);
    } else if (type == "Plane") {
        m_mesh = Primitives::createPlane(1.0f, 1.0f, 1.0f);
        m_colliderMin = glm::vec3(-0.5f, -0.05f, -0.5f);
        m_colliderMax = glm::vec3(0.5f, 0.05f, 0.5f);
    } else if (type == "Ramp") {
        m_mesh = Primitives::createRamp();
        m_colliderMin = glm::vec3(-0.5f, 0.0f, -0.5f);
        m_colliderMax = glm::vec3(0.5f, 1.0f, 0.5f);
    }
}

glm::mat4 Entity::getTransformMatrix() const {
    glm::mat4 t = glm::translate(glm::mat4(1.0f), m_position);
    t = glm::rotate(t, glm::radians(m_rotation.y), glm::vec3(0, 1, 0));
    t = glm::rotate(t, glm::radians(m_rotation.x), glm::vec3(1, 0, 0));
    t = glm::rotate(t, glm::radians(m_rotation.z), glm::vec3(0, 0, 1));
    t = glm::scale(t, m_scale);
    return t;
}

void Entity::getWorldAABB(glm::vec3& outMin, glm::vec3& outMax) const {
    glm::vec3 half = (m_colliderMax - m_colliderMin) * 0.5f * m_scale;
    outMin = m_position - half;
    outMax = m_position + half;
}

void Entity::setAttributeString(const std::string& key, const std::string& val) {
    m_stringAttributes[key] = val;
}

std::string Entity::getAttributeString(const std::string& key, const std::string& defaultVal) const {
    auto it = m_stringAttributes.find(key);
    return (it != m_stringAttributes.end()) ? it->second : defaultVal;
}

void Entity::setAttributeFloat(const std::string& key, float val) {
    m_floatAttributes[key] = val;
}

float Entity::getAttributeFloat(const std::string& key, float defaultVal) const {
    auto it = m_floatAttributes.find(key);
    return (it != m_floatAttributes.end()) ? it->second : defaultVal;
}

void Entity::setAttributeBool(const std::string& key, bool val) {
    m_boolAttributes[key] = val;
}

bool Entity::getAttributeBool(const std::string& key, bool defaultVal) const {
    auto it = m_boolAttributes.find(key);
    return (it != m_boolAttributes.end()) ? it->second : defaultVal;
}

nlohmann::json Entity::toJson() const {
    nlohmann::json j;
    j["id"] = m_id;
    j["name"] = m_name;
    j["active"] = m_active;
    j["position"] = { m_position.x, m_position.y, m_position.z };
    j["rotation"] = { m_rotation.x, m_rotation.y, m_rotation.z };
    j["scale"] = { m_scale.x, m_scale.y, m_scale.z };
    j["primitiveType"] = m_primitiveType;
    j["modelPath"] = m_modelPath;
    j["material"] = m_material.toJson();
    j["hasCollider"] = m_hasCollider;
    j["isTrigger"] = m_isTrigger;
    j["hasLight"] = m_hasLight;
    if (m_hasLight) {
        j["lightColor"] = { m_light.color.r, m_light.color.g, m_light.color.b };
        j["lightIntensity"] = m_light.intensity;
        j["lightRadius"] = m_light.radius;
    }
    j["scriptPath"] = m_scriptPath;
    j["scriptType"] = m_scriptType;
    j["stringAttrs"] = m_stringAttributes;
    j["floatAttrs"] = m_floatAttributes;
    j["boolAttrs"] = m_boolAttributes;
    return j;
}

void Entity::fromJson(const nlohmann::json& j) {
    if (j.contains("id")) m_id = j["id"];
    if (j.contains("name")) m_name = j["name"];
    if (j.contains("active")) m_active = j["active"];
    if (j.contains("position")) m_position = glm::vec3(j["position"][0], j["position"][1], j["position"][2]);
    if (j.contains("rotation")) m_rotation = glm::vec3(j["rotation"][0], j["rotation"][1], j["rotation"][2]);
    if (j.contains("scale")) m_scale = glm::vec3(j["scale"][0], j["scale"][1], j["scale"][2]);
    if (j.contains("primitiveType")) setPrimitiveType(j["primitiveType"]);
    if (j.contains("material")) m_material = Material::fromJson(j["material"]);
    if (j.contains("hasCollider")) m_hasCollider = j["hasCollider"];
    if (j.contains("isTrigger")) m_isTrigger = j["isTrigger"];
    if (j.contains("hasLight")) m_hasLight = j["hasLight"];
    if (m_hasLight && j.contains("lightColor")) {
        m_light.color = glm::vec3(j["lightColor"][0], j["lightColor"][1], j["lightColor"][2]);
        m_light.intensity = j["lightIntensity"];
        m_light.radius = j["lightRadius"];
    }
    if (j.contains("scriptPath")) m_scriptPath = j["scriptPath"];
    if (j.contains("scriptType")) m_scriptType = j["scriptType"];
    if (j.contains("stringAttrs")) m_stringAttributes = j["stringAttrs"].get<std::unordered_map<std::string, std::string>>();
    if (j.contains("floatAttrs")) m_floatAttributes = j["floatAttrs"].get<std::unordered_map<std::string, float>>();
    if (j.contains("boolAttrs")) m_boolAttributes = j["boolAttrs"].get<std::unordered_map<std::string, bool>>();
}

} // namespace Djusov
