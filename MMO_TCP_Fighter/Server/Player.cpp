#include "Player.h"
#define dfPACKET_MOVE_DIR_LL					0
#define dfPACKET_MOVE_DIR_LU					1
#define dfPACKET_MOVE_DIR_UU					2
#define dfPACKET_MOVE_DIR_RU					3
#define dfPACKET_MOVE_DIR_RR					4
#define dfPACKET_MOVE_DIR_RD					5
#define dfPACKET_MOVE_DIR_DD					6
#define dfPACKET_MOVE_DIR_LD					7

namespace univ_dev
{
	std::unordered_map<DWORD, Player*> g_PlayerMap;
	univ_dev::ObjectFreeList<Player> g_PlayerObjectPool;
	Player* FindPlayer(DWORD sessionID)
	{
		auto iter = g_PlayerMap.find(sessionID);
		if (iter == g_PlayerMap.end())
		{
			int* ptr = nullptr;
			*ptr = 100;
			return nullptr;
		}
		return iter->second;
	}
	Player* CreatePlayer(Session* session)
	{
		Player* newPlayer = g_PlayerObjectPool.Alloc();
		newPlayer->action = 12345;
		newPlayer->moveDirection = 12345;
		newPlayer->direction = rand() % 2 == 0 ? dfPACKET_MOVE_DIR_LL : dfPACKET_MOVE_DIR_RR;
		newPlayer->pSession = session;
		newPlayer->sessionID = session->sessionID;
		newPlayer->xPos = rand() % 6400;
		newPlayer->yPos = rand() % 6400;
		newPlayer->HP = 100;
		newPlayer->oldSector.x = newPlayer->curSector.x = newPlayer->xPos / univ_dev::SECTOR_WIDTH;
		newPlayer->oldSector.y = newPlayer->curSector.y = newPlayer->yPos / univ_dev::SECTOR_WIDTH;

		if (newPlayer->oldSector.x >= univ_dev::SECTOR_MAX_X)
			newPlayer->oldSector.x = univ_dev::SECTOR_MAX_X - 1;
		if (newPlayer->oldSector.y >= univ_dev::SECTOR_MAX_X)
			newPlayer->oldSector.y = univ_dev::SECTOR_MAX_Y - 1;
		if (newPlayer->oldSector.x < 0) newPlayer->oldSector.x = 0;
		if (newPlayer->oldSector.y < 0) newPlayer->oldSector.y = 0;
		
		g_PlayerMap.emplace(std::make_pair(newPlayer->sessionID, newPlayer));
		return newPlayer;
	}
	void RemovePlayer(Session* session)
	{
		auto iter = g_PlayerMap.find(session->sessionID);
		if (iter == g_PlayerMap.end())
		{
			int* ptr = nullptr;
			*ptr = 100;
			return;
		}
		Player* removePlayer = iter->second;
		g_PlayerMap.erase(session->sessionID);
		g_PlayerObjectPool.Free(removePlayer);
	}
}