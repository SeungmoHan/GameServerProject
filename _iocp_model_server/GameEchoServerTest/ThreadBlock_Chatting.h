#pragma once
#ifndef __CHATTING_THREAD_BLOCK_DEF__
#define __CHATTING_THREAD_BLOCK_DEF__
#define __UNIV_DEVELOPER_
#include <BaseServer.h>
#include "SS_MoniteringProtocol.h"

namespace univ_dev
{
	class ChattingThreadBlock : public BaseServer::BasicThreadBlock
	{
	private:
		constexpr static int ID_MAX_LEN = 20;
		constexpr static int ID_MAX_SIZE = ID_MAX_LEN * sizeof(WCHAR);
		constexpr static int NICK_NAME_MAX_LEN = 20;
		constexpr static int NICK_NAME_MAX_SIZE = NICK_NAME_MAX_LEN * sizeof(WCHAR);
		constexpr static int TOKEN_KEY_SIZE = 64;

		constexpr static int SECTOR_X_SIZE = 50;
		constexpr static int SECTOR_Y_SIZE = 50;
		struct Sector
		{
			std::unordered_set<Player*> _PlayerSet;
			SRWLOCK _SectorLock;
			Sector()
			{
				this->_PlayerSet.clear();
				InitializeSRWLock(&_SectorLock);
			}
		};
	public:
		ChattingThreadBlock(DWORD framePerSec, BaseServer* server,std::string blockName);
	private:
		void OnUpdate() final;
		void OnMessage(ULONGLONG sessionID, Packet* packet) final;

		void OnPlayerJoined(ULONGLONG sessionID, Player* player) final;
		void OnPlayerLeaved(ULONGLONG sessionID, Player* player) final;

		void OnPlayerMoveJoin(ULONGLONG sessionID, Player* player) final;
		void OnPlayerMoveLeave(ULONGLONG sessionID, Player* player) final;

		void OnTimeOut(ULONGLONG sessionID) final;
		void RunMonitering(HardWareMoniter& h, ProcessMoniter& p) final;

		void PacketProcRequestLogin(ULONGLONG sessionID, Packet* packet);
		void PacketProcRequestMoveSector(ULONGLONG sessionID, Packet* packet);
		void PacketProcRequestChat(ULONGLONG sessionID, Packet* packet);

		void MakePacketResponseLogin(Packet* packet, INT64 accountNo, BYTE status);
		void MakePacketResponseMoveSector(Packet* packet, INT64 accountNo, WORD sectorX, WORD sectorY);
		void MakePacketResponseMessage(Packet* packet, INT64 accountNo, const WCHAR* ID, const WCHAR* nickName, WORD messageLen, const WCHAR* message);

		Player* FindPlayer(ULONGLONG sessionID);

		void InsertPlayer(ULONGLONG sessionID, Player* player);
		void RemovePlayer(ULONGLONG sessionID);

		void PacketProc(ULONGLONG sessionID, Packet* packet, WORD type);

		std::unordered_map<ULONGLONG, Player*> _PlayerMap;
		LockFreeMemoryPool<Player> _PlayerPool;
		Sector** _PlayerSector;
		LogClass _ChattingBlockLog;

		// DebugField
		ULONGLONG _PlayerPoolSize;
		ULONGLONG _PlayerPoolCapacity;
		ULONGLONG _PlayerPoolChunkCount;
		ULONGLONG _PlayerMapSize;

		ULONGLONG _TotalChatting;
		ULONGLONG _ChattingTPS;

		ULONGLONG _TotalLogin;
		ULONGLONG _LoginTPS;

		ULONGLONG _TotalSectorMove;
		ULONGLONG _SectorMoveTPS;

	};
}

#endif // !__CHATTING_THREAD_BLOCK_DEF__
