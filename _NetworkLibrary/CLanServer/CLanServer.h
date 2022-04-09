#pragma once
#ifndef __SERVER_CORE_CLASS_DEF__
#define __SERVER_CORE_CLASS_DEF__
#define __UNIV_DEVELOPER_

#include "CoreBase.h"
#include "NetCoreErrorDefine.h"
#include "Session.h"

#include <stack>

#define dfECHO_PACKET_HEADER_LENGTH (short)sizeof(CLanServer::LANPacketHeader)
//#define dfMAX_NUM_OF_THREAD 16

//new delete��... ������ ���µ�
//��°�� object pool�� ������ �Ǵ°�...
//�ϴ� ��Ʈ��ũ ���̺귯��ȭ ���ѳ��� ������



namespace univ_dev
{


	class CLanServer
	{
	public:
		struct LANPacketHeader
		{
			unsigned short _payloadSize;
		};
		~CLanServer	();
		CLanServer	(USHORT port, DWORD backlogQueueSize, DWORD threadPoolSize, DWORD runningThread, DWORD nagleOff, ULONGLONG maxSessionCounts);
	public:
		void	SendPacket							(ULONGLONG sessionID, Packet* packet);
		bool	GetNetCoreInitializeFlag			() { return _ServerOnFlag; }
		DWORD	GetNetCoreErrorCode					() { return _ErrorCode; }
		DWORD	GetLastAPIErrorCode					() { return _APIErrorCode; }
		void	DisconnectSession					(ULONGLONG sessionID);
		void	Run();


	private:
		//------------------------------------------------------------------------------------------------
		// ���� ���� �����Լ�
		void CLanServerStartup();
		void CLanServerCleanup();
		//------------------------------------------------------------------------------------------------


		//------------------------------------------------------------------------------------------------
		// _beginthreadex�Լ��� ���޵Ǵ� �Լ������͵� param �� this
		friend unsigned __stdcall WorkerThread		(void* param);
		friend unsigned __stdcall AcceptThread		(void* param);
		friend unsigned __stdcall MoniteringThread	(void* param);
		// ���� ���� ��������� ȣ���� �Լ���
		unsigned int CLanServerWorkerThread			(void* param);
		unsigned int CLanServerAcceptThread			(void* param);
		unsigned int CLanServerMoniteringThread		(void* param);
		//------------------------------------------------------------------------------------------------


		//------------------------------------------------------------------------------------------------
		//�����Լ� -> �������̵� �ʼ�
		virtual void OnRecv					(ULONGLONG sessionID, Packet* recvPacket) = 0;
		virtual void OnErrorOccured			(DWORD errorCode,const WCHAR* error) = 0;
		virtual bool OnConnectionRequest	(WCHAR* ipStr, DWORD ip, USHORT port) = 0;
		virtual void OnClientJoin			(WCHAR* ipStr, DWORD ip, USHORT port, ULONGLONG sessionID) = 0;
		virtual void OnClientLeave			(ULONGLONG sessionID) = 0; // Release�� ȣ��

		//------------------------------------------------------------------------------------------------
		//������ ������ ������ �����ڵ带 �����ϰ� OnErrorOccured�Լ� ȣ��
		void		DispatchError			(DWORD errorCode,DWORD APIErrorCode,const WCHAR* error);
		//------------------------------------------------------------------------------------------------

		//------------------------------------------------------------------------------------------------
		//Send�Ϸ�����, Recv�Ϸ������� ����
		void		RecvProc				(Session* session, DWORD byteTransfered);
		void		SendProc				(Session* session, DWORD byteTransfered);
		//------------------------------------------------------------------------------------------------


		//------------------------------------------------------------------------------------------------
		//���� WSASend, WSARecvȣ���ϴ� �Լ�
		void		RecvPost				(Session* session);
		void		SendPost				(Session* session);
		//------------------------------------------------------------------------------------------------


		//------------------------------------------------------------------------------------------------
		// Accept ������ ���� �Ұ����̸� clientSocket�� INVALID_SOCKET��, false ��ȯ�ÿ��� AcceptThread ����
		BOOL		TryAccept				(SOCKET& clientSocket, sockaddr_in& clientAddr);
		// �ϳ��� �ϼ��� ��Ŷ�� ���� �Լ�
		BOOL		TryGetCompletedPacket	(Session* session, Packet* packet, LANPacketHeader& header);


		//------------------------------------------------------------------------------------------------
		// ����ID -> ���������� 
		Session*	FindAndLockSession		(ULONGLONG sessionID);
		Session*	FindSession				(ULONGLONG sessionID);
		bool		IsSessionValid			(Session* session);
		bool		IsSessionValid			(ULONGLONG sessionID);
		//------------------------------------------------------------------------------------------------

		
		//------------------------------------------------------------------------------------------------
		// �����Լ� sockaddr_in ����ü�� ������ wide string ���� ��ȯ
		void		GetStringIP				(WCHAR* str,DWORD bufferLen, sockaddr_in& addr);
		//------------------------------------------------------------------------------------------------


		//------------------------------------------------------------------------------------------------
		// ���� �����̳ʿ� ���� ��
		void		SessionMapLock			();
		void		SessionMapUnlock		();
		// ���� ��ü�� ���� ��(������� ����)
		void		LockSession				(Session* session);
		void		UnlockSession			(Session* session);
		//------------------------------------------------------------------------------------------------


		//------------------------------------------------------------------------------------------------
		// ���� ������ �����Լ�
		Session*	CreateSession			(SOCKET key, sockaddr_in clientaddr, ULONGLONG sessionID);
		void		ReleaseSession			(ULONGLONG sessionID);
		void		DisconnectSession		(Session* session);
		//------------------------------------------------------------------------------------------------
	private:
		//------------------------------------------------------------------------------------------------
		//���� ���� ��ü��
		CRITICAL_SECTION						_SessionMapLock;
		Session*								_SessionArr;
		std::stack<DWORD>							_SessionIdx;

		
		bool		PopSessionIndex			(DWORD& ret);
		void		PushSessionIndex		(DWORD idx);

		std::unordered_map<ULONGLONG, Session*> _SessionMap;
		ObjectFreeList<Session>					_SessionPool;
		//------------------------------------------------------------------------------------------------

		//------------------------------------------------------------------------------------------------
		// �������� ����ϴ� ������
		HANDLE									_IOCP;
		SOCKET									_ListenSocket;
		USHORT									_ServerPort;

		//Thread Handler
		HANDLE									*_WorkerThreads;
		HANDLE									_AcceptThread;
		HANDLE									_LogThread;
		DWORD									_ThreadPoolSize;
		DWORD									_RunningThreadCount;
		DWORD									_NagleOff;
		ULONGLONG								_MaxSessionCounts;

		//Server Status
		BOOL									_ShutDownFlag;
		HANDLE									_RunningEvent;

		//Error and codes
		static DWORD							_ServerOnFlag;
		DWORD									_ErrorCode;
		DWORD									_APIErrorCode;
		//------------------------------------------------------------------------------------------------


	protected:
		//�ӽ÷� ����� ��ŶǮ
		univ_dev::ObjectFreeList<Packet>		_PacketPool;

		DWORD									_BeginTime;
	private:
		





		//Debug Field
	private:
		enum class PROFILING_FLAG
		{
			OFF_FLAG, MAIN_LOOP_FLAG, PACKET_PROCESS_LOOP_FLAG
		};

		PROFILING_FLAG _ProfilingFlag;

		std::unordered_map<DWORD, INT>			_ThreadIdMap;


		//�� Send, Recv ���� ó���Ϸ� ī��Ʈ
		ULONGLONG								_SendSuccessCount;
		ULONGLONG								_RecvSuccessCount;
		//�� Send, Recv �񵿱� ó���Ϸ� ī��Ʈ
		ULONGLONG								_SendIOPendingCount;
		ULONGLONG								_RecvIOPendingCount;

		// ��Ŷ ó�� ��ġ �� ��Ŷó�� �Ϸ� ����Ʈ��
		ULONGLONG								_TotalPacket;
		ULONGLONG								_PacketPerSec;
		LONGLONG								_TotalProcessedBytes;

		//Accept Thread���� ����ϴ� ����
		ULONGLONG								_AcceptPerSec;
		ULONGLONG								_TotalAcceptSession;
		ULONGLONG								_TotalReleasedSession;
	};
}



#endif // !__SERVER_CORE_CLASS_DEF__
