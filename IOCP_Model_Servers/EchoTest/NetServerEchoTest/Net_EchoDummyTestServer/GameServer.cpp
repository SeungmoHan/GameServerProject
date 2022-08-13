#include "GameServer.h"

#include "ThreadBlock_Chatting.h"
#include "ThreadBlock_Game.h"
#include "ThreadBlock_Login.h"

#include <iostream>

#define SHARED_TRUE true
#define SHARED_FALSE false

namespace univ_dev
{
	void GameServer::AttachGameBlock()
	{
		DWORD gameBlockFrame = 1;
		std::wstring tempName;
		do
		{
			if (!g_ConfigReader.SetCurrentSection(L"GameBlock")) break;
			if (!g_ConfigReader.Find(L"FramePerSec")) break;
			if (!g_ConfigReader.Find(L"GameBlockName")) break;
			gameBlockFrame = g_ConfigReader.Get(L"FramePerSec", 1);
			tempName = g_ConfigReader.Get(L"GameBlockName");
			std::string blockName{ tempName.begin(),tempName.end() };

			BaseServer::BasicThreadBlock* gameThreadBlock = new univ_dev::GameThreadBlock(gameBlockFrame, this, blockName);
			this->Attach(gameThreadBlock);
			this->_ThreadBlockMap.emplace(std::make_pair(blockName, gameThreadBlock));
			printf("---GameServer Attached---\n");
			printf("GameBlock Frame : %u\n", gameBlockFrame);
			printf("GameBlock Name : %s\n", blockName.c_str());
			printf("-----------------------------\n");
			system("pause");
			return;
		} while (0);
		CRASH();
	}

	void GameServer::AttachAuthBlock()
	{
		DWORD loginBlockFrame = 1;
		std::wstring tempName;
		do
		{
			if (!g_ConfigReader.SetCurrentSection(L"AuthBlock")) break;
			if (!g_ConfigReader.Find(L"FramePerSec")) break;
			if (!g_ConfigReader.Find(L"AuthBlockName")) break;
			loginBlockFrame = g_ConfigReader.Get(L"FramePerSec", 1);
			tempName = g_ConfigReader.Get(L"AuthBlockName");
			std::string blockName{ tempName.begin(),tempName.end() };

			BaseServer::BasicThreadBlock* loginThreadBlock = new univ_dev::AuthThreadBlock(loginBlockFrame, this, blockName);
			this->Attach(loginThreadBlock);
			this->_ThreadBlockMap.emplace(std::make_pair(blockName,loginThreadBlock));
			printf("---LoginServer Attached---\n");
			printf("AuthBlock Frame : %u\n", loginBlockFrame);
			printf("AuthBlock Name : %s\n", blockName.c_str());
			printf("-----------------------------\n");
			system("pause");
			return;
		} while (0);
		CRASH();
	}


	bool GameServer::SetServerConfig()
	{
		do
		{
			if (!g_ConfigReader.SetCurrentSection(L"NetServerConfig")) break;
			if (!g_ConfigReader.Find(L"Port")) break;
			if (!g_ConfigReader.Find(L"BacklogQueue")) break;
			if (!g_ConfigReader.Find(L"WorkerThread")) break;
			if (!g_ConfigReader.Find(L"RunningThread"))break;
			if (!g_ConfigReader.Find(L"NagleOff"))break;
			if (!g_ConfigReader.Find(L"MaxSessionCount"))break;
			if (!g_ConfigReader.Find(L"TimeOutClock"))break;

			this->_ServerPort = g_ConfigReader.Get(L"Port", 1);
			this->_BackLogQueueSize = g_ConfigReader.Get(L"BacklogQueue", 1);
			this->_WorkerThreadCount = g_ConfigReader.Get(L"WorkerThread", 1);
			this->_RunningThreadCount = g_ConfigReader.Get(L"RunningThread", 1);
			this->_NagleOffOption = g_ConfigReader.Get(L"NagleOff", 1);
			this->_MaxSessionCount = g_ConfigReader.Get(L"MaxSessionCount", 1);
			this->_TimeOutClock = g_ConfigReader.Get(L"TimeOutClock", 1);

			this->InitBaseServer(this->_ServerPort, this->_BackLogQueueSize, this->_WorkerThreadCount, this->_RunningThreadCount, this->_NagleOffOption, this->_MaxSessionCount, this->_TimeOutClock);
			this->Start();
			return true;
		} while (0);
		return false;
	}
	void GameServer::OnRecv(ULONGLONG sessionID, Packet* packet)
	{
		this->LockSessionThreadBlock(SHARED_TRUE);
		auto iter = this->_SessionThreadBlockMap.find(sessionID);
		if (iter == this->_SessionThreadBlockMap.end())
		{
			this->DisconnectSession(sessionID);
			this->UnlockSessionThreadBlock(SHARED_TRUE);
			Packet::Free(packet);
			return;
		}
		BasicThreadBlock* curBlock = iter->second;
		this->UnlockSessionThreadBlock(SHARED_TRUE);

		this->LockPlayerMap(SHARED_TRUE);
		auto playerIter = this->_PlayerMap.find(sessionID);
		if (playerIter == this->_PlayerMap.end())
		{
			this->DisconnectSession(sessionID);
			this->UnlockPlayerMap(SHARED_TRUE);
			Packet::Free(packet);
			return;
		}
		Player* player = playerIter->second;
		this->UnlockPlayerMap(SHARED_TRUE);

		JobMessage job;
		job._Packet = packet;
		job._Type = JobMessage::Type::MESSAGE;
		job._SessionID = sessionID;
		return curBlock->JobEnqueue(job);
	}


	void GameServer::OnErrorOccured(DWORD errorCode, const WCHAR* error, LogClass::LogLevel level)
	{
		return this->_GameServerLog.LOG(error, level);
	}
	

	bool GameServer::OnConnectionRequest(WCHAR* ipStr, DWORD ip, USHORT port)
	{
		if (this->_ThreadBlockMap.size() == 0)
			return false;
		return !this->_ShutDownFlag && this->_RunningFlag;
	}


	void GameServer::OnClientJoin(WCHAR* ipStr, DWORD ip, USHORT port, ULONGLONG sessionID)
	{
		Player* player = this->CreatePlayer(sessionID);
		JobMessage job;
		job._Player = player;
		job._Packet = nullptr;
		job._Type = JobMessage::Type::CLIENT_ENTER;
		job._SessionID = sessionID;
		job._ThreadBlockName = this->_DefaultThreadBlock->GetThreadBlockName();

		this->LockPlayerMap(SHARED_FALSE);
		this->_PlayerMap.emplace(std::make_pair(sessionID, player));
		this->UnlockPlayerMap(SHARED_FALSE);
		
		this->LockSessionThreadBlock(SHARED_FALSE);
		this->_SessionThreadBlockMap.emplace(std::make_pair(sessionID, this->_DefaultThreadBlock));
		this->UnlockSessionThreadBlock(SHARED_FALSE);
		return this->_DefaultThreadBlock->JobEnqueue(job);
	}


	void GameServer::OnClientLeave(ULONGLONG sessionID)
	{
		this->LockPlayerMap(SHARED_FALSE);
		auto playerIter = this->_PlayerMap.find(sessionID);
		if (playerIter == this->_PlayerMap.end())
		{
			CRASH();
			return;
		}
		Player* player = playerIter->second;
		this->UnlockPlayerMap(SHARED_FALSE);

		this->LockSessionThreadBlock(SHARED_FALSE);
		auto tagIter = this->_SessionThreadBlockMap.find(sessionID);
		if (tagIter == this->_SessionThreadBlockMap.end())
		{
			CRASH();
			return;
		}
		BasicThreadBlock* curBlock = tagIter->second;
		this->UnlockSessionThreadBlock(SHARED_FALSE);

		JobMessage job;
		job._Packet = nullptr;
		job._Player = player;
		job._SessionID = sessionID;
		job._Type = JobMessage::CLIENT_LEAVE;
		job._ThreadBlockName = curBlock->GetThreadBlockName();

		return curBlock->JobEnqueue(job);
	}


	void GameServer::OnTimeOut(ULONGLONG sessionID)
	{
		this->LockPlayerMap(SHARED_FALSE);
		auto playerIter = this->_PlayerMap.find(sessionID);
		if (playerIter == this->_PlayerMap.end())
		{
			CRASH();
			return;
		}
		Player* player = playerIter->second;
		this->UnlockPlayerMap(SHARED_FALSE);

		this->LockSessionThreadBlock(SHARED_FALSE);
		auto tagIter = this->_SessionThreadBlockMap.find(sessionID);
		if (tagIter == this->_SessionThreadBlockMap.end())
		{
			CRASH();
			return;
		}
		BasicThreadBlock* curBlock = tagIter->second;
		this->UnlockSessionThreadBlock(SHARED_FALSE);

		JobMessage job;
		job._Packet = nullptr;
		job._Player = player;
		job._SessionID = sessionID;
		job._Type = JobMessage::TIME_OUT;
		job._ThreadBlockName = curBlock->GetThreadBlockName();

		return curBlock->JobEnqueue(job);
	}


	void GameServer::OnSend(ULONGLONG sessionID)
	{
		//nothing
	}


	Player* GameServer::CreatePlayer(ULONGLONG sessionID)
	{
		Player* newPlayer = this->_PlayerPool.Alloc();
		newPlayer->_SessionID = sessionID;
		newPlayer->_ThreadBlockName = this->_DefaultThreadBlock->GetThreadBlockName();
		newPlayer->_Logined = false;
		return newPlayer;
	}


	Player* GameServer::FindPlayer(ULONGLONG sessionID)
	{
		this->LockPlayerMap(SHARED_TRUE);
		auto iter = this->_PlayerMap.find(sessionID);
		if (iter == this->_PlayerMap.end())
		{
			this->DisconnectSession(sessionID);
			this->UnlockPlayerMap(SHARED_TRUE);
			return nullptr;
		}
		Player* player = iter->second;
		this->UnlockPlayerMap(SHARED_TRUE);
		return player;
	}


	void GameServer::RemovePlayer(ULONGLONG sessionID)
	{
		this->LockSessionThreadBlock(SHARED_FALSE);
		auto tagIter = this->_SessionThreadBlockMap.find(sessionID);
		if (tagIter != this->_SessionThreadBlockMap.end())
			this->_SessionThreadBlockMap.erase(tagIter);
		this->UnlockSessionThreadBlock(SHARED_FALSE);

		this->LockPlayerMap(SHARED_FALSE);
		auto playerIter = this->_PlayerMap.find(sessionID);
		if (playerIter != this->_PlayerMap.end())
		{
			Player* player = playerIter->second;
			this->_PlayerMap.erase(playerIter);
			player->_Logined = false;
			this->_PlayerPool.Free(player);
		}
		this->UnlockPlayerMap(SHARED_FALSE);

		


	}





	void GameServer::SetPlayerThreadBlock(ULONGLONG sessionID ,std::string blockName)
	{
		BasicThreadBlock* nextBlock = this->FindThreadBlock(blockName);
		if (nextBlock == nullptr)
			return;

		this->LockSessionThreadBlock(SHARED_FALSE);
		auto iter = this->_SessionThreadBlockMap.find(sessionID);
		if (iter == this->_SessionThreadBlockMap.end())
		{
			this->DisconnectSession(sessionID);
			this->UnlockSessionThreadBlock(SHARED_FALSE);
			return;
		}
		iter->second = nextBlock;
		return this->UnlockSessionThreadBlock(SHARED_FALSE);
	}

	BaseServer::BasicThreadBlock* GameServer::FindThreadBlock(ULONGLONG sessionID)
	{
		this->LockSessionThreadBlock(SHARED_TRUE);
		auto tagIter = this->_SessionThreadBlockMap.find(sessionID);
		if (tagIter == this->_SessionThreadBlockMap.end())
		{
			this->UnlockSessionThreadBlock(SHARED_TRUE);
			return nullptr;
		}
		BasicThreadBlock* curBlock = tagIter->second;
		this->UnlockSessionThreadBlock(SHARED_TRUE);
		return curBlock;
	}
	BaseServer::BasicThreadBlock* GameServer::FindThreadBlock(std::string blockName)
	{
		this->LockPlayerMap(SHARED_TRUE);
		auto blockIter = this->_ThreadBlockMap.find(blockName);
		if (blockIter == this->_ThreadBlockMap.end())
		{
			this->UnlockPlayerMap(SHARED_TRUE);
			return nullptr;
		}
		BasicThreadBlock* curBlock = blockIter->second;
		this->UnlockPlayerMap(SHARED_TRUE);
		return curBlock;
	}


	void GameServer::MoveRequest(ULONGLONG sessionID, std::string blockName)
	{
		BasicThreadBlock* newBlock = this->FindThreadBlock(blockName);
		if (newBlock == nullptr)
		{
			this->DisconnectSession(sessionID);
			return;
		}

		Player* player = this->FindPlayer(sessionID);
		if(player ==nullptr)
		{
			this->DisconnectSession(sessionID);
			return;
		}

		JobMessage otherJob;
		otherJob._Type = JobMessage::Type::CLIENT_MOVE_ENTER;
		otherJob._SessionID = sessionID;
		otherJob._Packet = nullptr;
		otherJob._Player = player;
		newBlock->JobEnqueue(otherJob);

		return SetPlayerThreadBlock(sessionID, blockName);
	}

}

