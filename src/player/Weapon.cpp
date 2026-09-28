#include "player/Weapon.hpp"
#include "player/ViewModel.hpp"
#include "player/FPSController.hpp"
#include "audio/AudioEngine.hpp"
#include <algorithm>
#include <iostream>

namespace Djusov {

Weapon::Weapon()
    : m_type(WeaponType::None),
      m_mesh(nullptr),
      m_ammo(0), m_maxAmmo(0), m_reserveAmmo(0),
      m_fireCooldown(0.0f), m_fireRate(10.0f),
      m_damage(0.0f), m_pelletCount(1),
      m_isReloading(false), m_reloadTimer(0.0f), m_reloadDuration(2.0f),
      m_muzzleFlashTimer(0.0f), m_muzzleFlashPos(0.0f) {

    m_material = MaterialManager::createMetal();
    m_material.name = "WeaponPBR";
    m_material.albedo = glm::vec3(0.18f, 0.18f, 0.20f);
    m_material.roughness = 0.35f;
    m_material.metallic = 0.85f;
}

std::string Weapon::getWeaponName() const {
    switch (m_type) {
        case WeaponType::Glock: return "GLOCK 17";
        case WeaponType::Revolver: return ".357 REVOLVER";
        case WeaponType::M4: return "M4A1 CARBINE";
        case WeaponType::Shotgun: return "12G SHOTGUN";
        default: return "UNARMED";
    }
}

void Weapon::setTypeByName(const std::string& name, int ammo, int reserve) {
    if (name == "M4" || name == "Rifle" || name == "M4A1") setType(WeaponType::M4, ammo, reserve);
    else if (name == "Revolver" || name == "Magnum") setType(WeaponType::Revolver, ammo, reserve);
    else if (name == "Shotgun" || name == "PumpShotgun") setType(WeaponType::Shotgun, ammo, reserve);
    else if (name == "Glock" || name == "Pistol") setType(WeaponType::Glock, ammo, reserve);
    else setType(WeaponType::None);
}

void Weapon::setType(WeaponType type, int ammo, int reserve) {
    m_type = type;
    m_isReloading = false;
    m_reloadTimer = 0.0f;
    m_fireCooldown = 0.0f;

    switch (type) {
        case WeaponType::M4:
            m_maxAmmo = 30;
            m_damage = 32.0f;
            m_fireRate = 11.5f;
            m_reloadDuration = 2.4f;
            m_pelletCount = 1;
            m_mesh = Mesh::loadModel("assets/models/weapons/m4/source/M4.fbx", 0.72f);
            break;

        case WeaponType::Revolver:
            m_maxAmmo = 6;
            m_damage = 75.0f;
            m_fireRate = 2.2f;
            m_reloadDuration = 2.8f;
            m_pelletCount = 1;
            m_mesh = Mesh::loadModel("assets/models/weapons/revolver/source/Revolver.fbx", 0.32f);
            break;

        case WeaponType::Shotgun:
            m_maxAmmo = 8;
            m_damage = 18.0f; // 8 pellets * 18 = 144 max dmg
            m_fireRate = 1.3f;
            m_reloadDuration = 3.2f;
            m_pelletCount = 8;
            m_mesh = Mesh::loadModel("assets/models/weapons/shotgun/source/Shotgun.fbx", 0.85f);
            break;

        case WeaponType::Glock:
            m_maxAmmo = 17;
            m_damage = 26.0f;
            m_fireRate = 7.5f;
            m_reloadDuration = 1.8f;
            m_pelletCount = 1;
            m_mesh = Mesh::loadModel("assets/models/weapons/glock/source/glock.obj", 0.24f);
            break;

        default:
            m_maxAmmo = 0;
            m_damage = 0.0f;
            m_fireRate = 0.0f;
            m_reloadDuration = 0.0f;
            m_pelletCount = 0;
            m_mesh = nullptr;
            break;
    }

    if (m_type == WeaponType::None) {
        m_ammo = 0;
        m_reserveAmmo = 0;
    } else {
        m_ammo = (ammo >= 0) ? ammo : m_maxAmmo;
        m_reserveAmmo = (reserve >= 0) ? reserve : (m_maxAmmo * 4);
    }
}

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
    if (m_type == WeaponType::None) {
        return false; // Unarmed cannot shoot bullets
    }

    if (m_isReloading || m_fireCooldown > 0.0f || m_ammo <= 0) {
        if (m_ammo <= 0 && !m_isReloading) {
            AudioEngine::play("weapon_empty");
            reload();
        }
        return false;
    }

    m_ammo--;
    m_fireCooldown = 1.0f / m_fireRate;

    // Source Engine gunshot sound!
    AudioEngine::play("weapon_shoot");

    // Trigger visual muzzle flash
    m_muzzleFlashTimer = 0.06f;
    m_muzzleFlashPos = viewModel.getMuzzlePositionWorld(controller.getViewMatrix());

    // ViewModel weapon kickback
    float recoilKick = (m_type == WeaponType::Revolver || m_type == WeaponType::Shotgun) ? 1.8f : 1.0f;
    viewModel.triggerRecoil(recoilKick);

    // Camera recoil kick
    float randomYawSpread = ((rand() % 100) / 100.0f - 0.5f) * 0.4f;
    controller.addCameraRecoil(recoilKick * 1.2f, randomYawSpread);

    // Add bullet tracers (supports multi-pellet for Shotgun)
    for (int p = 0; p < m_pelletCount; ++p) {
        glm::vec3 spreadDir = rayDir;
        if (m_pelletCount > 1) {
            float spread = 0.045f;
            spreadDir.x += ((rand() % 100) / 100.0f - 0.5f) * spread;
            spreadDir.y += ((rand() % 100) / 100.0f - 0.5f) * spread;
            spreadDir.z += ((rand() % 100) / 100.0f - 0.5f) * spread;
            spreadDir = glm::normalize(spreadDir);
        }
        glm::vec3 hitPoint = rayOrigin + spreadDir * 150.0f;
        m_tracers.push_back({ m_muzzleFlashPos, hitPoint, 0.15f, 0.15f });
    }

    return true;
}

void Weapon::reload() {
    if (m_type == WeaponType::None) return;
    if (m_isReloading || m_ammo >= m_maxAmmo || m_reserveAmmo <= 0) return;
    m_isReloading = true;
    m_reloadTimer = m_reloadDuration;
    AudioEngine::play("weapon_reload");
}

void Weapon::registerHit(float damage, bool headshot) {
    m_hitmarker.timeRemaining = 0.35f;
    m_hitmarker.damage = damage;
    m_hitmarker.isHeadshot = headshot;
    AudioEngine::play("hitmarker");
}

} // namespace Djusov
