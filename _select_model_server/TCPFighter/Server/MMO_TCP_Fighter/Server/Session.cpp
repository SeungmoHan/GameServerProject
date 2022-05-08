#include "Session.h"
namespace univ_dev
{
	std::unordered_map<SOCKET, Session*> g_SessionMap;
	univ_dev::ObjectFreeList<Session> g_SessionObjectPool;
	int g_SessionID = 1;
	Session* FindSession(SOCKET sock)
	{
		auto iter = g_SessionMap.find(sock);
		if (iter == g_SessionMap.end())
		{
			int* ptr = nullptr;
			*ptr = 100;
			return nullptr;
		}
		return iter->second;
	}
	Session* CreateSession(SOCKET sock)
	{
		Session* newSession = g_SessionObjectPool.Alloc();
		newSession->lastRecvTime = timeGetTime();
		newSession->sessionID = g_SessionID++;
		newSession->sock = sock;
		newSession->RQ.ClearBuffer();
		newSession->SQ.ClearBuffer();
		g_SessionMap.emplace(std::make_pair(sock, newSession));
		return newSession;
	}
	void RemoveSession(SOCKET sock)
	{
		auto iter = g_SessionMap.find(sock);
		if (iter == g_SessionMap.end())
		{
			int* ptr = nullptr;
			*ptr = 100;
			return;
		}
		Session* removeSession = iter->second;
		removeSession->RQ.ClearBuffer();
		removeSession->SQ.ClearBuffer();
		closesocket(removeSession->sock);

		g_SessionObjectPool.Free(removeSession);

		g_SessionMap.erase(sock);
	}

}