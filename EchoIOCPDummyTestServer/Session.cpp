#include "BaseCore.h"
#include "Session.h"

namespace univ_dev
{
    std::unordered_map<SOCKET, Session*> g_SessionMap;
    univ_dev::ObjectFreeList<Session> g_SessionObjectPool;
    SRWLOCK g_SessionLock;

    Session* FindSession(SOCKET key)
    {
        auto iter = g_SessionMap.find(key);
        if (iter == g_SessionMap.end())
            return nullptr;
        return iter->second;
    }

    Session* CreateSession(SOCKET key, sockaddr_in clientaddr,HANDLE hIOCP)
    {
        //Session* newSession = new Session();
        AcquireSRWLockExclusive(&g_SessionLock);
        Session* newSession = g_SessionObjectPool.Alloc();
        newSession->ip = clientaddr.sin_addr.S_un.S_addr;
        newSession->port = clientaddr.sin_port;
        newSession->ioCounts = 0;
        newSession->sock = key;
        newSession->recvJob.isRecv = true;
        newSession->sendJob.isRecv = false;
        ZeroMemory(&newSession->recvJob.overlapped, sizeof(OVERLAPPED));
        ZeroMemory(&newSession->sendJob.overlapped, sizeof(OVERLAPPED));
        g_SessionMap.emplace(std::make_pair(key, newSession));
        ReleaseSRWLockExclusive(&g_SessionLock);
        CreateIoCompletionPort((HANDLE)key, hIOCP, (ULONG_PTR)newSession, 0);
        return newSession;
    }
    void ReleaseSession(SOCKET key)
    {
        auto removeSessionIter = g_SessionMap.find(key);
        if (removeSessionIter == g_SessionMap.end())
        {
            printf("removeSessionIter is g_SessionMap.end()\n");
            return;
        }
        Session* removeSession = removeSessionIter->second;
        AcquireSRWLockExclusive(&g_SessionLock);
        closesocket(key);
        g_SessionMap.erase(key);
        g_SessionObjectPool.Free(removeSession);
        //delete removeSession;
        ReleaseSRWLockExclusive(&g_SessionLock);
        return;
    }
}
