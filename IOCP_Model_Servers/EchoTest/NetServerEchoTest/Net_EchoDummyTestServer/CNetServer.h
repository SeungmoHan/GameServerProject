#pragma once
#ifndef __NETSERVER_CORE_CLASS_DEF__
#define __NETSERVER_CORE_CLASS_DEF__
#define __UNIV_DEVELOPER_

#include "CoreBase.h"
#include "NetCoreErrorDefine.h"
#include "Session.h"
#include <HardWareMoniteringClass.h>
#include <ProcessMoniteringClass.h>

#include <stack>

//#define dfMAX_NUM_OF_THREAD 16

//new delete��... ������ ���µ�
//��°�� object pool�� ������ �Ǵ°�...
//�ϴ� ��Ʈ��ũ ���̺귯��ȭ ���ѳ��� ������

namespace univ_dev
{
	class CNetServer
	{
	public:
		friend class BaseServer;
		~CNetServer();
		CNetServer();
	public:
		void	InitNetServer(USHORT port, DWORD backlogQueueSize, DWORD threadPoolSize, DWORD runningThread, DWORD nagleOff, ULONGLONG maxSessionCounts, DWORD timeOutClock);
		bool	GetNetCoreInitializeFlag() { return this->_ServerOnFlag; }
		DWORD	GetLastCoreErrno() { return this->_ErrorCode; }
		DWORD	GetLastAPIErrno() { return this->_APIErrorCode; }
		void	Run(HANDLE* threadArr, size_t size);
		void	SendToMoniteringSession(Packet* packet);
		void	DisconnectSession(ULONGLONG sessionID);
		void	SendPacket(ULONGLONG sessionID, Packet* packet);

	private:
		//------------------------------------------------------------------------------------------------
		// ���� ���� �����Լ�
		void Startup();
		void Cleanup();
		//------------------------------------------------------------------------------------------------
	private:
		//------------------------------------------------------------------------------------------------
		// _beginthreadex�Լ��� ���޵Ǵ� �Լ������͵� param �� this
		friend unsigned __stdcall _NET_WorkerThread(void* param);
		friend unsigned __stdcall _NET_AcceptThread(void* param);
		friend unsigned __stdcall _NET_TimeOutThread(void* param);
		friend unsigned __stdcall _NET_MoniteringConnectThread(void* param);
		friend unsigned __stdcall _NET_SendThread(void* param);


		// ���� ���� ��������� ȣ���� �Լ���
		unsigned int CNetServerWorkerThread(void* param);
		unsigned int CNetServerAcceptThread(void* param);
		unsigned int CNetServerTimeOutThread(void* param);
		unsigned int CNetServerMoniteringThread(void* param);
		unsigned int CNetServerSendThread(void* param);
		//------------------------------------------------------------------------------------------------

		//------------------------------------------------------------------------------------------------
		// ����͸� ���� �����Լ� -> �������� �翬�� ��� �ݺ�
		void ConnectMonitoringSession();
		void RecvFromMonitoringSession();


	protected:

		//------------------------------------------------------------------------------------------------
		//�����Լ� -> �������̵� �ʼ�
		virtual void OnRecv(ULONGLONG sessionID, Packet* recvPacket) = 0;
		virtual void OnErrorOccured(DWORD errorCode, const WCHAR* error, LogClass::LogLevel level) = 0;
		virtual bool OnConnectionRequest(WCHAR* ipStr, DWORD ip, USHORT port) = 0;
		virtual void OnClientJoin(WCHAR* ipStr, DWORD ip, USHORT port, ULONGLONG sessionID) = 0;
		virtual void OnClientLeave(ULONGLONG sessionID) = 0; // Release�� ȣ��
		virtual void OnTimeOut(ULONGLONG sessionID) = 0;

		virtual void OnSend(ULONGLONG sessionID) = 0;

		inline void	PostOnClientLeave(ULONGLONG sessionID)
		{
			PostQueuedCompletionStatus(this->_IOCP, 0, sessionID, (LPOVERLAPPED)0xffffffff);
		}
	protected:
		//------------------------------------------------------------------------------------------------
		// ���������� �����ڵ� �����ϸ鼭 OnErrorOccured ȣ��
		inline void DispatchError(DWORD errorCode, DWORD APIErrorCode, const WCHAR* errorStr)
		{
			//���̺귯�� ��ü �����ڵ� ��� �� API���� �ڵ� ���
			this->_ErrorCode = errorCode;
			this->_APIErrorCode = APIErrorCode;
			// OnErrorOccured �Լ��� ���̺귯�� �����ڵ带 �����ϹǷ� GetLastAPIErrorCode �Լ� ȣ���ؼ� ���� ���ߵ�.
			this->OnErrorOccured(errorCode, errorStr, LogClass::LogLevel::LOG_LEVEL_LIBRARY);
		}
		//------------------------------------------------------------------------------------------------
	private:

		//------------------------------------------------------------------------------------------------
		//Send�Ϸ�����, Recv�Ϸ������� ����
		void		RecvProc(Session* session, DWORD byteTransfered);
		void		SendProc(Session* session, DWORD byteTransfered);
		//------------------------------------------------------------------------------------------------

		//------------------------------------------------------------------------------------------------
		//���� WSASend, WSARecvȣ���ϴ� �Լ�
		void		RecvPost(Session* session);
		void		SendPost(Session* session);
		//------------------------------------------------------------------------------------------------


		//------------------------------------------------------------------------------------------------
		// Accept ������ ���� �Ұ����̸� clientSocket�� INVALID_SOCKET��, false ��ȯ�ÿ��� AcceptThread ����
		BOOL		TryAccept(SOCKET& clientSocket, sockaddr_in& clientAddr);
		// �ϳ��� �ϼ��� ��Ŷ�� ���� �Լ�
		BOOL		TryGetCompletedPacket(Session* session, Packet* packet, NetServerHeader& header);


		//------------------------------------------------------------------------------------------------
		// ����ID -> ���������� 
		Session* FindAndLockSession(ULONGLONG sessionID);
		inline Session* FindSession(ULONGLONG sessionID)
		{
			return &this->_SessionArr[sessionID & 0xffff];
		}
		//------------------------------------------------------------------------------------------------


		//------------------------------------------------------------------------------------------------
		// �����Լ� sockaddr_in ����ü�� ������ wide string ���� ��ȯ
		inline void GetStringIP(WCHAR* str, DWORD bufferLen, sockaddr_in& addr)
		{
			wsprintf(str, L"%d.%d.%d.%d", addr.sin_addr.S_un.S_un_b.s_b1, addr.sin_addr.S_un.S_un_b.s_b2, addr.sin_addr.S_un.S_un_b.s_b3, addr.sin_addr.S_un.S_un_b.s_b4);
		}
		//------------------------------------------------------------------------------------------------


		//------------------------------------------------------------------------------------------------
		// ���� �����̳ʿ� ���� ��
		void		SessionMapLock();
		void		SessionMapUnlock();
		//------------------------------------------------------------------------------------------------
		//------------------------------------------------------------------------------------------------
		// ���� ���� ȹ�� �� ��ȯ �Լ�
		Session*	AcquireSession(ULONGLONG SessionID);
		//void		ReturnSession(ULONGLONG SessionID);
		void		ReturnSession(Session* session);
		//------------------------------------------------------------------------------------------------
		inline void SetSessionTimer(Session* session)
		{
			//InterlockedExchange(&session->_TimeOutTimer, timeGetTime());
			session->_TimeOutTimer = timeGetTime();
		}

		//------------------------------------------------------------------------------------------------
		// ���� ������ �����Լ�
		Session*	CreateSession(SOCKET key, sockaddr_in clientaddr, ULONGLONG sessionID);
		void		ReleaseSession(Session* session);
		//------------------------------------------------------------------------------------------------
	private:

		//------------------------------------------------------------------------------------------------

		//------------------------------------------------------------------------------------------------
		// �������� ����ϴ� ������
		// ReadOnly Variable
		HANDLE									_IOCP = nullptr;
		volatile SOCKET							_ListenSocket = INVALID_SOCKET;
		USHORT									_ServerPort;
		//Thread Handler
		HANDLE*									_WorkerThreads = nullptr;
		HANDLE									_AcceptThread = nullptr;
		HANDLE									_TimeOutThread = nullptr;
		HANDLE									_SendThread = nullptr;
		DWORD									_ThreadPoolSize;
		DWORD									_RunningThreadCount;
		DWORD									_NagleOff;
		ULONGLONG								_MaxSessionCounts;
		DWORD									_TimeOutClock;
		//Server Status
	public:
		volatile BOOL							_ShutDownFlag;
	protected:
		HANDLE									_ThreadStartEvent = nullptr;
	private:
		DWORD									_BackLogQueueSize;
		
		DWORD									_ServerTime;

		HANDLE									_MonitoringSignal = nullptr;
		HANDLE									_MoniteringConnectThread = nullptr;
		BOOL									_MoniteringFlag = false;
		Session									_MonitoringServerSession;

	private:
		//Error and codes
		volatile static DWORD					_ServerOnFlag;
		volatile DWORD							_ErrorCode;
		volatile DWORD							_APIErrorCode;
		//------------------------------------------------------------------------------------------------
		//------------------------------------------------------------------------------------------------
		//���� ���� ��ü��
		CRITICAL_SECTION						_SessionMapLock;
		Session*								_SessionArr;
		LockFreeStack<DWORD>					_SessionIdx;


		//Logging Class
		LogClass								_LibraryLog;

		inline bool PopSessionIndex(DWORD& ret)
		{
			return this->_SessionIdx.pop(ret);
		}
		inline void PushSessionIndex(DWORD idx)
		{
			return this->_SessionIdx.push(idx);
		}

	public:
		struct MoniteringInfo
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
	protected:
		inline void WaitForMoniteringSignal()
		{
			::WaitForSingleObject(this->_MonitoringSignal, 2000);
		}
		
		void PostServerStop()
		{
			this->_LibraryLog.LOG_SET_DIRECTORY(L"ServerLog\\InitLog");
			this->_ShutDownFlag = true;
			closesocket(this->_ListenSocket);
		}

		alignas(64) ULONGLONG					_BeginTime = 0;

		// ��Ŷ ó�� ��ġ �� ��Ŷó�� �Ϸ� ����Ʈ��
		alignas(64) ULONGLONG					_CurSessionCount = 0;

		alignas(64) ULONGLONG					_TotalSendPacketCount = 0;
		alignas(64) ULONGLONG					_TotalRecvPacketCount = 0;

		alignas(64) ULONGLONG					_TotalProcessedBytes = 0;

		alignas(64) ULONGLONG					_TotalAcceptSessionCount = 0;
		alignas(64) ULONGLONG					_TotalReleaseSessionCount = 0;

		void GetMoniteringInfo(MoniteringInfo& info);
		DWORD GetBeginTime()const { return _BeginTime; }
	};
}



#endif // !__NETSERVER_CORE_CLASS_DEF__
