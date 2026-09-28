#pragma once

#include <cstdint>
#include <string>
#include <cstring>
#include <vector>
#include <glm/glm.hpp>

namespace Djusov {

constexpr uint16_t NET_MAGIC = 0xDE01;
constexpr int NET_DEFAULT_PORT = 7777;

enum class PacketType : uint8_t {
    ConnectRequest = 1,
    ConnectAccept = 2,
    PlayerState = 3,
    PlayerDisconnect = 4,
    WeaponFire = 5,
    PlayerHit = 6,
    ChatMessage = 7,
    Ping = 8,
    Pong = 9
};

#pragma pack(push, 1)

struct PacketHeader {
    uint16_t magic = NET_MAGIC;
    uint8_t type = 0;
    uint32_t payloadSize = 0;
};

struct PktConnectRequest {
    char playerName[32];
};

struct PktConnectAccept {
    uint32_t playerId;
    float spawnX, spawnY, spawnZ;
};

struct PktPlayerState {
    uint32_t playerId;
    float posX, posY, posZ;
    float velX, velY, velZ;
    float yaw, pitch;
    float health;
    uint8_t flags; // bit 0: isSprinting, bit 1: isAiming, bit 2: isFiring
};

struct PktPlayerDisconnect {
    uint32_t playerId;
};

struct PktWeaponFire {
    uint32_t playerId;
    float origX, origY, origZ;
    float dirX, dirY, dirZ;
};

struct PktPlayerHit {
    uint32_t targetPlayerId;
    uint32_t attackerPlayerId;
    float damage;
    uint8_t isHeadshot;
    uint8_t isDead;
};

struct PktChatMessage {
    char sender[32];
    char text[128];
};

#pragma pack(pop)

} // namespace Djusov
