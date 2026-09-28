#include "network/NetClient.hpp"
#include "player/Player.hpp"
#include "render/Primitives.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <glm/gtc/matrix_transform.hpp>

namespace Djusov {

glm::mat4 RemotePlayerProxy::getTransform() const {
    glm::mat4 t = glm::translate(glm::mat4(1.0f), currentPosition);
    t = glm::rotate(t, glm::radians(currentYaw), glm::vec3(0, 1, 0));
    return t;
}

NetClient::NetClient()
    : m_socketFd(-1), m_connected(false), m_assignedPlayerId(0),
      m_playerName("Player"), m_sendStateTimer(0.0f) {

    m_avatarMesh = Primitives::createCharacterAvatarMesh();
    m_avatarMaterial.name = "RemotePlayerAvatar";
    m_avatarMaterial.albedo = glm::vec3(0.2f, 0.45f, 0.85f); // Distinct blue avatar color
    m_avatarMaterial.metallic = 0.1f;
    m_avatarMaterial.roughness = 0.4f;
}

NetClient::~NetClient() {
    disconnect();
}

bool NetClient::connectToServer(const std::string& host, int port, const std::string& playerName) {
    disconnect();
    m_playerName = playerName;

    m_socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socketFd < 0) {
        std::cerr << "[NetClient] Failed to create socket!" << std::endl;
        return false;
    }

    sockaddr_in servAddr{};
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(port);
    if (inet_pton(AF_INET, host.c_str(), &servAddr.sin_addr) <= 0) {
        std::cerr << "[NetClient] Invalid IP address: " << host << std::endl;
        close(m_socketFd);
        m_socketFd = -1;
        return false;
    }

    if (connect(m_socketFd, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0) {
        std::cerr << "[NetClient] Connection to " << host << ":" << port << " failed!" << std::endl;
        close(m_socketFd);
        m_socketFd = -1;
        return false;
    }

    // Set non-blocking
    int flags = fcntl(m_socketFd, F_GETFL, 0);
    fcntl(m_socketFd, F_SETFL, flags | O_NONBLOCK);

    m_connected = true;

    // Send ConnectRequest
    PktConnectRequest req{};
    std::strncpy(req.playerName, playerName.c_str(), sizeof(req.playerName) - 1);
    sendPacket(PacketType::ConnectRequest, &req, sizeof(req));

    std::cout << "[NetClient] Connected to server " << host << ":" << port << std::endl;
    return true;
}

void NetClient::disconnect() {
    if (m_socketFd >= 0) {
        close(m_socketFd);
        m_socketFd = -1;
    }
    m_connected = false;
    m_remotePlayers.clear();
    m_rxBuffer.clear();
}

void NetClient::sendPacket(PacketType type, const void* payload, size_t payloadSize) {
    if (!m_connected || m_socketFd < 0) return;

    PacketHeader hdr;
    hdr.magic = NET_MAGIC;
    hdr.type = static_cast<uint8_t>(type);
    hdr.payloadSize = static_cast<uint32_t>(payloadSize);

    std::vector<uint8_t> buffer(sizeof(PacketHeader) + payloadSize);
    std::memcpy(buffer.data(), &hdr, sizeof(PacketHeader));
    if (payload && payloadSize > 0) {
        std::memcpy(buffer.data() + sizeof(PacketHeader), payload, payloadSize);
    }

    send(m_socketFd, buffer.data(), buffer.size(), MSG_NOSIGNAL);
}

void NetClient::sendWeaponFire(const glm::vec3& origin, const glm::vec3& dir) {
    if (!m_connected) return;
    PktWeaponFire fire{};
    fire.playerId = m_assignedPlayerId;
    fire.origX = origin.x; fire.origY = origin.y; fire.origZ = origin.z;
    fire.dirX = dir.x; fire.dirY = dir.y; fire.dirZ = dir.z;
    sendPacket(PacketType::WeaponFire, &fire, sizeof(fire));
}

void NetClient::sendChatMessage(const std::string& text) {
    if (!m_connected) return;
    PktChatMessage msg{};
    std::strncpy(msg.sender, m_playerName.c_str(), sizeof(msg.sender) - 1);
    std::strncpy(msg.text, text.c_str(), sizeof(msg.text) - 1);
    sendPacket(PacketType::ChatMessage, &msg, sizeof(msg));
}

void NetClient::receivePackets() {
    if (!m_connected || m_socketFd < 0) return;

    uint8_t tempBuf[2048];
    while (true) {
        ssize_t bytesRead = recv(m_socketFd, tempBuf, sizeof(tempBuf), 0);
        if (bytesRead > 0) {
            m_rxBuffer.insert(m_rxBuffer.end(), tempBuf, tempBuf + bytesRead);
        } else if (bytesRead == 0) {
            disconnect();
            std::cout << "[NetClient] Disconnected from server." << std::endl;
            return;
        } else {
            break;
        }
    }

    while (m_rxBuffer.size() >= sizeof(PacketHeader)) {
        PacketHeader hdr;
        std::memcpy(&hdr, m_rxBuffer.data(), sizeof(PacketHeader));

        if (hdr.magic != NET_MAGIC) {
            m_rxBuffer.erase(m_rxBuffer.begin());
            continue;
        }

        size_t totalPacketSize = sizeof(PacketHeader) + hdr.payloadSize;
        if (m_rxBuffer.size() < totalPacketSize) break;

        const uint8_t* payload = m_rxBuffer.data() + sizeof(PacketHeader);
        PacketType type = static_cast<PacketType>(hdr.type);

        if (type == PacketType::ConnectAccept) {
            if (hdr.payloadSize >= sizeof(PktConnectAccept)) {
                PktConnectAccept acc;
                std::memcpy(&acc, payload, sizeof(PktConnectAccept));
                m_assignedPlayerId = acc.playerId;
                std::cout << "[NetClient] Server accepted connection! Assigned Player ID: " << m_assignedPlayerId << std::endl;
            }
        } else if (type == PacketType::PlayerState) {
            if (hdr.payloadSize >= sizeof(PktPlayerState)) {
                PktPlayerState st;
                std::memcpy(&st, payload, sizeof(PktPlayerState));

                if (st.playerId != m_assignedPlayerId) {
                    auto& proxy = m_remotePlayers[st.playerId];
                    proxy.playerId = st.playerId;
                    proxy.targetPosition = glm::vec3(st.posX, st.posY, st.posZ);
                    proxy.targetYaw = st.yaw;
                    proxy.targetPitch = st.pitch;
                    proxy.health = st.health;
                    proxy.isSprinting = (st.flags & 1) != 0;
                    proxy.isAiming = (st.flags & 2) != 0;
                    proxy.isFiring = (st.flags & 4) != 0;

                    // If newly joined, snap position immediately
                    if (glm::length(proxy.currentPosition) < 0.001f) {
                        proxy.currentPosition = proxy.targetPosition;
                        proxy.currentYaw = proxy.targetYaw;
                    }
                }
            }
        } else if (type == PacketType::PlayerDisconnect) {
            if (hdr.payloadSize >= sizeof(PktPlayerDisconnect)) {
                PktPlayerDisconnect disc;
                std::memcpy(&disc, payload, sizeof(PktPlayerDisconnect));
                m_remotePlayers.erase(disc.playerId);
            }
        } else if (type == PacketType::ChatMessage) {
            if (hdr.payloadSize >= sizeof(PktChatMessage)) {
                PktChatMessage msg;
                std::memcpy(&msg, payload, sizeof(PktChatMessage));
                msg.sender[sizeof(msg.sender) - 1] = '\0';
                msg.text[sizeof(msg.text) - 1] = '\0';
                m_chatLog.push_back(msg);
                if (m_chatLog.size() > 50) {
                    m_chatLog.erase(m_chatLog.begin());
                }
            }
        }

        m_rxBuffer.erase(m_rxBuffer.begin(), m_rxBuffer.begin() + totalPacketSize);
    }
}

void NetClient::update(float dt, const Player& localPlayer) {
    if (!m_connected) return;

    receivePackets();

    // Smoothly interpolate remote players
    for (auto& [id, proxy] : m_remotePlayers) {
        proxy.currentPosition = glm::mix(proxy.currentPosition, proxy.targetPosition, dt * 15.0f);
        proxy.currentYaw = glm::mix(proxy.currentYaw, proxy.targetYaw, dt * 15.0f);
        proxy.currentPitch = glm::mix(proxy.currentPitch, proxy.targetPitch, dt * 15.0f);
    }

    // Send local player state at 30 Hz
    m_sendStateTimer += dt;
    if (m_sendStateTimer >= (1.0f / 30.0f)) {
        m_sendStateTimer = 0.0f;

        PktPlayerState st{};
        st.playerId = m_assignedPlayerId;
        st.posX = localPlayer.getPosition().x;
        st.posY = localPlayer.getPosition().y;
        st.posZ = localPlayer.getPosition().z;
        st.velX = localPlayer.getVelocity().x;
        st.velY = localPlayer.getVelocity().y;
        st.velZ = localPlayer.getVelocity().z;
        st.yaw = localPlayer.getYaw();
        st.pitch = localPlayer.getPitch();
        st.health = localPlayer.getHealth();
        st.flags = 0;
        if (localPlayer.isSprinting()) st.flags |= 1;
        if (localPlayer.isAiming()) st.flags |= 2;

        sendPacket(PacketType::PlayerState, &st, sizeof(st));
    }
}

std::vector<RenderObject> NetClient::getRemotePlayerRenderObjects() const {
    std::vector<RenderObject> list;
    list.reserve(m_remotePlayers.size());

    for (const auto& [id, proxy] : m_remotePlayers) {
        RenderObject obj;
        obj.mesh = m_avatarMesh;
        obj.transform = proxy.getTransform();
        obj.material = m_avatarMaterial;
        list.push_back(obj);
    }
    return list;
}

} // namespace Djusov
