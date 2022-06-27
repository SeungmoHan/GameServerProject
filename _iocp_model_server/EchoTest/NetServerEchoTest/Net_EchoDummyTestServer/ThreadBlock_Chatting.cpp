#include <ThreadBlock_Chatting.h>
#include <CommonProtocol.h>

namespace univ_dev
{
	///--------------------------------------------------------------------------
///--------------------------------------------------------------------------
///--------------------------------------------------------------------------
///--------------------------------------------------------------------------
/// Chatting Thread Block Defines Begin

	ChattingThreadBlock::ChattingThreadBlock(DWORD framePerSec, BaseServer* server,std::string blockName) : BaseServer::BasicThreadBlock(framePerSec, server,blockName),
		_PlayerMapSize(0), _PlayerPoolCapacity(0), _PlayerPoolChunkCount(0), _PlayerPoolSize(0), _ChattingTPS(0), _TotalChatting(0), _TotalLogin(0), _LoginTPS(0), _TotalSectorMove(0), _SectorMoveTPS(0)
	{
		this->_PlayerSector = new Sector * [this->SECTOR_Y_SIZE];

		for (int i = 0; i < this->SECTOR_Y_SIZE; i++)
			this->_PlayerSector[i] = new Sector[this->SECTOR_X_SIZE];

		this->_ChattingBlockLog.LOG_SET_DIRECTORY(L"ServerLog\\ChattingServerLog");
		this->_ChattingBlockLog.LOG_SET_LEVEL(LogClass::LogLevel::LOG_LEVEL_LIBRARY);
	}

	void ChattingThreadBlock::OnUpdate()
	{
		// nothing
	}


	void ChattingThreadBlock::OnMessage(ULONGLONG sessionID, Packet* packet)
	{
		WORD type;
		(*packet) >> type;
		this->PacketProc(sessionID, packet, type);
	}

	void ChattingThreadBlock::OnPlayerJoined(ULONGLONG sessionID, Player* player)
	{
		//this->PushSession(sessionID);
	}

	void ChattingThreadBlock::OnPlayerLeaved(ULONGLONG sessionID, Player* player)
	{
		//this->PopSession(sessionID);
		this->RemovePlayer(sessionID);
	}

	void ChattingThreadBlock::OnPlayerMoveJoin(ULONGLONG sessionID, Player* player)
	{
	}

	void ChattingThreadBlock::OnPlayerMoveLeave(ULONGLONG sessionID,Player* player)
	{
	}

	void ChattingThreadBlock::OnTimeOut(ULONGLONG sessionID)
	{
		WCHAR* tlsErr = (WCHAR*)TlsGetValue(GetTlsErrIdx());
		wsprintf(tlsErr, L"Time Out SessionID : %I64u", sessionID);
		this->_ChattingBlockLog.LOG(tlsErr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
		this->Disconnect(sessionID);
	}

	void ChattingThreadBlock::PacketProc(ULONGLONG sessionID, Packet* packet, WORD type)
	{
		switch (type)
		{
			case PACKET_TYPE::en_PACKET_CS_CHAT_REQ_LOGIN:
			{
				this->PacketProcRequestLogin(sessionID, packet);
				this->_LoginTPS++;
				break;
			}
			case PACKET_TYPE::en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
			{
				this->PacketProcRequestMoveSector(sessionID, packet);
				this->_SectorMoveTPS++;
				break;
			}
			case PACKET_TYPE::en_PACKET_CS_CHAT_REQ_MESSAGE:
			{
				this->PacketProcRequestChat(sessionID, packet);
				this->_ChattingTPS++;
				break;
			}
			default:
			{
				WCHAR* tlsErr = (WCHAR*)TlsGetValue(GetTlsErrIdx());
				wsprintf(tlsErr, L"Session Default Case Type : %d, SessionID : %I64u", type, sessionID);
				this->_ChattingBlockLog.LOG(tlsErr, LogClass::LogLevel::LOG_LEVEL_ERROR);
				this->Disconnect(sessionID);
				break;
			}
		}
	}

	void ChattingThreadBlock::RunMonitering(HardWareMoniter& h, ProcessMoniter& p)
	{
		ULONGLONG chatTPS = InterlockedExchange(&this->_ChattingTPS, 0);
		ULONGLONG loginTPS = InterlockedExchange(&this->_LoginTPS, 0);
		ULONGLONG sectorMoveTPS = InterlockedExchange(&this->_SectorMoveTPS, 0);
		this->_TotalChatting += chatTPS;
		this->_TotalLogin += loginTPS;
		this->_TotalSectorMove += sectorMoveTPS;

		CNetServer::MoniteringInfo info;
		this->GetLibraryMoniteringInfo(info);
		

		printf("----------------------CHATTING_SERVER_MONITERING----------------------\n");
		printf("Total Chatting / TPS : %llu / %llu\n", this->_TotalChatting, chatTPS);
		printf("Total Login / TPS : %llu / %llu\n", this->_TotalLogin, loginTPS);
		printf("Total SectorMove / TPS : %llu / %llu\n", this->_TotalSectorMove, sectorMoveTPS);
		printf("JobQueue Size / Capacity : %d / %d\n", this->GetJobQueueSize(), this->GetJobQueueCapacity());
		printf("----------------------------------------------------------------------\n");
	}

	void ChattingThreadBlock::PacketProcRequestLogin(ULONGLONG sessionID, Packet* packet)
	{
		WCHAR* tlsErr = (WCHAR*)TlsGetValue(GetTlsErrIdx());
		BYTE status = false;
		INT64 accountNo;

		if (packet->GetBufferSize() < sizeof(accountNo))
		{
			wsprintf(tlsErr, L"PacketProcRequestLogin Packet BufferSize < sizeof(accountNo) SessionID : %I64u", sessionID);
			this->_ChattingBlockLog.LOG(tlsErr, LogClass::LogLevel::LOG_LEVEL_ERROR);
			this->Disconnect(sessionID);
			return;
		}
		(*packet) >> accountNo;
		if (packet->GetBufferSize() != ID_MAX_SIZE + NICK_NAME_MAX_SIZE + TOKEN_KEY_SIZE)
		{
			wsprintf(tlsErr, L"PacketProcRequestLogin Packet Available Packet Length is not enough SessionID : %I64u", sessionID);
			this->_ChattingBlockLog.LOG(tlsErr, LogClass::LogLevel::LOG_LEVEL_ERROR);
			this->Disconnect(sessionID);
			return;
		}

		Player* player;
		auto iter = this->_PlayerMap.find(sessionID);
		if (iter == this->_PlayerMap.end())
		{
			//Player 생성 및 초기화
			player = this->_PlayerPool.Alloc();
			//풀에서 받은 플레이어의 로그인이 이미 true다? 이건 중대결함.
			if (player->_Logined == true)
			{
				wsprintf(tlsErr, L"PacketProcRequestLogin NewPlayer's _Logined field is true SessionID : %I64u", sessionID);
				this->_ChattingBlockLog.LOG(tlsErr, LogClass::LogLevel::LOG_LEVEL_ERROR);
				CRASH();
				return;
			}
			player->_AccountNo = accountNo;
			packet->GetBuffer((char*)player->_ID, 40);
			packet->GetBuffer((char*)player->_NickName, 40);
			packet->GetBuffer(player->_TokenKey, 64);
			player->_SectorX = -1;
			player->_SectorY = -1;
			player->_SessionID = sessionID;
			player->_Logined = true;
			status = true;
			this->InsertPlayer(sessionID, player);
		}


		Packet* resLoginPacket = Packet::Alloc();
		this->MakePacketResponseLogin(resLoginPacket, accountNo, status);

		this->SendPacket(sessionID, resLoginPacket);
		if (status == false)
		{
			wsprintf(tlsErr, L"PacketProcRequestLogin Login Failed status is false SessionID : %I64u", sessionID);
			this->_ChattingBlockLog.LOG(tlsErr, LogClass::LogLevel::LOG_LEVEL_ERROR);
			this->Disconnect(sessionID);
		}
		//InterlockedIncrement(&_LoginTPS);
	}

	void ChattingThreadBlock::PacketProcRequestMoveSector(ULONGLONG sessionID, Packet* packet)
	{
		WCHAR* tlsErr = (WCHAR*)TlsGetValue(GetTlsErrIdx());
		INT64 accountNo;
		WORD sectorX;
		WORD sectorY;

		if (packet->GetBufferSize() != (sizeof(accountNo) + sizeof(sectorX) + sizeof(sectorY)))
		{
			wsprintf(tlsErr, L"PacketProcMoveSector Packet Length is not enough SessionID : %I64u", sessionID);
			this->_ChattingBlockLog.LOG(tlsErr, LogClass::LogLevel::LOG_LEVEL_ERROR);
			this->Disconnect(sessionID);
			return;
		}
		(*packet) >> accountNo >> sectorX >> sectorY;
		Player* player = this->FindPlayer(sessionID);
		// 플레이어가 없는데 무브섹터를 보내면 디스커넥트 대상
		if (player == nullptr)
		{
			wsprintf(tlsErr, L"PacketProcMoveSector FindPlayer returned nullptr SessionID : %I64u", sessionID);
			this->_ChattingBlockLog.LOG(tlsErr, LogClass::LogLevel::LOG_LEVEL_ERROR);
			this->Disconnect(sessionID);
			return;
		}
		// 로그인이 안됬거나 어카운트 넘버가 맞지 않으면 디스커넥트 대상
		else if (player->_Logined == false)
		{
			wsprintf(tlsErr, L"PacketProcMoveSector Player Already Logined SessionID : %I64u", sessionID);
			this->_ChattingBlockLog.LOG(tlsErr, LogClass::LogLevel::LOG_LEVEL_ERROR);
			this->Disconnect(sessionID);
			return;
		}
		else if (accountNo != player->_AccountNo)
		{
			wsprintf(tlsErr, L"PacketProcMoveSector Player AccountNo is wrong SessionID : %I64u", sessionID);
			this->_ChattingBlockLog.LOG(tlsErr, LogClass::LogLevel::LOG_LEVEL_ERROR);
			this->Disconnect(sessionID);
			return;
		}
		// 섹터가 50개밖에 없는데 그 이상이면 디스커넥트 대상(unsigned 라 음수면 아주큰값)
		else if (sectorX >= 50 || sectorY >= 50)
		{
			wsprintf(tlsErr, L"PacketProcMoveSector Player Sector X : %d, Sector Y : %d SessionID : %I64u", sectorX, sectorY, sessionID);
			this->_ChattingBlockLog.LOG(tlsErr, LogClass::LogLevel::LOG_LEVEL_ERROR);
			this->Disconnect(sessionID);
			return;
		}


		constexpr WORD comp = -1;
		//처음 들어온 케이스
		if (player->_SectorX == comp && player->_SectorY == comp)
		{
			this->_PlayerSector[sectorY][sectorX]._PlayerSet.emplace(player);
			player->_SectorX = sectorX;
			player->_SectorY = sectorY;
		}
		//이동하는 케이스
		else if (player->_SectorX != sectorX || player->_SectorY != sectorY)
		{
			int curX = player->_SectorX;
			int curY = player->_SectorY;
			player->_SectorX = sectorX;
			player->_SectorY = sectorY;
			//두개 섹터 다 락걸릴때까지 루프
			for (auto iter = this->_PlayerSector[curY][curX]._PlayerSet.begin();
				iter != this->_PlayerSector[curY][curX]._PlayerSet.end(); ++iter)
			{
				if ((*iter)->_SessionID == sessionID)
				{
					this->_PlayerSector[curY][curX]._PlayerSet.erase(iter);
					break;
				}
			}
			this->_PlayerSector[sectorY][sectorX]._PlayerSet.emplace(player);
		}
		Packet* moveSectorPacket = Packet::Alloc();
		this->MakePacketResponseMoveSector(moveSectorPacket, player->_AccountNo, player->_SectorX, player->_SectorY);
		this->SendPacket(sessionID, moveSectorPacket);
	}

	void ChattingThreadBlock::PacketProcRequestChat(ULONGLONG sessionID, Packet* packet)
	{
		WCHAR* tlsErr = (WCHAR*)TlsGetValue(GetTlsErrIdx());
		INT64 accountNo;
		WORD messageLen;
		WCHAR message[512];

		if (packet->GetBufferSize() < sizeof(accountNo) + sizeof(messageLen))
		{
			wsprintf(tlsErr, L"PacketProcChatRequire Packet BufferSize is not enough SessionID : %I64u", sessionID);
			this->_ChattingBlockLog.LOG(tlsErr, LogClass::LogLevel::LOG_LEVEL_ERROR);
			this->Disconnect(sessionID);
			return;
		}
		(*packet) >> accountNo >> messageLen;

		if (packet->GetBufferSize() != messageLen)
		{
			wsprintf(tlsErr, L"PacketProcChatRequire Packet BufferSize is not MessageLen SessionID : %I64u", sessionID);
			this->_ChattingBlockLog.LOG(tlsErr, LogClass::LogLevel::LOG_LEVEL_ERROR);
			this->Disconnect(sessionID);
			return;
		}
		packet->GetBuffer((char*)message, messageLen);

		Player* player = this->FindPlayer(sessionID);
		if (player == nullptr)
		{
			wsprintf(tlsErr, L"PacketProcChatRequire FindPlayer returned nullptr SessionID : %I64u", sessionID);
			this->_ChattingBlockLog.LOG(tlsErr, LogClass::LogLevel::LOG_LEVEL_ERROR);
			this->Disconnect(sessionID);
			return;
		}
		else if (player->_Logined == false)
		{
			wsprintf(tlsErr, L"PacketProcChatRequire Player Already Logined SessionID : %I64u", sessionID);
			this->_ChattingBlockLog.LOG(tlsErr, LogClass::LogLevel::LOG_LEVEL_ERROR);
			this->Disconnect(sessionID);
			return;
		}
		else if (player->_AccountNo != accountNo)
		{
			wsprintf(tlsErr, L"PacketProcChatRequire Player AccountNo is wrong SessionID : %I64u", sessionID);
			this->_ChattingBlockLog.LOG(tlsErr, LogClass::LogLevel::LOG_LEVEL_ERROR);
			this->Disconnect(sessionID);
		}
		else if (player->_SectorX >= 50 || player->_SectorY >= 50)
		{
			wsprintf(tlsErr, L"PacketProcChatRequire Player Sector X : %d, Sector Y : %d SessionID : %I64u", player->_SectorX, player->_SectorY, sessionID);
			this->_ChattingBlockLog.LOG(tlsErr, LogClass::LogLevel::LOG_LEVEL_ERROR);
			this->Disconnect(sessionID);
			return;
		}

		int beginY = player->_SectorY - 1;
		int beginX = player->_SectorX - 1;

		message[messageLen / 2] = L'\0';

		Packet* messagePacket = Packet::Alloc();
		MakePacketResponseMessage(messagePacket, player->_AccountNo, player->_ID, player->_NickName, messageLen, message);

		messagePacket->AddRef();
		for (int y = 0; y < 3; y++)
		{
			if (((beginY + y) < 0) || (beginY + y) >= 50) continue;
			for (int x = 0; x < 3; x++)
			{
				if (((beginX + x) < 0) || ((beginX + x) >= 50)) continue;

				for (auto iter = this->_PlayerSector[beginY + y][beginX + x]._PlayerSet.begin(); iter != this->_PlayerSector[beginY + y][beginX + x]._PlayerSet.end(); ++iter)
				{
					this->SendPacket((*iter)->_SessionID, messagePacket);
					//InterlockedIncrement(&_ChatTPS);
				}
			}
		}
		Packet::Free(messagePacket);
	}

	void ChattingThreadBlock::MakePacketResponseLogin(Packet* packet, INT64 accountNo, BYTE status)
	{
		WORD type = PACKET_TYPE::en_PACKET_CS_CHAT_RES_LOGIN;
		(*packet) << type << status << accountNo;
	}

	void ChattingThreadBlock::MakePacketResponseMoveSector(Packet* packet, INT64 accountNo, WORD sectorX, WORD sectorY)
	{
		WORD type = PACKET_TYPE::en_PACKET_CS_CHAT_RES_SECTOR_MOVE;
		(*packet) << type << accountNo << sectorX << sectorY;
	}

	void ChattingThreadBlock::MakePacketResponseMessage(Packet* packet, INT64 accountNo, const WCHAR* ID, const WCHAR* nickName, WORD messageLen, const WCHAR* message)
	{
		WORD type = PACKET_TYPE::en_PACKET_CS_CHAT_RES_MESSAGE;
		(*packet) << type << accountNo;
		packet->PutBuffer((char*)ID, 40);
		packet->PutBuffer((char*)nickName, 40);
		(*packet) << messageLen;
		packet->PutBuffer((char*)message, messageLen);
	}

	//ChattingThreadBlock::Player* ChattingThreadBlock::FindPlayer(ULONGLONG sessionID)
	//{
	//	auto iter = this->_PlayerMap.find(sessionID);
	//	if (iter == this->_PlayerMap.end())
	//		return nullptr;
	//	return iter->second;
	//}

	void ChattingThreadBlock::InsertPlayer(ULONGLONG sessionID, Player* player)
	{
		this->_PlayerMap.emplace(std::make_pair(sessionID, player));
	}

	void ChattingThreadBlock::RemovePlayer(ULONGLONG sessionID)
	{
		auto iter = this->_PlayerMap.find(sessionID);
		if (iter == this->_PlayerMap.end())
			return;
		Player* removePlayer = iter->second;
		this->_PlayerMap.erase(sessionID);
		removePlayer->_Logined = false;

		constexpr WORD comp = -1;
		WORD sectorX = removePlayer->_SectorX;
		WORD sectorY = removePlayer->_SectorY;
		removePlayer->_SectorX = comp;
		removePlayer->_SectorY = comp;
		if (sectorX == comp || sectorY == comp)
		{
			this->_PlayerPool.Free(removePlayer);
			return;
		}

		for (auto iter = this->_PlayerSector[sectorY][sectorX]._PlayerSet.begin(); iter != this->_PlayerSector[sectorY][sectorX]._PlayerSet.end(); ++iter)
		{
			if ((*iter)->_SessionID == sessionID)
			{
				this->_PlayerSector[sectorY][sectorX]._PlayerSet.erase(iter);
				break;
			}
		}
		this->_PlayerPool.Free(removePlayer);
	}


	/// Chatting Thread Block Defines End
	///--------------------------------------------------------------------------
	///--------------------------------------------------------------------------
	///--------------------------------------------------------------------------
	///--------------------------------------------------------------------------
}