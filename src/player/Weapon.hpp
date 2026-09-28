#pragma once

#include <vector>
#include <string>
#include <memory>
#include <glm/glm.hpp>
#include "render/Mesh.hpp"
#include "render/Material.hpp"

namespace Djusov {

class ViewModel;
class FPSController;
class Player;

enum class WeaponType {
    None = 0,
    Glock,
    Revolver,
    M4,
    Shotgun
};

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

    void setType(WeaponType type, int ammo = -1, int reserve = -1);
    void setTypeByName(const std::string& name, int ammo = -1, int reserve = -1);
    bool hasWeapon() const { return m_type != WeaponType::None; }
    WeaponType getType() const { return m_type; }
    std::string getWeaponName() const;

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

    std::shared_ptr<Mesh> getMesh() const { return m_mesh; }
    const Material& getMaterial() const { return m_material; }

private:
    void loadCurrentModel();

    WeaponType m_type;
    std::shared_ptr<Mesh> m_mesh;
    Material m_material;

    int m_ammo;
    int m_maxAmmo;
    int m_reserveAmmo;

    float m_fireCooldown;
    float m_fireRate; // shots per second
    float m_damage;
    int m_pelletCount;

    bool m_isReloading;
    float m_reloadTimer;
    float m_reloadDuration;

    float m_muzzleFlashTimer;
    glm::vec3 m_muzzleFlashPos;

    Hitmarker m_hitmarker;
    std::vector<BulletTracer> m_tracers;
};

} // namespace Djusov
