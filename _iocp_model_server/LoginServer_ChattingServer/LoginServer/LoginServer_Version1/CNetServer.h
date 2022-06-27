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
		CNetServer(USHORT port, DWORD backlogQueueSize, DWORD threadPoolSize, DWORD runningThread, DWORD nagleOff, ULONGLONG maxSessionCounts, DWORD timeOutTime);
	public:
		void	SendPacket(ULONGLONG sessionID, Packet* packet);
		bool	GetNetCoreInitializeFlag() { return _ServerOnFlag; }
		DWORD	GetLastCoreErrno() { return _ErrorCode; }
		DWORD	GetLastAPIErrno() { return _APIErrorCode; }
		void	DisconnectSession(ULONGLONG sessionID);
		void	Run();


	private:
		//------------------------------------------------------------------------------------------------
		// 서버 시작 종료함수
		void CNetServerStartup();
		void CNetServerCleanup();
		//------------------------------------------------------------------------------------------------


		//------------------------------------------------------------------------------------------------
		// _beginthreadex함수에 전달되는 함수포인터들 param 은 this
		friend unsigned __stdcall WorkerThread(void* param);
		friend unsigned __stdcall AcceptThread(void* param);
		friend unsigned __stdcall TimeOutThread(void* param);
		//friend unsigned __stdcall MoniteringThread(void* param);

		// 실제 러닝 스레드들이 호출할 함수들
		unsigned int CNetServerWorkerThread(void* param);
		unsigned int CNetServerAcceptThread(void* param);
		unsigned int CNetServerTimeOutThread(void* param);
		//unsigned int CNetServerMoniteringThread(void* param);
		//------------------------------------------------------------------------------------------------


		//------------------------------------------------------------------------------------------------
		//가상함수 -> 오버라이딩 필수
		virtual void OnRecv(ULONGLONG sessionID, Packet* recvPacket) = 0;
		virtual void OnErrorOccured(DWORD errorCode, const WCHAR* error) = 0;
		virtual bool OnConnectionRequest(WCHAR* ipStr, DWORD ip, USHORT port) = 0;
		virtual void OnClientJoin(WCHAR* ipStr, DWORD ip, USHORT port, ULONGLONG sessionID) = 0;
		virtual void OnClientLeave(ULONGLONG sessionID) = 0; // Release후 호출
		virtual void OnTimeOut(ULONGLONG sessionID) = 0;

		virtual void OnSend(ULONGLONG sessionID) = 0;

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
		void		SetSessionTimer(Session* session);

		//------------------------------------------------------------------------------------------------
		// 세션 생성및 삭제함수
		Session*	CreateSession(SOCKET key, sockaddr_in clientaddr, ULONGLONG sessionID);
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
		HANDLE									_RunningEvent;

		DWORD									_ServerTime;

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

	private:
		//Debug Field
	protected:
		struct MoniteringInfo
		{
			DWORD								_WorkerThreadCount;
			DWORD								_RunningThreadCount;
			ULONGLONG							_SessionCnt;
			ULONGLONG							_TotalPacket;
			ULONGLONG							_TotalProecessedBytes;
			ULONGLONG							_TotalAcceptSession;
			ULONGLONG							_TotalReleaseSession;
			ULONGLONG							_SendPacketPerSec;
			//마지막 GetMoniteringInfo 이후 PacketCount;
			ULONGLONG							_RecvPacketCount;
			ULONGLONG							_SendPacketCount;
			//마지막 GetMoniteringInfo 이후 AcceptCount;
			ULONGLONG							_AccpeptCount;
			ULONGLONG							_LockFreeQueueSize;
			ULONGLONG							_LockFreeQueueSizeAvr;
			ULONGLONG							_LockFreeQueueCapacity;
			ULONGLONG							_LockFreeMaxCapacity;
			ULONGLONG							_LockFreeStackSize;
			ULONGLONG							_LockFreeStackCapacity;
		};

		void PostNetServerStop();
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

		MoniteringInfo GetMoniteringInfo();
		DWORD GetBeginTime()const { return _BeginTime; }
	};
}



#endif // !__NETSERVER_CORE_CLASS_DEF__
