#include "IOCP_Echo_Server.h"
#include <ctime>
namespace univ_dev
{

	EchoServer::EchoServer(USHORT port, DWORD backlogQueueSize, DWORD threadPoolSize, DWORD runningThread, DWORD nagleOff, ULONGLONG maxSessionCounts)
		: CLanServer(port, backlogQueueSize, threadPoolSize, runningThread, nagleOff, maxSessionCounts) {};

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
