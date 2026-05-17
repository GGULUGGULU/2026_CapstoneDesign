#pragma once

#include <vector>
#include "ServerNetworkTypes.h"

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
    bool ConsumeCollisionEvent(CollisionEventNet& outEvent);
    bool ConsumeEffectEvent(EffectEventNet& outEvent);
    bool ConsumeRaceFinish(RaceRecordNet& outEvent);
    bool ConsumeRaceResult(RaceResultNet& outEvent);

    void SendCollisionEvent(const CollisionEventNet& ev);
    void SendEffectEvent(const EffectEventNet& ev);
    void SendRaceFinish(const RaceRecordNet& ev);
    void SendRaceResult(const RaceResultNet& ev);

    bool IsConnected() const;
    bool IsHosting() const;
    bool IsEnabled() const;

    void AddServerRecord(const RaceRecordNet& record);
    void SetTotalPlayerCount(std::uint32_t count);
    bool HasAllRecords() const;
    RaceResultNet CalculateRankings();

    bool ConsumeWelomeEvent(int& outPlayerId);

    //


    bool ConsumeItemEvent(ItemEventNet& outEvent);
    void SendItemEvent(const ItemEventNet& ev);





private:
    void DisconnectPeer();
    void TryAcceptClient();
    void TryReceivePackets();
    void TrySendLocalState(const PlayerNetState& state);
    void FlushPendingSends();

    //

    std::vector<ItemEventNet> m_itemEvents;

private:
    CNetworkManagerImpl* m_pImpl = nullptr;
    MODE m_eMode = MODE::NONE;

    bool m_bConnected = false;
    bool m_bHasRemoteState = false;

    PlayerNetState m_latestRemoteState{};
    std::vector<char> m_recvBuffer;
    std::vector<CollisionEventNet> m_collisionEvents;
    std::vector<EffectEventNet> m_effectEvents;
    std::vector<RaceRecordNet> m_raceFinishEvents;
    std::vector<RaceResultNet> m_raceResultEvents;

    std::vector<RaceRecordNet> m_serverRaceRecords;
    std::uint32_t m_nTotalPlayerCount = 0;

    bool m_bHasWelcomedId = false;
    int m_nWelcomePlayerId = 0;
};
