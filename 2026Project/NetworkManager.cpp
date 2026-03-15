#include "stdafx.h"

#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>

#pragma comment(lib, "ws2_32.lib")

#include <algorithm>
#include <cstring>
#include <vector>

#include "NetworkManager.h"

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

    if (m_pImpl)
    {
        delete m_pImpl;
        m_pImpl = nullptr;
    }
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
    m_pImpl->pendingSendBuffer.clear();

    OutputDebugStringA("[Network] Connected to host.\n");
    return true;
}

void CNetworkManager::Shutdown()
{
    DisconnectPeer();
    CloseSocketSafe(m_pImpl->listenSocket);

    m_bHasRemoteState = false;
    m_recvBuffer.clear();
    if (m_pImpl) m_pImpl->pendingSendBuffer.clear();
    m_eMode = MODE::NONE;

    if (m_pImpl && m_pImpl->wsaStarted)
    {
        WSACleanup();
        m_pImpl->wsaStarted = false;
    }
}

void CNetworkManager::DisconnectPeer()
{
    if (m_pImpl) m_pImpl->pendingSendBuffer.clear();
    CloseSocketSafe(m_pImpl->peerSocket);
    m_bConnected = false;
    m_bHasRemoteState = false;
    m_recvBuffer.clear();
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
    m_pImpl->pendingSendBuffer.clear();

    OutputDebugStringA("[Network] Client connected.\n");
}

void CNetworkManager::TryReceivePackets()
{
    if (!m_bConnected) return;
    if (!m_pImpl || (m_pImpl->peerSocket == INVALID_SOCKET)) return;

    char tempBuffer[512]{};

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

    while (m_recvBuffer.size() >= NET_PACKET_SIZE)
    {
        PlayerNetPacket packet{};
        std::memcpy(&packet, m_recvBuffer.data(), NET_PACKET_SIZE);
        m_recvBuffer.erase(m_recvBuffer.begin(), m_recvBuffer.begin() + static_cast<std::ptrdiff_t>(NET_PACKET_SIZE));

        if ((packet.magic != NET_MAGIC) || (packet.version != 1))
        {
            OutputDebugStringA("[Network] Invalid packet received.\n");
            continue;
        }

        m_latestRemoteState = packet.state;
        m_bHasRemoteState = true;
    }
}

void CNetworkManager::TrySendLocalState(const PlayerNetState& state)
{
    if (!m_pImpl) return;
    if (!m_bConnected) return;
    if (m_pImpl->peerSocket == INVALID_SOCKET) return;

    PlayerNetPacket packet{};
    packet.state = state;

    const char* packetBytes = reinterpret_cast<const char*>(&packet);
    m_pImpl->pendingSendBuffer.insert(
        m_pImpl->pendingSendBuffer.end(),
        packetBytes,
        packetBytes + sizeof(packet)
    );

    while (!m_pImpl->pendingSendBuffer.empty())
    {
        const int sentBytes = send(
            m_pImpl->peerSocket,
            m_pImpl->pendingSendBuffer.data(),
            static_cast<int>(m_pImpl->pendingSendBuffer.size()),
            0
        );

        if (sentBytes > 0)
        {
            m_pImpl->pendingSendBuffer.erase(
                m_pImpl->pendingSendBuffer.begin(),
                m_pImpl->pendingSendBuffer.begin() + sentBytes
            );
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
    }
}

bool CNetworkManager::ConsumeRemoteState(PlayerNetState& outState)
{
    if (!m_bHasRemoteState) return false;

    outState = m_latestRemoteState;
    m_bHasRemoteState = false;
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
