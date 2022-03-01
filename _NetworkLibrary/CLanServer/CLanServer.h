#pragma once
#ifndef __SERVER_CORE_CLASS_DEF__
#define __SERVER_CORE_CLASS_DEF__
#define __UNIV_DEVELOPER_

#include "CoreBase.h"
#include "NetCoreErrorDefine.h"
#include "Session.h"
#define dfECHO_PACKET_HEADER_LENGTH (short)sizeof(CLanServer::LANPacketHeader)
//#define dfMAX_NUM_OF_THREAD 16

//new delete는... 문제가 없는데
//어째서 object pool은 문제가 되는가...
//일단 네트워크 라이브러리화 시켜놓자 내일은



namespace univ_dev
{


	class CLanServer
	{
	public:
		struct LANPacketHeader
		{
			unsigned short _payloadSize;
		};
		~CLanServer();
		CLanServer(USHORT port, DWORD backlogQueueSize, DWORD threadPoolSize, DWORD runningThread, DWORD nagleOff, ULONGLONG maxSessionCounts);
	public:
		void SendPacket(ULONGLONG sessionID, Packet* packet);
		bool GetNetCoreInitializeFlag() { return _ServerOnFlag; }
		DWORD GetNetCoreErrorCode() { return _ErrorCode; }
		DWORD GetLastAPIErrorCode() { return _APIErrorCode; }
		void DisconnectSession(ULONGLONG sessionID);
		void Run();


	private:
		void CLanServerStartup();
		friend unsigned __stdcall WorkerThread(void* param);
		friend unsigned __stdcall AcceptThread(void* param);
		friend unsigned __stdcall MoniteringThread(void* param);

		//If Packet Arrived OnRecv Will Call By Core
		virtual void OnRecv(ULONGLONG sessionID, Packet* recvPacket) = 0;
		//If Network Core Has Critical Error OnErrorOccured Will Call By Core
		virtual void OnErrorOccured(DWORD errorCode,const WCHAR* error) = 0;
		//If accept return OnConnectionRequest Will Call By Core
		virtual bool OnConnectionRequest(WCHAR* ipStr, DWORD ip, USHORT port) = 0;
		//If OnConnectionRequest Return Was True Core Will CreateSession And Call OnClientJoin By Core
		virtual void OnClientJoin(WCHAR* ipStr, DWORD ip, USHORT port, ULONGLONG sessionID) = 0;
		//If Session Leave And Session Deleted OnClientLeave Will Call By Core
		virtual void OnClientLeave(ULONGLONG sessionID) = 0; // Release후 호출

		void DispatchError(DWORD errorCode,DWORD APIErrorCode,const WCHAR* error);


		void RecvProc(Session* session, DWORD byteTransfered);
		void SendProc(Session* session, DWORD byteTransfered);

		void RecvPost(Session* session);
		void SendPost(Session* session);

		unsigned int CLanServerWorkerThread(void* param);
		unsigned int CLanServerAcceptThread(void* param);
		unsigned int CLanServerMoniteringThread(void* param);

		BOOL TryAccept(SOCKET& clientSocket, sockaddr_in& clientAddr);
		BOOL TryGetCompletedPacket(Session* session, Packet* packet, LANPacketHeader& header);

		Session* FindAndLockSession(ULONGLONG sessionID);
		Session* FindSession(ULONGLONG sessionID);

		void GetStringIP(WCHAR* str,DWORD bufferLen, sockaddr_in& addr);

		void SessionMapLock();
		void SessionMapUnlock();

		void LockSession(Session* session);
		void UnlockSession(Session* session);

		Session* CreateSession(SOCKET key, sockaddr_in clientaddr, ULONGLONG sessionID);
		void ReleaseSession(ULONGLONG sessionID);
		void DisconnectSession(Session* session);
		void CLanServerCleanup();

	private:


		HANDLE _IOCP;
		SOCKET _ListenSocket;
		USHORT _ServerPort;

		//Thread Handler
		HANDLE* _WorkerThreads;
		HANDLE _AcceptThread;
		HANDLE _LogThread;
		DWORD _ThreadPoolSize;
		DWORD _RunningThreadCount;

		DWORD _NagleOff;
		ULONGLONG _MaxSessionCounts;

		//sessionMap Locker
		CRITICAL_SECTION _SessionMapLock;
		std::unordered_map<ULONGLONG, Session*> _SessionMap;
		univ_dev::ObjectFreeList<Session> _SessionPool;
		univ_dev::ObjectFreeList<Packet> _PacketPool;

		//Server Status
		BOOL _ShutDownFlag;
		HANDLE _RunningEvent;


		//Error and codes
		static DWORD _ServerOnFlag;
		DWORD _ErrorCode;
		DWORD _APIErrorCode;


		//Debug Field
	private:
		enum class PROFILING_FLAG
		{
			OFF_FLAG, MAIN_LOOP_FLAG, PACKET_PROCESS_LOOP_FLAG
		};

		PROFILING_FLAG _ProfilingFlag;

		std::unordered_map<DWORD, INT> _ThreadIdMap;

		ULONGLONG _SendSuccessCount;
		ULONGLONG _RecvSuccessCount;
		ULONGLONG _SendIOPendingCount;
		ULONGLONG _RecvIOPendingCount;
		ULONGLONG _PacketPerSec;
		ULONGLONG _TotalPacket;
		LONGLONG _TotalProcessedBytes;
		ULONGLONG _TotalReleasedSession;
		ULONGLONG _TotalAcceptSession;
	};
}



#endif // !__SERVER_CORE_CLASS_DEF__
