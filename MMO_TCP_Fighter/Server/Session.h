#pragma once
#ifndef __SESSION_HEADER__
#define __SESSION_HEADER__
#define __UNIV_DEVELOPER_

#include "RingBuffer.h"
#include <WinSock2.h>
#include <unordered_map>
#include "ObjectFreeList.hpp"

namespace univ_dev
{
	constexpr int RECV_TIMEOUT = 60000;

	struct Session
	{
		SOCKET sock;
		DWORD sessionID;
		RingBuffer RQ;
		RingBuffer SQ;
		DWORD lastRecvTime;
	};
	extern std::unordered_map<SOCKET, Session*>g_SessionMap;
	extern ObjectFreeList<Session> g_SessionObjectPool;

	Session* FindSession(SOCKET sock);
	Session* CreateSession(SOCKET sock);
	void RemoveSession(SOCKET sock);
	
}




#endif // !__SESSION_HEADER__
