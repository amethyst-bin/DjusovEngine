#pragma once

#include "player/Player.hpp"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace Djusov {

class FPSController {
public:
    FPSController(Player* player);

    void handleInput(GLFWwindow* window, float dt);
    void handleMouseMovement(float xoffset, float yoffset);

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspect) const;
    glm::mat4 getViewModelProjectionMatrix(float aspect) const;

    glm::vec3 getCameraPosition() const;
    glm::vec3 getCameraForward() const;
    glm::vec3 getCameraRight() const;
    glm::vec3 getCameraUp() const;

    float getFOV() const { return m_currentFov; }
    void setBaseFOV(float fov) { m_baseFov = fov; }
    void setADSFOV(float fov) { m_adsFov = fov; }
    void setSensitivity(float sens) { m_mouseSensitivity = sens; }
    float getSensitivity() const { return m_mouseSensitivity; }

    float getADSProgress() const { return m_adsProgress; }
    bool isGrounded() const { return m_isGrounded; }

    void addCameraRecoil(float pitchOffset, float yawOffset);

private:
    Player* m_player;

    float m_baseFov;
    float m_adsFov;
    float m_currentFov;
    float m_adsProgress; // 0.0 to 1.0

    float m_mouseSensitivity;
    float m_eyeHeight;
    bool m_isGrounded;

    float m_recoilPitch;
    float m_recoilYaw;
};

} // namespace Djusov
