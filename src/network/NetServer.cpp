#include "network/NetServer.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <algorithm>

namespace Djusov {

NetServer::NetServer()
    : m_serverFd(-1), m_port(NET_DEFAULT_PORT), m_running(false), m_nextPlayerId(100) {}

NetServer::~NetServer() {
    stop();
}

bool NetServer::start(int port) {
    m_port = port;

    m_serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_serverFd < 0) {
        std::cerr << "[NetServer] Failed to create socket!" << std::endl;
        return false;
    }

    int opt = 1;
    setsockopt(m_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Make socket non-blocking
    int flags = fcntl(m_serverFd, F_GETFL, 0);
    fcntl(m_serverFd, F_SETFL, flags | O_NONBLOCK);

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(m_port);

    if (bind(m_serverFd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "[NetServer] Bind failed on port " << m_port << std::endl;
        close(m_serverFd);
        m_serverFd = -1;
        return false;
    }

    if (listen(m_serverFd, 16) < 0) {
        std::cerr << "[NetServer] Listen failed!" << std::endl;
        close(m_serverFd);
        m_serverFd = -1;
        return false;
    }

    m_running = true;
    std::cout << "[NetServer] Started listening on port " << m_port << std::endl;
    return true;
}

void NetServer::stop() {
    if (!m_running) return;
    m_running = false;

    for (auto& client : m_clients) {
        if (client.socketFd >= 0) {
            close(client.socketFd);
        }
    }
    m_clients.clear();

    if (m_serverFd >= 0) {
        close(m_serverFd);
        m_serverFd = -1;
    }

    std::cout << "[NetServer] Server stopped." << std::endl;
}

void NetServer::acceptNewClients() {
    sockaddr_in clientAddr{};
    socklen_t clientLen = sizeof(clientAddr);

    while (true) {
        int clientFd = accept(m_serverFd, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientFd < 0) {
            break; // No more incoming connections
        }

        // Set client socket to non-blocking
        int flags = fcntl(clientFd, F_GETFL, 0);
        fcntl(clientFd, F_SETFL, flags | O_NONBLOCK);

        RemoteClient client;
        client.socketFd = clientFd;
        client.playerId = m_nextPlayerId++;
        client.name = "Guest_" + std::to_string(client.playerId);

        m_clients.push_back(client);
        std::cout << "[NetServer] Accepted connection from " 
                  << inet_ntoa(clientAddr.sin_addr) 
                  << " (Assigned ID " << client.playerId << ")" << std::endl;
    }
}

void NetServer::sendPacket(int socketFd, PacketType type, const void* payload, size_t payloadSize) {
    PacketHeader hdr;
    hdr.magic = NET_MAGIC;
    hdr.type = static_cast<uint8_t>(type);
    hdr.payloadSize = static_cast<uint32_t>(payloadSize);

    std::vector<uint8_t> buffer(sizeof(PacketHeader) + payloadSize);
    std::memcpy(buffer.data(), &hdr, sizeof(PacketHeader));
    if (payload && payloadSize > 0) {
        std::memcpy(buffer.data() + sizeof(PacketHeader), payload, payloadSize);
    }

    send(socketFd, buffer.data(), buffer.size(), MSG_NOSIGNAL);
}

void NetServer::broadcast(PacketType type, const void* payload, size_t payloadSize, int excludeFd) {
    for (const auto& client : m_clients) {
        if (client.socketFd != excludeFd && client.socketFd >= 0) {
            sendPacket(client.socketFd, type, payload, payloadSize);
        }
    }
}

void NetServer::processClientMessages(RemoteClient& client) {
    uint8_t tempBuf[2048];
    while (true) {
        ssize_t bytesRead = recv(client.socketFd, tempBuf, sizeof(tempBuf), 0);
        if (bytesRead > 0) {
            client.rxBuffer.insert(client.rxBuffer.end(), tempBuf, tempBuf + bytesRead);
            client.timeoutTimer = 0.0f;
        } else if (bytesRead == 0) {
            // Client closed connection cleanly
            client.socketFd = -1;
            break;
        } else {
            // EAGAIN or EWOULDBLOCK
            break;
        }
    }

    // Parse packets from client buffer
    while (client.rxBuffer.size() >= sizeof(PacketHeader)) {
        PacketHeader hdr;
        std::memcpy(&hdr, client.rxBuffer.data(), sizeof(PacketHeader));

        if (hdr.magic != NET_MAGIC) {
            // Corrupted stream, discard byte
            client.rxBuffer.erase(client.rxBuffer.begin());
            continue;
        }

        size_t totalPacketSize = sizeof(PacketHeader) + hdr.payloadSize;
        if (client.rxBuffer.size() < totalPacketSize) {
            break; // Incomplete packet, wait for more data
        }

        const uint8_t* payload = client.rxBuffer.data() + sizeof(PacketHeader);
        PacketType type = static_cast<PacketType>(hdr.type);

        if (type == PacketType::ConnectRequest) {
            if (hdr.payloadSize >= sizeof(PktConnectRequest)) {
                PktConnectRequest req;
                std::memcpy(&req, payload, sizeof(PktConnectRequest));
                req.playerName[sizeof(req.playerName) - 1] = '\0';
                client.name = req.playerName;

                // Send accept packet
                PktConnectAccept acceptPkt;
                acceptPkt.playerId = client.playerId;
                acceptPkt.spawnX = 0.0f;
                acceptPkt.spawnY = 1.0f;
                acceptPkt.spawnZ = 0.0f;
                sendPacket(client.socketFd, PacketType::ConnectAccept, &acceptPkt, sizeof(acceptPkt));

                std::cout << "[NetServer] Player '" << client.name << "' (ID " << client.playerId << ") joined." << std::endl;
            }
        } else if (type == PacketType::PlayerState) {
            if (hdr.payloadSize >= sizeof(PktPlayerState)) {
                PktPlayerState st;
                std::memcpy(&st, payload, sizeof(PktPlayerState));
                st.playerId = client.playerId; // enforce true player ID
                client.lastState = st;

                // Relay state to all other players
                broadcast(PacketType::PlayerState, &st, sizeof(st), client.socketFd);
            }
        } else if (type == PacketType::WeaponFire) {
            if (hdr.payloadSize >= sizeof(PktWeaponFire)) {
                PktWeaponFire fire;
                std::memcpy(&fire, payload, sizeof(PktWeaponFire));
                fire.playerId = client.playerId;

                // Relay weapon fire event to all other players
                broadcast(PacketType::WeaponFire, &fire, sizeof(fire), client.socketFd);
            }
        } else if (type == PacketType::ChatMessage) {
            if (hdr.payloadSize >= sizeof(PktChatMessage)) {
                PktChatMessage msg;
                std::memcpy(&msg, payload, sizeof(PktChatMessage));
                msg.sender[sizeof(msg.sender) - 1] = '\0';
                msg.text[sizeof(msg.text) - 1] = '\0';

                // Broadcast chat message to everyone
                broadcast(PacketType::ChatMessage, &msg, sizeof(msg), -1);
            }
        }

        // Remove processed packet from buffer
        client.rxBuffer.erase(client.rxBuffer.begin(), client.rxBuffer.begin() + totalPacketSize);
    }
}

void NetServer::update(float dt) {
    if (!m_running) return;

    acceptNewClients();

    for (auto it = m_clients.begin(); it != m_clients.end();) {
        if (it->socketFd < 0) {
            // Notify disconnection
            PktPlayerDisconnect disc;
            disc.playerId = it->playerId;
            broadcast(PacketType::PlayerDisconnect, &disc, sizeof(disc), -1);

            std::cout << "[NetServer] Player '" << it->name << "' disconnected." << std::endl;
            it = m_clients.erase(it);
            continue;
        }

        it->timeoutTimer += dt;
        if (it->timeoutTimer > 15.0f) { // 15 sec timeout
            close(it->socketFd);
            it->socketFd = -1;
        } else {
            processClientMessages(*it);
        }
        ++it;
    }
}

} // namespace Djusov
