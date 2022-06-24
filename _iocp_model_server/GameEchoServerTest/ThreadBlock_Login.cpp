#include <ThreadBlock_Login.h>
#include "GameServer.h"


namespace univ_dev
{
	///--------------------------------------------------------------------------
	///--------------------------------------------------------------------------
	///--------------------------------------------------------------------------
	///--------------------------------------------------------------------------
	/// Auth Thread Block Defines Begin
	AuthThreadBlock::AuthThreadBlock(DWORD framePerSec, GameServer* server, std::string blockName) : BaseServer::BasicThreadBlock(framePerSec, server, blockName), _GameServer(server)
	{
		int _ = _wmkdir(L"ServerLog");
		_ = _wmkdir(L"ServerLog\\LoginBlockLog");
		this->_LoginBlockLog.LOG_SET_DIRECTORY(L"ServerLog\\LoginBlockLog");
		this->_LoginBlockLog.LOG_SET_LEVEL(LogClass::LogLevel::LOG_LEVEL_SYSTEM);

		DWORD err;
		if (err = this->DataBaseInit())
		{
			this->_LoginBlockLog.LOG(L"Database Init Failed", LogClass::LogLevel::LOG_LEVEL_ERROR);
			CRASH();
		}
	}





	DWORD AuthThreadBlock::DataBaseInit()
	{
		if (!g_ConfigReader.SetCurrentSection(L"AuthBlock"))return 1;
		if (!g_ConfigReader.Find(L"DBIP")) return 2;
		if (!g_ConfigReader.Find(L"DBRoot")) return 3;
		if (!g_ConfigReader.Find(L"DBPassword")) return 4;
		if (!g_ConfigReader.Find(L"DBInitSchema")) return 5;
		if (!g_ConfigReader.Find(L"DBSlowQuery")) return 6;
		if (!g_ConfigReader.Find(L"DBPort")) return 7;

		std::wstring tempIP = g_ConfigReader.Get(L"DBIP");
		std::wstring tempRoot = g_ConfigReader.Get(L"DBRoot");
		std::wstring tempPassword = g_ConfigReader.Get(L"DBPassword");
		std::wstring tempInitSchema = g_ConfigReader.Get(L"DBInitSchema");

		this->_DBIPStr = { tempIP.begin(),tempIP.end() };
		this->_DBRootName = { tempRoot.begin(), tempRoot.end() };
		this->_DBRootPassword = { tempPassword.begin(),tempPassword.end() };
		this->_DBInitSchema = { tempInitSchema.begin(),tempInitSchema.end() };
		this->_DBSlowQuery = g_ConfigReader.Get(L"DBSlowQuery", 1);
		this->_DBPort = g_ConfigReader.Get(L"DBPort", 1);

		this->_DataBase = new DBConnector(this->_DBIPStr.c_str(), this->_DBRootName.c_str(), this->_DBRootPassword.c_str(), this->_DBInitSchema.c_str(), this->_DBSlowQuery, this->_DBPort);
		if (!this->_DataBase->DBConnect()) return 8;

		return 0;
	}





	void AuthThreadBlock::OnUpdate()
	{
		this->_TotalAuthBlockFPS++;
		// nothing now
	}

	void AuthThreadBlock::OnMessage(ULONGLONG sessionID, Packet* packet)
	{
		//이 메시지의 주인이 내 쓰레드 블럭에 있는지부터 확인
		auto iter = this->_PlayerMap.find(sessionID);
		if (iter == this->_PlayerMap.end())
		{
			// 이경우에는 내 쓰레드 블럭에 없는데 나한테 메시지가 들어온거
			// 원래 줘야하는 쓰레드로 전달해줘야됨.
			BasicThreadBlock* other = this->_GameServer->FindThreadBlock(sessionID);
			JobMessage job;
			job._Packet = packet;
			job._Type = JobMessage::Type::MESSAGE;
			job._SessionID = sessionID;
			job._ThreadBlockName = other->GetThreadBlockName();
			return other->JobEnqueue(job);
		}
		WORD type;
		(*packet) >> type;
		this->PacketProc(sessionID, packet, type);
	}

	void AuthThreadBlock::OnPlayerJoined(ULONGLONG sessionID, Player* player)
	{
		this->_PlayerMap.emplace(std::make_pair(sessionID, player));
	}

	void AuthThreadBlock::OnPlayerLeaved(ULONGLONG sessionID, Player* player)
	{
		auto iter = this->_PlayerMap.find(sessionID);
		if (iter == this->_PlayerMap.end())
		{
			//없으면 게임 서버의 플레이어 맵에서 찾아서 거기 쓰레드 블럭에 넣어줘야됨.
			BasicThreadBlock* nextBlock = this->_GameServer->FindThreadBlock(sessionID);
			if (nextBlock == this)
			{
				this->_LoginBlockLog.LOG(L"OnPlayerMoveLeave -> nextBlock is this", LogClass::LogLevel::LOG_LEVEL_ERROR);
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

	void AuthThreadBlock::OnPlayerMoveJoin(ULONGLONG sessionID, Player* player)
	{
		this->_PlayerMap.emplace(std::make_pair(sessionID, player));
	}

	void AuthThreadBlock::OnPlayerMoveLeave(ULONGLONG sessionID, Player* player)
	{
		auto iter = this->_PlayerMap.find(sessionID);
		if (iter == this->_PlayerMap.end())
		{
			//없으면 게임 서버의 플레이어 맵에서 찾아서 거기 쓰레드 블럭에 넣어줘야됨.
			BasicThreadBlock* nextBlock = this->_GameServer->FindThreadBlock(sessionID);
			if (nextBlock == this)
			{
				this->_LoginBlockLog.LOG(L"OnPlayerMoveLeave -> nextBlock is this", LogClass::LogLevel::LOG_LEVEL_ERROR);
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

	void AuthThreadBlock::OnTimeOut(ULONGLONG sessionID)
	{
		WCHAR* errStr = this->_GameServer->GetErrString();
		wsprintf(errStr, L"Time Out Session : %I64u", sessionID);
		this->_LoginBlockLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
		this->Disconnect(sessionID);
	}

	void AuthThreadBlock::PacketProc(ULONGLONG sessionID, Packet* packet, WORD type)
	{
		switch (type)
		{
		case PACKET_TYPE::en_PACKET_CS_GAME_REQ_LOGIN:
		{
			PacketProcRequestLogin(sessionID, packet);
			break;
		}
		case PACKET_TYPE::en_PACKET_CS_GAME_REQ_ECHO:
		{
			WCHAR* errStr = this->_GameServer->GetErrString();
			wsprintf(errStr, L"Packet ECHO Type in Login Block SessionID : %I64u", sessionID);
			this->_LoginBlockLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_ERROR);
			CRASH();
			break;
		}
		case PACKET_TYPE::en_PACKET_CS_GAME_REQ_HEARTBEAT:
		{
			//nothing
			break;
		}
		default:
		{
			this->Disconnect(sessionID);
			break;
		}
		}
	}

	void AuthThreadBlock::PacketProcRequestLogin(ULONGLONG sessionID, Packet* packet)
	{
		auto iter = this->_PlayerMap.find(sessionID);
		if (iter == this->_PlayerMap.end())
		{
			this->Disconnect(sessionID);
			return;
		}
		Player* player = iter->second;
		if (player->_Logined != false)
		{
			this->Disconnect(sessionID);
			return;
		}
		if (packet->GetBufferSize() != sizeof(INT64) + this->TOKEN_KEY_SIZE + sizeof(int))
		{
			WCHAR* errStr = this->_GameServer->GetErrString();
			wsprintf(errStr, L"Packet BufferSize not enough sizeof(INT64) + this->TOKEN_KEY_SIZE");
			this->_LoginBlockLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_ERROR);
			this->Disconnect(sessionID);
			return;
		}

		INT64 accountNo;
		char sessionKey[65]{ 0 };
		int version;

		(*packet) >> accountNo;
		packet->GetBuffer(sessionKey, this->TOKEN_KEY_SIZE);
		(*packet) >> version;

		char query[512];
		sprintf_s(query, 512, "select accountno, userid, userpass, usernick from account where accountno = %I64d", accountNo);

		DWORD dbBegin = timeGetTime();
		this->_DataBase->QuerySave(query);
		MYSQL_RES* result = this->_DataBase->GetQueryResult();
		DWORD queryTime = timeGetTime() - dbBegin;

		MYSQL_ROW row;
		BYTE status = en_PACKET_CS_LOGIN_RES_LOGIN::dfLOGIN_STATUS_FAIL;

		do
		{
			if ((row = this->_DataBase->FetchRow(result)) == nullptr)
			{
				// DB에 세션 키가 없음
				status = en_PACKET_CS_LOGIN_RES_LOGIN::dfLOGIN_STATUS_ACCOUNT_MISS;
				break;
			}
			//if (memcmp(tokenKey, row[2], this->TOKEN_KEY_SIZE) != 0) 
			//{
			//    //세션 키값이 다름
			//    status = en_PACKET_CS_LOGIN_RES_LOGIN::dfLOGIN_STATUS_ACCOUNT_MISS;
			//    MakePacketResponseLogin(packet, accountNo, status, nullptr, nullptr);
			//    break;
			//}

			size_t size;
			status = en_PACKET_CS_LOGIN_RES_LOGIN::dfLOGIN_STATUS_OK;
			mbstowcs_s(&size, player->_ID, this->ID_MAX_LEN, row[1], _TRUNCATE);
			mbstowcs_s(&size, player->_NickName, this->NICK_NAME_MAX_LEN, row[3], _TRUNCATE);
			player->_Logined = true;
		} while (0);
		this->_DataBase->FreeResult(result);

		Packet* sendPacket = Packet::Alloc();
		this->MakePacketResponseLogin(sendPacket, accountNo, status);

		if (status != en_PACKET_CS_LOGIN_RES_LOGIN::dfLOGIN_STATUS_OK)
		{
			WCHAR* errStr = this->_GameServer->GetErrString();
			wsprintf(errStr, L"Login Status Is not LOGIN_STATUS_OK %d", status);
			this->_LoginBlockLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_ERROR);
			this->SendPacket(sessionID, sendPacket);
			this->Disconnect(sessionID);
			return;
		}
		this->MoveSessionToGameServer(sessionID);
		this->SendPacket(sessionID, sendPacket);
		this->_TotalAuth++;
	}

	void AuthThreadBlock::MoveSessionToGameServer(ULONGLONG sessionID)
	{
		this->_PlayerMap.erase(sessionID);
		this->_GameServer->MoveRequest(sessionID, this->GAME_BLOCK_NAME);
	}

	void AuthThreadBlock::MakePacketResponseLogin(Packet* packet, INT64 accountNo, BYTE status)
	{
		WORD type = PACKET_TYPE::en_PACKET_CS_GAME_RES_LOGIN;
		(*packet) << type << status << accountNo;
	}



	void AuthThreadBlock::RunMonitering(HardWareMoniter& h, ProcessMoniter& p)
	{
		Packet* packet[50];
		int packetCount = 0;
		int curTime = ::time(nullptr);


		//printf("|-------------------------------------Auth Block---------------------------------------\n");
		//printf("| Total Auth / TPS : %llu / %llu\n", this->_TotalAuth, this->_TotalAuth - this->_LastTotalAuth);
		//printf("| Auth Player / Auth Block FPS : %llu / %llu\n", this->_PlayerMap.size(), this->_TotalAuthBlockFPS - this->_LastAuthBlockFPS);

		packet[packetCount] = Packet::Alloc();
		this->MakePacketMoniteringInfo(packet[packetCount++],
			this->ServerType, GAME_SERVER_MONITERING_TYPE::G_AUTH_PLAYER,
			this->_PlayerMap.size(), curTime);
		packet[packetCount] = Packet::Alloc();
		this->MakePacketMoniteringInfo(packet[packetCount++],
			this->ServerType, GAME_SERVER_MONITERING_TYPE::G_AUTH_THREAD_FPS,
			this->_TotalAuthBlockFPS - this->_LastAuthBlockFPS, curTime);

		this->_LastTotalAuth = this->_TotalAuth;
		this->_LastAuthBlockFPS = this->_TotalAuthBlockFPS;
		// 모조리 보내기
		for (int i = 0; i < packetCount; i++)
			this->SendToMoniteringServer(packet[i]);
	}

	void AuthThreadBlock::OnThreadBlockStop()
	{

	}

	/// Login Thread Block Defines End
	///--------------------------------------------------------------------------
	///--------------------------------------------------------------------------
	///--------------------------------------------------------------------------
	///--------------------------------------------------------------------------
}
