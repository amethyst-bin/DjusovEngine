#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include "render/Mesh.hpp"
#include "render/Material.hpp"
#include "render/PBRRenderer.hpp"

namespace Djusov {

class Entity {
public:
    Entity(uint32_t id, const std::string& name = "Entity");

    uint32_t getId() const { return m_id; }
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }

    bool isActive() const { return m_active; }
    void setActive(bool active) { m_active = active; }

    const glm::vec3& getPosition() const { return m_position; }
    void setPosition(const glm::vec3& pos) { m_position = pos; }

    const glm::vec3& getRotation() const { return m_rotation; }
    void setRotation(const glm::vec3& rot) { m_rotation = rot; }

    const glm::vec3& getScale() const { return m_scale; }
    void setScale(const glm::vec3& scale) { m_scale = scale; }

    const std::string& getPrimitiveType() const { return m_primitiveType; }
    void setPrimitiveType(const std::string& type);

    std::shared_ptr<Mesh> getMesh() const { return m_mesh; }
    void setMesh(std::shared_ptr<Mesh> mesh) { m_mesh = mesh; }

    Material& getMaterial() { return m_material; }
    const Material& getMaterial() const { return m_material; }
    void setMaterial(const Material& mat) { m_material = mat; }

    bool hasCollider() const { return m_hasCollider; }
    void setHasCollider(bool has) { m_hasCollider = has; }

    bool isTrigger() const { return m_isTrigger; }
    void setTrigger(bool trigger) { m_isTrigger = trigger; }

    bool hasLight() const { return m_hasLight; }
    void setHasLight(bool has) { m_hasLight = has; }
    PointLightData& getLight() { return m_light; }
    const PointLightData& getLight() const { return m_light; }

    // Sound Component
    bool hasSound() const { return m_hasSound; }
    void setHasSound(bool has) { m_hasSound = has; }
    const std::string& getSoundName() const { return m_soundName; }
    void setSoundName(const std::string& name) { m_soundName = name; }
    float getSoundVolume() const { return m_soundVolume; }
    void setSoundVolume(float vol) { m_soundVolume = vol; }
    float getSoundPitch() const { return m_soundPitch; }
    void setSoundPitch(float pitch) { m_soundPitch = pitch; }
    float getSoundRadius() const { return m_soundRadius; }
    void setSoundRadius(float radius) { m_soundRadius = radius; }
    bool isSoundLoop() const { return m_soundLoop; }
    void setSoundLoop(bool loop) { m_soundLoop = loop; }
    void playSound();
    void stopSound();

    const std::string& getScriptPath() const { return m_scriptPath; }
    void setScriptPath(const std::string& path) { m_scriptPath = path; }

    const std::string& getScriptType() const { return m_scriptType; }
    void setScriptType(const std::string& type) { m_scriptType = type; }

    glm::mat4 getTransformMatrix() const;
    void getWorldAABB(glm::vec3& outMin, glm::vec3& outMax) const;

    // Custom script attributes for story state
    void setAttributeString(const std::string& key, const std::string& val);
    std::string getAttributeString(const std::string& key, const std::string& defaultVal = "") const;
    void setAttributeFloat(const std::string& key, float val);
    float getAttributeFloat(const std::string& key, float defaultVal = 0.0f) const;
    void setAttributeBool(const std::string& key, bool val);
    bool getAttributeBool(const std::string& key, bool defaultVal = false) const;

    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& j);

private:
    uint32_t m_id;
    std::string m_name;
    bool m_active;

    glm::vec3 m_position;
    glm::vec3 m_rotation; // Euler angles in degrees
    glm::vec3 m_scale;

    std::string m_primitiveType;
    std::string m_modelPath;
    std::shared_ptr<Mesh> m_mesh;
    Material m_material;

    bool m_hasCollider;
    bool m_isTrigger;
    glm::vec3 m_colliderMin;
    glm::vec3 m_colliderMax;

    bool m_hasLight;
    PointLightData m_light;

    bool m_hasSound;
    std::string m_soundName;
    float m_soundVolume;
    float m_soundPitch;
    float m_soundRadius;
    bool m_soundLoop;

    std::string m_scriptPath;
    std::string m_scriptType; // "Script" or "LocalScript"

    std::unordered_map<std::string, std::string> m_stringAttributes;
    std::unordered_map<std::string, float> m_floatAttributes;
    std::unordered_map<std::string, bool> m_boolAttributes;
};

} // namespace Djusov
