#pragma once
#ifndef __NETSERVER_CORE_CLASS_DEF__
#define __NETSERVER_CORE_CLASS_DEF__
#define __UNIV_DEVELOPER_

#include "CoreBase.h"
#include "NetCoreErrorDefine.h"
#include "Session.h"
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
		~CNetServer();
		CNetServer();
	protected:
		void	InitNetServer(USHORT port, DWORD backlogQueueSize, DWORD threadPoolSize, DWORD runningThread, DWORD nagleOff, ULONGLONG maxSessionCounts,DWORD timeOutClock);
	public:
		void	SendToMoniteringSession(Packet* packet);
		void	SendPacket(ULONGLONG sessionID, Packet* packet);
		bool	GetNetCoreInitializeFlag() { return _ServerOnFlag; }
		DWORD	GetLastCoreErrno() { return _ErrorCode; }
		DWORD	GetLastAPIErrno() { return _APIErrorCode; }
		void	DisconnectSession(ULONGLONG sessionID);
		void	Run(HANDLE* threadArr, size_t size);

		inline void MonitoringLog(WCHAR* logStr,LogClass::LogLevel level)
		{
			this->_MonitoringLog.LOG(logStr, level);
		}
	private:
		//------------------------------------------------------------------------------------------------
		// ���� ���� �����Լ�
		void Startup();
		void Cleanup();
		//------------------------------------------------------------------------------------------------


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
		inline void WaitForMoniteringSignal()
		{
			WaitForSingleObject(this->_MonitoringSignal, 2000);
		}
	private:
		//------------------------------------------------------------------------------------------------
		//�����Լ� -> �������̵� �ʼ�
		virtual void OnRecv(ULONGLONG sessionID, Packet* recvPacket) = 0;
		virtual void OnErrorOccured(DWORD errorCode, const WCHAR* error) = 0;
		virtual bool OnConnectionRequest(WCHAR* ipStr, DWORD ip, USHORT port) = 0;
		virtual void OnClientJoin(WCHAR* ipStr, DWORD ip, USHORT port, ULONGLONG sessionID) = 0;
		virtual void OnClientLeave(ULONGLONG sessionID) = 0; // Release�� ȣ��
		virtual void OnTimeOut(ULONGLONG sessionID) = 0;
		virtual void OnSend(ULONGLONG sessionID) = 0;

		inline void PostOnClientLeave(ULONGLONG sessionID)
		{
			PostQueuedCompletionStatus(_IOCP, 0, sessionID, (LPOVERLAPPED)0xffffffff);
		}
	protected:
		//------------------------------------------------------------------------------------------------
		inline void DispatchError(DWORD errorCode, DWORD APIErrorCode, const WCHAR* errorStr)
		{
			//���̺귯�� ��ü �����ڵ� ��� �� API���� �ڵ� ���
			this->_ErrorCode = errorCode;
			this->_APIErrorCode = APIErrorCode;
			// OnErrorOccured �Լ��� ���̺귯�� �����ڵ带 �����ϹǷ� GetLastAPIErrorCode �Լ� ȣ���ؼ� ���� ���ߵ�.
			OnErrorOccured(errorCode, errorStr);
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
			session->_TimeOutTimer = timeGetTime();
			//InterlockedExchange(&session->_TimeOutTimer, timeGetTime());
		}

		//------------------------------------------------------------------------------------------------
		// ���� ������ �����Լ�
		Session*	CreateSession(SOCKET key, sockaddr_in clientaddr, ULONGLONG sessionID);
		void		ReleaseSession(Session* session);
		//------------------------------------------------------------------------------------------------
	private:

		//std::unordered_map<ULONGLONG, Session*> _SessionMap;
		//------------------------------------------------------------------------------------------------

		//USHORT port, DWORD backlogQueueSize, DWORD threadPoolSize, DWORD runningThread, DWORD nagleOff, ULONGLONG maxSessionCounts
		//------------------------------------------------------------------------------------------------
		// �������� ����ϴ� ������
		// ReadOnly Variable
		HANDLE									_IOCP;
		SOCKET									_ListenSocket;
		USHORT									_ServerPort;
		UINT									_BackLogQueueSize;
		//Thread Handle
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

		DWORD									_ServerTime;

		HANDLE									_SendThread;

		BOOL									_MoniteringFlag;
		Session									_MonitoringServerSession;
		HANDLE									_MoniteringConnectThread;

		LogClass								_LibraryLog;
		LogClass								_MonitoringLog;
	private:

		//Error and codes
		static DWORD							_ServerOnFlag;
		DWORD									_ErrorCode;
		DWORD									_APIErrorCode;
		//------------------------------------------------------------------------------------------------
		//------------------------------------------------------------------------------------------------
		//���� ���� ��ü��
		CRITICAL_SECTION						_SessionMapLock;
		Session*								_SessionArr;
		LockFreeStack<DWORD>					_SessionIdx;

		inline bool PopSessionIndex(DWORD& ret)
		{
			return this->_SessionIdx.pop(ret);
		}
		inline void PushSessionIndex(DWORD idx)
		{
			return this->_SessionIdx.push(idx);
		}

	private:
		//Debug Field
	protected:
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

		HANDLE									_MonitoringSignal;
		inline void PostServerStop()
		{
			this->_ShutDownFlag = true;
			closesocket(this->_ListenSocket);
			this->_ListenSocket = INVALID_SOCKET;
			TerminateThread(this->_TimeOutThread, 0);
		}

		alignas(64) ULONGLONG					_BeginTime;

		// ��Ŷ ó�� ��ġ �� ��Ŷó�� �Ϸ� ����Ʈ��
		alignas(64) ULONGLONG					_CurSessionCount;

		alignas(64) ULONGLONG					_TotalSendPacketCount;
		alignas(64) ULONGLONG					_TotalRecvPacketCount;

		alignas(64) ULONGLONG					_TotalProcessedBytes;

		alignas(64) ULONGLONG					_TotalAcceptSessionCount;
		alignas(64) ULONGLONG					_TotalReleaseSessionCount;

		void GetMoniteringInfo(MoniteringInfo& ret);
		inline DWORD GetBeginTime()const { return _BeginTime; }
	};
}



#endif // !__NETSERVER_CORE_CLASS_DEF__
