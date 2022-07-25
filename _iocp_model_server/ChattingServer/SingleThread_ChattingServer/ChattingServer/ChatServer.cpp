#include "ChatServer.h"
#include <process.h>
#include <conio.h>
#include <time.h>

#define ERR_CHAT_WRONG_PACKET_TYPE 11920
#define ERR_CHAT_TIME_OUT 11930
#define ERR_CHAT_PLAYER_ITER_END 11940

namespace univ_dev
{
    unsigned int ChatServer::ChatServerUpdateThread(void* param)
    {
        DWORD prev = ::timeGetTime();
        while (!this->_RunningFlag);
        while (this->_RunningFlag)
        {
            ::WaitForSingleObject(this->_DequeueEvent, INFINITE);
            JobMessage job;
            while (this->_JobQueue.dequeue(job))
            {
                this->_TotalUpdateCount++;
                this->PacketProc(job._Packet, job._SessionID, job._Type);
                if (job._Packet != nullptr)
                {
                    Packet::Free(job._Packet);
                }
            }
        }
        this->_ChatServerLog.LOG(L"UpdateThread End...", LogClass::LogLevel::LOG_LEVEL_SYSTEM);
        return 0;
    }
    unsigned int ChatServer::ChatServerMonitoringThread(void* param)
    {
        WCHAR logStr[512];
        DWORD begin = ::timeGetTime();
        DWORD prev;
        DWORD cur;
        DWORD simpleLogTimer;
        DWORD longLogTimer;
        longLogTimer = simpleLogTimer = cur = prev = begin;

        ULONGLONG lastTotalAccept = 0;
        ULONGLONG lastTotalRelease = 0;
        ULONGLONG lastTotalSend = 0;
        ULONGLONG lastTotalRecv = 0;
        ULONGLONG lastTotalUpdate = 0;
        while (!this->_RunningFlag);

        while (this->_RunningFlag)
        {
            this->CNetServer::WaitForMoniteringSignal();
            if (::_kbhit())
            {
                int key = ::_getch();
                if (::toupper(key) == 'Q')
                {
                    this->_RunningFlag = false;
                    this->CNetServer::PostServerStop();
                    ::SetEvent(this->_DequeueEvent);
                    ::WaitForSingleObject(this->_UpdateThread, INFINITE);
                    this->_ChatServerLog.LOG(L"ChatServer::MonitoringThread End...", LogClass::LogLevel::LOG_LEVEL_SYSTEM);
                    return 0;
                }
                else if (::toupper(key) == 'S')
                {
                    SaveProfiling();
                    ResetProfiling();
                }
            }

            cur = timeGetTime();

            MoniteringInfo info;
            this->CNetServer::GetMoniteringInfo(info);
            this->_HardWareMoniter.UpdateHardWareTime();
            this->_ProcessMoniter.UpdateProcessTime();

            ULONGLONG loginTPS = InterlockedExchange(&this->_LoginTPS, 0);
            ULONGLONG chatTPS = InterlockedExchange(&this->_ChatTPS, 0);
            ULONGLONG sectorMoveTPS = InterlockedExchange(&this->_SectorMoveTPS, 0);

            DWORD afterServerOn = (::timeGetTime() - begin) / 1000;

            DWORD day = afterServerOn / 86400;
            DWORD hour = (afterServerOn % 86400) / 3600;
            DWORD minute = (afterServerOn % 3600) / 60;
            DWORD sec = afterServerOn % 60;
            if (cur - simpleLogTimer > 60000)
            {
                simpleLogTimer = cur;
                if (cur - longLogTimer > 300000)
                {
                    longLogTimer = cur;
                    
                    wsprintf(logStr, L"\n-------------------------------------MONITERING----------------------------------------");
                    this->MonitoringLog(logStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
                    wsprintf(logStr, L"| After Server On : %u day / %u h / %u m / %u s", day, hour, minute, sec);
                    this->MonitoringLog(logStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
                    wsprintf(logStr, L"|----------------------------------------POOL------------------------------------------");
                    this->MonitoringLog(logStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
                    wsprintf(logStr, L"| LockFreeQueue Size / Capacity / Max : %u / %u / %u", info._SessionSendQueueSize, info._SessionSendQueueCapacity, info._SessionSendQueueMax);
                    this->MonitoringLog(logStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
                    wsprintf(logStr, L"| Packet Chunk Capacity / UseCount : %d / %d", Packet::GetCapacityCount(), Packet::GetUseCount());
                    this->MonitoringLog(logStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
                    wsprintf(logStr, L"| Packet ToTal UseCount : %d", Packet::GetTotalPacketCount());
                    this->MonitoringLog(logStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
                    wsprintf(logStr, L"|----------------------------------------USER------------------------------------------");
                    this->MonitoringLog(logStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
                    wsprintf(logStr, L"| PlayerMap Size / Capacity : %I64u / %I64u", this->_PlayerMap.size(), this->_PlayerMap.bucket_count());
                    this->MonitoringLog(logStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
                    wsprintf(logStr, L"| Player Capacity / UseCount :  / %d / %d", this->_PlayerPool.GetCapacityCount(), this->_PlayerPool.GetUseCount());
                    this->MonitoringLog(logStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
                    wsprintf(logStr, L"| Session Count / IDX Capacity / IDX Size : %u / %u / %u", info._CurrentSessionCount, info._SessionIndexStackCapacity, info._SessionIndexStackSize);
                    this->MonitoringLog(logStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
                    wsprintf(logStr, L"|----------------------------------------JOBQUEUESIZE----------------------------------");
                    this->MonitoringLog(logStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
                    wsprintf(logStr, L"| Job Queue Size / Capacity / Total Count : %d / %d / %d", this->_JobQueue.size(), this->_JobQueue.GetCapacityCount(), this->_JobMessagePool.GetTotalUseCount());
                    this->MonitoringLog(logStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
                    wsprintf(logStr, L"| Job Chunk Count / Capacity : %d / %d", this->_JobMessagePool.GetUseCount(), this->_JobMessagePool.GetCapacityCount());
                    this->MonitoringLog(logStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
                    wsprintf(logStr, L"|----------------------------------------USAGE_MONITER---------------------------------");
                    this->MonitoringLog(logStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
                    wsprintf(logStr, L"| Available / NPPool / Private Mem : %I64uMb / %I64uMb / %I64uKb", this->_HardWareMoniter.AvailableMemoryMBytes(), this->_HardWareMoniter.NonPagedPoolMBytes(), this->_ProcessMoniter.PrivateMemoryKBytes());
                    this->MonitoringLog(logStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
                    wsprintf(logStr, L"---------------------------------------------------------------------------------------");
                    this->MonitoringLog(logStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
                }
                else
                {
                    wsprintf(logStr, L"Available / NPPool / Private Mem : %I64uMb / %I64uMb / %I64uKb", this->_HardWareMoniter.AvailableMemoryMBytes(), this->_HardWareMoniter.NonPagedPoolMBytes(), this->_ProcessMoniter.PrivateMemoryKBytes());
                    this->MonitoringLog(logStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
                }
            }

            prev = cur;
            ::printf("\n-------------------------------------MONITERING----------------------------------------\n");
            ::printf("| After Server On : %u day / %u h / %u m / %u s\n", day, hour, minute, sec);
            ::printf("|----------------------------------------THREAD----------------------------------------\n");
            ::printf("| Worker Thread / Running  thread : %u / %u\n", info._WorkerThreadCount, info._RunningThreadCount);
            ::printf("|----------------------------------------TOTAL-----------------------------------------\n");
            ::printf("| Total Accept / Release : %llu /  %llu\n", info._TotalAcceptSessionCount, info._TotalReleaseSessionCount);
            ::printf("| Total Processed Bytes : %llu\n", info._TotalProcessedBytes);
            ::printf("|----------------------------------------TPS-------------------------------------------\n");
            ::printf("| Accept Per Sec / Update TPS : %llu / %llu\n", info._TotalAcceptSessionCount - lastTotalAccept, this->_TotalUpdateCount - lastTotalUpdate);
            ::printf("| TPS// Recv / Send / Total  : %llu / %llu / %llu\n", info._TotalRecvPacketCount - lastTotalRecv, info._TotalSendPacketCount - lastTotalSend, (info._TotalRecvPacketCount + info._TotalSendPacketCount) - (lastTotalRecv + lastTotalSend));
            ::printf("| TPS// Login / Chat / MoveSector : %llu / %llu / %llu\n", loginTPS, chatTPS, sectorMoveTPS);
            ::printf("| LockFreeQueue Size / Capacity / Max : %u / %u / %u\n", info._SessionSendQueueSize, info._SessionSendQueueCapacity, info._SessionSendQueueMax);
            ::printf("|----------------------------------------POOL------------------------------------------\n");
            ::printf("| Packet Chunk Capacity / UseCount : %d / %d\n", Packet::GetCapacityCount(), Packet::GetUseCount());
            ::printf("| Packet ToTal UseCount : %d\n", Packet::GetTotalPacketCount());
            ::printf("|----------------------------------------USER------------------------------------------\n");
            ::printf("| PlayerMap Size / Capacity : %llu / %llu\n", this->_PlayerMap.size(), this->_PlayerMap.bucket_count());
            ::printf("| Player Capacity / UseCount :  / %d / %d\n", this->_PlayerPool.GetCapacityCount(), this->_PlayerPool.GetUseCount());
            ::printf("| Session Count / IDX Capacity / IDX Size : %u / %u / %u\n", info._CurrentSessionCount, info._SessionIndexStackCapacity, info._SessionIndexStackSize);
            ::printf("|----------------------------------------JOBQUEUESIZE----------------------------------\n");
            ::printf("| Job Queue Size / Capacity / Total Count : %d / %d / %d\n", this->_JobQueue.size(), this->_JobQueue.GetCapacityCount(), this->_JobMessagePool.GetTotalUseCount());
            ::printf("| Job Chunk Count / Capacity : %d / %d\n", this->_JobMessagePool.GetUseCount(), this->_JobMessagePool.GetCapacityCount());
            ::printf("|----------------------------------------USAGE_MONITER---------------------------------\n");
            ::printf("| NIC Send / Recv (10KB) : %.1llf / %.1llf\n", this->_HardWareMoniter.EthernetSendKBytes(), this->_HardWareMoniter.EthernetRecvKBytes());
            ::printf("| Available / NPPool / Private Mem : %lluMb / %lluMb / %lluKb\n", this->_HardWareMoniter.AvailableMemoryMBytes(), this->_HardWareMoniter.NonPagedPoolMBytes(), this->_ProcessMoniter.PrivateMemoryKBytes());
            ::printf("| PROCESS / CPU : [T %.1llf%% K %.1llf%% U %.1llf%%] / [T %.1llf%% K %.1llf%% U %.1llf%%]   \n", this->_ProcessMoniter.ProcessTotal(), this->_ProcessMoniter.ProcessKernel(), this->_ProcessMoniter.ProcessUser(), this->_HardWareMoniter.ProcessorTotal(), this->_HardWareMoniter.ProcessorKernel(), this->_HardWareMoniter.ProcessorUser());
            ::printf("---------------------------------------------------------------------------------------\n");


            Packet* packet[50];

            int curTime = ::time(nullptr);
            for (int i = 1; i < CHAT_SERVER_MONITERING_TYPE::C_MAX; i++)
                packet[i] = Packet::Alloc();

            // OnOff 플래그
            this->MakePacketMoniteringInfo(packet[CHAT_SERVER_MONITERING_TYPE::C_ON_OFF], 
                this->ServerType, CHAT_SERVER_MONITERING_TYPE::C_ON_OFF, 
                true, curTime);
            // CPU 사용량 
            this->MakePacketMoniteringInfo(packet[CHAT_SERVER_MONITERING_TYPE::C_CPU_USAGE], 
                this->ServerType, CHAT_SERVER_MONITERING_TYPE::C_CPU_USAGE, 
                this->_ProcessMoniter.ProcessTotal(), curTime);
            // 메모리 사용량
            this->MakePacketMoniteringInfo(packet[CHAT_SERVER_MONITERING_TYPE::C_PRIVATE_BYTES],
                this->ServerType, CHAT_SERVER_MONITERING_TYPE::C_PRIVATE_BYTES,
                this->_ProcessMoniter.PrivateMemoryMBytes(), curTime);
            // 세션수
            this->MakePacketMoniteringInfo(packet[CHAT_SERVER_MONITERING_TYPE::C_SESSION_COUNTS],
                this->ServerType, CHAT_SERVER_MONITERING_TYPE::C_SESSION_COUNTS, 
                info._CurrentSessionCount, curTime);
            // 플레이어수
            this->MakePacketMoniteringInfo(packet[CHAT_SERVER_MONITERING_TYPE::C_PLAYER_COUNTS],
                this->ServerType, CHAT_SERVER_MONITERING_TYPE::C_PLAYER_COUNTS, 
                this->_PlayerMap.size(), curTime);
            // 업데이트 루프수
            this->MakePacketMoniteringInfo(packet[CHAT_SERVER_MONITERING_TYPE::C_UPDATE_TPS], 
                this->ServerType, CHAT_SERVER_MONITERING_TYPE::C_UPDATE_TPS, 
                this->_TotalUpdateCount - lastTotalUpdate, curTime);
            // 패킷풀 사용량
            this->MakePacketMoniteringInfo(packet[CHAT_SERVER_MONITERING_TYPE::C_PACKET_POOL_USAGE], 
                this->ServerType, CHAT_SERVER_MONITERING_TYPE::C_PACKET_POOL_USAGE, 
                Packet::GetTotalPacketCount(), curTime);
            // 업데이트 큐 사이즈
            this->MakePacketMoniteringInfo(packet[CHAT_SERVER_MONITERING_TYPE::C_UPDATE_MSG_QUEUE_SIZE], 
                this->ServerType, CHAT_SERVER_MONITERING_TYPE::C_UPDATE_MSG_QUEUE_SIZE, 
                this->_JobQueue.size(), curTime);
            // 모조리 보내기
            for (int i = 1; i < CHAT_SERVER_MONITERING_TYPE::C_MAX; i++)
                this->SendToMoniteringSession(packet[i]);

            lastTotalRecv = info._TotalRecvPacketCount;
            lastTotalSend = info._TotalSendPacketCount;
            lastTotalAccept = info._TotalAcceptSessionCount;
            lastTotalRelease = info._TotalReleaseSessionCount;
            lastTotalUpdate = this->_TotalUpdateCount;
        }
        this->_ChatServerLog.LOG(L"UpdateThread Not Normally End...", LogClass::LogLevel::LOG_LEVEL_SYSTEM);
        return -1;
    }

    void ChatServer::PacketProc(Packet* packet, ULONGLONG sessionID,WORD type)
    {
        WCHAR errStr[512];
        switch (type)
        {
            case PACKET_TYPE::en_PACKET_CS_CHAT_REQ_LOGIN:
            {
                this->PacketProcRequestLogin(packet, sessionID);
                break;
            }
            case PACKET_TYPE::en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
            {
                this->PacketProcMoveSector(packet, sessionID);
                break;
            }
            case PACKET_TYPE::en_PACKET_CS_CHAT_REQ_MESSAGE:
            {
                this->PacketProcChatRequire(packet, sessionID);
                break;
            }
            case PACKET_TYPE::en_ON_CLIENT_LEAVE:
            {
                this->RemovePlayer(sessionID);
                break;
            }
            case PACKET_TYPE::en_ON_TIME_OUT:
            {
                ::wsprintf(errStr, L"Time Out Error SessionID : %I64u", sessionID);
                this->_ChatServerLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
                this->DisconnectSession(sessionID);
                break;
            }
            case PACKET_TYPE::en_PACKET_CS_CHAT_REQ_HEARTBEAT:
            {
                this->PacketProcHeartBeating(packet, sessionID);
                break;
            }
            default:
            {
                ::wsprintf(errStr, L"Session Default Case SessionID : %I64u", sessionID);
                this->_ChatServerLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_ERROR);
                this->DisconnectSession(sessionID);
                break;
            }
        }
    }

    void ChatServer::PacketProcRequestLogin(Packet* packet, ULONGLONG sessionID)
    {
        WCHAR errStr[512];
        BYTE status = false;
        INT64 accountNo;

        if (packet->GetBufferSize() < sizeof(accountNo))
        {
            ::wsprintf(errStr, L"PacketProcRequestLogin Packet BufferSize < sizeof(accountNo) SessionID : %I64u", sessionID);
            this->_ChatServerLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_ERROR);
            this->DisconnectSession(sessionID);
            return;
        }
        (*packet) >> accountNo;
        if (packet->GetBufferSize() != this->ID_MAX_SIZE + this->NICK_NAME_MAX_SIZE + this->TOKEN_KEY_SIZE)
        {
            ::wsprintf(errStr, L"PacketProcRequestLogin Packet Available Packet Length is not enough SessionID : %I64u", sessionID);
            this->_ChatServerLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_ERROR);
            this->DisconnectSession(sessionID);
            return;
        }

        Player* player = this->FindPlayer(sessionID);
        if (player == nullptr)
        {
            //Player 생성 및 초기화
            player = this->_PlayerPool.Alloc();
            player->_AccountNo = accountNo;
            packet->GetBuffer((char*)player->_ID, this->ID_MAX_SIZE);
            packet->GetBuffer((char*)player->_NickName, this->NICK_NAME_MAX_SIZE );
            packet->GetBuffer(player->_TokenKey, this->TOKEN_KEY_SIZE);
            player->_SectorX = -1;
            player->_SectorY = -1;
            player->_SessionID = sessionID;
            player->_Logined = true;
            status = true;
            this->InsertPlayer(sessionID, player);
            player->_LastAction = 1;
        }

        Packet* resLoginPacket = Packet::Alloc();
        this->MakePacketResponseLogin(resLoginPacket, accountNo, status);

        this->SendPacket(sessionID, resLoginPacket);
        //login 실패시 보내고 끊기
        if (status == false)
        {
            this->DispatchError(sessionID, 20202020, L"Login Request -> player is not nullptr");
            this->DisconnectSession(sessionID);
        }
        ::InterlockedIncrement(&_LoginTPS);
    }

    void ChatServer::PacketProcMoveSector(Packet* packet, ULONGLONG sessionID)
    {
        WCHAR errStr[512];
        INT64 accountNo;
        WORD sectorX;
        WORD sectorY;
        
        if (packet->GetBufferSize() != (sizeof(accountNo) + sizeof(sectorX) + sizeof(sectorY)))
        {
            ::wsprintf(errStr, L"PacketProcMoveSector Packet Length is not enough SessionID : %I64u", sessionID);
            this->_ChatServerLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_ERROR);
            this->DisconnectSession(sessionID);
            return;
        }
        
        (*packet) >> accountNo >> sectorX >> sectorY;

        Player* player = this->FindPlayer(sessionID);
        //플레이어가 만들어지지도 않았는데 move sector 들어오는케이스
        if (player == nullptr)
        {
            ::wsprintf(errStr, L"PacketProcMoveSector FindPlayer returned nullptr SessionID : %I64u", sessionID);
            this->_ChatServerLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_ERROR);
            this->DisconnectSession(sessionID);
            return;
        }
        //로그인을 안했거나 기록된 AccountNum이 잘못된 경우
        else if (player->_Logined == false)
        {
            ::wsprintf(errStr, L"PacketProcMoveSector Player Already Logined SessionID : %I64u", sessionID);
            this->_ChatServerLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_ERROR);
            this->DisconnectSession(sessionID);
            return;
        }
        else if (accountNo != player->_AccountNo)
        {
            ::wsprintf(errStr, L"PacketProcMoveSector Player AccountNo is wrong SessionID : %I64u", sessionID);
            this->_ChatServerLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_ERROR);
            this->DisconnectSession(sessionID);
            return;
        }
        //날라온 섹터의 값이 50이상일경우
        else if (sectorX >= 50 || sectorY >= 50)
        {
            ::wsprintf(errStr, L"PacketProcMoveSector Player Sector X : %d, Sector Y : %d SessionID : %I64u", sectorX, sectorY, sessionID);
            this->_ChatServerLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_ERROR);
            this->DisconnectSession(sessionID);
            return;
        }

        constexpr WORD comp = -1;
        if (player->_SectorX != comp && player->_SectorY != comp)
        {
            auto iter = this->_Sector[player->_SectorY][player->_SectorX].find(player);
            if (iter != this->_Sector[player->_SectorY][player->_SectorX].end())
                this->_Sector[player->_SectorY][player->_SectorX].erase(iter);
        }
        player->_SectorX = sectorX;
        player->_SectorY = sectorY;

        this->_Sector[player->_SectorY][player->_SectorX].emplace(player);
        player->_LastAction = 2;
        Packet* moveSectorPacket = Packet::Alloc();
        this->MakePacketResponseMoveSector(moveSectorPacket, player->_AccountNo, player->_SectorX, player->_SectorY);
        this->SendPacket(sessionID, moveSectorPacket);
        ::InterlockedIncrement(&this->_SectorMoveTPS);
    }

    void ChatServer::PacketProcChatRequire(Packet* packet, ULONGLONG sessionID)
    {
        WCHAR errStr[512];
        INT64 accountNo;
        WORD messageLen;
        WCHAR message[512];

        if (packet->GetBufferSize() < sizeof(accountNo) + sizeof(messageLen))
        {
            ::wsprintf(errStr, L"PacketProcChatRequire Packet BufferSize is not enough SessionID : %I64u", sessionID);
            this->_ChatServerLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_ERROR);
            this->DisconnectSession(sessionID);
            return;
        }
        (*packet) >> accountNo >> messageLen;

        if (packet->GetBufferSize() != messageLen)
        {
            ::wsprintf(errStr, L"PacketProcChatRequire Packet BufferSize is not MessageLen SessionID : %I64u", sessionID);
            this->_ChatServerLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_ERROR);
            this->DisconnectSession(sessionID);
            return;
        }
        packet->GetBuffer((char*)message, messageLen);

        Player* player = this->FindPlayer(sessionID);
        if (player == nullptr)
        {
            ::wsprintf(errStr, L"PacketProcChatRequire FindPlayer returned nullptr SessionID : %I64u", sessionID);
            this->_ChatServerLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_ERROR);
            this->DisconnectSession(sessionID);
            return;
        }
        else if (player->_Logined == false)
        {
            ::wsprintf(errStr, L"PacketProcChatRequire Player Already Logined SessionID : %I64u", sessionID);
            this->_ChatServerLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_ERROR);
            this->DisconnectSession(sessionID);
            return;
        }
        else if (player->_AccountNo != accountNo)
        {
            ::wsprintf(errStr, L"PacketProcChatRequire Player AccountNo is wrong SessionID : %I64u", sessionID);
            this->_ChatServerLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_ERROR);
            this->DisconnectSession(sessionID);
        }
        else if (player->_SectorX >= 50 || player->_SectorY >= 50)
        {
            ::wsprintf(errStr, L"PacketProcChatRequire Player Sector X : %d, Sector Y : %d SessionID : %I64u", player->_SectorX, player->_SectorY, sessionID);
            this->_ChatServerLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_ERROR);
            this->DisconnectSession(sessionID);
            return;
        }
        int beginX = player->_SectorX - 1;
        int beginY = player->_SectorY - 1;

        message[messageLen / 2] = '\0';

        Packet* messagePacket = Packet::Alloc();
        this->MakePacketResponseMessage(messagePacket, player->_AccountNo, player->_ID, player->_NickName, messageLen, message);

        //if (wcscmp(L"=", message) == 0)
        //{
        //    this->_LeaveSessionCount++;
        //}

        messagePacket->AddRef();
        player->_LastAction = 3;
        for (int y = 0; y < 3; y++)
        {
            if (((beginY + y) < 0) || (beginY + y) >= 50) continue;
            for (int x = 0; x < 3; x++)
            {
                if (((beginX + x) < 0) || ((beginX + x) >= 50)) continue;

                for (auto iter = this->_Sector[beginY + y][beginX + x].begin(); iter != this->_Sector[beginY + y][beginX + x].end(); ++iter)
                {
                    Player* tempPlayer = *iter;
                    this->SendPacket(tempPlayer->_SessionID, messagePacket);
                    ::InterlockedIncrement(&this->_ChatTPS);
                }
            }
        }
        Packet::Free(messagePacket);
    }

    void ChatServer::PacketProcHeartBeating(Packet* packet, ULONGLONG sessionID)
    {
        Player* player = this->FindPlayer(sessionID);
        if (player == nullptr)
        {
            this->DisconnectSession(sessionID);
            return;
        }
    }


    void ChatServer::RemovePlayer(ULONGLONG sessionID)
    {
        auto iter = this->_PlayerMap.find(sessionID);

        if (iter == this->_PlayerMap.end())
        {
            this->DisconnectSession(sessionID);
            return;
        }
        Player* removePlayer = iter->second;

        constexpr WORD comp = -1;
        WORD sectorX = removePlayer->_SectorX;
        WORD sectorY = removePlayer->_SectorY;
        do
        {
            removePlayer->_Logined = false;
            if (sectorX == comp || sectorY == comp)
                break;
            for (auto iter = _Sector[sectorY][sectorX].begin(); iter != _Sector[sectorY][sectorX].end(); ++iter)
            {
                if ((*iter)->_SessionID == sessionID)
                {
                    _Sector[sectorY][sectorX].erase(iter);
                    break;
                }
            }
        } while (0);
        this->_PlayerMap.erase(sessionID);
        this->_PlayerPool.Free(removePlayer);
    }


    unsigned __stdcall UpdateThread(void* param)
    {
        if (param == nullptr)
            return -1;
        ChatServer* server = (ChatServer*)param;
        printf("Update\n");
        return server->ChatServerUpdateThread(server);
    }

    unsigned __stdcall MonitoringThread(void* param)
    {
        if (param == nullptr)
            return -1;
        ChatServer* server = (ChatServer*)param;
        printf("Monitoring\n");
        return server->ChatServerMonitoringThread(server);
    }





    void ChatServer::OnRecv(ULONGLONG sessionID, Packet* recvPacket)
    {
        JobMessage job;
        job._SessionID = sessionID;
        job._Packet = recvPacket;
        *(job._Packet) >> job._Type;
        this->_JobQueue.enqueue(job);
        SetEvent(this->_DequeueEvent);
    }

    void ChatServer::OnErrorOccured(DWORD errorCode, const WCHAR* error)
    {
        this->_ChatServerLog.LOG(error, LogClass::LogLevel::LOG_LEVEL_LIBRARY);
    }

    bool ChatServer::OnConnectionRequest(WCHAR* ipStr, DWORD ip, USHORT port)
    {
        return this->_RunningFlag;
    }

    void ChatServer::OnClientJoin(WCHAR* ipStr, DWORD ip, USHORT port, ULONGLONG sessionID)
    {
        // 만약 라이브러리에 TimeOut 체크 기능이 없다면 이 구간에서 체크 시작을 해줘야됨.
    }

    void ChatServer::OnClientLeave(ULONGLONG sessionID)
    {
        JobMessage message;
        message._SessionID = sessionID;
        message._Type = PACKET_TYPE::en_ON_CLIENT_LEAVE;
        message._Packet = nullptr;
        this->_JobQueue.enqueue(message);
        SetEvent(this->_DequeueEvent);
    }

    void ChatServer::OnTimeOut(ULONGLONG sessionID)
    {
        JobMessage timeOutJob;
        timeOutJob._SessionID = sessionID;
        timeOutJob._Type = PACKET_TYPE::en_ON_TIME_OUT;
        timeOutJob._Packet = nullptr;
        this->_JobQueue.enqueue(timeOutJob);
        SetEvent(this->_DequeueEvent);
    }

    void ChatServer::Start()
    {
        WCHAR errStr[512];
        int _ = _wmkdir(L"ServerLog");
        _ = _wmkdir(L"ServerLog\\ChatServerLog");

        this->_ChatServerLog.LOG_SET_DIRECTORY(L"ServerLog\\ChatServerLog");
        this->_ChatServerLog.LOG_SET_LEVEL(LogClass::LogLevel::LOG_LEVEL_SYSTEM);

        this->_ChatServerLog.LOG(L"--------------- Chatting Server Start -----------------", LogClass::LogLevel::LOG_LEVEL_SYSTEM);
        if (!GetNetCoreInitializeFlag())
        {
            DWORD coreErr = this->GetLastCoreErrno();
            DWORD lastErr = this->GetLastAPIErrno();
            wsprintf(errStr, L"NetCore Initialize Failed, API errno : %u / NetCore errno : %u", lastErr, coreErr);
            this->_ChatServerLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);

            printf("API Error : %d\nCore Error : %d\n", lastErr, coreErr);
            this->_RunningFlag = false;
            return;
        }
        this->_ChatServerLog.LOG(L"NetCore Initialized Success", LogClass::LogLevel::LOG_LEVEL_SYSTEM);


        this->_Sector = new std::unordered_set<Player*>*[SECTOR_Y_SIZE];
        for (int i = 0; i < SECTOR_Y_SIZE; i++)
        {
            this->_Sector[i] = new std::unordered_set<Player*>[SECTOR_X_SIZE];
        }
        this->_ChatServerLog.LOG(L"Sector Created", LogClass::LogLevel::LOG_LEVEL_SYSTEM);


        this->_MonitoringThread = nullptr;
        this->_MonitoringThread = (HANDLE)_beginthreadex(nullptr, 0, MonitoringThread, this, 0, nullptr);
        if (this->_MonitoringThread == nullptr)
        {
            this->_RunningFlag = false;
            this->_ChatServerLog.LOG(L"ChatServer::MonitoringThread Create Failed", LogClass::LogLevel::LOG_LEVEL_ERROR);
            return;
        }
        this->_ChatServerLog.LOG(L"ChatServer::MonitoringThread Created Success", LogClass::LogLevel::LOG_LEVEL_SYSTEM);

        this->_UpdateThread = nullptr;
        this->_UpdateThread = (HANDLE)_beginthreadex(nullptr, 0, UpdateThread, this, 0, nullptr);
        if (this->_UpdateThread == nullptr)
        {
            this->_RunningFlag = false;
            this->_ChatServerLog.LOG(L"ChatServer::UpdateThread Created Failed", LogClass::LogLevel::LOG_LEVEL_ERROR);
            return;
        }
        this->_ChatServerLog.LOG(L"ChatServer::UpdateThread Created Success", LogClass::LogLevel::LOG_LEVEL_SYSTEM);

        this->_DequeueEvent = nullptr;
        this->_DequeueEvent = CreateEvent(nullptr, false, false, nullptr);
        if (this->_DequeueEvent == nullptr)
        {
            this->_RunningFlag = false;
            this->_ChatServerLog.LOG(L"ChatServer::Job Dequeue Event Created Failed", LogClass::LogLevel::LOG_LEVEL_ERROR);
            return;
        }
        this->_ChatServerLog.LOG(L"ChatServer::Job Dequeue Event Created Success", LogClass::LogLevel::LOG_LEVEL_SYSTEM);


        HANDLE threads[2]{ _MonitoringThread,_UpdateThread };
        this->_RunningFlag = true;
        this->_ChatServerLog.LOG(L"WaitFor Server Off...", LogClass::LogLevel::LOG_LEVEL_SYSTEM);
        this->CNetServer::Run(threads, 2);
        this->_ChatServerLog.LOG(L"Server Stop", LogClass::LogLevel::LOG_LEVEL_SYSTEM);
    }

    void ChatServer::Close()
    {
        for (int i = 0; i < SECTOR_Y_SIZE; i++)
            delete[] _Sector[i];
        delete[] _Sector;
        this->_ChatServerLog.LOG(L"---------------------- Server Off ---------------------", LogClass::LogLevel::LOG_LEVEL_SYSTEM);
    }

    ChatServer::ChatServer()
        : CNetServer(), _RunningFlag(false), _UpdateThread(nullptr), _LoginTPS(0),_SectorMoveTPS(0),_ChatTPS(0),_DequeueEvent(nullptr),_LeaveSessionCount(0),_MonitoringThread(nullptr),_Sector(nullptr)
    {
    }
    ChatServer::~ChatServer()
    {
        this->Close();
    }
    //USHORT port, DWORD backlogQueueSize, DWORD threadPoolSize, DWORD runningThread, DWORD nagleOff, ULONGLONG maxSessionCounts
    void ChatServer::ChatServerInit()
    {
        do
        {
            if (!g_ConfigReader.SetCurrentSection(L"NetServerConfig")) break;
            if (!g_ConfigReader.Find(L"Port")) break;
            if (!g_ConfigReader.Find(L"BacklogQueue"))break;
            if (!g_ConfigReader.Find(L"WorkerThread"))break;
            if (!g_ConfigReader.Find(L"RunningThread"))break;
            if (!g_ConfigReader.Find(L"NagleOff"))break;
            if (!g_ConfigReader.Find(L"MaxSessionCount"))break;
            if (!g_ConfigReader.Find(L"TimeOutClock"))break;

            USHORT port = g_ConfigReader.Get(L"Port", 1);
            DWORD backlogQueueSize = g_ConfigReader.Get(L"BacklogQueue", 1);
            DWORD workerThreadCount = g_ConfigReader.Get(L"WorkerThread", 1);
            DWORD runningThread = g_ConfigReader.Get(L"RunningThread", 1);
            DWORD nagleOffOption = g_ConfigReader.Get(L"NagleOff", 1);
            DWORD maxSessionCount = g_ConfigReader.Get(L"MaxSessionCount", 1);
            DWORD timeOutTimer = g_ConfigReader.Get(L"TimeOutClock", 1);

            printf("Port : %u\n", port);
            printf("BacklogQueue : %u\n", backlogQueueSize);
            printf("workerThreadCount : %u\n", workerThreadCount);
            printf("runningThread : %u\n", runningThread);
            printf("nagleOffOption : %u\n", nagleOffOption);
            printf("Port : %u\n", maxSessionCount);
            printf("timeOutTimer : %u\n", timeOutTimer);
            system("pause");

            this->InitNetServer(port, backlogQueueSize, workerThreadCount, runningThread, nagleOffOption, maxSessionCount,timeOutTimer);
            this->Start();
            return;
        } while (0);
        CRASH();
    }
}




