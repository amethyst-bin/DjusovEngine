#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>

namespace Djusov {

class ViewModel;
class FPSController;
class Player;

struct BulletTracer {
    glm::vec3 start;
    glm::vec3 end;
    float lifetime = 0.15f;
    float maxLifetime = 0.15f;
};

struct Hitmarker {
    float timeRemaining = 0.0f;
    float damage = 0.0f;
    bool isHeadshot = false;
};

class Weapon {
public:
    Weapon();

    void update(float dt, ViewModel& viewModel, FPSController& controller);
    bool fire(ViewModel& viewModel, FPSController& controller, const glm::vec3& rayOrigin, const glm::vec3& rayDir);
    void reload();

    int getAmmo() const { return m_ammo; }
    int getMaxAmmo() const { return m_maxAmmo; }
    int getReserveAmmo() const { return m_reserveAmmo; }
    bool isReloading() const { return m_isReloading; }

    bool hasMuzzleFlash() const { return m_muzzleFlashTimer > 0.0f; }
    glm::vec3 getMuzzleFlashPos() const { return m_muzzleFlashPos; }

    const Hitmarker& getHitmarker() const { return m_hitmarker; }
    const std::vector<BulletTracer>& getTracers() const { return m_tracers; }

    void registerHit(float damage, bool headshot);

private:
    int m_ammo;
    int m_maxAmmo;
    int m_reserveAmmo;

    float m_fireCooldown;
    float m_fireRate; // shots per second
    float m_damage;

    bool m_isReloading;
    float m_reloadTimer;
    float m_reloadDuration;

    float m_muzzleFlashTimer;
    glm::vec3 m_muzzleFlashPos;

    Hitmarker m_hitmarker;
    std::vector<BulletTracer> m_tracers;
};

} // namespace Djusov
