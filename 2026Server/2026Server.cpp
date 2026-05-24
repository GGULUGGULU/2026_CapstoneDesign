#include <iostream>
#include <vector>
#include <algorithm>
#include <WinSock2.h>
#include <chrono>
#include <map>
#include "ServerNetworkTypes.h" 

#pragma comment(lib, "ws2_32.lib")

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
        int nSend = send(s, reinterpret_cast<const char*>(&pkt), sizeof(pkt), 0);
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
                send(s, reinterpret_cast<const char*>(&respawnPkt), sizeof(respawnPkt), 0);
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
            send(otherSocket, packetBuffer, bufferSize, 0);
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
    int nextPlayerId = 1; 
	std::vector<RaceRecordNet> raceRecords;

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
                clientSockets.push_back(clientSocket);

                std::cout << "Player " << nextPlayerId
                    << " 들어옴 (현재 대기실 인원: "
                    << clientSockets.size() << "명)" << std::endl;

                WelcomePacket welcomePkt{};
                welcomePkt.header.type = static_cast<unsigned int>(NET_MESSAGE_TYPE::WELCOME_ASSIGN_ID);
                welcomePkt.header.size = sizeof(WelcomePacket);
                welcomePkt.assignedPlayerId = nextPlayerId;

                send(clientSocket, reinterpret_cast<const char*>(&welcomePkt), sizeof(welcomePkt), 0);

                ++nextPlayerId;

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
                    int offset = 0;

                    while (offset < recvBytes)
                    {
                        if (recvBytes - offset < sizeof(NetMessageHeader)) break;

                        NetMessageHeader* pHeader = reinterpret_cast<NetMessageHeader*>(buffer + offset);

                        if (pHeader->size == 0 || offset + pHeader->size > recvBytes) break;

                        if (pHeader->type == static_cast<unsigned int>(NET_MESSAGE_TYPE::PLAYER_STATE))
                        {
                            for (SOCKET otherSocket : clientSockets) {
                                if (otherSocket != currentSocket) {
                                    send(otherSocket, buffer + offset, pHeader->size, 0);
                                }
                            }
                        }
                        else if (pHeader->type == static_cast<unsigned int>(NET_MESSAGE_TYPE::MAP_ITEM_EVENT))
                        {
                            // 아이템 선점 처리
                            MapItemEventPacket* pItemPkt = reinterpret_cast<MapItemEventPacket*>(buffer + offset);
                            HandleMapItemRequest(
                                pItemPkt->eventData.itemIndex,
                                pItemPkt->eventData.playerId,
                                clientSockets,
                                buffer + offset,
                                pHeader->size
                            );
                        }
                        else if (pHeader->type == static_cast<unsigned int>(NET_MESSAGE_TYPE::RACE_FINISH))
                        {
                            // 완주 패킷 처리
                            RaceFinishPacket* pFinishPkt = reinterpret_cast<RaceFinishPacket*>(buffer + offset);
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
                                    send(otherSocket, reinterpret_cast<const char*>(&resultPkt), sizeof(resultPkt), 0);
                                }

                                raceRecords.clear(); // 기록 초기화
                            }
                        }
                        else
                        {
                            // 이동/아이템/완주가 아닌 일반 패킷(이펙트, 충돌 등) 브로드캐스트
                            for (SOCKET otherSocket : clientSockets) {
                                if (otherSocket != currentSocket) {
                                    send(otherSocket, buffer + offset, pHeader->size, 0);
                                }
                            }
                        }

                        offset += pHeader->size;
                    }
                    ++it;
                }
                else
                {
                    std::cout << "퇴장. (현재 남은 인원: "
                        << clientSockets.size() - 1 << "명)" << std::endl;

                    closesocket(currentSocket);
                    it = clientSockets.erase(it);

                    if (clientSockets.empty())
                    {
                        nextPlayerId = 1;
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