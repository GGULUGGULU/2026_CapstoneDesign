#pragma once

#include <vector>
#include "NetworkTypes.h"

class CNetworkManagerImpl;

struct CollisionEventNet
{
    int type = 0;
    int objectIndex = -1;
    float x = 0.0f, y = 0.0f, z = 0.0f;
    float nx = 0.0f, ny = 0.0f, nz = 0.0f;
    float reboundPower = 0.0f;
};

struct EffectEventNet
{
    int effectType = 0;
    float x = 0.0f, y = 0.0f, z = 0.0f;
    float sx = 0.0f, sy = 0.0f;
    float r = 1.0f, g = 1.0f, b = 1.0f;
};

class CNetworkManager
{
public:
    enum class MODE
    {
        NONE = 0,
        HOST,
        CLIENT
    };

public:
    CNetworkManager();
    ~CNetworkManager();

    CNetworkManager(const CNetworkManager&) = delete;
    CNetworkManager& operator=(const CNetworkManager&) = delete;

    bool StartHost(unsigned short port = NET_DEFAULT_PORT);
    bool ConnectToHost(const char* pszAddress, unsigned short port = NET_DEFAULT_PORT);

    void Shutdown();
    void Update(float fTimeElapsed, const PlayerNetState* pLocalState);

    bool ConsumeRemoteState(PlayerNetState& outState);
    bool ConsumeCollisionEvent(CollisionEventNet& outEvent);
    bool ConsumeEffectEvent(EffectEventNet& outEvent);

    void SendCollisionEvent(const CollisionEventNet& ev);
    void SendEffectEvent(const EffectEventNet& ev);

    bool IsConnected() const;
    bool IsHosting() const;
    bool IsEnabled() const;

private:
    void DisconnectPeer();
    void TryAcceptClient();
    void TryReceivePackets();
    void TrySendLocalState(const PlayerNetState& state);
    void FlushPendingSends();

private:
    CNetworkManagerImpl* m_pImpl = nullptr;
    MODE m_eMode = MODE::NONE;

    bool m_bConnected = false;
    bool m_bHasRemoteState = false;

    PlayerNetState m_latestRemoteState{};
    std::vector<char> m_recvBuffer;
    std::vector<CollisionEventNet> m_collisionEvents;
    std::vector<EffectEventNet> m_effectEvents;
};
