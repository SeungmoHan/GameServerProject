#pragma once
#ifndef __LOGIN_THREAD_BLOCK_DEF__
#define __LOGIN_THREAD_BLOCK_DEF__
#define __UNIV_DEVELOPER_
#include "BaseServer.h"
#include "DBConnector.h"
#include "SS_MoniteringProtocol.h"

namespace univ_dev
{
	class GameServer;
	class AuthThreadBlock : public BaseServer::BasicThreadBlock
	{
	private:
		constexpr static int ServerType = SERVER_TYPE::GAME_SERVER;
		constexpr static int ID_MAX_LEN = 20;
		constexpr static int ID_MAX_SIZE = ID_MAX_LEN * sizeof(WCHAR);
		constexpr static int NICK_NAME_MAX_LEN = 20;
		constexpr static int NICK_NAME_MAX_SIZE = NICK_NAME_MAX_LEN * sizeof(WCHAR);
		constexpr static int TOKEN_KEY_SIZE = 64;
		const std::string GAME_BLOCK_NAME = "GAME_BLOCK_1";
	public:
		AuthThreadBlock(DWORD framePerSec, GameServer* server, std::string blockName);
	private:
		void OnUpdate() final;
		void OnMessage(ULONGLONG sessionID, Packet* packet) final;

		void OnPlayerJoined(ULONGLONG sessionID, Player* player) final;
		void OnPlayerLeaved(ULONGLONG sessionID, Player* player) final;

		void OnPlayerMoveJoin(ULONGLONG sessionID, Player* player) final;
		void OnPlayerMoveLeave(ULONGLONG sessionID, Player* player) final;
		
		void OnTimeOut(ULONGLONG sessionID) final;

		void RunMonitering(HardWareMoniter& h, ProcessMoniter& p) final;

		void OnThreadBlockStop()final;


		void PacketProc(ULONGLONG sessionID, Packet* packet, WORD type);
		void PacketProcRequestLogin(ULONGLONG sessionID, Packet* packet);
		void MakePacketResponseLogin(Packet * packet, INT64 accountNo, BYTE status );

		void MoveSessionToGameServer(ULONGLONG sessionID);
		DWORD DataBaseInit();

	private:
		inline void MakePacketMoniteringInfo(Packet* packet, BYTE serverNo, BYTE dataType, int dataValue, int timeStamp)
		{
			WORD type = PACKET_TYPE::en_PACKET_SS_MONITOR_DATA_UPDATE;
			(*packet) << type << serverNo << dataType << dataValue << timeStamp;
		}

		ULONGLONG								_TotalAuth			= 0;
		ULONGLONG								_LastTotalAuth		= 0;

		ULONGLONG								_TotalAuthBlockFPS	= 0;
		ULONGLONG								_LastAuthBlockFPS	= 0;

		GameServer*								_GameServer;

		std::unordered_map<ULONGLONG, Player*>	_PlayerMap;
		LogClass								_LoginBlockLog;
		
		DBConnector*							_DataBase;
		std::string								_DBIPStr;
		std::string								_DBRootName;
		std::string								_DBRootPassword;
		std::string								_DBInitSchema;
		DWORD									_DBSlowQuery;
		USHORT									_DBPort;
	};
}

#endif // !__LOGIN_THREAD_BLOCK_DEF__
