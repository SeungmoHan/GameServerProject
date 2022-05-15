#include "IOCP_Echo_Server.h"
#include <ctime>
#include <conio.h>
#include <process.h>

namespace univ_dev
{

	EchoServer::EchoServer(USHORT port, DWORD backlogQueueSize, DWORD threadPoolSize, DWORD runningThread, DWORD nagleOff, ULONGLONG maxSessionCounts) 
		: CLanServer(port, backlogQueueSize, threadPoolSize, runningThread, nagleOff, maxSessionCounts) ,_MoniteringThread(nullptr),_RunningFlag(false) {
		Start();
	};
	
	
	unsigned __stdcall MoniteringThread(void* param)
	{
		if (param == nullptr)
			return -1;
		EchoServer* server = (EchoServer*)param;
		server->EchoServerMoniteringThread(server);
		return 0;
	}

	unsigned int EchoServer::EchoServerMoniteringThread(void* param)
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
                    this->CLanServer::PostLanServerStop();
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

            MoniteringInfo info = this->CLanServer::GetMoniteringInfo();
            this->_HardWareMoniter.UpdateHardWareTime();
            this->_ProcessMoniter.UpdateProcessTime();


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
                    fprintf(file, "| Packet ToTal UseCount : %d\n", Packet::GetTotalPacketCount());
                    fprintf(file, "| Session Count / IDX Capacity / IDX Size : %llu / %llu / %llu\n", info._SessionCnt, info._LockFreeStackCapacity, info._LockFreeStackSize);
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
            printf("| Total Accept / Release  : %llu /  %llu \n", info._TotalAcceptSession, info._TotalReleaseSession);
            printf("| Total Processed Bytes : %llu\n", info._TotalProecessedBytes);
            printf("|----------------------------------------TPS-------------------------------------------\n");
            printf("| Accept Per Sec : %llu\n", info._AccpeptCount);
            printf("| TPS// Recv / Send / Total  : %llu / %llu / %llu\n", info._RecvPacketCount, info._SendPacketCount, info._SendPacketCount + info._RecvPacketCount);
            printf("| LockFreeQueue Size / Capacity / Max : %llu / %llu / %llu\n", info._LockFreeQueueSize, info._LockFreeQueueCapacity, info._LockFreeMaxCapacity);
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

	void EchoServer::Start()
	{
		if (!GetNetCoreInitializeFlag())
		{
			DWORD coreErr = this->GetNetCoreErrorCode();
			DWORD lastErr = this->GetLastAPIErrorCode();
			printf("API Error : %d\nCore Error : %d\n", lastErr, coreErr);
			this->_RunningFlag = false;
			return;
		}

		this->_MoniteringThread = (HANDLE)_beginthreadex(nullptr, 0, MoniteringThread, this, 0, nullptr);
		if (this->_MoniteringThread == nullptr)
		{
			this->_RunningFlag = false;
			return;
		}
		this->_RunningFlag = true;
		this->CLanServer::Run();
	}

	void EchoServer::Close()
	{
		WaitForSingleObject(this->_MoniteringThread, INFINITE);
	}

	void EchoServer::OnRecv(ULONGLONG sessionID, Packet* packet)
	{
		ULONGLONG data;
		unsigned short packetSize = 8;
		Packet* sendPacket = Packet::Alloc();
		(*packet) >> data;
		(*sendPacket) << data;
		SendPacket(sessionID, sendPacket);
	}

	void EchoServer::OnErrorOccured(DWORD errorCode,const WCHAR* error)
	{
		WCHAR timeStr[50]{ 0 };
		WCHAR temp[20]{ 0 };
		tm t;
		time_t cur = time(nullptr);
		localtime_s(&t, &cur);
		DWORD afterBegin = timeGetTime() - GetBeginTime();
		_itow_s(t.tm_year+1900, temp, 10);
		wcscat_s(timeStr, temp);
		_itow_s(t.tm_mon+1, temp, 10);
		wcscat_s(timeStr, temp);
		_itow_s(t.tm_mday, temp, 10);
		wcscat_s(timeStr, temp);
		_itow_s(t.tm_hour, temp, 10);
		wcscat_s(timeStr, temp);
		_itow_s(t.tm_min, temp, 10);
		wcscat_s(timeStr, temp);
		
		FILE* errorLog = nullptr;
		while (errorLog == nullptr)
			fopen_s(&errorLog, "libraryErrorLog.txt", "ab");

		fwprintf(errorLog, L"Error Occured At : %s, %u, err code : %u : %s api err : %d\n", timeStr, afterBegin / 1000, errorCode, error, GetLastAPIErrorCode());
		fclose(errorLog);
	}

	bool EchoServer::OnConnectionRequest(WCHAR* ipStr, DWORD ip, USHORT port)
	{
		//white IP 
		return _RunningFlag;
	}

	void EchoServer::OnClientJoin(WCHAR* ipStr, DWORD ip, USHORT port, ULONGLONG sessionID)
	{
		Packet* loginPacket = Packet::Alloc();

		unsigned short payloadSize = 8;
		ULONGLONG data = 0x7fffffffffffffff;
		(*loginPacket) << data;
		SendPacket(sessionID, loginPacket);
	}

	void EchoServer::OnClientLeave(ULONGLONG sessionID)
	{
		//printf("SessionLeave :%llu\n",sessionID);
	}
	void EchoServer::OnTimeOut(ULONGLONG sessionID)
	{
		DisconnectSession(sessionID);
	}
}
