#pragma once

#include <vector>
#include "NetworkTypes.h"

class CNetworkManagerImpl;

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

    bool IsConnected() const;
    bool IsHosting() const;
    bool IsEnabled() const;

private:
    void DisconnectPeer();
    void TryAcceptClient();
    void TryReceivePackets();
    void TrySendLocalState(const PlayerNetState& state);

private:
    CNetworkManagerImpl* m_pImpl = nullptr;
    MODE m_eMode = MODE::NONE;

    bool m_bConnected = false;
    bool m_bHasRemoteState = false;

    PlayerNetState m_latestRemoteState{};
    std::vector<char> m_recvBuffer;
};
