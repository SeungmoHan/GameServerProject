#include "IOCP_Echo_Server.h"

namespace univ_dev
{

	EchoServer::EchoServer(USHORT port, DWORD backlogQueueSize, DWORD threadPoolSize, DWORD runningThread, DWORD nagleOff, ULONGLONG maxSessionCounts)
		: CLanServer(port, backlogQueueSize, threadPoolSize, runningThread, nagleOff, maxSessionCounts)
	{

	}

	void EchoServer::OnRecv(ULONGLONG sessionID, Packet* packet)
	{
		ULONGLONG data;
		unsigned short packetSize = packet->GetBufferSize();
		Packet sendPacket;
		(*packet) >> data;
		sendPacket << packetSize << data;
		SendPacket(sessionID, &sendPacket);
	}

	void EchoServer::OnErrorOccured(DWORD errorCode,const WCHAR* error)
	{
		//wprintf(L"%u : %s\n", errorCode, error);
	}

	bool EchoServer::OnConnectionRequest(WCHAR* ipStr, DWORD ip, USHORT port)
	{
		//wprintf(L"%s:%d request\n", ipStr, port);
		return true;
	}

	void EchoServer::OnClientJoin(WCHAR* ipStr, DWORD ip, USHORT port, ULONGLONG sessionID)
	{
		//wprintf(L"%s:%d join\n", ipStr, port);
	}

	void EchoServer::OnClientLeave(ULONGLONG sessionID)
	{
		//printf("%llu session leave\n", sessionID);
	}




}
