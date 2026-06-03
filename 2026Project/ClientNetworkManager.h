#pragma once

#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>

#pragma comment(lib, "ws2_32.lib")

#include <algorithm>
#include <cstring>
#include <vector>

#include "ClientNetworkTypes.h"

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
    bool ConsumeMapItemEvent(MapItemEventNet& outEvent);
    bool ConsumeRoomSyncEvent(RoomSyncEventNet& outEvent);

    void SendCollisionEvent(const CollisionEventNet& ev);
    void SendEffectEvent(const EffectEventNet& ev);
    void SendRaceFinish(const RaceRecordNet& ev);
    void SendRaceResult(const RaceResultNet& ev);
    void SendMapItemEvent(const MapItemEventNet& ev);
    void SendRoomSyncEvent(const RoomSyncEventNet& ev);

    bool IsConnected() const;
    bool IsHosting() const;
    bool IsEnabled() const;

    void AddServerRecord(const RaceRecordNet& record);
    void SetTotalPlayerCount(std::uint32_t count);
    bool HasAllRecords() const;
    RaceResultNet CalculateRankings();

	bool ConsumeWelomeEvent(int& outPlayerId);

    std::uint32_t GetCurrentPlayerCount() const;

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
    std::vector<MapItemEventNet> m_mapItemEvents;
private:
    CNetworkManagerImpl* m_pImpl = nullptr;
    MODE m_eMode = MODE::NONE;

    bool m_bConnected = false;
    bool m_bHasRemoteState = false;

    std::vector<PlayerNetState> m_RemoteState;
    std::vector<char> m_recvBuffer;
    std::vector<CollisionEventNet> m_collisionEvents;
    std::vector<EffectEventNet> m_effectEvents;
    std::vector<RaceRecordNet> m_raceFinishEvents;
    std::vector<RaceResultNet> m_raceResultEvents;
    std::vector<RaceRecordNet> m_serverRaceRecords;
	std::uint32_t m_nTotalPlayerCount = 0;
    std::vector<RoomSyncEventNet> m_roomSyncEvents;

	bool m_bHasWelcomeId = false;
	int m_nWelcomePlayerId = 0;
};
