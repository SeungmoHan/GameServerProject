#pragma once
#ifndef __ECHO_SERVER_SESSION__
#define __ECHO_SERVER_SESSION__
#define __UNIV_DEVELOPER_

#include "BaseCore.h"
#include <unordered_map>

namespace univ_dev
{
    struct JobInfo
    {
        OVERLAPPED overlapped;
        DWORD isRecv;
        univ_dev::RingBuffer ringBuffer;
    };


    struct Session
    {
        ULONGLONG sessionID;
        JobInfo sendJob;
        JobInfo recvJob;
        SOCKET sock;
        ULONG ip;
        USHORT port;
        DWORD ioFlag;
        DWORD ioCounts;
        SRWLOCK lock;
    };

    //extern std::unordered_map<SOCKET, Session*> g_SessionMap;
    //extern univ_dev::ObjectFreeList<Session> g_SessionObjectPool;
    //extern SRWLOCK g_SessionLock;
    //Session* FindAndLockSession(SOCKET key);
    //Session* CreateSession(SOCKET key, sockaddr_in clientaddr, HANDLE hIOCP);
    //void ReleaseSession(SOCKET key);
}


#endif // !__ECHO_SERVER_SESSION_
