#include <ThreadBlock_Game.h>
#include "GameServer.h"
namespace univ_dev
{
	///--------------------------------------------------------------------------
	///--------------------------------------------------------------------------
	///--------------------------------------------------------------------------
	///--------------------------------------------------------------------------
	/// Game Thread Block Defines Begin


	GameThreadBlock::GameThreadBlock(DWORD framePerSec, GameServer* server, std::string blockName) : BaseServer::BasicThreadBlock(framePerSec, server, blockName) ,_GameServer(server)
	{
		int _ = _wmkdir(L"ServerLog");
		_ = _wmkdir(L"ServerLog\\LoginBlockLog");
		this->_GameBlockLog.LOG_SET_DIRECTORY(L"ServerLog\\LoginBlockLog");
		this->_GameBlockLog.LOG_SET_LEVEL(LogClass::LogLevel::LOG_LEVEL_SYSTEM);
	}
	void GameThreadBlock::OnUpdate()
	{
		//nothing now
		this->_TotalGameBlockFPS++;
	}

	void GameThreadBlock::OnMessage(ULONGLONG sessionID, Packet* packet)
	{
		this->PacketProc(sessionID, packet);
	}

	void GameThreadBlock::OnPlayerJoined(ULONGLONG sessionID, Player* player)
	{
		this->_PlayerMap.emplace(std::make_pair(sessionID, player));
	}

	void GameThreadBlock::OnPlayerLeaved(ULONGLONG sessionID, Player* player)
	{
		auto iter = this->_PlayerMap.find(sessionID);
		if (iter == this->_PlayerMap.end())
		{
			//없으면 게임 서버의 플레이어 맵에서 찾아서 거기 쓰레드 블럭에 넣어줘야됨.
			BasicThreadBlock* nextBlock = this->_GameServer->FindThreadBlock(sessionID);
			if (nextBlock == this)
			{
				this->_GameBlockLog.LOG(L"OnPlayerMoveLeave -> nextBlock is this", LogClass::LogLevel::LOG_LEVEL_ERROR);
				CRASH();
			}
			if (nextBlock == nullptr)
				return;
			JobMessage job;
			job._Player = player;
			job._SessionID = sessionID;
			job._Type = JobMessage::Type::CLIENT_LEAVE;
			job._ThreadBlockName = nextBlock->GetThreadBlockName();
			return nextBlock->JobEnqueue(job);
		}
		this->_PlayerMap.erase(sessionID);
		this->_GameServer->RemovePlayer(sessionID);
	}

	void GameThreadBlock::OnPlayerMoveJoin(ULONGLONG sessionID, Player* player)
	{
		this->_PlayerMap.emplace(std::make_pair(sessionID, player));
	}

	void GameThreadBlock::OnPlayerMoveLeave(ULONGLONG sessionID, Player* player)
	{
		auto iter = this->_PlayerMap.find(sessionID);
		if (iter == this->_PlayerMap.end())
		{
			//없으면 게임 서버의 플레이어 맵에서 찾아서 거기 쓰레드 블럭에 넣어줘야됨.
			BasicThreadBlock* nextBlock = this->_GameServer->FindThreadBlock(sessionID);
			if (nextBlock == this)
			{
				this->_GameBlockLog.LOG(L"OnPlayerMoveLeave -> nextBlock is this", LogClass::LogLevel::LOG_LEVEL_ERROR);
				CRASH();
			}
			if (nextBlock == nullptr)
				return;
			JobMessage job;
			job._Player = player;
			job._SessionID = sessionID;
			job._Type = JobMessage::Type::CLIENT_MOVE_LEAVE;
			job._ThreadBlockName = nextBlock->GetThreadBlockName();
			return nextBlock->JobEnqueue(job);
		}
		this->_PlayerMap.erase(sessionID);
	}

	void GameThreadBlock::OnTimeOut(ULONGLONG sessionID)
	{
		WCHAR* errStr = this->_GameServer->GetErrString();
		wsprintf(errStr, L"Time Out Session : %I64u", sessionID);
		this->_GameBlockLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
		this->Disconnect(sessionID);
	}

	void GameThreadBlock::RunMonitering(HardWareMoniter& h, ProcessMoniter& p)
	{
		Packet* packet[50];
		int packetCount = 0;
		int curTime = ::time(nullptr);


		printf("|-------------------------------------Game Block---------------------------------------\n");
		printf("| Total Game Frmae / FPS : %llu / %llu\n", this->_TotalGameBlockFPS, this->_TotalGameBlockFPS - this->_LastGameBlockFPS);
		printf("| Game Player : %llu\n", this->_PlayerMap.size());

		packet[packetCount] = Packet::Alloc();
		this->MakePacketMoniteringInfo(packet[packetCount++],
			this->ServerType, GAME_SERVER_MONITERING_TYPE::G_GAME_PLAYER,
			this->_PlayerMap.size(), curTime);
		packet[packetCount] = Packet::Alloc();
		this->MakePacketMoniteringInfo(packet[packetCount++],
			this->ServerType, GAME_SERVER_MONITERING_TYPE::G_DB_WRITE_TPS,
			10, curTime);
		packet[packetCount] = Packet::Alloc();
		this->MakePacketMoniteringInfo(packet[packetCount++],
			this->ServerType, GAME_SERVER_MONITERING_TYPE::G_DB_MSG_QUEUE_SIZE,
			20, curTime);
		packet[packetCount] = Packet::Alloc();
		this->MakePacketMoniteringInfo(packet[packetCount++],
			this->ServerType, GAME_SERVER_MONITERING_TYPE::G_GAME_THREAD_FPS,
			this->_TotalGameBlockFPS - this->_LastGameBlockFPS, curTime);

		this->_LastGameBlockFPS = this->_TotalGameBlockFPS;
		// 모조리 보내기
		for (int i = 0; i < packetCount; i++)
			this->SendToMoniteringServer(packet[i]);
	}

	void GameThreadBlock::OnThreadBlockStop()
	{
		
	}

	void GameThreadBlock::PacketProc(ULONGLONG sessionID, Packet* packet)
	{
		WORD type;
		(*packet) >> type;
		switch (type)
		{
			case PACKET_TYPE::en_PACKET_CS_GAME_REQ_LOGIN:
			{
				WCHAR* errStr = this->_GameServer->GetErrString();
				wsprintf(errStr, L"Packet Login Type in Game Block SessionID : %I64u", sessionID);
				this->_GameBlockLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_ERROR);
				break;
			}
			case PACKET_TYPE::en_PACKET_CS_GAME_REQ_ECHO:
			{
				this->PacketProcEchoRequest(sessionID, packet);
				break;
			}
			case PACKET_TYPE::en_PACKET_CS_GAME_REQ_HEARTBEAT:
			{
				//nothing
				break;
			}
			default:
			{
				WCHAR* errStr = this->_GameServer->GetErrString();
				wsprintf(errStr, L"GameBlock::PacketProc default SessionID : %I64u ,type : %u", sessionID, type);
				this->_GameBlockLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_ERROR);
				this->Disconnect(sessionID);
				break;
			}
		}
	}

	void GameThreadBlock::PacketProcEchoRequest(ULONGLONG sessionID, Packet* packet)
	{
		//auto iter = this->_PlayerMap.find(sessionID);
		//if (iter == this->_PlayerMap.end())
		//{
		//	this->_GameBlockLog.LOG(L"GameBlock::PlayerMap.end()", LogClass::LogLevel::LOG_LEVEL_ERROR);
		//	this->Disconnect(sessionID);
		//	return;
		//}
		//Player* player = iter->second;
		//if (player->_Logined == false)
		//{
		//	this->_GameBlockLog.LOG(L"GameBlock::player logined", LogClass::LogLevel::LOG_LEVEL_ERROR);
		//	this->Disconnect(sessionID);
		//	return;
		//}
		INT64 accountNo;
		LONGLONG sendTick;

		(*packet) >> accountNo >> sendTick;


		Packet* sendPacket = Packet::Alloc();
		this->MakePacketEchoResponse(sendPacket, accountNo, sendTick);
		this->SendPacket(sessionID, sendPacket);
	}

#define ECHO_TYPE_DECLAIR(name) WORD name = PACKET_TYPE::en_PACKET_CS_GAME_RES_ECHO
	void GameThreadBlock::MakePacketEchoResponse(Packet* packet, INT64 accounNo, LONGLONG sendTick)
	{
		ECHO_TYPE_DECLAIR(ECHO_TYPE);
		(*packet) << ECHO_TYPE << accounNo << sendTick;
	}

	/// Game Thread Block Defines End
	///--------------------------------------------------------------------------
	///--------------------------------------------------------------------------
	///--------------------------------------------------------------------------
	///--------------------------------------------------------------------------
}
