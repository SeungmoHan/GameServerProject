#pragma once
#ifndef __CHAT_SERVER_PLAYER__
#define __CHAT_SERVER_PLAYER__
#define __UNIV_DEVELOPER_
#include "CoreBase.h"

namespace univ_dev
{
	struct Player
	{
		Player() : _Logined(0), _SessionID(0), _AccountNo(0), _SectorX(0), _SectorY(0), _ID{ 0 }, _NickName{ 0 }, _TokenKey{ 0 }, _LastAction{ 0 }{};
		Player(ULONGLONG sessionID, INT64 accountNo, WORD sectorX, WORD sectorY, const WCHAR* ID, const WCHAR* nickname)
			:_SessionID(sessionID), _AccountNo(accountNo), _SectorX(sectorX), _SectorY(sectorY), _TokenKey{ 0 }, _LastAction{ 0 }
		{
			memcpy_s(_ID, 20, ID, 20);
			memcpy_s(_NickName, 20, nickname, 20);
		}

		DWORD _Logined;

		ULONGLONG _SessionID;

		INT64 _AccountNo;

		WORD _SectorX;
		WORD _SectorY;

		WCHAR _ID[20];
		WCHAR _NickName[20];
		char _TokenKey[64];

		DWORD _LastAction;
	};
}


#endif // !__CHAT_SERVER_PLAYER__
