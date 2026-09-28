#include "player/ViewModel.hpp"
#include "player/FPSController.hpp"
#include "render/Primitives.hpp"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

namespace Djusov {

ViewModel::ViewModel()
    : m_hipPosition(0.18f, -0.22f, -0.42f),
      m_adsPosition(0.00f, -0.165f, -0.32f), // Exact center aperture alignment
      m_currentPosition(0.18f, -0.22f, -0.42f),
      m_swayOffset(0.0f), m_swayRotation(0.0f),
      m_bobTimer(0.0f), m_bobOffset(0.0f),
      m_recoilOffset(0.0f), m_recoilRotation(0.0f),
      m_adsProgress(0.0f),
      m_handsTransform(1.0f), m_weaponTransform(1.0f) {

    // Materials
    m_handsMaterial.name = "HandsSkin";
    m_handsMaterial.albedo = glm::vec3(0.85f, 0.65f, 0.52f); // natural skin tone
    m_handsMaterial.metallic = 0.0f;
    m_handsMaterial.roughness = 0.65f;

    m_weaponMaterial = MaterialManager::createMetal();
    m_weaponMaterial.name = "TacticalWeapon";
    m_weaponMaterial.albedo = glm::vec3(0.15f, 0.15f, 0.16f); // dark tactical matte steel
    m_weaponMaterial.roughness = 0.35f;
    m_weaponMaterial.metallic = 0.90f;
}

bool ViewModel::init() {
    m_handsMesh = std::make_shared<SkeletalMesh>();
    // Try to load the user-provided rigged hands FBX
    if (!m_handsMesh->loadFBX("assets/models/fps-hands.fbx")) {
        std::cerr << "[ViewModel] Warning: Could not load 'assets/models/fps-hands.fbx', hands will be omitted." << std::endl;
    }

    m_weaponMesh = Primitives::createWeaponMesh();
    return true;
}

void ViewModel::triggerRecoil(float recoilStrength) {
    m_recoilOffset += 0.06f * recoilStrength;
    m_recoilRotation += 5.0f * recoilStrength;
}

void ViewModel::update(float dt, const FPSController& controller, float mouseDeltaX, float mouseDeltaY) {
    m_adsProgress = controller.getADSProgress();

    // 1. Mouse Sway with spring recovery
    float swayScale = glm::mix(1.0f, 0.2f, m_adsProgress);
    glm::vec3 targetSway(
        -mouseDeltaX * 0.0012f * swayScale,
        -mouseDeltaY * 0.0012f * swayScale,
        0.0f
    );
    // Clamp sway
    targetSway.x = std::clamp(targetSway.x, -0.06f, 0.06f);
    targetSway.y = std::clamp(targetSway.y, -0.06f, 0.06f);

    m_swayOffset = glm::mix(m_swayOffset, targetSway, dt * 10.0f);
    m_swayRotation.z = glm::mix(m_swayRotation.z, mouseDeltaX * 0.15f * swayScale, dt * 10.0f);
    m_swayRotation.x = glm::mix(m_swayRotation.x, mouseDeltaY * 0.15f * swayScale, dt * 10.0f);

    // 2. Procedural Bobbing based on velocity
    glm::vec3 vel = glm::vec3(1.0f); // Default walk/idle factor
    float speed = 2.0f; // placeholder for speed factor
    m_bobTimer += dt * 8.0f * speed;

    float bobFactor = glm::mix(1.0f, 0.1f, m_adsProgress); // Bobbing strongly reduced in ADS
    m_bobOffset.x = std::cos(m_bobTimer * 0.5f) * 0.012f * bobFactor;
    m_bobOffset.y = std::sin(m_bobTimer) * 0.016f * bobFactor;

    // 3. Recoil recovery
    m_recoilOffset = glm::mix(m_recoilOffset, 0.0f, dt * 14.0f);
    m_recoilRotation = glm::mix(m_recoilRotation, 0.0f, dt * 14.0f);

    // 4. Target position (lerp between hip and ADS)
    glm::vec3 targetPos = glm::mix(m_hipPosition, m_adsPosition, m_adsProgress);
    m_currentPosition = glm::mix(m_currentPosition, targetPos, dt * 16.0f);

    // Composite local transform
    glm::vec3 finalPos = m_currentPosition + m_swayOffset + m_bobOffset - glm::vec3(0, 0, m_recoilOffset);

    glm::mat4 localT = glm::translate(glm::mat4(1.0f), finalPos);
    localT = glm::rotate(localT, glm::radians(m_swayRotation.x + m_recoilRotation), glm::vec3(1, 0, 0));
    localT = glm::rotate(localT, glm::radians(m_swayRotation.z), glm::vec3(0, 0, 1));

    // Convert local camera space to world transform
    glm::mat4 camWorld = glm::inverse(controller.getViewMatrix());
    m_weaponTransform = camWorld * localT;

    // Hands transform slightly offset behind and around the weapon
    glm::mat4 handsOffset = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.05f, 0.12f));
    m_handsTransform = camWorld * localT * handsOffset;

    // Update skeletal hand posing
    if (m_handsMesh && m_handsMesh->isValid()) {
        m_handsMesh->updateAnimation(dt, speed, m_adsProgress, m_recoilRotation / 5.0f);
    }
}

glm::vec3 ViewModel::getMuzzlePositionWorld(const glm::mat4& cameraViewMatrix) const {
    // Barrel tip is at local z = -0.76f
    glm::vec4 muzzleLocal(0.0f, 0.015f, -0.76f, 1.0f);
    return glm::vec3(m_weaponTransform * muzzleLocal);
}

} // namespace Djusov
