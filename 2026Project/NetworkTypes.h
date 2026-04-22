#pragma once

#include <cstddef>
#include <cstdint>

constexpr unsigned short NET_DEFAULT_PORT = 7777;
constexpr std::uint32_t NET_MAGIC = 0x52414345; // "RACE"

#pragma pack(push, 1)
struct PlayerNetState
{
    std::uint32_t playerId = 0;

    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    float yaw = 0.0f;
    float speed = 0.0f;

    std::uint32_t stage = 0;
    std::uint32_t score = 0;
};

struct PlayerNetPacket
{
    std::uint32_t magic = NET_MAGIC;
    std::uint32_t version = 1;
    PlayerNetState state{};
};

struct RaceRecordNet
{
    std::uint32_t playerId;
    float finishTime;
};

struct RaceResultNet
{
    std::uint32_t firstId;
    float firstPlaceTime;
    std::uint32_t secondId;
    float secondPlaceTime;
};

#pragma pack(pop)

constexpr std::size_t NET_PACKET_SIZE = sizeof(PlayerNetPacket);

static_assert(sizeof(PlayerNetPacket) == 40, "Unexpected packet size.");
