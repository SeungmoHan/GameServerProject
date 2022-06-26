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
		// ���� ���� �����Լ�
		void StartUp();
		void CleanUp();
		//------------------------------------------------------------------------------------------------


		//------------------------------------------------------------------------------------------------
		// _beginthreadex�Լ��� ���޵Ǵ� �Լ������͵� param �� this
		friend unsigned __stdcall _NET_WorkerThread(void* param);
		friend unsigned __stdcall _NET_AcceptThread(void* param);
		friend unsigned __stdcall _NET_TimeOutThread(void* param);
		friend unsigned __stdcall _NET_MonitoringConnectThread(void* param);

		// ���� ���� ��������� ȣ���� �Լ���
		unsigned int CNetServerWorkerThread(void* param);
		unsigned int CNetServerAcceptThread(void* param);
		unsigned int CNetServerTimeOutThread(void* param);
		unsigned int CNetServerMonitoringThread(void* param);
		//------------------------------------------------------------------------------------------------


		//------------------------------------------------------------------------------------------------
		//�����Լ� -> �������̵� �ʼ�
		virtual void OnRecv(ULONGLONG sessionID, Packet* recvPacket) = 0;
		virtual void OnErrorOccured(DWORD errorCode, const WCHAR* error) = 0;
		virtual bool OnConnectionRequest(WCHAR* ipStr, DWORD ip, USHORT port) = 0;
		virtual void OnClientJoin(WCHAR* ipStr, DWORD ip, USHORT port, ULONGLONG sessionID) = 0;
		virtual void OnClientLeave(ULONGLONG sessionID) = 0; // Release�� ȣ��
		virtual void OnTimeOut(ULONGLONG sessionID) = 0;

		void		 PostOnClientLeave(ULONGLONG sessionID);
	protected:
		//------------------------------------------------------------------------------------------------
		//������ ������ ������ �����ڵ带 �����ϰ� OnErrorOccured�Լ� ȣ��
		void		DispatchError(DWORD errorCode, DWORD APIErrorCode, const WCHAR* error);
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
		Session* FindSession(ULONGLONG sessionID);
		//------------------------------------------------------------------------------------------------


		//------------------------------------------------------------------------------------------------
		// �����Լ� sockaddr_in ����ü�� ������ wide string ���� ��ȯ
		void		GetStringIP(WCHAR* str, DWORD bufferLen, sockaddr_in& addr);
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
		//���� ���� ��ü��
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

		// ��Ŷ ó�� ��ġ �� ��Ŷó�� �Ϸ� ����Ʈ��
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
