#pragma once
#ifndef __LANSERVER_CORE_CLASS_DEF__
#define __LANSERVER_CORE_CLASS_DEF__
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


	class CLanServer
	{
	public:

		~CLanServer	();
		CLanServer	();
	public:
		void	InitServer(USHORT port, DWORD backlogQueueSize, DWORD threadPoolSize, DWORD runningThread, DWORD nagleOff, ULONGLONG maxSessionCounts,DWORD timeOutClock);
		void	SendPacket							(ULONGLONG sessionID, Packet* packet);
		void	SendPacketNetHeader					(ULONGLONG sessionID, Packet* packet);
		bool	GetNetCoreInitializeFlag			() { return this->_ServerOnFlag; }
		DWORD	GetNetCoreErrorCode					() { return this->_ErrorCode; }
		DWORD	GetLastAPIErrorCode					() { return this->_APIErrorCode; }
		void	SendToMonitoringSession				(Packet* packet);
		void	DisconnectSession					(ULONGLONG sessionID);
		void	Run(HANDLE* threadArr, size_t size);


	private:
		//------------------------------------------------------------------------------------------------
		// 서버 시작 종료함수
		void Startup();
		void Cleanup();
		//------------------------------------------------------------------------------------------------


		//------------------------------------------------------------------------------------------------
		// _beginthreadex함수에 전달되는 함수포인터들 param 은 this
		friend unsigned __stdcall _LAN_WorkerThread		(void* param);
		friend unsigned __stdcall _LAN_AcceptThread		(void* param);
		friend unsigned __stdcall _LAN_TimeOutThread	(void* param);
		friend unsigned __stdcall _LAN_MonitoringThread	(void* param);


		// 실제 러닝 스레드들이 호출할 함수들
		unsigned int CLanServerWorkerThread			(void* param);
		unsigned int CLanServerAcceptThread			(void* param);
		unsigned int CLanServerTimeOutThread		(void* param);
		unsigned int CLanServerMonitoringThread		(void* param);
		//------------------------------------------------------------------------------------------------
		
		
		void ListenMonitoringSessionSocket();
		//------------------------------------------------------------------------------------------------
		// 모니터링 서버 연결함수 -> 끊어지면 재연결 계속 반복
		void AcceptMonitoringSession();
		void ConnectMonitoringSession();
		void RecvFromMonitoringSession();
		void DisconnectMonitoringSession();
	protected:
		inline void WaitForMonitoringSignal()
		{
			WaitForSingleObject(this->_MonitoringSignal, 2000);
		}

		//------------------------------------------------------------------------------------------------
		//가상함수 -> 오버라이딩 필수
		virtual void OnRecv(ULONGLONG sessionID, Packet* recvPacket) = 0;
		virtual void OnErrorOccured(DWORD errorCode, const WCHAR* error) = 0;
		virtual bool OnConnectionRequest(WCHAR* ipStr, DWORD ip, USHORT port) = 0;
		virtual void OnClientJoin(WCHAR* ipStr, DWORD ip, USHORT port, ULONGLONG sessionID) = 0;
		virtual void OnClientLeave(ULONGLONG sessionID) = 0; // Release후 호출
		virtual void OnTimeOut(ULONGLONG sessionID) = 0;

		void PostOnClientLeave(ULONGLONG sessionID);

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
		BOOL		TryGetCompletedPacket(Session* session, Packet* packet, LanServerHeader& header);


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
		Session* AcquireSession(ULONGLONG SessionID);
		//void		ReturnSession(ULONGLONG SessionID);
		void		ReturnSession(Session* session);
		//------------------------------------------------------------------------------------------------


		//------------------------------------------------------------------------------------------------
		// 세션 생성및 삭제함수
		Session* CreateSession(SOCKET key, sockaddr_in clientaddr, ULONGLONG sessionID);
		void		ReleaseSession(Session* session);
		//------------------------------------------------------------------------------------------------
	private:

		//std::unordered_map<ULONGLONG, Session*> _SessionMap;
		//------------------------------------------------------------------------------------------------

		//------------------------------------------------------------------------------------------------
		// 서버에서 사용하는 변수들
		// ReadOnly Variable
		HANDLE									_IOCP;
		SOCKET									_ListenSocket;
		USHORT									_ServerPort;
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
		DWORD									_BackLogQueueSize;

		HANDLE									_MonitoringSignal= nullptr;
		HANDLE									_MonitoringConnectThread;
		DWORD									_MonitoringFlag = false;
		Session									_MonitoringSession;
		USHORT									_MonitoringSessionConnectPort;
		SOCKET									_MonitoringListenSocket;

		DWORD									_LastMonitoringSessionHeartBeat = 0;

		LogClass								_LibraryLog;

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

	private:
		//Debug Field
	protected:
		struct MonitoringInfo
		{
			DWORD								_WorkerThreadCount;
			DWORD								_RunningThreadCount;
			ULONGLONG							_SessionCnt;
			ULONGLONG							_TotalPacket;
			ULONGLONG							_TotalProecessedBytes;
			ULONGLONG							_TotalAcceptSession;
			ULONGLONG							_TotalReleaseSession;
			ULONGLONG							_SendPacketPerSec;
			//마지막 GetMonitoringInfo 이후 PacketCount;
			ULONGLONG							_RecvPacketCount;
			ULONGLONG							_SendPacketCount;
			//마지막 GetMonitoringInfo 이후 AcceptCount;
			ULONGLONG							_AccpeptCount;
			ULONGLONG							_LockFreeQueueSize;
			ULONGLONG							_LockFreeQueueSizeAvr;
			ULONGLONG							_LockFreeQueueCapacity;
			ULONGLONG							_LockFreeMaxCapacity;
			ULONGLONG							_LockFreeStackSize;
			ULONGLONG							_LockFreeStackCapacity;
		};

		void PostLanServerStop();
		alignas(64) ULONGLONG					_BeginTime;

		// 패킷 처리 수치 및 패킷처리 완료 바이트수
		alignas(64) ULONGLONG					_CurSessionCount;
		alignas(64) ULONGLONG					_TotalPacket;
		alignas(64) ULONGLONG					_RecvPacketPerSec;
		alignas(64) ULONGLONG					_SendPacketPerSec;

		alignas(64) LONGLONG					_TotalProcessedBytes;

		//Accept Thread에서 사용하는 변수
		alignas(64) ULONGLONG					_AcceptPerSec;
		alignas(64) ULONGLONG					_TotalAcceptSession;
		alignas(64) ULONGLONG					_TotalReleasedSession;

		MonitoringInfo GetMonitoringInfo();
		DWORD GetBeginTime()const { return this->_BeginTime; }
	};
}



#endif // !__LANSERVER_CORE_CLASS_DEF__
