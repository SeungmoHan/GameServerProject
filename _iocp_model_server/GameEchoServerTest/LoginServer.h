#pragma once
#ifndef __LOGIN_SERVER__
#define __LOGIN_SERVER__
#define __UNIV_DEVELOPER_

#include <cpp_redis/cpp_redis>
#include "CNetServer.h"
#include "DBConnector.h"
#include "HardWareMoniteringClass.h"
#include "ProcessMoniteringClass.h"
#include "CommonProtocol.h"
#pragma comment (lib, "cpp_redis.lib")
#pragma comment (lib, "tacopie.lib")
// IP
// LOGIN SERVER : 10.0.2.1
// CHATTING SERVER : 10.0.2.1
// GAME SERVER : 10.0.2.1


// PORT
// LOGIN SERVER : 10435
// CHATTING SERVER : 10445
// GAME SERVER : 10455


//------------------------------------------------------
// Login Server
//------------------------------------------------------
//------------------------------------------------------------
// �α��� ������ Ŭ���̾�Ʈ �α��� ��û
//	{
//		WORD	Type
//
//		INT64	AccountNo
//		char	SessionKey[64]
//	}

// �α��� �������� Ŭ���̾�Ʈ�� �α��� ����
//	{
//		WORD	Type
//
//		INT64	AccountNo
//		BYTE	Status				// 0 (���ǿ���) / 1 (����) ...  �ϴ� defines ���
//
//		WCHAR	ID[20]				// ����� ID		. null ����
//		WCHAR	Nickname[20]		// ����� �г���	. null ����
//
//		WCHAR	GameServerIP[16]	// ���Ӵ�� ����,ä�� ���� ����
//		USHORT	GameServerPort
//		WCHAR	ChatServerIP[16]
//		USHORT	ChatServerPort
//	}
namespace univ_dev
{
	class LoginServer : public CNetServer
	{
	private:
		struct LoginRedisJob
		{
			ULONGLONG _AccountNo;
			ULONGLONG _SessionID;
			cpp_redis::reply _RedisReply;
			Packet* _Packet;
			//debugging
			DWORD _BeginTime;
		};

	private:
		constexpr static int SERVER_IP_LEN = 16;
		constexpr static int SERVER_IP_SIZE = SERVER_IP_LEN * sizeof(WCHAR);

		constexpr static WCHAR _ChatServerIP[16]{ L"10.0.2.1" };
		constexpr static USHORT _ChatServerPort = 10445;

		constexpr static WCHAR _GameServerIP[16]{ L"10.0.2.1" };
		constexpr static USHORT _GameServerPort = 10455;

		constexpr static int ID_MAX_LEN = 20;
		constexpr static int ID_MAX_SIZE = ID_MAX_LEN * sizeof(WCHAR);
		constexpr static int NICK_NAME_MAX_LEN = 20;
		constexpr static int NICK_NAME_MAX_SIZE = NICK_NAME_MAX_LEN * sizeof(WCHAR);
		constexpr static int TOKEN_KEY_SIZE = 64;


	private:
		friend unsigned __stdcall LoginMoniteringThread(void* param);
		unsigned int LoginServerMoniteringThread(void* param);

		friend unsigned __stdcall RedisThread(void* param);
		unsigned int LoginServerRedisThread(void* param);


	private:
		virtual void OnRecv(ULONGLONG sessionID, Packet* recvPacket) final;
		virtual void OnErrorOccured(DWORD errorCode, const WCHAR* error,LogClass::LogLevel level) final;
		virtual bool OnConnectionRequest(WCHAR* ipStr, DWORD ip, USHORT port) final;
		virtual void OnClientJoin(WCHAR* ipStr, DWORD ip, USHORT port, ULONGLONG sessionID) final;
		virtual void OnClientLeave(ULONGLONG sessionID) final; // Release�� ȣ��
		virtual void OnTimeOut(ULONGLONG sessionID) final;

		virtual void OnSend(ULONGLONG sessionID) final;

	private:
		void PacketProc(Packet* packet, ULONGLONG sessionID, WORD type);
		void PacketProcRequestLogin(Packet* packet, ULONGLONG sessionID);
		
		void MakePacketResponseLogin(Packet* packet, INT64 accountNo, BYTE status, const WCHAR* ID, const WCHAR* nickName);

		void Start();
		void Close();

		void InitLoginServerLog(const WCHAR* directory);

	public:
		LoginServer(USHORT port, DWORD backlogQueueSize, DWORD threadPoolSize, DWORD runningThread, DWORD nagleOff, ULONGLONG maxSessionCounts, DWORD timeOutTime);
		//LoginServer(USHORT port, DWORD backlogQueueSize, DWORD threadPoolSize, DWORD runningThread, DWORD nagleOff, ULONGLONG maxSessionCounts);
		~LoginServer();

		bool GetRunningFlag() { return _RunningFlag; }
	private:
		cpp_redis::client						_RedisClient;
		HANDLE									_RedisThread;
		HANDLE									_RedisEvent;
		LockFreeQueue<LoginRedisJob>			_RedisJobQueue;

	private:
		// server running flag
		volatile DWORD							_RunningFlag;
		
		// monitering class
		HardWareMoniter							_HardWareMoniter;
		ProcessMoniter							_ProcessMoniter;

		HANDLE									_MoniteringThread;
		

		LogClass								_LoginServerLog;


		//Join Session -> OnClientJoin�� ��������� ���� OnClientLeave�� ��������� ����
		//Total Join Session -> OnclientJoin�� ��������� ����
		//Login TPS -> Login Packet�� ���������
		//Total Login -> Login Packet�� ���������
		//Login SuccessTPS -> Login Packetó���� �Ϸ�Ǿ�����
		//Login Wait -> JoinSession - LoginSuccess
		//Leave Sesssion -> OnClientLeave�� ������ ���Ǽ�
		
		//DB Query Timer -> DB ������ �ɸ��� �ð� ��
		//DB Query Counter -> DB ������ Ƚ��
		//DB Query AVR -> (DB Query Timer / DB Query Counter) / 1000

		volatile ULONGLONG _TotalLogin = 0;
		volatile ULONGLONG _LoginTPS = 0;
		volatile ULONGLONG _LoginSuccessTPS = 0;

		volatile ULONGLONG _LoginTotal = 0;
		volatile ULONGLONG _LoginSuccessTotal = 0;

		volatile ULONGLONG _LoginWait = 0;
		volatile ULONGLONG _LoginTotalTime = 0;

		volatile ULONGLONG _DBQueryTPS = 0;
		volatile ULONGLONG _DBQueryCountTotal = 1;
		volatile ULONGLONG _DBTotalTime = 0;

		volatile ULONGLONG _JoinSession = 0;
		volatile ULONGLONG _TotalJoinSession = 0;
		volatile ULONGLONG _TotalLeaveSession = 0;

		volatile ULONGLONG _MaxQueryTime = 0;
		char _MaxQueryString[128]{ 0 };
		
	private:
		constexpr static char _DataBaseIPStr[16] = { "10.0.1.2" };
		constexpr static char _DataBaseRootName[20] = { "root" };
		constexpr static char _DataBaseRootPassword[20] = { "procademy" };
		constexpr static char _DataBaseInitSchemaName[20] = { "accountdb" };
		constexpr static int _DataBaseSlowQuery = 10;


		void LockDB();
		void UnlockDB();
		volatile static DWORD					_DBTlsIdx;
		//DBConnector								_LoginDB;
		SRWLOCK									_DBLock;
	};
}


#endif // !__LOGIN_SERVER__
