#pragma once
#ifndef __PLAYER_HEADER__
#define __PLAYER_HEADER__
#define __UNIV_DEVELOPER_
#include "Session.h"
#include "Sector.h"
#include "ObjectFreeList.hpp"
#include <unordered_map>


namespace univ_dev
{
	struct Session;

	struct Player
	{
		Session* pSession;
		DWORD sessionID;

		DWORD action;
		BYTE direction;
		BYTE moveDirection;

		char HP;

		short xPos;
		short yPos;


		Sector curSector;
		Sector oldSector;

	};
	extern std::unordered_map<DWORD, Player*> g_PlayerMap;
	extern ObjectFreeList<Player> g_PlayerObjectPool;
	Player* FindPlayer(DWORD sessionID);
	Player* CreatePlayer(Session* session);
	void RemovePlayer(Session* session);
}


#endif // !__PLAYER_HEADER__
