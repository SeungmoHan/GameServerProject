#pragma once
#ifndef __NETSERVER_CORE_CLASS_DEF__
#define __NETSERVER_CORE_CLASS_DEF__
#define __UNIV_DEVELOPER_

#include "CoreBase.h"
#include "NetCoreErrorDefine.h"
#include "Session.h"

#include <stack>

//#define dfMAX_NUM_OF_THREAD 16

//new delete는... 문제가 없는데
//어째서 object pool은 문제가 되는가...
//일단 네트워크 라이브러리화 시켜놓자 내일은

namespace univ_dev
{


	class CNetServer
	{
	public:
		~CNetServer();
		CNetServer();
	protected:
		void	InitServer(USHORT port, DWORD backlogQueueSize, DWORD threadPoolSize, DWORD runningThread, DWORD nagleOff, ULONGLONG maxSessionCounts,DWORD timeOutClock);
	public:
		void	SendPacket(ULONGLONG sessionID, Packet* packet);
		bool	GetNetCoreInitializeFlag() { return _ServerOnFlag; }
		DWORD	GetNetCoreErrorCode() { return _ErrorCode; }
		DWORD	GetLastAPIErrorCode() { return _APIErrorCode; }
		void	DisconnectSession(ULONGLONG sessionID);
		void	Run(HANDLE* threadArr, size_t size);


	private:
		//------------------------------------------------------------------------------------------------
		// 서버 시작 종료함수
		void StartUp();
		void CleanUp();
		//------------------------------------------------------------------------------------------------


		//------------------------------------------------------------------------------------------------
		// _beginthreadex함수에 전달되는 함수포인터들 param 은 this
		friend unsigned __stdcall _NET_WorkerThread(void* param);
		friend unsigned __stdcall _NET_AcceptThread(void* param);
		friend unsigned __stdcall _NET_TimeOutThread(void* param);
		friend unsigned __stdcall _NET_MonitoringConnectThread(void* param);

		// 실제 러닝 스레드들이 호출할 함수들
		unsigned int CNetServerWorkerThread(void* param);
		unsigned int CNetServerAcceptThread(void* param);
		unsigned int CNetServerTimeOutThread(void* param);
		unsigned int CNetServerMonitoringThread(void* param);
		//------------------------------------------------------------------------------------------------


		//------------------------------------------------------------------------------------------------
		//가상함수 -> 오버라이딩 필수
		virtual void OnRecv(ULONGLONG sessionID, Packet* recvPacket) = 0;
		virtual void OnErrorOccured(DWORD errorCode, const WCHAR* error) = 0;
		virtual bool OnConnectionRequest(WCHAR* ipStr, DWORD ip, USHORT port) = 0;
		virtual void OnClientJoin(WCHAR* ipStr, DWORD ip, USHORT port, ULONGLONG sessionID) = 0;
		virtual void OnClientLeave(ULONGLONG sessionID) = 0; // Release후 호출
		virtual void OnTimeOut(ULONGLONG sessionID) = 0;

		void		 PostOnClientLeave(ULONGLONG sessionID);
	protected:
		//------------------------------------------------------------------------------------------------
		//에러가 있을때 마지막 에러코드를 저장하고 OnErrorOccured함수 호출
		void		DispatchError(DWORD errorCode, DWORD APIErrorCode, const WCHAR* error);
		//------------------------------------------------------------------------------------------------
	private:

		//------------------------------------------------------------------------------------------------
		//Send완료통지, Recv완료통지시 수행
		void		RecvProc(Session* session, DWORD byteTransfered);
		void		SendProc(Session* session, DWORD byteTransfered);
		//------------------------------------------------------------------------------------------------


		//------------------------------------------------------------------------------------------------
		//실제 WSASend, WSARecv호출하는 함수
		void		RecvPost(Session* session);
		void		SendPost(Session* session);
		//------------------------------------------------------------------------------------------------


		//------------------------------------------------------------------------------------------------
		// Accept 수행후 수행 불가능이면 clientSocket이 INVALID_SOCKET임, false 반환시에는 AcceptThread 종료
		BOOL		TryAccept(SOCKET& clientSocket, sockaddr_in& clientAddr);
		// 하나의 완성된 패킷을 얻어내는 함수
		BOOL		TryGetCompletedPacket(Session* session, Packet* packet, NetServerHeader& header);


		//------------------------------------------------------------------------------------------------
		// 세션ID -> 세션포인터 
		Session* FindAndLockSession(ULONGLONG sessionID);
		Session* FindSession(ULONGLONG sessionID);
		//------------------------------------------------------------------------------------------------


		//------------------------------------------------------------------------------------------------
		// 헬퍼함수 sockaddr_in 구조체를 넣으면 wide string 으로 반환
		void		GetStringIP(WCHAR* str, DWORD bufferLen, sockaddr_in& addr);
		//------------------------------------------------------------------------------------------------


		//------------------------------------------------------------------------------------------------
		// 세션 컨테이너에 대한 락
		void		SessionMapLock();
		void		SessionMapUnlock();
		//------------------------------------------------------------------------------------------------
		//------------------------------------------------------------------------------------------------
		// 세션 사용권 획득 및 반환 함수
		Session*	AcquireSession(ULONGLONG SessionID);
		//void		ReturnSession(ULONGLONG SessionID);
		void		ReturnSession(Session* session);
		//------------------------------------------------------------------------------------------------


		//------------------------------------------------------------------------------------------------
		// 세션 생성및 삭제함수
		Session*	CreateSession(SOCKET key, sockaddr_in clientaddr, ULONGLONG sessionID);
		void		ReleaseSession(Session* session);
		//------------------------------------------------------------------------------------------------
	private:

		//std::unordered_map<ULONGLONG, Session*> _SessionMap;
		//------------------------------------------------------------------------------------------------

		//USHORT port, DWORD backlogQueueSize, DWORD threadPoolSize, DWORD runningThread, DWORD nagleOff, ULONGLONG maxSessionCounts
		//------------------------------------------------------------------------------------------------
		// 서버에서 사용하는 변수들
		// ReadOnly Variable
		HANDLE									_IOCP;
		SOCKET									_ListenSocket;
		USHORT									_ServerPort;
		UINT									_BackLogQueueSize;
		//Thread Handler
		HANDLE*									_WorkerThreads;
		HANDLE									_AcceptThread;
		HANDLE									_TimeOutThread;
		DWORD									_ThreadPoolSize;
		DWORD									_RunningThreadCount;
		DWORD									_NagleOff;
		ULONGLONG								_MaxSessionCounts;
		DWORD									_TimeOutClock;
		//Server Status
		BOOL									_ShutDownFlag;
		HANDLE									_ThreadStartEvent;


private:

		//Error and codes
		static DWORD							_ServerOnFlag;
		DWORD									_ErrorCode;
		DWORD									_APIErrorCode;
		//------------------------------------------------------------------------------------------------
		//------------------------------------------------------------------------------------------------
		//세션 관련 객체들
		CRITICAL_SECTION						_SessionMapLock;
		Session*								_SessionArr;
		LockFreeStack<DWORD>					_SessionIdx;


		bool		PopSessionIndex(DWORD& ret);
		void		PushSessionIndex(DWORD idx);

		LogClass								_LibraryLog;

	private:
		//Debug Field
	protected:
		struct MonitoringInfo
		{
			DWORD								_WorkerThreadCount;
			DWORD								_RunningThreadCount;
			DWORD								_CurrentSessionCount;

			ULONGLONG							_TotalProcessedBytes;

			ULONGLONG							_TotalAcceptSessionCount;
			ULONGLONG							_TotalReleaseSessionCount;

			ULONGLONG							_TotalSendPacketCount;
			ULONGLONG							_TotalRecvPacketCount;

			DWORD								_SessionSendQueueSize;
			DWORD								_SessionSendQueueCapacity;
			DWORD								_SessionSendQueueMax;

			DWORD								_SessionIndexStackSize;
			DWORD								_SessionIndexStackCapacity;
		};

		HANDLE									_MonitoringSignal = nullptr;
		void PostServerStop();
		alignas(64) ULONGLONG					_BeginTime = 0;

		// 패킷 처리 수치 및 패킷처리 완료 바이트수
		alignas(64) ULONGLONG					_CurSessionCount = 0;

		alignas(64) ULONGLONG					_TotalSendPacketCount = 0;
		alignas(64) ULONGLONG					_TotalRecvPacketCount = 0;

		alignas(64) ULONGLONG					_TotalProcessedBytes = 0;

		alignas(64) ULONGLONG					_TotalAcceptSessionCount = 0;
		alignas(64) ULONGLONG					_TotalReleaseSessionCount = 0;

		void GetMonitoringInfo(MonitoringInfo& ret);
		DWORD GetBeginTime()const { return _BeginTime; }
	};
}



#endif // !__NETSERVER_CORE_CLASS_DEF__
