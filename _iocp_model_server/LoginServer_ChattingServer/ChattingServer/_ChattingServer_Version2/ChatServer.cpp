#include "ChatServer.h"
#include <process.h>
#include <conio.h>
#include <time.h>
#include <stdio.h>
#include <iostream>

namespace univ_dev
{
    unsigned int ChatServer::ChatServerMoniteringThread(void* param)
    {
        DWORD begin = timeGetTime();
        DWORD prev;
        DWORD cur;
        DWORD simpleLogTimer;
        DWORD longLogTimer;
        longLogTimer = simpleLogTimer = cur = prev = begin;
        while (this->_RunningFlag)
        {
            if (_kbhit())
            {
                int key = _getch();
                if (toupper(key) == 'Q')
                {
                    this->_RunningFlag = false;
                    this->CNetServer::PostNetServerStop();
                    SetEvent(_RedisEvent);
                    WaitForSingleObject(_RedisThread, INFINITE);
                    return 0;
                }
                else if (toupper(key) == 'S')
                {
                    SaveProfiling();
                    ResetProfiling();
                }
            }

            cur = timeGetTime();
            if (cur - prev < 1000)
            {
                Sleep(100);
                continue;
            }

            MoniteringInfo info = this->CNetServer::GetMoniteringInfo();
            this->_HardWareMoniter.UpdateHardWareTime();
            this->_ProcessMoniter.UpdateProcessTime();

            ULONGLONG loginTPS = InterlockedExchange(&this->_LoginTPS, 0);
            ULONGLONG chatTPS = InterlockedExchange(&this->_ChatTPS, 0);
            ULONGLONG sectorMoveTPS = InterlockedExchange(&this->_SectorMoveTPS, 0);

            int tempSize;
            int tempArr[20]{ 0 };
            unsigned long long sectorSize = 0;
            unsigned long long sectorCapacity = 0;
            DWORD afterServerOn = (timeGetTime() - begin) / 1000;

            for (int i = 0; i < this->SECTOR_Y_SIZE; i++)
            {
                for (int j = 0; j < this->SECTOR_X_SIZE; j++)
                {
                    tempSize = this->_Sector[i][j]._PlayerSet.size();
                    sectorSize += tempSize;
                    sectorCapacity += this->_Sector[i][j]._PlayerSet.bucket_count();
                    if (tempSize < 0 || tempSize >= 14)
                        continue;
                    tempArr[tempSize]++;
                }
            }
            DWORD day = afterServerOn / 86400;
            DWORD hour = (afterServerOn % 86400) / 3600;
            DWORD minute = (afterServerOn % 3600) / 60;
            DWORD sec = afterServerOn % 60;
            if (cur - simpleLogTimer > 60000)
            {
                simpleLogTimer = cur;
                FILE* file = nullptr;
                while (file == nullptr)
                    fopen_s(&file, "__HardWareMemoryLog.txt", "ab");
                if (cur - longLogTimer > 300000)
                {
                    longLogTimer = cur;
                    fprintf(file, "\n-------------------------------------MONITERING----------------------------------------\n");
                    fprintf(file, "| After Server On : %u day / %u h / %u m / %u s\n", day, hour, minute, sec);
                    fprintf(file, "|----------------------------------------POOL------------------------------------------\n");
                    fprintf(file, "| LockFreeQueue Size / Capacity / Max : %llu / %llu / %llu\n", info._LockFreeQueueSize, info._LockFreeQueueCapacity, info._LockFreeMaxCapacity);
                    fprintf(file, "| Packet Chunk Capacity / UseCount : %d / %d\n", Packet::GetCapacityCount(), Packet::GetUseCount());
                    fprintf(file, "| Packet ToTal UseCount : %d\n", Packet::GetTotalPacketCount());
                    fprintf(file, "|----------------------------------------USER------------------------------------------\n");
                    fprintf(file, "| PlayerMap Size / Capacity : %llu / %llu\n", this->_PlayerMap.size(), this->_PlayerMap.bucket_count());
                    fprintf(file, "| Sector Size / Capacity : %llu / %llu\n", sectorSize, sectorCapacity);
                    fprintf(file, "| Player Capacity / UseCount :  / %d / %d\n", this->_PlayerPool.GetCapacityCount(), this->_PlayerPool.GetUseCount());
                    fprintf(file, "| Session Count / IDX Capacity / IDX Size : %llu / %llu / %llu\n", info._SessionCnt, info._LockFreeStackCapacity, info._LockFreeStackSize);
                    fprintf(file, "|----------------------------------------JOBQUEUESIZE----------------------------------\n");
                    fprintf(file, "| Redis Job Queue Size / Capacity : %d / %d\n", this->_RedisJobQueue.size(), this->_RedisJobQueue.GetCapacityCount());
                    fprintf(file, "|----------------------------------------USAGE_MONITER---------------------------------\n");
                    fprintf(file, "| Available / NPPool / Private Mem : %lluMb / %lluMb / %lluKb\n", this->_HardWareMoniter.AvailableMemoryMBytes(), this->_HardWareMoniter.NonPagedPoolMBytes(), this->_ProcessMoniter.PrivateMemoryKBytes());
                    fprintf(file, "---------------------------------------------------------------------------------------\n");
                }
                else
                    fprintf(file, "Available / NPPool / Private Mem : %lluMb / %lluMb / %lluKb\n", this->_HardWareMoniter.AvailableMemoryMBytes(), this->_HardWareMoniter.NonPagedPoolMBytes(), this->_ProcessMoniter.PrivateMemoryKBytes());

                fclose(file);
            }

            prev = cur;

            printf("\n-------------------------------------MONITERING----------------------------------------\n");
            printf("| After Server On : %u day / %u h / %u m / %u s\n", day, hour, minute, sec);
            printf("|----------------------------------------THREAD----------------------------------------\n");
            printf("| Worker Thread / Running  thread : %u / %u\n", info._WorkerThreadCount, info._RunningThreadCount);
            printf("|----------------------------------------TOTAL-----------------------------------------\n");
            printf("| Total Accept / Release / '=' Recved : %llu /  %llu / %llu\n", info._TotalAcceptSession, info._TotalReleaseSession, _LeaveSessionCount);
            printf("| Total Processed Bytes : %llu\n", info._TotalProecessedBytes);
            printf("|----------------------------------------TPS-------------------------------------------\n");
            printf("| Accept Per Sec : %llu\n", info._AccpeptCount);
            printf("| TPS// Recv / Send / Total  : %llu / %llu / %llu\n", info._RecvPacketCount, info._SendPacketCount, info._SendPacketCount + info._RecvPacketCount);
            printf("| TPS// Login / Chat / MoveSector : %llu / %llu / %llu\n", loginTPS, chatTPS, sectorMoveTPS);
            printf("| LockFreeQueue Size / Capacity / Max : %llu / %llu / %llu\n", info._LockFreeQueueSize, info._LockFreeQueueCapacity, info._LockFreeMaxCapacity);
            printf("|----------------------------------------POOL------------------------------------------\n");
            printf("| Packet Chunk Capacity / UseCount : %d / %d\n", Packet::GetCapacityCount(), Packet::GetUseCount());
            printf("| Packet ToTal UseCount : %d\n", Packet::GetTotalPacketCount());
            printf("| Redis Job Queue Size / Capacity : %d / %d\n", this->_RedisJobQueue.size(), this->_RedisJobQueue.GetCapacityCount());
            printf("|----------------------------------------USER------------------------------------------\n");
            printf("| PlayerMap Size / Capacity : %llu / %llu\n", this->_PlayerMap.size(), this->_PlayerMap.bucket_count());
            printf("| 0 %d/ 1 %d/ 2 %d/ 3 %d/ 4 %d/ 5 %d/ 6 %d/ 7 %d/ 8 %d/ 9 %d/ 10 %d/ 11 %d/ 12 %d/ 13 %d\n", 
                tempArr[0], tempArr[1], tempArr[2], tempArr[3], tempArr[4], tempArr[5], tempArr[6], tempArr[7], tempArr[8], tempArr[9], tempArr[10], tempArr[11], tempArr[12], tempArr[13]);
            printf("| Sector Size / Capacity : %llu / %llu\n", sectorSize, sectorCapacity);
            printf("| Player Capacity / UseCount :  / %d / %d\n", this->_PlayerPool.GetCapacityCount(), this->_PlayerPool.GetUseCount());
            printf("| Session Count / IDX Capacity / IDX Size : %llu / %llu / %llu\n", info._SessionCnt, info._LockFreeStackCapacity,info._LockFreeStackSize);
            printf("|----------------------------------------USAGE_MONITER---------------------------------\n");
            printf("| NIC Send / Recv (10KB) : %.1llf / %.1llf\n", this->_HardWareMoniter.EthernetSendKBytes(), this->_HardWareMoniter.EthernetRecvKBytes());
            printf("| Available / NPPool / Private Mem : %lluMb / %lluMb / %lluKb\n", this->_HardWareMoniter.AvailableMemoryMBytes(), this->_HardWareMoniter.NonPagedPoolMBytes(), this->_ProcessMoniter.PrivateMemoryKBytes());
            printf("| PROCESS / CPU : [T %.1llf%% K %.1llf%% U %.1llf%%] / [T %.1llf%% K %.1llf%% U %.1llf%%]   \n", this->_ProcessMoniter.ProcessTotal(), this->_ProcessMoniter.ProcessKernel(), this->_ProcessMoniter.ProcessUser(), this->_HardWareMoniter.ProcessorTotal(), this->_HardWareMoniter.ProcessorKernel(), this->_HardWareMoniter.ProcessorUser());
            printf("---------------------------------------------------------------------------------------\n");
        }
        return -1;
    }





    void ChatServer::PacketProc(Packet* packet, ULONGLONG sessionID,WORD type)
    {
        WCHAR errStr[512];
        switch (type)
        {
            case CHAT_PACKET_TYPE::PACKET_CS_CHAT_REQ_LOGIN:
            {
                this->PacketProcRequestLogin(packet, sessionID);
                break;
            }
            case CHAT_PACKET_TYPE::PACKET_CS_CHAT_REQ_SECTOR_MOVE:
            {
                this->PacketProcMoveSector(packet, sessionID);
                break;
            }
            case CHAT_PACKET_TYPE::PACKET_CS_CHAT_REQ_MESSAGE:
            {
                this->PacketProcChatRequire(packet, sessionID);
                break;
            }
            case CHAT_PACKET_TYPE::ON_CLIENT_LEAVE:
            {
                this->RemovePlayer(sessionID);
                break;
            }
            case CHAT_PACKET_TYPE::ON_TIME_OUT:
            {
                wsprintf(errStr, L"Time Out Error SessionID : %I64u", sessionID);
                this->_ChatServerLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
                this->DisconnectSession(sessionID);
                break;
            }
            default:
            {
                wsprintf(errStr, L"Session Default Case SessionID : %I64u", sessionID);
                this->_ChatServerLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_ERROR);
                this->DisconnectSession(sessionID);
                break;
            }
        }
    }


    unsigned int ChatServer::ChatServerRedisThread(void* param)
    {
        WCHAR errStr[512];
        RedisJob job;
        char tokenKeyStr[65];
        BYTE status;
        std::chrono::duration<double> duration(0.2);
        while(this->_RunningFlag)
        {
            status = false;
            WaitForSingleObject(this->_RedisEvent, INFINITE);

            while (this->_RedisJobQueue.dequeue(job))
            {
                //레디스 잡에서 하나씩 꺼내옴
                cpp_redis::reply tokenKey = job._RedisReply;
                INT64 accountNo = job._AccountNo;
                ULONGLONG sessionID = job._SessionID;

                cpp_redis::reply::type retType = tokenKey.get_type();

                switch (retType)
                {
                    case cpp_redis::reply::type::bulk_string:
                    case cpp_redis::reply::type::simple_string:
                    {
                        memcpy_s(tokenKeyStr, this->TOKEN_KEY_SIZE, tokenKey.as_string().c_str(), this->TOKEN_KEY_SIZE);
                        tokenKeyStr[this->TOKEN_KEY_SIZE] = 0;
                        this->_RedisClient.del({ std::move(std::to_string(accountNo) + ".chat") });
                        this->_RedisClient.sync_commit(duration);
                        status = true;
                        break;
                    }
                    default:
                    {
                        wsprintf(errStr, L"AccountNo(%I64u) tokenKey is not valid tokenKey Type : %d SessionID : %I64u", accountNo, retType, sessionID);
                        this->_ChatServerLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_ERROR);
                    }
                }

                Packet* resLoginPacket = Packet::Alloc();
                this->MakePacketResponseLogin(resLoginPacket, accountNo, status);

                this->SendPacket(sessionID, resLoginPacket);
                if (status == false)
                {
                    wsprintf(errStr, L"PacketProcRequestLogin Login Failed status is false SessionID : %I64u", sessionID);
                    this->_ChatServerLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_ERROR);
                    this->DisconnectSession(sessionID);
                }
                InterlockedIncrement(&this->_LoginTPS);
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
            wsprintf(errStr, L"PacketProcRequestLogin Packet BufferSize < sizeof(accountNo) SessionID : %I64u", sessionID);
            this->_ChatServerLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_ERROR);
            this->DisconnectSession(sessionID);
            return;
        }
        (*packet) >> accountNo;
        if (packet->GetBufferSize() != this->ID_MAX_SIZE + this->NICK_NAME_MAX_SIZE + this->TOKEN_KEY_SIZE)
        {
            wsprintf(errStr, L"PacketProcRequestLogin Packet Available Packet Length is not enough SessionID : %I64u", sessionID);
            this->_ChatServerLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_ERROR);
            this->DisconnectSession(sessionID);
            return;
        }
        
        Player* player;
        AcquireSRWLockExclusive(&this->_PlayerMapLock);
        auto iter = this->_PlayerMap.find(sessionID);
        if (iter == this->_PlayerMap.end())
        {
            //Player 생성 및 초기화
            player = this->_PlayerPool.Alloc();
            //풀에서 받은 플레이어의 로그인이 이미 true다? 이건 중대결함.
            if (player->_Logined == true)
            {
                wsprintf(errStr, L"PacketProcRequestLogin NewPlayer's _Logined field is true SessionID : %I64u", sessionID);
                this->_ChatServerLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_ERROR);
                CRASH();
                return;
            }
            player->_AccountNo = accountNo;
            packet->GetBuffer((char*)player->_ID, this->ID_MAX_SIZE);
            packet->GetBuffer((char*)player->_NickName, this->NICK_NAME_MAX_SIZE);
            packet->GetBuffer(player->_TokenKey, this->TOKEN_KEY_SIZE);
            player->_SectorX = -1;
            player->_SectorY = -1;
            player->_SessionID = sessionID;
            player->_Logined = true;
            status = true;
            this->InsertPlayer(sessionID, player);
        }
        ReleaseSRWLockExclusive(&this->_PlayerMapLock);


        this->_RedisClient.get(std::move(std::to_string(accountNo) + ".chat"), [=](const cpp_redis::reply& reply)
        {
            RedisJob job;
            job._SessionID = sessionID;
            job._AccountNo = accountNo;
            job._RedisReply = reply;
            this->_RedisJobQueue.enqueue(job);
            SetEvent(this->_RedisEvent);
        }).commit();

    }

    void ChatServer::PacketProcMoveSector(Packet* packet, ULONGLONG sessionID)
    {
        WCHAR errStr[512];
        INT64 accountNo;
        WORD sectorX;
        WORD sectorY;

        if (packet->GetBufferSize() != (sizeof(accountNo) + sizeof(sectorX) + sizeof(sectorY)))
        {
            wsprintf(errStr, L"PacketProcMoveSector Packet Length is not enough SessionID : %I64u", sessionID);
            this->_ChatServerLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_ERROR);
            this->DisconnectSession(sessionID);
            return;
        }
        (*packet) >> accountNo >> sectorX >> sectorY;
        Player* player = this->FindPlayer(sessionID);
        // 플레이어가 없는데 무브섹터를 보내면 디스커넥트 대상
        if (player == nullptr)
        {
            wsprintf(errStr, L"PacketProcMoveSector FindPlayer returned nullptr SessionID : %I64u", sessionID);
            this->_ChatServerLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_ERROR);
            this->DisconnectSession(sessionID);
            return;
        }
        // 로그인이 안됬거나 어카운트 넘버가 맞지 않으면 디스커넥트 대상
        else if (player->_Logined == false)
        {
            wsprintf(errStr, L"PacketProcMoveSector Player Already Logined SessionID : %I64u", sessionID);
            this->_ChatServerLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_ERROR);
            this->DisconnectSession(sessionID);
            return;
        }
        else if (accountNo != player->_AccountNo)
        {
            wsprintf(errStr, L"PacketProcMoveSector Player AccountNo is wrong SessionID : %I64u", sessionID);
            this->_ChatServerLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_ERROR);
            this->DisconnectSession(sessionID);
            return;
        }
        // 섹터가 50개밖에 없는데 그 이상이면 디스커넥트 대상(unsigned 라 음수면 아주큰값)
        else if (sectorX >= 50 || sectorY >= 50)
        {
            wsprintf(errStr, L"PacketProcMoveSector Player Sector X : %d, Sector Y : %d SessionID : %I64u", sectorX, sectorY, sessionID);
            this->_ChatServerLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_ERROR);
            this->DisconnectSession(sessionID);
            return;
        }


        constexpr WORD comp = -1;
        //처음 들어온 케이스
        if (player->_SectorX == comp && player->_SectorY == comp)
        {
            AcquireSRWLockExclusive(&this->_Sector[sectorY][sectorX]._SectorLock);
            this->_Sector[sectorY][sectorX]._PlayerSet.emplace(player);
            player->_SectorX = sectorX;
            player->_SectorY = sectorY;
            ReleaseSRWLockExclusive(&this->_Sector[sectorY][sectorX]._SectorLock);
        }
        //이동하는 케이스
        else if (player->_SectorX != sectorX || player->_SectorY != sectorY)
        {
            int curX = player->_SectorX;
            int curY = player->_SectorY;
            player->_SectorX = sectorX;
            player->_SectorY = sectorY;
            for (;;)
            {
                //두개 섹터 다 락걸릴때까지 루프
                if (TryAcquireSRWLockExclusive(&this->_Sector[curY][curX]._SectorLock) == false)
                    continue;
                if (TryAcquireSRWLockExclusive(&this->_Sector[sectorY][sectorX]._SectorLock) == false)
                {
                    ReleaseSRWLockExclusive(&this->_Sector[curY][curX]._SectorLock);
                    YieldProcessor();
                    continue;
                }

                for (auto iter = this->_Sector[curY][curX]._PlayerSet.begin();
                    iter != this->_Sector[curY][curX]._PlayerSet.end(); ++iter)
                {
                    if ((*iter)->_SessionID == sessionID)
                    {
                        this->_Sector[curY][curX]._PlayerSet.erase(iter);
                        break;
                    }
                }
                this->_Sector[sectorY][sectorX]._PlayerSet.emplace(player);

                ReleaseSRWLockExclusive(&this->_Sector[curY][curX]._SectorLock);
                ReleaseSRWLockExclusive(&this->_Sector[sectorY][sectorX]._SectorLock);
                break;
            }
        }
        Packet* moveSectorPacket = Packet::Alloc();
        this->MakePacketResponseMoveSector(moveSectorPacket, player->_AccountNo, player->_SectorX, player->_SectorY);
        this->SendPacket(sessionID, moveSectorPacket);
        InterlockedIncrement(&this->_SectorMoveTPS);
    }

    void ChatServer::PacketProcChatRequire(Packet* packet, ULONGLONG sessionID)
    {
        WCHAR errStr[512];
        INT64 accountNo;
        WORD messageLen;
        WCHAR message[512];

        if (packet->GetBufferSize() < sizeof(accountNo) + sizeof(messageLen))
        {
            wsprintf(errStr, L"PacketProcChatRequire Packet BufferSize is not enough SessionID : %I64u", sessionID);
            this->_ChatServerLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_ERROR);
            this->DisconnectSession(sessionID);
            return;
        }
        (*packet) >> accountNo >> messageLen;

        if (packet->GetBufferSize() != messageLen)
        {
            wsprintf(errStr, L"PacketProcChatRequire Packet BufferSize is not MessageLen SessionID : %I64u", sessionID);
            this->_ChatServerLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_ERROR);
            this->DisconnectSession(sessionID);
            return;
        }
        packet->GetBuffer((char*)message, messageLen);

        Player* player = this->FindPlayer(sessionID);
        if (player == nullptr)
        {
            wsprintf(errStr, L"PacketProcChatRequire FindPlayer returned nullptr SessionID : %I64u", sessionID);
            this->_ChatServerLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_ERROR);
            this->DisconnectSession(sessionID);
            return;
        }
        else if (player->_Logined == false)
        {
            wsprintf(errStr, L"PacketProcChatRequire Player Already Logined SessionID : %I64u", sessionID);
            this->_ChatServerLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_ERROR);
            this->DisconnectSession(sessionID);
            return;
        }
        else if (player->_AccountNo != accountNo)
        {
            wsprintf(errStr, L"PacketProcChatRequire Player AccountNo is wrong SessionID : %I64u", sessionID);
            this->_ChatServerLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_ERROR);
            this->DisconnectSession(sessionID);
        }
        else if (player->_SectorX >= 50 || player->_SectorY >= 50)
        {
            wsprintf(errStr, L"PacketProcChatRequire Player Sector X : %d, Sector Y : %d SessionID : %I64u", player->_SectorX, player->_SectorY, sessionID);
            this->_ChatServerLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_ERROR);
            this->DisconnectSession(sessionID);
            return;
        }

        int beginY = player->_SectorY - 1;
        int beginX = player->_SectorX - 1;

        message[messageLen / 2] = L'\0';

        if (wcscmp(L"=", message) == 0)
            this->_LeaveSessionCount++;

        Packet* messagePacket = Packet::Alloc();
        MakePacketResponseMessage(messagePacket, player->_AccountNo, player->_ID, player->_NickName, messageLen, message);
        for (int y = 0; y < 3; y++)
        {
            if (((beginY + y) < 0) || (beginY + y) >= 50) continue;
            for (int x = 0; x < 3; x++)
            {
                if (((beginX + x) < 0) || ((beginX + x) >= 50)) continue;
                AcquireSRWLockShared(&this->_Sector[beginY + y][beginX + x]._SectorLock);
            }
        }

        messagePacket->AddRef();
        for (int y = 0; y < 3; y++)
        {
            if (((beginY + y) < 0) || (beginY + y) >= 50) continue;
            for (int x = 0; x < 3; x++)
            {
                if (((beginX + x) < 0) || ((beginX + x) >= 50)) continue;

                for (auto iter = this->_Sector[beginY + y][beginX + x]._PlayerSet.begin(); iter != this->_Sector[beginY + y][beginX + x]._PlayerSet.end(); ++iter)
                {
                    this->SendPacket((*iter)->_SessionID, messagePacket);
                    InterlockedIncrement(&this->_ChatTPS);
                }
            }
        }
        Packet::Free(messagePacket);

        for (int y = 0; y < 3; y++)
        {
            if (((beginY + y) < 0) || (beginY + y) >= 50) continue;
            for (int x = 0; x < 3; x++)
            {
                if (((beginX + x) < 0) || ((beginX + x) >= 50)) continue;
                ReleaseSRWLockShared(&this->_Sector[beginY + y][beginX + x]._SectorLock);
            }
        }
    }

    void ChatServer::PacketProcHeartBeating(Packet* packet, ULONGLONG sessionID)
    {

    }

    void ChatServer::MakePacketResponseMoveSector(Packet* packet, INT64 accountNo, WORD sectorX, WORD sectorY)
    {
        WORD type = CHAT_PACKET_TYPE::PACKET_CS_CHAT_RES_SECTOR_MOVE;
        (*packet) << type << accountNo << sectorX << sectorY;
    }

    void ChatServer::MakePacketResponseMessage(Packet* packet, INT64 accountNo, const WCHAR* ID, const WCHAR* nickName, WORD messageLen, const WCHAR* message)
    {
        WORD type = CHAT_PACKET_TYPE::PACKET_CS_CHAT_RES_MESSAGE;
        (*packet) << type << accountNo;
        packet->PutBuffer((char*)ID, 40);
        packet->PutBuffer((char*)nickName, 40);
        (*packet) << messageLen;
        packet->PutBuffer((char*)message, messageLen);
    }

    void ChatServer::MakePacketResponseLogin(Packet* packet, INT64 accountNo, BYTE status)
    {
        WORD type = CHAT_PACKET_TYPE::PACKET_CS_CHAT_RES_LOGIN;
        (*packet) << type << status << accountNo;
    }

    void ChatServer::InsertPlayer(ULONGLONG sessionID, Player* player)
    {
        this->_PlayerMap.emplace(std::make_pair(sessionID, player));
    }
    void ChatServer::RemovePlayer(ULONGLONG sessionID)
    {
        AcquireSRWLockExclusive(&this->_PlayerMapLock);
        auto iter = this->_PlayerMap.find(sessionID);
        if (iter == this->_PlayerMap.end())
        {
            ReleaseSRWLockExclusive(&this->_PlayerMapLock);
            return;
        }
        Player* removePlayer = iter->second;
        this->_PlayerMap.erase(sessionID);
        ReleaseSRWLockExclusive(&this->_PlayerMapLock);
        removePlayer->_Logined = false;

        constexpr WORD comp = -1;
        WORD sectorX = removePlayer->_SectorX;
        WORD sectorY = removePlayer->_SectorY;
        removePlayer->_SectorX = comp;
        removePlayer->_SectorY = comp;
        if (sectorX == comp || sectorY == comp)
        {
            this->_PlayerPool.Free(removePlayer);
            return;
        }

        AcquireSRWLockExclusive(&this->_Sector[sectorY][sectorX]._SectorLock);
        for (auto iter = this->_Sector[sectorY][sectorX]._PlayerSet.begin(); iter != this->_Sector[sectorY][sectorX]._PlayerSet.end(); ++iter)
        {
            if ((*iter)->_SessionID == sessionID)
            {
                this->_Sector[sectorY][sectorX]._PlayerSet.erase(iter);
                break;
            }
        }
        ReleaseSRWLockExclusive(&this->_Sector[sectorY][sectorX]._SectorLock);

        this->_PlayerPool.Free(removePlayer);
    }

    Player* ChatServer::FindPlayer(ULONGLONG sessionID)
    {
        AcquireSRWLockShared(&this->_PlayerMapLock);
        auto iter = this->_PlayerMap.find(sessionID);
        if (iter == this->_PlayerMap.end())
        {
            ReleaseSRWLockShared(&this->_PlayerMapLock);
            return nullptr;
        }
        ReleaseSRWLockShared(&this->_PlayerMapLock);
        return iter->second;
    }


    unsigned __stdcall MoniteringThread(void* param)
    {
        if (param == nullptr)
            return -1;
        ChatServer* server = (ChatServer*)param;
        server->ChatServerMoniteringThread(server);
        return 0;
    }

    unsigned __stdcall RedisThread(void* param)
    {
        if (param == nullptr)
            return -1;
        ChatServer* server = (ChatServer*)param;
        server->ChatServerRedisThread(server);
    }





    void ChatServer::OnRecv(ULONGLONG sessionID, Packet* recvPacket)
    {
        JobMessage job;
        job._SessionID = sessionID;
        job._Packet = recvPacket;
        *(job._Packet) >> job._Type;
        this->PacketProc(job._Packet, sessionID, job._Type);
        Packet::Free(recvPacket);
    }

    void ChatServer::OnErrorOccured(DWORD errorCode, const WCHAR* error)
    {
        _ChatServerLog.LOG(error, LogClass::LogLevel::LOG_LEVEL_LIBRARY);
    }

    bool ChatServer::OnConnectionRequest(WCHAR* ipStr, DWORD ip, USHORT port)
    {
        return _RunningFlag;
    }

    void ChatServer::OnClientJoin(WCHAR* ipStr, DWORD ip, USHORT port, ULONGLONG sessionID)
    {
        // 만약 라이브러리에 TimeOut 체크 기능이 없다면 이 구간에서 체크 시작을 해줘야됨.
    }

    void ChatServer::OnClientLeave(ULONGLONG sessionID)
    {
        this->PacketProc(nullptr, sessionID, CHAT_PACKET_TYPE::ON_CLIENT_LEAVE);
    }

    void ChatServer::OnTimeOut(ULONGLONG sessionID)
    {
        this->PacketProc(nullptr, sessionID, CHAT_PACKET_TYPE::ON_TIME_OUT);
    }

    void ChatServer::OnSend(ULONGLONG sessionID) {};

    void ChatServer::Start()
    {
        if (!this->GetNetCoreInitializeFlag())
        {
            DWORD coreErr = this->GetNetCoreErrorCode();
            DWORD lastErr = this->GetLastAPIErrorCode();
            printf("API Error : %d\nCore Error : %d\n", lastErr, coreErr);
            this->_RunningFlag = false;
            return;
        }
        InitializeSRWLock(&this->_PlayerMapLock);
        this->_Sector = new Sector*[SECTOR_Y_SIZE];
        setlocale(LC_ALL, "korean");
        _wsetlocale(LC_ALL, L"korean");
        for (int i = 0; i < SECTOR_Y_SIZE; i++)
        {
            //this->_Sector[i] = new std::unordered_set<Player*>[SECTOR_X_SIZE];
            this->_Sector[i] = new Sector[SECTOR_X_SIZE];
        }

        this->_ChatServerLog.LOG_SET_DIRECTORY(L"Chat_Server_Log");
        this->_ChatServerLog.LOG_SET_LEVEL(LogClass::LogLevel::LOG_LEVEL_LIBRARY);

        printf("RedisConnectBegin\n");
        this->_RedisClient.connect("10.0.1.2", 6379);
        printf("RedisConnectEnd\n");
        this->_RedisEvent = CreateEvent(nullptr, false, false, nullptr);
        if (this->_RedisEvent == nullptr)
        {
            this->_RunningFlag = false;
            this->Close();
            return;
        }

        this->_MoniteringThread = (HANDLE)_beginthreadex(nullptr, 0, MoniteringThread, this, 0, nullptr);
        if (this->_MoniteringThread == nullptr)
        {
            this->_RunningFlag = false;
            this->Close();
            return;
        }

        this->_RedisThread = (HANDLE)_beginthreadex(nullptr, 0, RedisThread, this, 0, nullptr);
        if (this->_RedisThread == nullptr)
        {
            this->_RunningFlag = false;
            this->Close();
            return;
        }

        this->_RunningFlag = true;
        this->CNetServer::Run();
    }

    void ChatServer::Close()
    {
        HANDLE thread[2]{ this->_MoniteringThread , this->_RedisThread };
        WaitForMultipleObjects(2, thread, true, INFINITE);

        for (int i = 0; i < SECTOR_Y_SIZE; i++)
            delete[] _Sector[i];
        delete[] _Sector;
    }

    ChatServer::ChatServer(USHORT port, DWORD backlogQueueSize, DWORD threadPoolSize, DWORD runningThread, DWORD nagleOff, ULONGLONG maxSessionCounts, DWORD timeOutTime)
        : CNetServer(port, backlogQueueSize, threadPoolSize, runningThread, nagleOff, maxSessionCounts,timeOutTime), _RunningFlag(false), _LoginTPS(0),_SectorMoveTPS(0),_ChatTPS(0)
    {
        this->Start();
    }
    ChatServer::~ChatServer()
    {
        this->Close();
    }

}