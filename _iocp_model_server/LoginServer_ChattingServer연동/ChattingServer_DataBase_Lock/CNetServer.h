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
		// ���� ���� �����Լ�
		void CNetServerStartup();
		void CNetServerCleanup();
		//------------------------------------------------------------------------------------------------


		//------------------------------------------------------------------------------------------------
		// _beginthreadex�Լ��� ���޵Ǵ� �Լ������͵� param �� this
		friend unsigned __stdcall WorkerThread(void* param);
		friend unsigned __stdcall AcceptThread(void* param);
		friend unsigned __stdcall TimeOutThread(void* param);
		//friend unsigned __stdcall MoniteringThread(void* param);

		// ���� ���� ��������� ȣ���� �Լ���
		unsigned int CNetServerWorkerThread(void* param);
		unsigned int CNetServerAcceptThread(void* param);
		unsigned int CNetServerTimeOutThread(void* param);
		//unsigned int CNetServerMoniteringThread(void* param);
		//------------------------------------------------------------------------------------------------


		//------------------------------------------------------------------------------------------------
		//�����Լ� -> �������̵� �ʼ�
		virtual void OnRecv(ULONGLONG sessionID, Packet* recvPacket) = 0;
		virtual void OnErrorOccured(DWORD errorCode, const WCHAR* error) = 0;
		virtual bool OnConnectionRequest(WCHAR* ipStr, DWORD ip, USHORT port) = 0;
		virtual void OnClientJoin(WCHAR* ipStr, DWORD ip, USHORT port, ULONGLONG sessionID) = 0;
		virtual void OnClientLeave(ULONGLONG sessionID) = 0; // Release�� ȣ��
		virtual void OnTimeOut(ULONGLONG sessionID) = 0;

		virtual void OnSend(ULONGLONG sessionID) = 0;

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
		void		SetSessionTimer(Session* session);

		//------------------------------------------------------------------------------------------------
		// ���� ������ �����Լ�
		Session*	CreateSession(SOCKET key, sockaddr_in clientaddr, ULONGLONG sessionID);
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
		HANDLE									_RunningEvent;

		DWORD									_ServerTime;

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
			//������ GetMoniteringInfo ���� PacketCount;
			ULONGLONG							_RecvPacketCount;
			ULONGLONG							_SendPacketCount;
			//������ GetMoniteringInfo ���� AcceptCount;
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

		MoniteringInfo GetMoniteringInfo();
		DWORD GetBeginTime()const { return _BeginTime; }
	};
}



#endif // !__NETSERVER_CORE_CLASS_DEF__
