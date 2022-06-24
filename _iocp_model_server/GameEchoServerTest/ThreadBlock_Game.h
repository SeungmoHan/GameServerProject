#pragma once
#ifndef __GAME_THREAD_BLOCK_DEF__
#define __GAME_THREAD_BLOCK_DEF__
#define __UNIV_DEVELOPER_

#include "BaseServer.h"
#include "SS_MoniteringProtocol.h"

namespace univ_dev
{
	class GameServer;
	class GameThreadBlock : public BaseServer::BasicThreadBlock
	{
	private:
		constexpr static int ServerType = SERVER_TYPE::GAME_SERVER;
		constexpr static int ID_MAX_LEN = 20;
		constexpr static int ID_MAX_SIZE = ID_MAX_LEN * sizeof(WCHAR);
		constexpr static int NICK_NAME_MAX_LEN = 20;
		constexpr static int NICK_NAME_MAX_SIZE = NICK_NAME_MAX_LEN * sizeof(WCHAR);
		constexpr static int TOKEN_KEY_SIZE = 64;
		const std::string LOGIN_BLOCK_NAME = "LOGIN_BLOCK_1";
	public:
		GameThreadBlock(DWORD framePerSec, GameServer* server,std::string blockName);
	private:
		void OnUpdate() final;
		void OnMessage(ULONGLONG sessionID, Packet* packet) final;

		void OnPlayerJoined(ULONGLONG sessionID,Player* player) final;
		void OnPlayerLeaved(ULONGLONG sessionID, Player* player) final;

		void OnPlayerMoveJoin(ULONGLONG sessionID, Player* player) final;
		void OnPlayerMoveLeave(ULONGLONG sessionID, Player* player) final;

		void OnTimeOut(ULONGLONG sessionID) final;
		void RunMonitering(HardWareMoniter& h, ProcessMoniter& p) final;

		void OnThreadBlockStop()final;


		void PacketProc(ULONGLONG sessionID, Packet* packet, WORD type);

		void PacketProcEchoRequest(ULONGLONG sessionID, Packet* packet);

		void MakePacketEchoResponse(Packet* packet, INT64 accounNo, LONGLONG sendTick);

	private:
		inline void MakePacketMoniteringInfo(Packet* packet, BYTE serverNo, BYTE dataType, int dataValue, int timeStamp)
		{
			WORD type = PACKET_TYPE::en_PACKET_SS_MONITOR_DATA_UPDATE;
			(*packet) << type << serverNo << dataType << dataValue << timeStamp;
		}


		ULONGLONG _TotalGameBlockFPS;
		ULONGLONG _LastGameBlockFPS;

		GameServer*								_GameServer;
		
		std::unordered_map<ULONGLONG, Player*>	_PlayerMap;
		
		LogClass								_GameBlockLog;

	};
}

#endif // !__GAME_THREAD_BLOCK_DEF__
