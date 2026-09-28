#pragma once

#include "network/NetPacket.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <thread>
#include <atomic>

namespace Djusov {

struct RemoteClient {
    int socketFd = -1;
    uint32_t playerId = 0;
    std::string name;
    PktPlayerState lastState{};
    float timeoutTimer = 0.0f;
    std::vector<uint8_t> rxBuffer;
};

class NetServer {
public:
    NetServer();
    ~NetServer();

    bool start(int port = NET_DEFAULT_PORT);
    void stop();
    void update(float dt);

    bool isRunning() const { return m_running; }
    size_t getConnectedCount() const { return m_clients.size(); }
    int getPort() const { return m_port; }

    void broadcast(PacketType type, const void* payload, size_t payloadSize, int excludeFd = -1);
    void sendPacket(int socketFd, PacketType type, const void* payload, size_t payloadSize);

private:
    void acceptNewClients();
    void processClientMessages(RemoteClient& client);

    int m_serverFd;
    int m_port;
    std::atomic<bool> m_running;
    uint32_t m_nextPlayerId;

    std::vector<RemoteClient> m_clients;
};

} // namespace Djusov
