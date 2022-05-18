#pragma once
#ifndef __CHAT_SERVER__
#define __CHAT_SERVER__
#define __UNIV_DEVELOPER_

#include "ChatServerErrorDefine.h"
#include "CNetServer.h"
#include "JobMessage.h"
#include "Player.h"
#include "ChatServerProtocol.h"
#include "HardWareMoniteringClass.h"
#include "ProcessMoniteringClass.h"

#include <unordered_map>
#include <unordered_set>
#include <list>

namespace univ_dev
{
	class ChatServer : public CNetServer
	{
	private:
		struct Sector
		{
			std::unordered_set<Player*> _PlayerSet;
			SRWLOCK _SectorLock;
			Sector()
			{
				_PlayerSet.clear();
				InitializeSRWLock(&_SectorLock);
			}
		};

		constexpr static int INVALID_PLAYER_SECTOR = 51;
		constexpr static int SECTOR_X_SIZE = 50;
		constexpr static int SECTOR_Y_SIZE = 50;
		constexpr static int ID_MAX_LEN = 20;
		constexpr static int ID_MAX_SIZE = 40;
		constexpr static int NICK_NAME_MAX_LEN = 20;
		constexpr static int NICK_NAME_MAX_SIZE = 40;
		constexpr static int TOKEN_KEY_SIZE = 64;

		friend unsigned __stdcall MoniteringThread(void* param);

		unsigned int ChatServerMoniteringThread(void* param);


		void OnRecv(ULONGLONG sessionID, Packet* recvPacket) final;
		void OnErrorOccured(DWORD errorCode, const WCHAR* error) final;
		bool OnConnectionRequest(WCHAR* ipStr, DWORD ip, USHORT port) final;
		void OnClientJoin(WCHAR* ipStr, DWORD ip, USHORT port, ULONGLONG sessionID) final;
		void OnClientLeave(ULONGLONG sessionID) final;
		void OnTimeOut(ULONGLONG sessionID) final;


		void PacketProc(Packet* packet, ULONGLONG sessionID, WORD type);

		void PacketProcRequestLogin(Packet* packet, ULONGLONG sessionID);
		void PacketProcMoveSector(Packet* packet, ULONGLONG sessionID);
		void PacketProcChatRequire(Packet* packet, ULONGLONG sessionID);
		void PacketProcHeartBeating(Packet* packet, ULONGLONG sessionID);

		void MakePacketResponseLogin(Packet* packet, INT64 accountNo, BYTE status);
		void MakePacketResponseMoveSector(Packet* packet, INT64 accountNo, WORD sectorX, WORD sectorY);
		void MakePacketResponseMessage(Packet* packet, INT64 accountNo, const WCHAR* ID, const WCHAR* nickName, WORD messageLen, const WCHAR* message);

		void InsertPlayer(ULONGLONG sessionID, Player* player);
		void RemovePlayer(ULONGLONG sessionID);
		Player* FindPlayer(ULONGLONG sessionID);

		void Start();
		void Close();
	public:
		ChatServer(USHORT port, DWORD backlogQueueSize, DWORD threadPoolSize, DWORD runningThread, DWORD nagleOff, ULONGLONG maxSessionCounts);
		~ChatServer();
		bool GetRunningFlag() { return _RunningFlag; }
	private:
		HardWareMoniter							_HardWareMoniter;
		ProcessMoniter							_ProcessMoniter;

		ULONGLONG								_LeaveSessionCount;
		DWORD									_RunningFlag;

		HANDLE									_UpdateThread;
		HANDLE									_MoniteringThread;

		LogClass								_ChatServerLog;


		LockFreeQueue<JobMessage>				_JobQueue;
		LockFreeMemoryPool<Player>				_PlayerPool;
		LockFreeMemoryPoolTLS<JobMessage>		_JobMessagePool;

		//std::unordered_set<Player*>**			_Sector;
		Sector**								_Sector;
		std::unordered_map<ULONGLONG,Player*>	_PlayerMap;
		SRWLOCK									_PlayerMapLock;

		ULONGLONG _SectorMoveTPS;
		ULONGLONG _ChatTPS;
		ULONGLONG _LoginTPS;
	};
}


#endif // !__CHAT_SERVER__
