#pragma once

#include "network/NetPacket.hpp"
#include "render/Mesh.hpp"
#include "render/Material.hpp"
#include "render/PBRRenderer.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace Djusov {

class Player;

struct RemotePlayerProxy {
    uint32_t playerId = 0;
    std::string name;
    glm::vec3 currentPosition = glm::vec3(0.0f);
    glm::vec3 targetPosition = glm::vec3(0.0f);
    float currentYaw = 0.0f;
    float targetYaw = 0.0f;
    float currentPitch = 0.0f;
    float targetPitch = 0.0f;
    float health = 100.0f;
    bool isSprinting = false;
    bool isAiming = false;
    bool isFiring = false;

    glm::mat4 getTransform() const;
};

class NetClient {
public:
    NetClient();
    ~NetClient();

    bool connectToServer(const std::string& host = "127.0.0.1", int port = NET_DEFAULT_PORT, const std::string& playerName = "Player");
    void disconnect();
    void update(float dt, const Player& localPlayer);

    bool isConnected() const { return m_connected; }
    uint32_t getAssignedPlayerId() const { return m_assignedPlayerId; }

    void sendWeaponFire(const glm::vec3& origin, const glm::vec3& dir);
    void sendChatMessage(const std::string& text);

    const std::unordered_map<uint32_t, RemotePlayerProxy>& getRemotePlayers() const { return m_remotePlayers; }
    const std::vector<PktChatMessage>& getChatLog() const { return m_chatLog; }

    std::vector<RenderObject> getRemotePlayerRenderObjects() const;

private:
    void receivePackets();
    void sendPacket(PacketType type, const void* payload, size_t payloadSize);

    int m_socketFd;
    bool m_connected;
    uint32_t m_assignedPlayerId;
    std::string m_playerName;

    float m_sendStateTimer;
    std::vector<uint8_t> m_rxBuffer;

    std::unordered_map<uint32_t, RemotePlayerProxy> m_remotePlayers;
    std::vector<PktChatMessage> m_chatLog;

    std::shared_ptr<Mesh> m_avatarMesh;
    Material m_avatarMaterial;
};

} // namespace Djusov
