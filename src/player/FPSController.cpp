#include "player/FPSController.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace Djusov {

FPSController::FPSController(Player* player)
    : m_player(player),
      m_baseFov(75.0f), m_adsFov(45.0f), m_currentFov(75.0f), m_adsProgress(0.0f),
      m_mouseSensitivity(0.12f), m_eyeHeight(1.68f), m_isGrounded(true),
      m_recoilPitch(0.0f), m_recoilYaw(0.0f) {}

void FPSController::handleMouseMovement(float xoffset, float yoffset) {
    if (!m_player || m_player->isDead()) return;

    // Lower sensitivity during ADS for precision aiming
    float sens = m_mouseSensitivity * glm::mix(1.0f, 0.55f, m_adsProgress);

    float yaw = m_player->getYaw() + xoffset * sens;
    float pitch = m_player->getPitch() + yoffset * sens;

    // Constrain pitch to avoid screen flipping
    pitch = std::clamp(pitch, -88.0f, 88.0f);

    m_player->setRotation(yaw, pitch);
}

void FPSController::addCameraRecoil(float pitchOffset, float yawOffset) {
    m_recoilPitch += pitchOffset;
    m_recoilYaw += yawOffset;
}

void FPSController::handleInput(GLFWwindow* window, float dt) {
    if (!m_player || m_player->isDead()) return;

    // Smoothly decay camera recoil
    m_recoilPitch = glm::mix(m_recoilPitch, 0.0f, dt * 15.0f);
    m_recoilYaw = glm::mix(m_recoilYaw, 0.0f, dt * 15.0f);

    // Aiming (Right Mouse Button)
    bool isAiming = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
    m_player->setAiming(isAiming);

    float targetAds = isAiming ? 1.0f : 0.0f;
    m_adsProgress = glm::mix(m_adsProgress, targetAds, dt * 12.0f);
    m_currentFov = glm::mix(m_baseFov, m_adsFov, m_adsProgress);

    // Sprinting (Left Shift)
    bool wantsSprint = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) && !isAiming;
    bool hasStamina = m_player->getStamina() > 5.0f;
    bool isSprinting = wantsSprint && hasStamina;
    m_player->setSprinting(isSprinting);

    if (isSprinting) {
        m_player->consumeStamina(20.0f * dt);
    }

    // Direction vectors
    float yawRad = glm::radians(m_player->getYaw());
    glm::vec3 forward(std::cos(yawRad), 0.0f, std::sin(yawRad));
    forward = glm::normalize(forward);
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));

    // Movement inputs
    glm::vec3 moveDir(0.0f);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) moveDir += forward;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) moveDir -= forward;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) moveDir += right;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) moveDir -= right;

    float speed = isSprinting ? 8.2f : (isAiming ? 3.0f : 4.6f);

    glm::vec3 currentVel = m_player->getVelocity();

    if (glm::length(moveDir) > 0.001f) {
        moveDir = glm::normalize(moveDir);
        currentVel.x = glm::mix(currentVel.x, moveDir.x * speed, dt * 12.0f);
        currentVel.z = glm::mix(currentVel.z, moveDir.z * speed, dt * 12.0f);
    } else {
        currentVel.x = glm::mix(currentVel.x, 0.0f, dt * 14.0f);
        currentVel.z = glm::mix(currentVel.z, 0.0f, dt * 14.0f);
    }

    // Jumping
    if (m_isGrounded && (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)) {
        currentVel.y = 5.8f;
        m_isGrounded = false;
        m_player->consumeStamina(10.0f);
    }

    // Gravity
    currentVel.y -= 18.0f * dt;

    // Apply movement
    glm::vec3 pos = m_player->getPosition() + currentVel * dt;

    // Ground collision (flat base floor at y = 0)
    if (pos.y <= 0.0f) {
        pos.y = 0.0f;
        currentVel.y = 0.0f;
        m_isGrounded = true;
    }

    m_player->setPosition(pos);
    m_player->setVelocity(currentVel);
    m_player->update(dt);
}

glm::vec3 FPSController::getCameraPosition() const {
    if (!m_player) return glm::vec3(0, 1.7f, 0);
    return m_player->getPosition() + glm::vec3(0, m_eyeHeight, 0);
}

glm::vec3 FPSController::getCameraForward() const {
    if (!m_player) return glm::vec3(0, 0, -1);
    float yawRad = glm::radians(m_player->getYaw() + m_recoilYaw);
    float pitchRad = glm::radians(m_player->getPitch() + m_recoilPitch);

    glm::vec3 forward;
    forward.x = std::cos(yawRad) * std::cos(pitchRad);
    forward.y = std::sin(pitchRad);
    forward.z = std::sin(yawRad) * std::cos(pitchRad);
    return glm::normalize(forward);
}

glm::vec3 FPSController::getCameraRight() const {
    return glm::normalize(glm::cross(getCameraForward(), glm::vec3(0, 1, 0)));
}

glm::vec3 FPSController::getCameraUp() const {
    return glm::normalize(glm::cross(getCameraRight(), getCameraForward()));
}

glm::mat4 FPSController::getViewMatrix() const {
    glm::vec3 eye = getCameraPosition();
    return glm::lookAt(eye, eye + getCameraForward(), glm::vec3(0, 1, 0));
}

glm::mat4 FPSController::getProjectionMatrix(float aspect) const {
    return glm::perspective(glm::radians(m_currentFov), aspect, 0.05f, 500.0f);
}

glm::mat4 FPSController::getViewModelProjectionMatrix(float aspect) const {
    // Dedicated viewmodel FOV (e.g. 60 deg) so arms never distort or clip into walls
    float vmFov = glm::mix(60.0f, 42.0f, m_adsProgress);
    return glm::perspective(glm::radians(vmFov), aspect, 0.01f, 10.0f);
}

} // namespace Djusov
