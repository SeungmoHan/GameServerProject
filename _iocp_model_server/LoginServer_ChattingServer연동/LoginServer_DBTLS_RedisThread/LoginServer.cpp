#include "LoginServer.h"
#include <process.h>
#include <locale.h>
#include <conio.h>
#include "JobMessage.h"


namespace univ_dev
{
    volatile DWORD LoginServer::_DBTlsIdx = TlsAlloc();

    unsigned __stdcall MoniteringThread(void* param)
    {
        LoginServer* server = (LoginServer*)param;
        if (server == nullptr)
            CRASH();
        return server->LoginServerMoniteringThread(server);
    }

    unsigned __stdcall RedisThread(void* param)
    {
        LoginServer* server = (LoginServer*)param;
        if (server == nullptr)
            return -1;
        return server->LoginServerRedisThread(server);
    }

    void LoginServer::Start()
    {
        if (_wmkdir(L"Login_Server_Log"));
        //this->_LoginDB.SetDBLogDirectory(L"Login_Server_Log\\Login_DB_Log");
        this->InitLoginServerLog(L"Login_Server_Log\\Server_Log");

        setlocale(LC_ALL, "korean");
        _wsetlocale(LC_ALL, L"korean");

        if (!GetNetCoreInitializeFlag())
        {
            DWORD coreErr = this->GetLastCoreErrno();
            DWORD lastErr = this->GetLastAPIErrno();
            _LoginServerLog.LOG(L"Network Library Initialize Failed API Err : %u, Core Err : %u", LogClass::LogLevel::LOG_LEVEL_LIBRARY);
            printf("API Error : %d\nCore Error : %d\n", lastErr, coreErr);
            this->_RunningFlag = false;
            return;
        }
        InitializeSRWLock(&_DBLock);

        std::string ip{ "10.0.1.2" };
        this->_RedisClient.connect(ip, 6379, nullptr, 7000);
        if (!this->_RedisClient.is_connected())
        {
            this->_RunningFlag = false;
            Close();
            return;
        }
        this->_RedisEvent = CreateEvent(nullptr, false, false, nullptr);
        if (this->_RedisEvent == nullptr)
        {
            this->_RunningFlag = false;
            Close();
            return;
        }
        this->_RedisThread = (HANDLE)_beginthreadex(nullptr, 0, RedisThread, this, 0, nullptr);
        if (this->_RedisThread == nullptr)
        {
            this->_RunningFlag = false;
            Close();
            return;
        }

        this->_MoniteringThread = (HANDLE)_beginthreadex(nullptr, 0, MoniteringThread, this, 0, nullptr);
        if (this->_MoniteringThread == nullptr)
        {
            this->_RunningFlag = false;
            Close();
            return;
        }
        this->_RunningFlag = true;
        this->CNetServer::Run();
    }

    void LoginServer::Close()
    {
        HANDLE thread = this->_MoniteringThread;
        WaitForSingleObject(this->_MoniteringThread, INFINITE);
        //_LoginDB.DBClose();
        //for (int i = 0; i < SECTOR_Y_SIZE; i++)
        //    delete[] _Sector[i];
        //delete[] _Sector;
    }

    void LoginServer::InitLoginServerLog(const WCHAR* directory)
    {
        this->_LoginServerLog.LOG_SET_DIRECTORY(directory);
        this->_LoginServerLog.LOG_SET_LEVEL(LogClass::LogLevel::LOG_LEVEL_SYSTEM);
    }
    //(USHORT port, DWORD backlogQueueSize, DWORD threadPoolSize, DWORD runningThread, DWORD nagleOff, ULONGLONG maxSessionCounts, DWORD timeOutTime)
    //    : CNetServer(port, backlogQueueSize, threadPoolSize, runningThread, nagleOff, maxSessionCounts, timeOutTime), _RunningFlag(false)
    LoginServer::LoginServer(USHORT port, DWORD backlogQueueSize, DWORD threadPoolSize, DWORD runningThread, DWORD nagleOff, ULONGLONG maxSessionCounts, DWORD timeOutTime) : CNetServer(port, backlogQueueSize, threadPoolSize, runningThread, nagleOff, maxSessionCounts, timeOutTime)/*, _LoginDB(_DataBaseIPStr, _DataBaseRootName, _DataBaseRootPassword, _DataBaseInitSchemaName, _DataBaseSlowQuery)*/
    {
        Start();
    }
    LoginServer::~LoginServer()
    {
        Close();
    }
    void LoginServer::LockDB()
    {
        AcquireSRWLockExclusive(&this->_DBLock);
    }
    void LoginServer::UnlockDB()
    {
        ReleaseSRWLockExclusive(&this->_DBLock);
    }
    unsigned int LoginServer::LoginServerMoniteringThread(void* param)
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

            //ULONGLONG loginTPS = InterlockedExchange(&this->_LoginTPS, 0);
            //ULONGLONG chatTPS = InterlockedExchange(&this->_ChatTPS, 0);
            //ULONGLONG sectorMoveTPS = InterlockedExchange(&this->_SectorMoveTPS, 0);

            int tempSize;
            int tempArr[20]{ 0 };
            unsigned long long sectorSize = 0;
            unsigned long long sectorCapacity = 0;

            DWORD afterServerOn = (timeGetTime() - begin) / 1000;

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
                    fprintf(file, "| \n");
                    fprintf(file, "| Packet ToTal UseCount : %d\n", Packet::GetTotalPacketCount());
                    fprintf(file, "| Available / NPPool / Private Mem : %lluMb / %lluMb / %lluKb\n", this->_HardWareMoniter.AvailableMemoryMBytes(), this->_HardWareMoniter.NonPagedPoolMBytes(), this->_ProcessMoniter.PrivateMemoryKBytes());
                    fprintf(file, "---------------------------------------------------------------------------------------\n");
                }
                else
                    fprintf(file, "Available / NPPool / Private Mem : %lluMb / %lluMb / %lluKb\n", this->_HardWareMoniter.AvailableMemoryMBytes(), this->_HardWareMoniter.NonPagedPoolMBytes(), this->_ProcessMoniter.PrivateMemoryKBytes());

                fclose(file);
            }

            prev = cur;
            ULONGLONG tempLoginTPS = InterlockedExchange(&this->_LoginTPS, 0);
            ULONGLONG tempLoginSuccessTPS = InterlockedExchange(&this->_LoginSuccessTPS, 0);

            this->_LoginTotal += tempLoginTPS;
            this->_LoginSuccessTotal += tempLoginSuccessTPS;

            
            

            ULONGLONG tempDBQueryTPS = InterlockedExchange(&this->_DBQueryTPS, 0);
            this->_DBQueryCountTotal += tempDBQueryTPS;
            ULONGLONG tempDBQueryAVR = this->_DBTotalTime / (this->_DBQueryCountTotal + 1);

            printf("\n\n");
            printf("\n-------------------------------------MONITERING----------------------------------------\n");
            printf("| After Server On : %u day / %u h / %u m / %u s\n", day, hour, minute, sec);
            printf("|----------------------------------------THREAD----------------------------------------\n");
            printf("| Worker Thread / Running  thread : %u / %u\n", info._WorkerThreadCount, info._RunningThreadCount);
            printf("|----------------------------------------TOTAL-----------------------------------------\n");
            printf("| Total Accept / Release  : %llu /  %llu\n", info._TotalAcceptSession, info._TotalReleaseSession);
            printf("| Total Processed Bytes : %llu\n", info._TotalProecessedBytes);
            printf("|----------------------------------------TPS-------------------------------------------\n");
            printf("| Accept Per Sec : %llu\n", info._AccpeptCount);
            printf("| TPS// Recv / Send / Total  : %llu / %llu / %llu\n", info._RecvPacketCount, info._SendPacketCount, info._SendPacketCount + info._RecvPacketCount);
            printf("| LockFreeQueue Size / Capacity / Max : %llu / %llu / %llu\n", info._LockFreeQueueSize, info._LockFreeQueueCapacity, info._LockFreeMaxCapacity);
            printf("| RedisJobQueue Size / Capacity : %d / %d\n", this->_RedisJobQueue.size(), this->_RedisJobQueue.GetCapacityCount());
            printf("|----------------------------------------LOGIN-----------------------------------------\n");
            printf("| TotalLogin / TPS / SuccessTPS / Wait :  %llu / %llu / %llu / %llu\n", this->_LoginTotal, tempLoginTPS, tempLoginSuccessTPS, this->_LoginWait);
            printf("| TotalLoginTime / LoginAVR : %llu / %llf\n", this->_LoginTotalTime, (double)this->_LoginTotalTime / (double)this->_LoginTotal);
            printf("| Query Total Count/ TPS  : %llu / %llu\n", this->_DBQueryCountTotal, tempDBQueryTPS);
            printf("| MAX Query Time / TOTAL(AVR) : %llu / %llu(%llu)\n", this->_MaxQueryTime, this->_DBTotalTime, tempDBQueryAVR);
            printf("| Max Query String : %s\n", this->_MaxQueryString);
            printf("| Session Count / IDX Capacity / IDX Size : %llu / %llu / %llu\n", info._SessionCnt, info._LockFreeStackCapacity, info._LockFreeStackSize);
            printf("|----------------------------------------POOL------------------------------------------\n");
            printf("| Packet Chunk Capacity / UseCount : %d / %d\n", Packet::GetCapacityCount(), Packet::GetUseCount());
            printf("| Packet ToTal UseCount : %d\n", Packet::GetTotalPacketCount());
            printf("|----------------------------------------USAGE_MONITER---------------------------------\n");
            printf("| NIC Send / Recv (10KB) : %.1llf / %.1llf\n", this->_HardWareMoniter.EthernetSendKBytes(), this->_HardWareMoniter.EthernetRecvKBytes());
            printf("| Available / NPPool / Private Mem : %lluMb / %lluMb / %lluKb\n", this->_HardWareMoniter.AvailableMemoryMBytes(), this->_HardWareMoniter.NonPagedPoolMBytes(), this->_ProcessMoniter.PrivateMemoryKBytes());
            printf("| PROCESS / CPU : [T %.1llf%% K %.1llf%% U %.1llf%%] / [T %.1llf%% K %.1llf%% U %.1llf%%]   \n", this->_ProcessMoniter.ProcessTotal(), this->_ProcessMoniter.ProcessKernel(), this->_ProcessMoniter.ProcessUser(), this->_HardWareMoniter.ProcessorTotal(), this->_HardWareMoniter.ProcessorKernel(), this->_HardWareMoniter.ProcessorUser());
            printf("---------------------------------------------------------------------------------------\n");
        }
        return -1;
    }
    unsigned int LoginServer::LoginServerRedisThread(void* param)
    {
        LoginRedisJob job;
        INT64 accountNo;
        ULONGLONG sessionID;
        Packet* packet = nullptr;
        DWORD begin = 0;
        while (this->_RunningFlag)
        {
            WaitForSingleObject(this->_RedisEvent, INFINITE);

            while (this->_RedisJobQueue.dequeue(job))
            {
                accountNo = job._AccountNo;
                sessionID = job._SessionID;
                packet = job._Packet;
                begin = job._BeginTime;
                cpp_redis::reply reply = job._RedisReply;

                SendPacket(sessionID, packet);
                InterlockedIncrement(&this->_LoginSuccessTPS);
                DWORD loginTime = timeGetTime() - begin;
                InterlockedAdd64((LONG64*)&this->_LoginTotalTime, loginTime);
            }
        }
        return 0;
    }
    void LoginServer::OnRecv(ULONGLONG sessionID, Packet* recvPacket)
    {
        JobMessage job;
        job._SessionID = sessionID;
        job._Packet = recvPacket;
        *(job._Packet) >> job._Type;
        this->PacketProc(job._Packet, sessionID, job._Type);
        Packet::Free(recvPacket);
    }
    void LoginServer::OnErrorOccured(DWORD errorCode, const WCHAR* error)
    {
        //this->GetLastAPIErrno();
        //this->GetLastCoreErrno();
        this->_LoginServerLog.LOG(error, LogClass::LogLevel::LOG_LEVEL_LIBRARY);
    }
    bool LoginServer::OnConnectionRequest(WCHAR* ipStr, DWORD ip, USHORT port)
    {
        return this->_RunningFlag;
    }
    void LoginServer::OnClientJoin(WCHAR* ipStr, DWORD ip, USHORT port, ULONGLONG sessionID)
    {
        InterlockedIncrement(&this->_LoginWait);
        InterlockedIncrement(&this->_JoinSession);
        InterlockedIncrement(&this->_TotalJoinSession);
    }
    void LoginServer::OnClientLeave(ULONGLONG sessionID)
    {
        this->PacketProc(nullptr, sessionID, PACKET_TYPE::en_ON_CLIENT_LEAVE);
        InterlockedDecrement(&this->_LoginWait);
        InterlockedDecrement(&this->_JoinSession);
        InterlockedIncrement(&this->_TotalLeaveSession);
    }
    void LoginServer::OnTimeOut(ULONGLONG sessionID)
    {
        this->PacketProc(nullptr, sessionID, PACKET_TYPE::en_ON_TIME_OUT);
    }
    void LoginServer::OnSend(ULONGLONG sessionID)
    {
        //DisconnectSession(sessionID);
    }
    void LoginServer::PacketProc(Packet* packet, ULONGLONG sessionID, WORD type)
    {
        WCHAR errStr[512];
        switch (type)
        {
            case PACKET_TYPE::en_PACKET_CS_LOGIN_REQ_LOGIN:
            {
                this->PacketProcRequestLogin(packet, sessionID);
                break;
            }
            case PACKET_TYPE::en_ON_CLIENT_LEAVE:
            {
                break;
            }
            case PACKET_TYPE::en_ON_TIME_OUT:
            {
                wsprintf(errStr, L"On Time Out sessionID : %I64u", sessionID);
                this->_LoginServerLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
                this->DisconnectSession(sessionID);
                break;
            }
            default:
            {
                wsprintf(errStr, L"PacketProc default case Error sessionID : %I64u", sessionID);
                this->_LoginServerLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_ERROR);
                this->DisconnectSession(sessionID);
                break;
            }
        }
    }
    void LoginServer::PacketProcRequestLogin(Packet* packet, ULONGLONG sessionID)
    {
        DWORD begin = timeGetTime();
        InterlockedIncrement(&this->_TotalLogin);
        InterlockedIncrement(&this->_LoginTPS);
        WCHAR errStr[512];
        if (packet->GetBufferSize() != sizeof(INT64) + this->TOKEN_KEY_SIZE)
        {
            wsprintf(errStr, L"Packet BufferSize not enough sizeof(INT64) + this->TOKEN_KEY_SIZE");
            this->_LoginServerLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_ERROR);
            this->DisconnectSession(sessionID);
            return;
        }
        INT64 accountNo;
        (*packet) >> accountNo;

        char tokenKey[65];
        packet->GetBuffer(tokenKey, this->TOKEN_KEY_SIZE);
        tokenKey[64] = '\0';
        char query[512];
        sprintf_s(query, 512, "select accountno, userid, userpass, usernick from account where accountno = %I64d", accountNo);

        DBConnector* tlsDB = (DBConnector*)TlsGetValue(this->_DBTlsIdx);
        if (tlsDB == nullptr)
        {
            tlsDB = new DBConnector(this->_DataBaseIPStr, this->_DataBaseRootName, this->_DataBaseRootPassword, this->_DataBaseInitSchemaName, this->_DataBaseSlowQuery);
            if (!tlsDB->DBConnect())
                CRASH();
            TlsSetValue(this->_DBTlsIdx, tlsDB);
            tlsDB->SetDBLogDirectory(L"Login_Server_Log\\Login_DB_Log");
        }
        InterlockedIncrement(&this->_DBQueryTPS);
        DWORD dbBegin = timeGetTime();
        tlsDB->QuerySave(query);
        MYSQL_RES* result = tlsDB->GetQueryResult();
        DWORD queryTime = timeGetTime() - dbBegin;
        InterlockedAdd64((LONG64*)&this->_DBTotalTime, queryTime);
        if (this->_MaxQueryTime < queryTime)
        {
            strcpy_s(this->_MaxQueryString, query);
            this->_MaxQueryTime = queryTime;
        }

        MYSQL_ROW row;
        BYTE status = en_PACKET_CS_LOGIN_RES_LOGIN::dfLOGIN_STATUS_FAIL;
        WCHAR ID[20];
        WCHAR userNick[20];
        do
        {
            if ((row = tlsDB->FetchRow(result)) == nullptr)
            {
                // DB에 세션 키가 없음
                status = en_PACKET_CS_LOGIN_RES_LOGIN::dfLOGIN_STATUS_ACCOUNT_MISS;
                break;
            }
            //if (memcmp(tokenKey, row[2], this->TOKEN_KEY_SIZE) != 0) 
            //{
            //    //세션 키값이 다름
            //    status = en_PACKET_CS_LOGIN_RES_LOGIN::dfLOGIN_STATUS_ACCOUNT_MISS;
            //    MakePacketResponseLogin(packet, accountNo, status, nullptr, nullptr);
            //    break;
            //}
            //_LoginDB.FreeResult(result);
            //UnlockDB();

            size_t size;
            status = en_PACKET_CS_LOGIN_RES_LOGIN::dfLOGIN_STATUS_OK;
            mbstowcs_s(&size, ID, this->ID_MAX_LEN, row[1], _TRUNCATE);
            mbstowcs_s(&size, userNick, this->NICK_NAME_MAX_LEN, row[3], _TRUNCATE);
        } while (0);
        tlsDB->FreeResult(result);

        Packet* sendPacket = Packet::Alloc();
        MakePacketResponseLogin(sendPacket, accountNo, status, ID, userNick);
        if (status != en_PACKET_CS_LOGIN_RES_LOGIN::dfLOGIN_STATUS_OK)
        {
            wsprintf(errStr, L"Login Status Is not LOGIN_STATUS_OK %d", status);
            this->_LoginServerLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_ERROR);
            SendPacket(sessionID, sendPacket);
            DisconnectSession(sessionID);
            return;
        }
        this->_RedisClient.setex(std::to_string(accountNo) + ".chat", 10, tokenKey, [=](cpp_redis::reply& reply)
            {
                LoginRedisJob job;
                job._AccountNo = accountNo;
                job._RedisReply = reply;
                job._SessionID = sessionID;
                job._Packet = sendPacket;
                job._BeginTime = begin;
                this->_RedisJobQueue.enqueue(job);
                SetEvent(this->_RedisEvent);
            }).commit();
        //std::chrono::duration<double> duration(0.2);
        //_RedisClient.sync_commit(duration);
        //if (!ret.valid())
        //{
        //    wsprintf(errStr, L"redis set commmit failed");
        //    this->_LoginServerLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_ERROR);
        //    DisconnectSession(sessionID);
        //    return;
        //}
        //SendPacket(sessionID, sendPacket);
        //InterlockedIncrement(&this->_LoginSuccessTPS);
        //DWORD loginTime = timeGetTime() - begin;
        //InterlockedAdd64((LONG64*)&this->_LoginTotalTime, loginTime);
    }



    void LoginServer::MakePacketResponseLogin(Packet* packet, INT64 accountNo, BYTE status, const WCHAR* ID, const WCHAR* nickName)
    {
        WCHAR wrongBuffer[20]{ L"127.0.0.1" };
        USHORT wrongPort = 65535;
        WORD packetType = en_PACKET_CS_LOGIN_RES_LOGIN;
        (*packet) << packetType << accountNo << status;
        if (status != en_PACKET_CS_LOGIN_RES_LOGIN::dfLOGIN_STATUS_OK)
        {
            packet->PutBuffer((char*)wrongBuffer, this->ID_MAX_SIZE);
            packet->PutBuffer((char*)wrongBuffer, this->ID_MAX_SIZE);
            packet->PutBuffer((char*)wrongBuffer, this->SERVER_IP_SIZE);
            (*packet) << wrongPort;
            packet->PutBuffer((char*)wrongBuffer, this->SERVER_IP_SIZE);
            (*packet) << wrongPort;
            return;
        }
        packet->PutBuffer((char*)ID, this->ID_MAX_SIZE);
        packet->PutBuffer((char*)nickName, this->ID_MAX_SIZE);
        packet->PutBuffer((char*)this->_GameServerIP, this->SERVER_IP_SIZE);
        (*packet) << this->_GameServerPort;
        packet->PutBuffer((char*)this->_ChatServerIP, this->SERVER_IP_SIZE);
        (*packet) << this->_ChatServerPort;
    }
}
