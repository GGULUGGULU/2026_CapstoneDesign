#include "stdafx.h"

#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>

#pragma comment(lib, "ws2_32.lib")

#include <algorithm>
#include <cstring>
#include <vector>

#undef min
#undef max

#include "ClientNetworkManager.h"

struct CNetworkManagerImpl
{
    SOCKET listenSocket = INVALID_SOCKET;
    SOCKET peerSocket = INVALID_SOCKET;
    bool wsaStarted = false;
    std::vector<char> pendingSendBuffer;
};

namespace
{
    bool EnsureWinsockStarted(CNetworkManagerImpl* pImpl)
    {
        if (!pImpl) return false;
        if (pImpl->wsaStarted) return true;

        WSADATA wsaData{};
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        {
            OutputDebugStringA("[Network] WSAStartup failed.\n");
            return false;
        }

        pImpl->wsaStarted = true;
        return true;
    }

    void CloseSocketSafe(SOCKET& socketHandle)
    {
        if (socketHandle != INVALID_SOCKET)
        {
            closesocket(socketHandle);
            socketHandle = INVALID_SOCKET;
        }
    }

    bool SetNonBlocking(SOCKET socketHandle)
    {
        u_long nonBlocking = 1;
        return (ioctlsocket(socketHandle, FIONBIO, &nonBlocking) == 0);
    }

    void EnableNoDelay(SOCKET socketHandle)
    {
        BOOL flag = TRUE;
        setsockopt(socketHandle, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&flag), sizeof(flag));
    }
}

CNetworkManager::CNetworkManager()
{
    m_pImpl = new CNetworkManagerImpl();
}

CNetworkManager::~CNetworkManager()
{
    Shutdown();

    delete m_pImpl;
    m_pImpl = nullptr;
}

bool CNetworkManager::StartHost(unsigned short port)
{
    Shutdown();

    if (!EnsureWinsockStarted(m_pImpl)) return false;

    m_pImpl->listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_pImpl->listenSocket == INVALID_SOCKET)
    {
        OutputDebugStringA("[Network] listen socket creation failed.\n");
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    BOOL reuseAddr = TRUE;
    setsockopt(m_pImpl->listenSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuseAddr), sizeof(reuseAddr));

    if (bind(m_pImpl->listenSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR)
    {
        OutputDebugStringA("[Network] bind failed.\n");
        CloseSocketSafe(m_pImpl->listenSocket);
        return false;
    }

    if (listen(m_pImpl->listenSocket, 1) == SOCKET_ERROR)
    {
        OutputDebugStringA("[Network] listen failed.\n");
        CloseSocketSafe(m_pImpl->listenSocket);
        return false;
    }

    SetNonBlocking(m_pImpl->listenSocket);

    m_eMode = MODE::HOST;
    m_bConnected = false;
    m_bHasRemoteState = false;
    m_recvBuffer.clear();
    m_collisionEvents.clear();
    m_effectEvents.clear();
    m_raceFinishEvents.clear();
    m_raceResultEvents.clear();
    m_pImpl->pendingSendBuffer.clear();

    OutputDebugStringA("[Network] Host started. Waiting for client...\n");
    return true;
}

bool CNetworkManager::ConnectToHost(const char* pszAddress, unsigned short port)
{
    Shutdown();

    if (!EnsureWinsockStarted(m_pImpl)) return false;
    if (!pszAddress) return false;

    m_pImpl->peerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_pImpl->peerSocket == INVALID_SOCKET)
    {
        OutputDebugStringA("[Network] client socket creation failed.\n");
        CloseSocketSafe(m_pImpl->peerSocket);
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, pszAddress, &addr.sin_addr) != 1)
    {
        OutputDebugStringA("[Network] invalid IP address.\n");
        CloseSocketSafe(m_pImpl->peerSocket);
        return false;
    }

    if (connect(m_pImpl->peerSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR)
    {
        OutputDebugStringA("[Network] connect failed.\n");
        CloseSocketSafe(m_pImpl->peerSocket);
        return false;
    }

    EnableNoDelay(m_pImpl->peerSocket);
    SetNonBlocking(m_pImpl->peerSocket);

    m_eMode = MODE::CLIENT;
    m_bConnected = true;
    m_bHasRemoteState = false;
    m_recvBuffer.clear();
    m_collisionEvents.clear();
    m_effectEvents.clear();
    m_raceFinishEvents.clear();
    m_raceResultEvents.clear();
    m_pImpl->pendingSendBuffer.clear();

    OutputDebugStringA("[Network] Connected to host.\n");
    return true;
}

void CNetworkManager::Shutdown()
{
    DisconnectPeer();
    if (m_pImpl) CloseSocketSafe(m_pImpl->listenSocket);

    m_bHasRemoteState = false;
    m_recvBuffer.clear();
    m_collisionEvents.clear();
    m_effectEvents.clear();
    m_raceFinishEvents.clear();
    m_raceResultEvents.clear();
    m_serverRaceRecords.clear();
    m_itemEvents.clear();


    m_eMode = MODE::NONE;
    if (m_pImpl && m_pImpl->wsaStarted)
    {
        WSACleanup();
        m_pImpl->wsaStarted = false;
    }
}

void CNetworkManager::DisconnectPeer()
{
    if (m_pImpl)
    {
        m_pImpl->pendingSendBuffer.clear();
        CloseSocketSafe(m_pImpl->peerSocket);
    }

    m_bConnected = false;
    m_bHasRemoteState = false;
    m_recvBuffer.clear();
    m_collisionEvents.clear();
    m_effectEvents.clear();
    m_raceFinishEvents.clear();
    m_raceResultEvents.clear();
    m_serverRaceRecords.clear();
}

void CNetworkManager::TryAcceptClient()
{
    if (m_eMode != MODE::HOST) return;
    if (m_bConnected) return;
    if (!m_pImpl || (m_pImpl->listenSocket == INVALID_SOCKET)) return;

    SOCKET acceptedSocket = accept(m_pImpl->listenSocket, nullptr, nullptr);
    if (acceptedSocket == INVALID_SOCKET)
    {
        const int errorCode = WSAGetLastError();
        if (errorCode != WSAEWOULDBLOCK)
        {
            OutputDebugStringA("[Network] accept failed.\n");
        }
        return;
    }

    m_pImpl->peerSocket = acceptedSocket;
    EnableNoDelay(m_pImpl->peerSocket);
    SetNonBlocking(m_pImpl->peerSocket);

    m_bConnected = true;
    m_recvBuffer.clear();
    m_collisionEvents.clear();
    m_effectEvents.clear();
    m_raceFinishEvents.clear();
    m_raceResultEvents.clear();
    m_pImpl->pendingSendBuffer.clear();

    OutputDebugStringA("[Network] Client connected.\n");
}

void CNetworkManager::TryReceivePackets()
{
    if (!m_bConnected) return;
    if (!m_pImpl || (m_pImpl->peerSocket == INVALID_SOCKET)) return;

    char tempBuffer[1024]{};

    while (true)
    {
        const int recvBytes = recv(m_pImpl->peerSocket, tempBuffer, static_cast<int>(sizeof(tempBuffer)), 0);

        if (recvBytes > 0)
        {
            m_recvBuffer.insert(m_recvBuffer.end(), tempBuffer, tempBuffer + recvBytes);
        }
        else if (recvBytes == 0)
        {
            OutputDebugStringA("[Network] Peer disconnected.\n");
            DisconnectPeer();
            return;
        }
        else
        {
            const int errorCode = WSAGetLastError();
            if (errorCode != WSAEWOULDBLOCK)
            {
                OutputDebugStringA("[Network] recv failed.\n");
                DisconnectPeer();
            }
            break;
        }

    }

    while (m_recvBuffer.size() >= sizeof(NetMessageHeader))
    {
        NetMessageHeader header{};
        std::memcpy(&header, m_recvBuffer.data(), sizeof(NetMessageHeader));

        if (header.magic != NetMessageHeader{}.magic || header.version != 1 || header.size < sizeof(NetMessageHeader))
        {
            OutputDebugStringA("[Network] Invalid packet header.\n");
            m_recvBuffer.clear();
            return;
        }

        if (m_recvBuffer.size() < header.size) break;

        switch (static_cast<NET_MESSAGE_TYPE>(header.type))
        {
        case NET_MESSAGE_TYPE::WELCOME_ASSIGN_ID:
            if (header.size == sizeof(WelcomePacket))
            {
                WelcomePacket packet{};
                std::memcpy(&packet, m_recvBuffer.data(), sizeof(packet));
                m_nWelcomePlayerId = packet.assignedPlayerId;
                m_bHasWelcomeId = true;
            }
            break;
        case NET_MESSAGE_TYPE::PLAYER_STATE:
            if (header.size == sizeof(PlayerStatePacket))
            {
                PlayerStatePacket packet{};
                std::memcpy(&packet, m_recvBuffer.data(), sizeof(packet));
                m_RemoteState.push_back(packet.state);
                m_bHasRemoteState = true;
            }
            break;

        case NET_MESSAGE_TYPE::COLLISION_EVENT:
            if (header.size == sizeof(CollisionEventPacket))
            {
                CollisionEventPacket packet{};
                std::memcpy(&packet, m_recvBuffer.data(), sizeof(packet));
                m_collisionEvents.push_back(packet.eventData);
            }
            break;

        case NET_MESSAGE_TYPE::EFFECT_EVENT:
            if (header.size == sizeof(EffectEventPacket))
            {
                EffectEventPacket packet{};
                std::memcpy(&packet, m_recvBuffer.data(), sizeof(packet));
                m_effectEvents.push_back(packet.eventData);
            }
            break;
        case NET_MESSAGE_TYPE::RACE_FINISH:
            if (header.size == sizeof(RaceFinishPacket)) {
                RaceFinishPacket packet{};
                std::memcpy(&packet, m_recvBuffer.data(), sizeof(packet));
                m_raceFinishEvents.push_back(packet.record);
            }
            break;
        case NET_MESSAGE_TYPE::RACE_RESULT:
            if (header.size == sizeof(RaceResultPacket)) {
                RaceResultPacket packet{};
                std::memcpy(&packet, m_recvBuffer.data(), sizeof(packet));
                m_raceResultEvents.push_back(packet.result);
            }
            break;

        case NET_MESSAGE_TYPE::ITEM_EVENT:
            if (header.size == sizeof(ItemEventPacket))
            {
                ItemEventPacket packet{};
                std::memcpy(&packet, m_recvBuffer.data(), sizeof(packet));
                m_itemEvents.push_back(packet.eventData);
            }
            break;
        case NET_MESSAGE_TYPE::PLAYER_COUNT:
            if(header.size == sizeof(PlayerCountPacket))
            {
                PlayerCountPacket packet{};
                std::memcpy(&packet, m_recvBuffer.data(), sizeof(packet));
                m_nTotalPlayerCount = packet.currentCount;
            }
			break;

        default:
            OutputDebugStringA("[Network] Unknown packet type.\n");
            break;
        }

        m_recvBuffer.erase(
            m_recvBuffer.begin(),
            m_recvBuffer.begin() + static_cast<std::ptrdiff_t>(header.size)
        );
    }
}

void CNetworkManager::TrySendLocalState(const PlayerNetState& state)
{
    if (!m_pImpl || !m_bConnected || m_pImpl->peerSocket == INVALID_SOCKET) return;

    PlayerStatePacket packet{};
    packet.header.type = static_cast<unsigned int>(NET_MESSAGE_TYPE::PLAYER_STATE);
    packet.header.size = sizeof(PlayerStatePacket);
    packet.state = state;

    const char* bytes = reinterpret_cast<const char*>(&packet);
    m_pImpl->pendingSendBuffer.insert(m_pImpl->pendingSendBuffer.end(), bytes, bytes + sizeof(packet));

    FlushPendingSends();
}

void CNetworkManager::SendCollisionEvent(const CollisionEventNet& ev)
{
    if (!m_pImpl || !m_bConnected || m_pImpl->peerSocket == INVALID_SOCKET) return;

    CollisionEventPacket packet{};
    packet.header.type = static_cast<unsigned int>(NET_MESSAGE_TYPE::COLLISION_EVENT);
    packet.header.size = sizeof(CollisionEventPacket);
    packet.eventData = ev;

    const char* bytes = reinterpret_cast<const char*>(&packet);
    m_pImpl->pendingSendBuffer.insert(m_pImpl->pendingSendBuffer.end(), bytes, bytes + sizeof(packet));

    FlushPendingSends();
}

void CNetworkManager::SendEffectEvent(const EffectEventNet& ev)
{
    if (!m_pImpl || !m_bConnected || m_pImpl->peerSocket == INVALID_SOCKET) return;

    EffectEventPacket packet{};
    packet.header.type = static_cast<unsigned int>(NET_MESSAGE_TYPE::EFFECT_EVENT);
    packet.header.size = sizeof(EffectEventPacket);
    packet.eventData = ev;

    const char* bytes = reinterpret_cast<const char*>(&packet);
    m_pImpl->pendingSendBuffer.insert(m_pImpl->pendingSendBuffer.end(), bytes, bytes + sizeof(packet));

    FlushPendingSends();
}

void CNetworkManager::SendRaceFinish(const RaceRecordNet& ev)
{
    if (!m_pImpl || !m_bConnected || m_pImpl->peerSocket == INVALID_SOCKET) return;

    RaceFinishPacket packet{};
    packet.header.type = static_cast<unsigned int>(NET_MESSAGE_TYPE::RACE_FINISH);
    packet.header.size = sizeof(RaceFinishPacket);
    packet.record = ev;

    const char* bytes = reinterpret_cast<const char*>(&packet);
    m_pImpl->pendingSendBuffer.insert(m_pImpl->pendingSendBuffer.end(), bytes, bytes + sizeof(packet));

    FlushPendingSends();
}

void CNetworkManager::SendRaceResult(const RaceResultNet& ev)
{
    if (!m_pImpl || !m_bConnected || m_pImpl->peerSocket == INVALID_SOCKET) return;

    RaceResultPacket packet{};
    packet.header.type = static_cast<unsigned int>(NET_MESSAGE_TYPE::RACE_RESULT);
    packet.header.size = sizeof(RaceResultPacket);
    packet.result = ev;

    const char* bytes = reinterpret_cast<const char*>(&packet);
    m_pImpl->pendingSendBuffer.insert(m_pImpl->pendingSendBuffer.end(), bytes, bytes + sizeof(packet));

    FlushPendingSends();
}

void CNetworkManager::FlushPendingSends()
{
    if (!m_pImpl || !m_bConnected || m_pImpl->peerSocket == INVALID_SOCKET) return;

    while (!m_pImpl->pendingSendBuffer.empty())
    {
        const int sentBytes = send(
            m_pImpl->peerSocket,
            m_pImpl->pendingSendBuffer.data(),
            static_cast<int>(m_pImpl->pendingSendBuffer.size()),
            0);

        if (sentBytes > 0)
        {
            m_pImpl->pendingSendBuffer.erase(
                m_pImpl->pendingSendBuffer.begin(),
                m_pImpl->pendingSendBuffer.begin() + sentBytes);
        }
        else if (sentBytes == SOCKET_ERROR)
        {
            const int errorCode = WSAGetLastError();
            if (errorCode != WSAEWOULDBLOCK)
            {
                OutputDebugStringA("[Network] send failed.\n");
                DisconnectPeer();
            }
            break;
        }
        else
        {
            break;
        }
    }
}

void CNetworkManager::Update(float /*fTimeElapsed*/, const PlayerNetState* pLocalState)
{
    if (m_eMode == MODE::HOST)
    {
        TryAcceptClient();
    }

    if (m_bConnected)
    {
        TryReceivePackets();

        if (pLocalState)
        {
            TrySendLocalState(*pLocalState);
        }
        else
        {
            FlushPendingSends();
        }
    }
}

bool CNetworkManager::ConsumeRemoteState(PlayerNetState& outState)
{
	if (m_RemoteState.empty()) return false;
    outState = m_RemoteState.front();
	m_RemoteState.erase(m_RemoteState.begin());
    m_bHasRemoteState = false;
    return true;
}

bool CNetworkManager::ConsumeCollisionEvent(CollisionEventNet& outEvent)
{
    if (m_collisionEvents.empty()) return false;

    outEvent = m_collisionEvents.front();
    m_collisionEvents.erase(m_collisionEvents.begin());
    return true;
}

bool CNetworkManager::ConsumeEffectEvent(EffectEventNet& outEvent)
{
    if (m_effectEvents.empty()) return false;

    outEvent = m_effectEvents.front();
    m_effectEvents.erase(m_effectEvents.begin());
    return true;
}

bool CNetworkManager::ConsumeRaceFinish(RaceRecordNet& outEvent)
{
    if (m_raceFinishEvents.empty()) return false;

    outEvent = m_raceFinishEvents.front();
    m_raceFinishEvents.erase(m_raceFinishEvents.begin());
    return true;
}

bool CNetworkManager::ConsumeRaceResult(RaceResultNet& outEvent)
{
    if (m_raceResultEvents.empty()) return false;

    outEvent = m_raceResultEvents.front();
    m_raceResultEvents.erase(m_raceResultEvents.begin());
    return true;
}

bool CNetworkManager::IsConnected() const
{
    return m_bConnected;
}

bool CNetworkManager::IsHosting() const
{
    return (m_eMode == MODE::HOST);
}

bool CNetworkManager::IsEnabled() const
{
    return (m_eMode != MODE::NONE);
}

void CNetworkManager::AddServerRecord(const RaceRecordNet& record)
{
    m_serverRaceRecords.push_back(record);
}

void CNetworkManager::SetTotalPlayerCount(std::uint32_t count)
{
    m_nTotalPlayerCount = count;
}

bool CNetworkManager::HasAllRecords() const
{
    return m_serverRaceRecords.size() >= m_nTotalPlayerCount;
}

RaceResultNet CNetworkManager::CalculateRankings()
{
    RaceResultNet result{};
    result.playerCount = 0;

    if (m_serverRaceRecords.empty())
        return result;

    std::sort(m_serverRaceRecords.begin(), m_serverRaceRecords.end(),
        [](const RaceRecordNet& a, const RaceRecordNet& b) {
            return a.finishTime < b.finishTime;
        });

    result.playerCount = static_cast<std::uint32_t>(std::min(m_serverRaceRecords.size(), (size_t)4));

    for (std::uint32_t i = 0; i < result.playerCount; ++i)
    {
        result.playerRecords[i] = m_serverRaceRecords[i];
    }

    return result;
}
bool CNetworkManager::ConsumeWelomeEvent(int& outPlayerId)
{
    if (!m_bHasWelcomeId) return false;

    outPlayerId = m_nWelcomePlayerId;
    m_bHasWelcomeId = false;
    return true;
}
std::uint32_t CNetworkManager::GetCurrentPlayerCount() const
{
    return m_nTotalPlayerCount;
}
bool CNetworkManager::ConsumeItemEvent(ItemEventNet& outEvent)
{
    if (m_itemEvents.empty()) return false;

    outEvent = m_itemEvents.front();
    m_itemEvents.erase(m_itemEvents.begin());
    return true;
}

void CNetworkManager::SendItemEvent(const ItemEventNet& ev)
{
    if (!m_pImpl || !m_bConnected || m_pImpl->peerSocket == INVALID_SOCKET) return;

    ItemEventPacket packet{};
    packet.header.type = static_cast<unsigned int>(NET_MESSAGE_TYPE::ITEM_EVENT);
    packet.header.size = sizeof(ItemEventPacket);
    packet.eventData = ev;

    const char* bytes = reinterpret_cast<const char*>(&packet);
    m_pImpl->pendingSendBuffer.insert(
        m_pImpl->pendingSendBuffer.end(),
        bytes,
        bytes + sizeof(packet)
    );

    FlushPendingSends();
}