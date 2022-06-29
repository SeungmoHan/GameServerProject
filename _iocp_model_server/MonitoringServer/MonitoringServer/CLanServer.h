#pragma once
#ifndef __LANSERVER_CORE_CLASS_DEF__
#define __LANSERVER_CORE_CLASS_DEF__
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
		// ���� ���� �����Լ�
		void Startup();
		void Cleanup();
		//------------------------------------------------------------------------------------------------


		//------------------------------------------------------------------------------------------------
		// _beginthreadex�Լ��� ���޵Ǵ� �Լ������͵� param �� this
		friend unsigned __stdcall _LAN_WorkerThread		(void* param);
		friend unsigned __stdcall _LAN_AcceptThread		(void* param);
		friend unsigned __stdcall _LAN_TimeOutThread	(void* param);
		friend unsigned __stdcall _LAN_MonitoringThread	(void* param);


		// ���� ���� ��������� ȣ���� �Լ���
		unsigned int CLanServerWorkerThread			(void* param);
		unsigned int CLanServerAcceptThread			(void* param);
		unsigned int CLanServerTimeOutThread		(void* param);
		unsigned int CLanServerMonitoringThread		(void* param);
		//------------------------------------------------------------------------------------------------
		
		
		void ListenMonitoringSessionSocket();
		//------------------------------------------------------------------------------------------------
		// ����͸� ���� �����Լ� -> �������� �翬�� ��� �ݺ�
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
		//�����Լ� -> �������̵� �ʼ�
		virtual void OnRecv(ULONGLONG sessionID, Packet* recvPacket) = 0;
		virtual void OnErrorOccured(DWORD errorCode, const WCHAR* error) = 0;
		virtual bool OnConnectionRequest(WCHAR* ipStr, DWORD ip, USHORT port) = 0;
		virtual void OnClientJoin(WCHAR* ipStr, DWORD ip, USHORT port, ULONGLONG sessionID) = 0;
		virtual void OnClientLeave(ULONGLONG sessionID) = 0; // Release�� ȣ��
		virtual void OnTimeOut(ULONGLONG sessionID) = 0;

		void PostOnClientLeave(ULONGLONG sessionID);

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
		BOOL		TryGetCompletedPacket(Session* session, Packet* packet, LanServerHeader& header);


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
		Session* AcquireSession(ULONGLONG SessionID);
		//void		ReturnSession(ULONGLONG SessionID);
		void		ReturnSession(Session* session);
		//------------------------------------------------------------------------------------------------


		//------------------------------------------------------------------------------------------------
		// ���� ������ �����Լ�
		Session* CreateSession(SOCKET key, sockaddr_in clientaddr, ULONGLONG sessionID);
		void		ReleaseSession(Session* session);
		//------------------------------------------------------------------------------------------------
	private:

		//std::unordered_map<ULONGLONG, Session*> _SessionMap;
		//------------------------------------------------------------------------------------------------

		//------------------------------------------------------------------------------------------------
		// �������� ����ϴ� ������
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
		//���� ���� ��ü��
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
			//������ GetMonitoringInfo ���� PacketCount;
			ULONGLONG							_RecvPacketCount;
			ULONGLONG							_SendPacketCount;
			//������ GetMonitoringInfo ���� AcceptCount;
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

		// ��Ŷ ó�� ��ġ �� ��Ŷó�� �Ϸ� ����Ʈ��
		alignas(64) ULONGLONG					_CurSessionCount;
		alignas(64) ULONGLONG					_TotalPacket;
		alignas(64) ULONGLONG					_RecvPacketPerSec;
		alignas(64) ULONGLONG					_SendPacketPerSec;

		alignas(64) LONGLONG					_TotalProcessedBytes;

		//Accept Thread���� ����ϴ� ����
		alignas(64) ULONGLONG					_AcceptPerSec;
		alignas(64) ULONGLONG					_TotalAcceptSession;
		alignas(64) ULONGLONG					_TotalReleasedSession;

		MonitoringInfo GetMonitoringInfo();
		DWORD GetBeginTime()const { return this->_BeginTime; }
	};
}



#endif // !__LANSERVER_CORE_CLASS_DEF__
