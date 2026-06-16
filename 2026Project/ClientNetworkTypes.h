#pragma once

#include <cstddef>
#include <cstdint>

constexpr unsigned short NET_DEFAULT_PORT = 7777;
constexpr std::uint32_t NET_MAGIC = 0x52414345; // "RACE"

enum class NET_MESSAGE_TYPE
{
    WELCOME_ASSIGN_ID = 0,
    PLAYER_STATE = 1,
    COLLISION_EVENT = 2,
    EFFECT_EVENT = 3,
    RACE_FINISH = 4,
    RACE_RESULT = 5,
    ITEM_EVENT = 6,
    PLAYER_COUNT = 7,
    MAP_ITEM_EVENT = 8,
    ROOM_SYNC_EVENT = 9,
    LOAD_COMPLETE = 10,
    GAME_START_SIGN = 11
};

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

    std::uint32_t currentLap = 0;
    std::uint32_t passedCheckpoints = 0;

    float distToNextCP = 0.0f;
};

struct CollisionEventNet
{
    std::uint32_t playerId;
    int type = 0;
    int objectIndex = -1;
    float x = 0.0f, y = 0.0f, z = 0.0f;
    float nx = 0.0f, ny = 0.0f, nz = 0.0f;
    float reboundPower = 0.0f;
};

struct EffectEventNet
{
    int effectType = 0;
    int action = 0;
    float x = 0.0f, y = 0.0f, z = 0.0f;
    float lx = 0.0f, ly = 0.0f, lz = 0.0f;
    float sx = 0.0f, sy = 0.0f;
    float r = 1.0f, g = 1.0f, b = 1.0f;
};

struct ItemEventNet
{
    int itemType = 0;
    float duration = 0.0f;
};

struct RaceRecordNet
{
    std::uint32_t playerId = 0;
    float finishTime = 0.0f;
    wchar_t playerName[16]{};
};

struct RaceResultNet
{
    std::uint32_t playerCount;
    RaceRecordNet playerRecords[4];
};

struct MapItemEventNet
{
    std::uint32_t playerId;
    int itemIndex;
    bool IsActive;
};

struct RoomSyncEventNet
{
    std::uint32_t playerId = 0;
    int selectedCarIndex = 0;
    int selectedMapIndex = 0;
    bool isReady = false;
    wchar_t playerName[16]{};
};

struct LoadCompleteNet
{
    std::uint32_t playerId;
};

struct GameStartSignNet
{
    bool startSign;
};

/////
struct NetMessageHeader
{
    unsigned magic = NET_MAGIC;
    unsigned version = 1;
    unsigned type = 0;
    unsigned size = 0;
};

struct WelcomePacket
{
    NetMessageHeader header{};
    int assignedPlayerId{};
};

struct PlayerStatePacket
{
    NetMessageHeader header{};
    PlayerNetState state{};
};

struct CollisionEventPacket
{
    NetMessageHeader header{};
    CollisionEventNet eventData{};
};

struct EffectEventPacket
{
    NetMessageHeader header{};
    EffectEventNet eventData{};
};

struct RaceFinishPacket
{
    NetMessageHeader header{};
    RaceRecordNet record{};
};

struct RaceResultPacket
{
    NetMessageHeader header{};
    RaceResultNet result{};
};

struct ItemEventPacket
{
    NetMessageHeader header{};
    ItemEventNet eventData{};
};

struct PlayerCountPacket
{
    NetMessageHeader header{};
    std::uint32_t currentCount = 0;
};

struct MapItemEventPacket
{
    NetMessageHeader header{};
    MapItemEventNet eventData{};
};

struct RoomSyncEventPacket
{
    NetMessageHeader header{};
    RoomSyncEventNet eventData{};
};

struct LoadCompletePacket
{
    NetMessageHeader header{};
    LoadCompleteNet eventData{};
};

struct GameStartSignPacket
{
    NetMessageHeader header{};
    GameStartSignNet eventData{};
};

#pragma pack(pop)