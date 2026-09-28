#pragma once

#include "render/SkeletalMesh.hpp"
#include "render/Mesh.hpp"
#include "render/Material.hpp"
#include <memory>
#include <glm/glm.hpp>

namespace Djusov {

class FPSController;

class ViewModel {
public:
    ViewModel();

    bool init();
    void update(float dt, const FPSController& controller, float mouseDeltaX, float mouseDeltaY, bool hasWeapon = true);
    void setEquippedWeapon(std::shared_ptr<Mesh> mesh, const Material& mat);

    void triggerRecoil(float recoilStrength = 1.0f);

    const SkeletalMesh* getHandsMesh() const { return m_handsMesh.get(); }
    const Mesh* getWeaponMesh() const { return m_weaponMesh.get(); }

    const glm::mat4& getHandsTransform() const { return m_handsTransform; }
    const glm::mat4& getWeaponTransform() const { return m_weaponTransform; }

    const Material& getHandsMaterial() const { return m_handsMaterial; }
    const Material& getWeaponMaterial() const { return m_weaponMaterial; }

    float getADSProgress() const { return m_adsProgress; }
    glm::vec3 getMuzzlePositionWorld(const glm::mat4& cameraViewMatrix) const;

private:
    std::shared_ptr<SkeletalMesh> m_handsMesh;
    std::shared_ptr<Mesh> m_weaponMesh;

    Material m_handsMaterial;
    Material m_weaponMaterial;

    glm::vec3 m_hipPosition;
    glm::vec3 m_adsPosition;
    glm::vec3 m_currentPosition;

    glm::vec3 m_swayOffset;
    glm::vec3 m_swayRotation;

    float m_bobTimer;
    glm::vec3 m_bobOffset;

    float m_recoilOffset;
    float m_recoilRotation;

    float m_adsProgress;

    glm::mat4 m_handsTransform;
    glm::mat4 m_weaponTransform;
};

} // namespace Djusov
