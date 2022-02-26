#pragma once
#ifndef __SERVER_CORE_CLASS_DEF__
#define __SERVER_CORE_CLASS_DEF__
#define __UNIV_DEVELOPER_

#include "Session.h"
#include "NetCoreErrorDefine.h"
#define dfECHO_PACKET_HEADER_LENGTH (int)sizeof(EchoPacketHeader)
#define dfMAX_NUM_OF_THREAD 16

namespace univ_dev
{
	
	struct EchoPacketHeader
	{
		unsigned short payloadSize;
	};

	class NetCore
	{
	public:
		~NetCore();
		NetCore(USHORT port, DWORD backlogQueueSize, DWORD threadPoolSize, DWORD runningThread);
	public:
		void Run();
		void SendPacket(ULONGLONG sessionID, Packet* packet);
		bool GetNetCoreInitializeFlag() { return serverOnFlag; }
		DWORD GetNetCoreErrorCode() { return errorCode; }
		DWORD GetLastAPIErrorCode() { return APIErrorCode; }

	private:
		friend unsigned __stdcall WorkerThread(void* param);
		friend unsigned __stdcall AcceptThread(void* param);
		friend unsigned __stdcall MoniteringThread(void* param);

		virtual void OnRecv(Session* session, Packet* recvPacket,Packet* sendPacket,EchoPacketHeader header);
		
		void SendPacket(Session* session, Packet* packet);


		void RecvProc(Session* session, DWORD byteTransfered);
		void SendProc(Session* session, DWORD byteTransfered);
		
		void RecvPost(Session* session);
		void SendPost(Session* session);

		unsigned int NetCoreWorkerThread(void* param);
		unsigned int NetCoreAcceptThread(void* param);
		unsigned int NetCoreMoniteringThread(void* param);

		BOOL TryAccept(SOCKET& clientSocket, sockaddr_in& clientAddr);
		BOOL TryGetCompletedPacket(Session* session, Packet* packet,EchoPacketHeader &header);


		Session* FindAndLockSession(ULONGLONG sessionID);
		Session* FindSession(ULONGLONG sessionID);
		
		void SessionMapLock();
		void SessionMapUnlock();

		void LockSession(Session* session);
		void UnlockSession(Session* session);

		Session* CreateSession(SOCKET key, sockaddr_in clientaddr,ULONGLONG sessionID);
		void ReleaseSession(ULONGLONG sessionID);
		void DisconnectSession(ULONGLONG sessionID);
		void NetCoreCleanup();

	private:
		HANDLE IOCP;
		SOCKET listenSocket;
		USHORT serverPort;

		//Thread Handler
		HANDLE* workerThread;
		HANDLE acceptThread;
		HANDLE logThread;
		DWORD threadPoolSize;
		DWORD runningThreadCount;

		//sessionMap Locker
		SRWLOCK sessionMapLock;
		std::unordered_map<ULONGLONG, Session*> sessionMap;
		univ_dev::ObjectFreeList<Session> sessionPool;
		univ_dev::ObjectFreeList<Packet> packetPool;

		//Server Status
		BOOL shutDownFlag;
		HANDLE runningEvent;
		

		//Error and codes
		static DWORD serverOnFlag;
		DWORD errorCode;
		DWORD APIErrorCode;


		//Debug Field
	private:
		enum class PROFILING_FLAG
		{
			OFF_FLAG, MAIN_LOOP_FLAG, PACKET_PROCESS_LOOP_FLAG
		};

		PROFILING_FLAG profilingFlag;

		std::unordered_map<DWORD, INT> threadIds;

		ULONGLONG sendSuccessCount;
		ULONGLONG recvSuccessCount;
		ULONGLONG sendIOPendingCount;
		ULONGLONG recvIOPendingCount;
		ULONGLONG packetPerSec;
		ULONGLONG totalPacket;
		LONGLONG totalProcessedBytes;

		ULONGLONG totalReleasedSession;

	};
}



#endif // !__SERVER_CORE_CLASS_DEF__
