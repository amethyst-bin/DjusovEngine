#include "player/Player.hpp"
#include <algorithm>

namespace Djusov {

Player::Player(int id, const std::string& name)
    : m_id(id), m_name(name),
      m_health(100.0f), m_maxHealth(100.0f),
      m_stamina(100.0f), m_maxStamina(100.0f),
      m_position(0.0f, 1.0f, 0.0f), m_velocity(0.0f),
      m_yaw(-90.0f), m_pitch(0.0f),
      m_isSprinting(false), m_isAiming(false),
      m_damageVignette(0.0f), m_staminaRegenDelay(0.0f) {}

void Player::update(float dt) {
    // Smooth damage vignette fade
    if (m_damageVignette > 0.0f) {
        m_damageVignette -= dt * 1.5f;
        if (m_damageVignette < 0.0f) m_damageVignette = 0.0f;
    }

    // Stamina regeneration
    if (m_staminaRegenDelay > 0.0f) {
        m_staminaRegenDelay -= dt;
    } else {
        if (!m_isSprinting && m_stamina < m_maxStamina) {
            m_stamina += dt * 25.0f; // regens in 4 seconds
            if (m_stamina > m_maxStamina) m_stamina = m_maxStamina;
        }
    }
}

void Player::takeDamage(float amount, const std::string& attackerName) {
    if (isDead()) return;

    m_health -= amount;
    m_damageVignette = 1.0f;
    if (m_health < 0.0f) {
        m_health = 0.0f;
    }
}

void Player::heal(float amount) {
    m_health += amount;
    if (m_health > m_maxHealth) {
        m_health = m_maxHealth;
    }
}

bool Player::consumeStamina(float amount) {
    if (m_stamina >= amount) {
        m_stamina -= amount;
        m_staminaRegenDelay = 0.8f;
        return true;
    }
    return false;
}

void Player::respawn(const glm::vec3& spawnPos) {
    m_health = m_maxHealth;
    m_stamina = m_maxStamina;
    m_position = spawnPos;
    m_velocity = glm::vec3(0.0f);
    m_damageVignette = 0.0f;
}

nlohmann::json Player::toJson() const {
    nlohmann::json j;
    j["id"] = m_id;
    j["name"] = m_name;
    j["health"] = m_health;
    j["maxHealth"] = m_maxHealth;
    j["stamina"] = m_stamina;
    j["maxStamina"] = m_maxStamina;
    j["position"] = { m_position.x, m_position.y, m_position.z };
    j["yaw"] = m_yaw;
    j["pitch"] = m_pitch;
    return j;
}

void Player::fromJson(const nlohmann::json& j) {
    if (j.contains("name")) m_name = j["name"];
    if (j.contains("health")) m_health = j["health"];
    if (j.contains("maxHealth")) m_maxHealth = j["maxHealth"];
    if (j.contains("stamina")) m_stamina = j["stamina"];
    if (j.contains("maxStamina")) m_maxStamina = j["maxStamina"];
    if (j.contains("position")) {
        m_position = glm::vec3(j["position"][0], j["position"][1], j["position"][2]);
    }
    if (j.contains("yaw")) m_yaw = j["yaw"];
    if (j.contains("pitch")) m_pitch = j["pitch"];
}

} // namespace Djusov
