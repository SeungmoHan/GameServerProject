#include "IOCP_Echo_Server.h"
#include <ctime>
#include <conio.h>
#include <process.h>

namespace univ_dev
{

	EchoServer::EchoServer(USHORT port, DWORD backlogQueueSize, DWORD threadPoolSize, DWORD runningThread, DWORD nagleOff, ULONGLONG maxSessionCounts) 
		: CLanServer(port, backlogQueueSize, threadPoolSize, runningThread, nagleOff, maxSessionCounts) ,_MoniteringThread(nullptr) {
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
		DWORD prev = timeGetTime();
		DWORD cur = prev;
		while (true)
		{
			if (_kbhit())
			{
				int key = _getch();
				if (toupper(key) == 'Q')
				{
					this->CLanServer::PostLanServerStop();
					return 0;
				}
			}
			cur = timeGetTime();
			if (cur - prev < 1000)
			{
				Sleep(100);
				continue;
			}
			prev = cur;
			MoniteringInfo info = this->GetMoniteringInfo();

			printf("\n----------------------------------------------------\n");
			printf("Excute Timer : %u\n", (timeGetTime() - begin) / 1000);
			printf("-------------------------------\n");
			printf("Worker Thread Count : %u\n", info._WorkerThreadCount);
			printf("Running Thread Count : %u\n", info._RunningThreadCount);
			printf("-------------------------------\n");
			printf("Total Accept Count : %llu\n", info._TotalAcceptSession);
			printf("Total Release Count : %llu\n", info._TotalReleaseSession);
			printf("Total Processed Packet : %llu\n", info._TotalPacket);
			printf("Total Processed Bytes : %llu\n", info._TotalProecessedBytes);
			printf("-------------------------------\n");
			printf("Accept Per Sec : %llu\n", info._AccpeptCount);
			printf("Recv TPS : %llu\n", info._RecvPacketCount);
			printf("Send TPS : %llu\n", info._SendPacketCount);
			printf("Total TPS : %llu\n", info._SendPacketCount + info._RecvPacketCount);
			printf("-------------------------------\n");
			printf("Packet Chunk Capacity : %d\n", Packet::GetCapacityCount());
			printf("Packet Chunk UseCount : %d\n", Packet::GetUseCount());
			printf("Packet UseCount : %d\n", Packet::GetTotalPacketCount());
			printf("-------------------------------\n");
			printf("----------------------------------------------------\n");

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
			return;
		}
		this->_MoniteringThread = (HANDLE)_beginthreadex(nullptr, 0, MoniteringThread, this, 0, nullptr);
		if (_MoniteringThread == nullptr)
			return;
		Run();
	}

	void EchoServer::Close()
	{
		WaitForSingleObject(this->_MoniteringThread, INFINITE);
	}

	void EchoServer::OnRecv(ULONGLONG sessionID, Packet* packet)
	{
		ULONGLONG data;
		unsigned short packetSize = 8;
		PRO_BEGIN("Alloc");
		Packet* sendPacket = Packet::Alloc();
		PRO_END("Alloc");
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
		return true;
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
}
