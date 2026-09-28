#include "player/Weapon.hpp"
#include "player/ViewModel.hpp"
#include "player/FPSController.hpp"
#include <algorithm>

namespace Djusov {

Weapon::Weapon()
    : m_ammo(30), m_maxAmmo(30), m_reserveAmmo(120),
      m_fireCooldown(0.0f), m_fireRate(11.0f), // ~660 RPM
      m_damage(28.0f),
      m_isReloading(false), m_reloadTimer(0.0f), m_reloadDuration(2.2f),
      m_muzzleFlashTimer(0.0f), m_muzzleFlashPos(0.0f) {}

void Weapon::update(float dt, ViewModel& viewModel, FPSController& controller) {
    if (m_fireCooldown > 0.0f) {
        m_fireCooldown -= dt;
    }

    if (m_muzzleFlashTimer > 0.0f) {
        m_muzzleFlashTimer -= dt;
    }

    if (m_hitmarker.timeRemaining > 0.0f) {
        m_hitmarker.timeRemaining -= dt;
    }

    // Update bullet tracers
    for (auto it = m_tracers.begin(); it != m_tracers.end();) {
        it->lifetime -= dt;
        if (it->lifetime <= 0.0f) {
            it = m_tracers.erase(it);
        } else {
            ++it;
        }
    }

    // Reloading
    if (m_isReloading) {
        m_reloadTimer -= dt;
        if (m_reloadTimer <= 0.0f) {
            int needed = m_maxAmmo - m_ammo;
            int take = std::min(needed, m_reserveAmmo);
            m_ammo += take;
            m_reserveAmmo -= take;
            m_isReloading = false;
        }
    }
}

bool Weapon::fire(ViewModel& viewModel, FPSController& controller, const glm::vec3& rayOrigin, const glm::vec3& rayDir) {
    if (m_isReloading || m_fireCooldown > 0.0f || m_ammo <= 0) {
        if (m_ammo <= 0 && !m_isReloading) {
            reload();
        }
        return false;
    }

    m_ammo--;
    m_fireCooldown = 1.0f / m_fireRate;

    // Trigger visual muzzle flash
    m_muzzleFlashTimer = 0.06f;
    m_muzzleFlashPos = viewModel.getMuzzlePositionWorld(controller.getViewMatrix());

    // ViewModel weapon kickback
    viewModel.triggerRecoil(1.0f);

    // Camera recoil kick (upwards with slight random horizontal spread)
    float randomYawSpread = ((rand() % 100) / 100.0f - 0.5f) * 0.35f;
    controller.addCameraRecoil(1.1f, randomYawSpread);

    // Add bullet tracer line
    glm::vec3 hitPoint = rayOrigin + rayDir * 150.0f;
    m_tracers.push_back({ m_muzzleFlashPos, hitPoint, 0.15f, 0.15f });

    return true;
}

void Weapon::reload() {
    if (m_isReloading || m_ammo >= m_maxAmmo || m_reserveAmmo <= 0) return;
    m_isReloading = true;
    m_reloadTimer = m_reloadDuration;
}

void Weapon::registerHit(float damage, bool headshot) {
    m_hitmarker.timeRemaining = 0.35f;
    m_hitmarker.damage = damage;
    m_hitmarker.isHeadshot = headshot;
}

} // namespace Djusov
