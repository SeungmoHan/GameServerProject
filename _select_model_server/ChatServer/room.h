#pragma once
#ifndef __ROOM_HEADER__
#define __ROOM_HEADER__
#define __UNIV_DEVELOPER_
#define ROOM_NAME_MAX_LEN 256

struct Session;

namespace univ_dev
{
	struct Room
	{
		DWORD roomNumber;
		WCHAR roomName[ROOM_NAME_MAX_LEN];
		std::unordered_set<Session*> participants;
	};
}


#endif // !__ROOM_HEADER__
