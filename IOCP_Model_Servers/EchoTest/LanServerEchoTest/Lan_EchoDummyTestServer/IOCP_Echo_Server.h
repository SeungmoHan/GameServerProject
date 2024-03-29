#pragma once
#ifndef __IOCP_ECHO_SERVER__
#define __IOCP_ECHO_SERVER__
#define __UNIV_DEVELOPER_
#include "CLanServer.h"
#include "HardWareMoniteringClass.h"
#include "ProcessMoniteringClass.h"

namespace univ_dev
{
	class EchoServer final : public CLanServer
	{
	public:
		EchoServer(USHORT port, DWORD backlogQueueSize, DWORD threadPoolSize, DWORD runningThread, DWORD nagleOff, ULONGLONG maxSessionCounts);
		~EchoServer()
		{
			Close();
		}
	private:
		void Start();
		void Close();
		friend unsigned __stdcall MoniteringThread(void* param);

		unsigned int EchoServerMoniteringThread(void* param);

		HardWareMoniter							_HardWareMoniter;
		ProcessMoniter							_ProcessMoniter;
		DWORD									_RunningFlag;

		HANDLE _MoniteringThread;

		void OnRecv(ULONGLONG sessionID, Packet* recvPacket) override;
		void OnErrorOccured(DWORD errorCode,const WCHAR* error) override;
		bool OnConnectionRequest(WCHAR* ipStr, DWORD ip, USHORT port) override;
		void OnClientJoin(WCHAR* ipStr, DWORD ip, USHORT port, ULONGLONG sessionID) override;
		void OnClientLeave(ULONGLONG sessionID) override; // Release�� ȣ��
		void OnTimeOut(ULONGLONG sessionID) override;
	};
}



#endif // !__IOCP_ECHO_SERVER__
