#include <iostream>
#include <vector>
#include <algorithm>
#include <WinSock2.h>
#include "ServerNetworkTypes.h" 

#pragma comment(lib, "ws2_32.lib")

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

    while (true)
    {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(listenSocket, &readSet);

        for (SOCKET s : clientSockets)
        {
            FD_SET(s, &readSet);
        }

        int activity = select(0, &readSet, nullptr, nullptr, nullptr);
        if (activity == SOCKET_ERROR) break;

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
                    NetMessageHeader* pHeader = reinterpret_cast<NetMessageHeader*>(buffer);

                    if (pHeader->type == static_cast<unsigned int>(NET_MESSAGE_TYPE::RACE_FINISH))
                    {
                        RaceFinishPacket* pFinishPkt = reinterpret_cast<RaceFinishPacket*>(buffer);
                        raceRecords.push_back(pFinishPkt->record);

                        std::cout << "Player " << pFinishPkt->record.playerId
                            << " 완주! (기록: " << pFinishPkt->record.finishTime << "초)" << std::endl;

                        if (raceRecords.size() == clientSockets.size())
                        {
                            std::cout << "모든 유저 완주 완료. 랭킹 전송" << std::endl;

                            std::sort(raceRecords.begin(), raceRecords.end(),
                                [](const RaceRecordNet& a, const RaceRecordNet& b) {
                                    return a.finishTime < b.finishTime;
                                });

                            RaceResultPacket resultPkt{};
                            resultPkt.header.type = static_cast<unsigned int>(NET_MESSAGE_TYPE::RACE_RESULT);
                            resultPkt.header.size = sizeof(RaceResultPacket);
                            resultPkt.result.playerCount = static_cast<std::uint32_t>(raceRecords.size());

                            for (size_t i = 0; i < raceRecords.size(); ++i) {
                                resultPkt.result.playerRecords[i] = raceRecords[i];
                            }

                            for (SOCKET otherSocket : clientSockets) {
                                send(otherSocket, reinterpret_cast<const char*>(&resultPkt), sizeof(resultPkt), 0);
                            }// 모든 플레이어에게 결과 브로드캐스트

                            raceRecords.clear();
                        }
                    }
                    else
                    {
                        for (SOCKET otherSocket : clientSockets)
                        {
                            if (otherSocket != currentSocket)
                            {
                                send(otherSocket, buffer, recvBytes, 0);
                            }
                        }
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