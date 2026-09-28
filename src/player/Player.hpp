#pragma once

#include <string>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

namespace Djusov {

class Player {
public:
    Player(int id = 0, const std::string& name = "Player");

    void update(float dt);
    void takeDamage(float amount, const std::string& attackerName = "");
    void heal(float amount);
    bool consumeStamina(float amount);
    void respawn(const glm::vec3& spawnPos);

    // Getters & Setters
    int getId() const { return m_id; }
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }

    float getHealth() const { return m_health; }
    float getMaxHealth() const { return m_maxHealth; }
    void setHealth(float hp) { m_health = glm::clamp(hp, 0.0f, m_maxHealth); }

    float getStamina() const { return m_stamina; }
    float getMaxStamina() const { return m_maxStamina; }

    bool isDead() const { return m_health <= 0.001f; }
    bool isSprinting() const { return m_isSprinting; }
    void setSprinting(bool sprinting) { m_isSprinting = sprinting; }

    bool isAiming() const { return m_isAiming; }
    void setAiming(bool aiming) { m_isAiming = aiming; }

    const glm::vec3& getPosition() const { return m_position; }
    void setPosition(const glm::vec3& pos) { m_position = pos; }

    const glm::vec3& getVelocity() const { return m_velocity; }
    void setVelocity(const glm::vec3& vel) { m_velocity = vel; }

    float getYaw() const { return m_yaw; }
    float getPitch() const { return m_pitch; }
    void setRotation(float yaw, float pitch) { m_yaw = yaw; m_pitch = pitch; }

    float getDamageVignette() const { return m_damageVignette; }

    const std::string& getEquippedWeaponName() const { return m_equippedWeapon; }
    void setEquippedWeaponName(const std::string& name) { m_equippedWeapon = name; }
    bool hasWeapon() const { return !m_equippedWeapon.empty() && m_equippedWeapon != "None"; }
    int getWeaponAmmo() const { return m_weaponAmmo; }
    int getWeaponReserve() const { return m_weaponReserve; }
    void setWeaponAmmo(int a, int r) { m_weaponAmmo = a; m_weaponReserve = r; }

    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& j);

private:
    int m_id;
    std::string m_name;

    float m_health;
    float m_maxHealth;
    float m_stamina;
    float m_maxStamina;

    glm::vec3 m_position;
    glm::vec3 m_velocity;
    float m_yaw;
    float m_pitch;

    bool m_isSprinting;
    bool m_isAiming;
    float m_damageVignette;
    float m_staminaRegenDelay;

    std::string m_equippedWeapon;
    int m_weaponAmmo;
    int m_weaponReserve;
};

} // namespace Djusov
