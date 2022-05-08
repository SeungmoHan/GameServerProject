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
        DWORD prev = timeGetTime();

        while (this->_RunningFlag)
        {
            JobMessage job;
            if (!this->_JobQueue.dequeue(job))
                continue;

            this->PacketProc(job._Packet, job._SessionID, job._Type);

            if (job._Packet != nullptr)
            {
                Packet::Free(job._Packet);
            }
        }
        return 0;
    }
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
                    WaitForSingleObject(_UpdateThread, INFINITE);
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

            for (int i = 0; i < SECTOR_Y_SIZE; i++)
            {
                for (int j = 0; j < SECTOR_X_SIZE; j++)
                {
                    tempSize = _Sector[i][j].size();
                    sectorSize += tempSize;
                    sectorCapacity += _Sector[i][j].bucket_count();
                    if (tempSize >= 14)
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
                    fprintf(file, "| Job Queue Size / Capacity / Total Count : %d / %d / %d\n", this->_JobQueue.size(), this->_JobQueue.GetCapacityCount(), this->_JobMessagePool.GetTotalUseCount());
                    fprintf(file, "| Job Chunk Count / Capacity : %d / %d\n", this->_JobMessagePool.GetUseCount(), this->_JobMessagePool.GetCapacityCount());
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
            printf("|----------------------------------------USER------------------------------------------\n");
            printf("| PlayerMap Size / Capacity : %llu / %llu\n", this->_PlayerMap.size(), this->_PlayerMap.bucket_count());
            printf("| 0 %d/ 1 %d/ 2 %d/ 3 %d/ 4 %d/ 5 %d/ 6 %d/ 7 %d/ 8 %d/ 9 %d/ 10 %d/ 11 %d/ 12 %d/ 13 %d\n", 
                tempArr[0], tempArr[1], tempArr[2], tempArr[3], tempArr[4], tempArr[5], tempArr[6], tempArr[7], tempArr[8], tempArr[9], tempArr[10], tempArr[11], tempArr[12], tempArr[13]);
            printf("| Sector Size / Capacity : %llu / %llu\n", sectorSize, sectorCapacity);
            printf("| Player Capacity / UseCount :  / %d / %d\n", this->_PlayerPool.GetCapacityCount(), this->_PlayerPool.GetUseCount());
            printf("| Session Count / IDX Capacity / IDX Size : %llu / %llu / %llu\n", info._SessionCnt, info._LockFreeStackCapacity,info._LockFreeStackSize);
            printf("|----------------------------------------JOBQUEUESIZE----------------------------------\n");
            printf("| Job Queue Size / Capacity / Total Count : %d / %d / %d\n", this->_JobQueue.size(), this->_JobQueue.GetCapacityCount(), this->_JobMessagePool.GetTotalUseCount());
            printf("| Job Chunk Count / Capacity : %d / %d\n", this->_JobMessagePool.GetUseCount(), this->_JobMessagePool.GetCapacityCount());
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
                this->DisconnectSession(sessionID);
                DispatchError(ERR_CHAT_TIME_OUT, sessionID, L"On Time Out API Err : sessionID");
                break;
            }
            default:
            {
                DispatchError(ERR_CHAT_WRONG_PACKET_TYPE, ERR_CHAT_WRONG_PACKET_TYPE, L"Packet Type is Default");
                this->DisconnectSession(sessionID);
                break;
            }
        }
    }

    void ChatServer::PacketProcRequestLogin(Packet* packet, ULONGLONG sessionID)
    {
        BYTE status = false;
        INT64 accountNo;
        (*packet) >> accountNo;
        Player* player = this->FindPlayer(sessionID);
        if (player == nullptr)
        {
            //Player 생성 및 초기화
            player = this->_PlayerPool.Alloc();
            if (player->_Logined == true)
                return;
            player->_AccountNo = accountNo;
            packet->GetBuffer((char*)player->_ID, 40);
            packet->GetBuffer((char*)player->_NickName, 40);
            packet->GetBuffer(player->_TokenKey, 64);
            player->_SectorX = -1;
            player->_SectorY = -1;
            player->_SessionID = sessionID;
            player->_Logined = true;
            status = true;
            this->InsertPlayer(sessionID, player);
        }

        Packet* resLoginPacket = Packet::Alloc();
        this->MakePacketResponseLogin(resLoginPacket, player->_AccountNo, status);

        this->SendPacket(sessionID, resLoginPacket);
        if (status == false)
        {
            DispatchError(10101010, 20202020, L"Login Request -> player is not nullptr");
            this->DisconnectSession(sessionID);
        }
        InterlockedIncrement(&_LoginTPS);
    }

    void ChatServer::PacketProcMoveSector(Packet* packet, ULONGLONG sessionID)
    {
        INT64 accountNo;
        WORD sectorX;
        WORD sectorY;

        (*packet) >> accountNo >> sectorX >> sectorY;

        Player* player = this->FindPlayer(sessionID);
        if (player == nullptr)
        {
            this->DisconnectSession(sessionID);
            return;
        }
        else if (player->_Logined == false || accountNo != player->_AccountNo)
        {
            this->DisconnectSession(sessionID);
            return;
        }

        constexpr WORD comp = -1;
        if (player->_SectorX != comp && player->_SectorY != comp)
        {
            for (auto iter = this->_Sector[player->_SectorY][player->_SectorX].begin();
                iter != this->_Sector[player->_SectorY][player->_SectorX].end(); ++iter )
            {
                if ((*iter)->_SessionID == sessionID)
                {
                    this->_Sector[player->_SectorY][player->_SectorX].erase(iter);
                    break;
                }
            }
        }
        player->_SectorX = sectorX;
        player->_SectorY = sectorY;

        this->_Sector[player->_SectorY][player->_SectorX].emplace(player);

        Packet* moveSectorPacket = Packet::Alloc();
        this->MakePacketResponseMoveSector(moveSectorPacket, player->_AccountNo, player->_SectorX, player->_SectorY);
        this->SendPacket(sessionID, moveSectorPacket);
        InterlockedIncrement(&_SectorMoveTPS);
    }

    void ChatServer::PacketProcChatRequire(Packet* packet, ULONGLONG sessionID)
    {
        INT64 accountNo;
        WORD messageLen;
        WCHAR message[512];
        (*packet) >> accountNo >> messageLen;
        packet->GetBuffer((char*)message, messageLen);

        Player* player = this->FindPlayer(sessionID);
        if (player == nullptr)
        {
            this->DisconnectSession(sessionID);
            return;
        }
        else if (player->_Logined == false || player->_AccountNo != accountNo)
        {
            this->DisconnectSession(sessionID);
            return;
        }
        
        int beginY = player->_SectorY - 1;
        int beginX = player->_SectorX - 1;

        message[messageLen / 2] = '\0';
        if (wcscmp(L"=", message) == 0)
            _LeaveSessionCount++;

        Packet* messagePacket = Packet::Alloc();
        messagePacket->AddRef();
        MakePacketResponseMessage(messagePacket, player->_AccountNo, player->_ID, player->_NickName, messageLen, message);

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
                    InterlockedIncrement(&_ChatTPS);
                }
            }
        }
        Packet::Free(messagePacket);
    }

    void ChatServer::PacketProcHeartBeating(Packet* packet, ULONGLONG sessionID)
    {
        Player* player = FindPlayer(sessionID);
        if (player == nullptr)
        {
            DisconnectSession(sessionID);
            return;
        }
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
        auto iter = this->_PlayerMap.find(sessionID);

        if (iter == this->_PlayerMap.end()) return;

        Player* removePlayer = iter->second;

        constexpr WORD comp = -1;
        WORD sectorX = removePlayer->_SectorX;
        WORD sectorY = removePlayer->_SectorY;

        if (sectorX == comp || sectorY == comp) return;

        for (auto iter = _Sector[sectorY][sectorX].begin(); iter != _Sector[sectorY][sectorX].end(); ++iter)
        {
            if ((*iter)->_SessionID == sessionID)
            {
                _Sector[sectorY][sectorX].erase(iter);
                removePlayer->_Logined = false;
                break;
            }
        }
        this->_PlayerMap.erase(sessionID);
        this->_PlayerPool.Free(removePlayer);
    }

    Player* ChatServer::FindPlayer(ULONGLONG sessionID)
    {
        auto iter = this->_PlayerMap.find(sessionID);
        if (iter == this->_PlayerMap.end())
            return nullptr;
        return iter->second;
    }

    unsigned __stdcall UpdateThread(void* param)
    {
        if (param == nullptr)
            return -1;
        ChatServer* server = (ChatServer*)param;
        server->ChatServerUpdateThread(server);
        return 0;
    }

    unsigned __stdcall MoniteringThread(void* param)
    {
        if (param == nullptr)
            return -1;
        ChatServer* server = (ChatServer*)param;
        server->ChatServerMoniteringThread(server);
        return 0;
    }





    void ChatServer::OnRecv(ULONGLONG sessionID, Packet* recvPacket)
    {
        JobMessage job;
        job._SessionID = sessionID;
        job._Packet = recvPacket;
        *(job._Packet) >> job._Type;
        this->_JobQueue.enqueue(job);
    }

    void ChatServer::OnErrorOccured(DWORD errorCode, const WCHAR* error)
    {
        WCHAR timeStr[50]{ 0 };
        WCHAR dayStr[50]{ 0 };
        WCHAR temp[20]{ 0 };
        tm t;
        time_t cur = time(nullptr);
        localtime_s(&t, &cur);
        DWORD afterBegin = timeGetTime() - GetBeginTime();
        _itow_s(t.tm_year + 1900, temp, 10);
        wcscat_s(timeStr, temp);
        wcscat_s(timeStr, L"_");
        wcscat_s(dayStr, temp);
        wcscat_s(dayStr, L"_");
        _itow_s(t.tm_mon + 1, temp, 10);
        wcscat_s(timeStr, temp);
        wcscat_s(timeStr, L"_");
        wcscat_s(dayStr, temp);
        wcscat_s(dayStr, L"_");
        _itow_s(t.tm_mday, temp, 10);
        wcscat_s(timeStr, temp);
        wcscat_s(timeStr, L"_");
        wcscat_s(dayStr, temp);
        wcscat_s(dayStr, L"_");
        _itow_s(t.tm_hour, temp, 10);
        wcscat_s(timeStr, temp);
        wcscat_s(timeStr, L"_");
        _itow_s(t.tm_min, temp, 10);
        wcscat_s(timeStr, temp);

        FILE* errorLog = nullptr;
        while (errorLog == nullptr)
            fopen_s(&errorLog, "%s_libraryErrorLog.txt", "ab");

        fwprintf(errorLog, L"Error Occured At : %s, %u, err code : %u : %s api err : %d\n", timeStr, afterBegin / 1000, errorCode, error, GetLastAPIErrorCode());
        fclose(errorLog);
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
        JobMessage message;
        message._SessionID = sessionID;
        message._Type = CHAT_PACKET_TYPE::ON_CLIENT_LEAVE;
        message._Packet = nullptr;
        this->_JobQueue.enqueue(message);
    }

    void ChatServer::OnTimeOut(ULONGLONG sessionID)
    {
        JobMessage timeOutJob;
        timeOutJob._SessionID = sessionID;
        timeOutJob._Type = CHAT_PACKET_TYPE::ON_TIME_OUT;
        timeOutJob._Packet = nullptr;
        _JobQueue.enqueue(timeOutJob);
    }

    void ChatServer::Start()
    {
        if (!GetNetCoreInitializeFlag())
        {
            DWORD coreErr = this->GetNetCoreErrorCode();
            DWORD lastErr = this->GetLastAPIErrorCode();
            printf("API Error : %d\nCore Error : %d\n", lastErr, coreErr);
            this->_RunningFlag = false;
            return;
        }
        this->_Sector = new std::unordered_set<Player*>*[SECTOR_Y_SIZE];
        for (int i = 0; i < SECTOR_Y_SIZE; i++)
        {
            this->_Sector[i] = new std::unordered_set<Player*>[SECTOR_X_SIZE];
        }

        this->_MoniteringThread = (HANDLE)_beginthreadex(nullptr, 0, MoniteringThread, this, 0, nullptr);
        if (this->_MoniteringThread == nullptr)
        {
            this->_RunningFlag = false;
            return;
        }
        this->_UpdateThread = (HANDLE)_beginthreadex(nullptr, 0, UpdateThread, this, 0, nullptr);
        if (this->_UpdateThread == nullptr)
        {
            this->_RunningFlag = false;
            return;
        }
        this->_RunningFlag = true;
        this->CNetServer::Run();
    }

    void ChatServer::Close()
    {
        HANDLE thread[2]{ this->_UpdateThread, this->_MoniteringThread };
        WaitForMultipleObjects(2, thread, true, INFINITE);

        for (int i = 0; i < SECTOR_Y_SIZE; i++)
            delete[] _Sector[i];
        delete[] _Sector;
    }

    ChatServer::ChatServer(USHORT port, DWORD backlogQueueSize, DWORD threadPoolSize, DWORD runningThread, DWORD nagleOff, ULONGLONG maxSessionCounts)
        : CNetServer(port, backlogQueueSize, threadPoolSize, runningThread, nagleOff, maxSessionCounts), _RunningFlag(false), _UpdateThread(nullptr), _LoginTPS(0),_SectorMoveTPS(0),_ChatTPS(0)
    {
        this->Start();
    }
    ChatServer::~ChatServer()
    {
        this->Close();
    }
}




