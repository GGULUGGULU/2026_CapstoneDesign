#include <iostream>
#include <vector>
#include <algorithm>
#include <WinSock2.h>
#include <mstcpip.h>
#include <chrono>
#include <map>
#include <set>
#include <cstring>
#include "ServerNetworkTypes.h" 

#pragma comment(lib, "ws2_32.lib")

int SendPacket(SOCKET socket, const char* data, int size, int flags)
{
    int offset = 0;
    while (offset < size) {
        int sent = send(socket, data + offset, size - offset, flags);
        if (sent <= 0) { shutdown(socket, SD_BOTH); return SOCKET_ERROR; }
        offset += sent;
    }
    return offset;
}

std::map<int, float> g_deadItems;

void BroadcastPlayerCount(const std::vector<SOCKET>& sockets)
{
    PlayerCountPacket pkt{};
    pkt.header.type = static_cast<unsigned int>(NET_MESSAGE_TYPE::PLAYER_COUNT);
    pkt.header.size = sizeof(PlayerCountPacket);
    pkt.currentCount = static_cast<std::uint32_t>(sockets.size());


    std::cout << "현재 방 인원(" << pkt.currentCount
        << "명)을 " << sockets.size() << "개의 클라이언트에게 전송 시도. (Size: " << pkt.header.size << ")" << std::endl;

    for (SOCKET s : sockets)
    {
        int nSend = SendPacket(s, reinterpret_cast<const char*>(&pkt), sizeof(pkt), 0);
    }
}

void UpdateServerItems(float elapsed, const std::vector<SOCKET>& clientSockets)
{
    for (auto it = g_deadItems.begin(); it != g_deadItems.end(); )
    {
        it->second -= elapsed;
        if (it->second <= 0.0f)
        {
            MapItemEventPacket respawnPkt{};
            respawnPkt.header.type = static_cast<unsigned int>(NET_MESSAGE_TYPE::MAP_ITEM_EVENT);
            respawnPkt.header.size = sizeof(MapItemEventPacket);
            respawnPkt.eventData.itemIndex = it->first;
            respawnPkt.eventData.IsActive = true;  // 부활 활성화
            respawnPkt.eventData.playerId = 0;      // 획득 유저 없음

            // 모든 클라이언트에게 아이템 생성 브로드캐스트
            for (SOCKET s : clientSockets) {
                SendPacket(s, reinterpret_cast<const char*>(&respawnPkt), sizeof(respawnPkt), 0);
            }

            it = g_deadItems.erase(it); 
        }
        else {
            ++it;
        }
    }
}

void HandleMapItemRequest(int itemIndex, std::uint32_t playerId, const std::vector<SOCKET>& clientSockets, const char* packetBuffer, int bufferSize)
{
    if (g_deadItems.find(itemIndex) == g_deadItems.end())
    {
        g_deadItems[itemIndex] = 3.0f;

        // 아이템 먹은 유저 정보 브로드캐스트
        for (SOCKET otherSocket : clientSockets) {
            SendPacket(otherSocket, packetBuffer, bufferSize, 0);
        }
    }
}

void ResetServerItems()
{
    g_deadItems.clear();
}

int main()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "WSAStartup 실패" << std::endl;
        return 1;
    }

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET)
    {
        std::cerr << "소켓 생성 실패" << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(NET_DEFAULT_PORT); // 7777번 포트
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cerr << "Bind 실패" << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        std::cerr << "Listen 실패" << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "===========================================" << std::endl;
    std::cout << "[Server] 레이싱 게임 릴레이 서버 가동 시작!" << std::endl;
    std::cout << "[Server] 접속 대기 중... (포트: " << NET_DEFAULT_PORT << ")" << std::endl;
    std::cout << "===========================================\n" << std::endl;

    std::vector<SOCKET> clientSockets;
    std::map<SOCKET, int> clientIds;
	std::vector<RaceRecordNet> raceRecords;
    std::map<SOCKET, std::vector<char>> receiveBuffers;
    std::map<SOCKET, bool> readyPlayers;
    std::set<SOCKET> loadedPlayers;
    enum class Phase { Waiting, Playing, Results };
    Phase phase = Phase::Waiting;

    auto lastTime = std::chrono::system_clock::now();

    while (true)
    {
        auto currTime = std::chrono::system_clock::now();
        float fElapsed = std::chrono::duration<float>(currTime - lastTime).count();
        lastTime = currTime;

        UpdateServerItems(fElapsed, clientSockets);

        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(listenSocket, &readSet);

        for (SOCKET s : clientSockets)
        {
            FD_SET(s, &readSet);
        }

        timeval tv{ 0, 10000 };
        int activity = select(0, &readSet, nullptr, nullptr, &tv);
        if (activity == SOCKET_ERROR) break;
        if (activity == 0) continue;

        if (FD_ISSET(listenSocket, &readSet))
        {
            SOCKET clientSocket = accept(listenSocket, nullptr, nullptr);
            if (clientSocket != INVALID_SOCKET)
            {
                if (phase != Phase::Waiting) { closesocket(clientSocket); continue; }
                DWORD timeout = 2000;
                setsockopt(clientSocket, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
                tcp_keepalive keepAlive{1, 10000, 1000};
                DWORD returned = 0;
                WSAIoctl(clientSocket, SIO_KEEPALIVE_VALS, &keepAlive, sizeof(keepAlive), nullptr, 0, &returned, nullptr, nullptr);
                int assignedId = 1;
                while (assignedId <= 4) {
                    bool used = false;
                    for (SOCKET& s : clientSockets) {
                        if (clientIds[s] == assignedId) {
                            used = true;
                            break;
                        }
                    }

                    if (!used) {
                        break;
                    }

                    ++assignedId;
                }

                if (assignedId > 4) {
                    std::cout << "방이 가득참" << std::endl;
                    closesocket(clientSocket);
                    continue;
                }

                clientSockets.push_back(clientSocket);
                clientIds[clientSocket] = assignedId;
                readyPlayers[clientSocket] = false;

                std::cout << "Player " << assignedId
                    << " 들어옴 (현재 대기실 인원: "
                    << clientSockets.size() << "명)" << std::endl;

                WelcomePacket welcomePkt{};
                welcomePkt.header.type = static_cast<unsigned int>(NET_MESSAGE_TYPE::WELCOME_ASSIGN_ID);
                welcomePkt.header.size = sizeof(WelcomePacket);
                welcomePkt.assignedPlayerId = assignedId;

                SendPacket(clientSocket, reinterpret_cast<const char*>(&welcomePkt), sizeof(welcomePkt), 0);

                BroadcastPlayerCount(clientSockets);
            }
        }

        for (auto it = clientSockets.begin(); it != clientSockets.end(); )
        {
            SOCKET currentSocket = *it;

            if (FD_ISSET(currentSocket, &readSet))
            {
                char buffer[1024];
                int recvBytes = recv(currentSocket, buffer, sizeof(buffer), 0);

                if (recvBytes > 0)
                {
                    auto& pending = receiveBuffers[currentSocket];
                    pending.insert(pending.end(), buffer, buffer + recvBytes);
                    size_t offset = 0;

                    while (offset < pending.size())
                    {
                        if (pending.size() - offset < sizeof(NetMessageHeader)) break;

                        NetMessageHeader* pHeader = reinterpret_cast<NetMessageHeader*>(pending.data() + offset);

                        if (pHeader->magic != NET_MAGIC || pHeader->version != 1 ||
                            pHeader->size < sizeof(NetMessageHeader) || pHeader->size > 4096) {
                            shutdown(currentSocket, SD_BOTH);
                            pending.clear(); offset = 0; break;
                        }
                        if (offset + pHeader->size > pending.size()) break;
                        auto type = static_cast<NET_MESSAGE_TYPE>(pHeader->type);
                        size_t expected = 0;
                        switch (type) {
                        case NET_MESSAGE_TYPE::ROOM_SYNC_EVENT: expected = sizeof(RoomSyncEventPacket); break;
                        case NET_MESSAGE_TYPE::LOAD_COMPLETE: expected = sizeof(LoadCompletePacket); break;
                        case NET_MESSAGE_TYPE::PLAYER_STATE: expected = sizeof(PlayerStatePacket); break;
                        case NET_MESSAGE_TYPE::RACE_FINISH: expected = sizeof(RaceFinishPacket); break;
                        case NET_MESSAGE_TYPE::MAP_ITEM_EVENT: expected = sizeof(MapItemEventPacket); break;
                        case NET_MESSAGE_TYPE::COLLISION_EVENT: expected = sizeof(CollisionEventPacket); break;
                        case NET_MESSAGE_TYPE::EFFECT_EVENT: expected = sizeof(EffectEventPacket); break;
                        case NET_MESSAGE_TYPE::ITEM_EVENT: expected = sizeof(ItemEventPacket); break;
                        case NET_MESSAGE_TYPE::BANANA_EVENT: expected = sizeof(BananaEventPacket); break;
                        default: break;
                        }
                        if (!expected || pHeader->size != expected || phase == Phase::Results) {
                            offset += pHeader->size; continue;
                        }
                        if (type == NET_MESSAGE_TYPE::ROOM_SYNC_EVENT) {
                            if (phase != Phase::Waiting) { offset += pHeader->size; continue; }
                            auto* room = reinterpret_cast<RoomSyncEventPacket*>(pending.data() + offset);
                            room->eventData.playerId = clientIds[currentSocket];
                            readyPlayers[currentSocket] = room->eventData.isReady;
                            if (std::all_of(clientSockets.begin(), clientSockets.end(),
                                [&](SOCKET peer) { return readyPlayers[peer]; })) {
                                phase = Phase::Playing;
                                raceRecords.clear(); loadedPlayers.clear(); ResetServerItems();
                            }
                        }
                        else if (type == NET_MESSAGE_TYPE::LOAD_COMPLETE) {
                            if (phase == Phase::Playing && loadedPlayers.insert(currentSocket).second &&
                                loadedPlayers.size() == clientSockets.size()) {
                                GameStartSignPacket start{};
                                start.header.type = static_cast<unsigned>(NET_MESSAGE_TYPE::GAME_START_SIGN);
                                start.header.size = sizeof(start);
                                start.eventData.startSign = true;
                                for (SOCKET peer : clientSockets)
                                    SendPacket(peer, reinterpret_cast<const char*>(&start), sizeof(start), 0);
                            }
                            offset += pHeader->size; continue;
                        }
                        else if (phase != Phase::Playing) { offset += pHeader->size; continue; }

                        if (pHeader->type == static_cast<unsigned int>(NET_MESSAGE_TYPE::PLAYER_STATE))
                        {
                            for (SOCKET otherSocket : clientSockets) {
                                if (otherSocket != currentSocket) {
                                    SendPacket(otherSocket, pending.data() + offset, pHeader->size, 0);
                                }
                            }
                        }
                        else if (pHeader->type == static_cast<unsigned int>(NET_MESSAGE_TYPE::MAP_ITEM_EVENT))
                        {
                            // 아이템 선점 처리
                            MapItemEventPacket* pItemPkt = reinterpret_cast<MapItemEventPacket*>(pending.data() + offset);
                            HandleMapItemRequest(
                                pItemPkt->eventData.itemIndex,
                                pItemPkt->eventData.playerId,
                                clientSockets,
                                pending.data() + offset,
                                pHeader->size
                            );
                        }
                        else if (pHeader->type == static_cast<unsigned int>(NET_MESSAGE_TYPE::RACE_FINISH))
                        {
                            // 완주 패킷 처리
                            RaceFinishPacket* pFinishPkt = reinterpret_cast<RaceFinishPacket*>(pending.data() + offset);
                            pFinishPkt->record.playerId = clientIds[currentSocket];
                            if (std::any_of(raceRecords.begin(), raceRecords.end(), [&](const RaceRecordNet& record) {
                                return record.playerId == pFinishPkt->record.playerId;
                            })) { offset += pHeader->size; continue; }
                            raceRecords.push_back(pFinishPkt->record);

                            std::cout << "[Server] Player " << pFinishPkt->record.playerId
                                << " Finished! (Time: " << pFinishPkt->record.finishTime << "s)\n";

                            // 모든 접속자가 완주했는지 확인
                            if (raceRecords.size() >= clientSockets.size())
                            {
                                std::cout << "[Server] All players finished. Broadcasting results.\n";

                                RaceResultPacket resultPkt{};
                                resultPkt.header.type = static_cast<unsigned int>(NET_MESSAGE_TYPE::RACE_RESULT);
                                resultPkt.header.size = sizeof(RaceResultPacket);

                                // 시간순으로 등수 정렬
                                std::sort(raceRecords.begin(), raceRecords.end(),
                                    [](const RaceRecordNet& a, const RaceRecordNet& b) {
                                        return a.finishTime < b.finishTime;
                                    });

                                resultPkt.result.playerCount = static_cast<std::uint32_t>(raceRecords.size());
                                for (size_t i = 0; i < raceRecords.size(); ++i) {
                                    resultPkt.result.playerRecords[i] = raceRecords[i];
                                }

                                // 모든 플레이어에게 결과 브로드캐스트
                                for (SOCKET otherSocket : clientSockets) {
                                    SendPacket(otherSocket, reinterpret_cast<const char*>(&resultPkt), sizeof(resultPkt), 0);
                                }

                                raceRecords.clear(); // 기록 초기화
                                phase = Phase::Results;
                                loadedPlayers.clear(); readyPlayers.clear(); ResetServerItems();
                            }
                        }
                        else
                        {
                            // 이동/아이템/완주가 아닌 일반 패킷(이펙트, 충돌 등) 브로드캐스트
                            for (SOCKET otherSocket : clientSockets) {
                                if (otherSocket != currentSocket) {
                                    SendPacket(otherSocket, pending.data() + offset, pHeader->size, 0);
                                }
                            }
                        }

                        offset += pHeader->size;
                    }
                    pending.erase(pending.begin(), pending.begin() + offset);
                    ++it;
                }
                else
                {
                    int leftPlayerId = clientIds[currentSocket];

                    std::cout << clientIds[currentSocket] <<"퇴장. (현재 남은 인원: "
                        << clientSockets.size() - 1 << "명)" << std::endl;

                    receiveBuffers.erase(currentSocket);
                    readyPlayers.erase(currentSocket);
                    loadedPlayers.erase(currentSocket);
                    clientIds.erase(currentSocket);
                    closesocket(currentSocket);
                    it = clientSockets.erase(it);

                    // A race/loading participant (or room host) leaving invalidates this single room.
                    if (phase == Phase::Playing || (phase == Phase::Waiting && leftPlayerId == 1)) {
                        for (SOCKET peer : clientSockets) { shutdown(peer, SD_BOTH); closesocket(peer); }
                        clientSockets.clear(); clientIds.clear(); receiveBuffers.clear();
                        readyPlayers.clear(); loadedPlayers.clear(); raceRecords.clear(); ResetServerItems();
                        phase = Phase::Waiting;
                        break;
                    }

                    RoomSyncEventPacket leavePkt{};
                    leavePkt.header.type = static_cast<unsigned int>(NET_MESSAGE_TYPE::ROOM_SYNC_EVENT);
                    leavePkt.header.size = sizeof(RoomSyncEventPacket);
                    leavePkt.eventData.playerId = leftPlayerId;
                    leavePkt.eventData.selectedCarIndex = -1; // -1을 보내서 빈자리로 만듦!
                    leavePkt.eventData.selectedMapIndex = 0;
                    leavePkt.eventData.isReady = false; // 레디 상태도 강제로 풂

                    for (SOCKET s : clientSockets) {
                        SendPacket(s, reinterpret_cast<const char*>(&leavePkt), sizeof(leavePkt), 0);
                    }

                    if (clientSockets.empty())
                    {
                        phase = Phase::Waiting;
                        readyPlayers.clear(); loadedPlayers.clear(); receiveBuffers.clear();
                        raceRecords.clear();
                        ResetServerItems();
                    }

                    BroadcastPlayerCount(clientSockets);
                }
            }
            else
            {
                ++it;
            }
        }
    }

    closesocket(listenSocket);
    WSACleanup();
    return 0;
}