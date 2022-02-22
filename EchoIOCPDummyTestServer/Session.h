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
        bool isRecv;
        univ_dev::RingBuffer ringBuffer;
    };


    struct Session
    {
        JobInfo sendJob;
        JobInfo recvJob;
        SOCKET sock;

        unsigned int ioCounts;
        DWORD ioFlag;

        unsigned long ip;
        unsigned short port;
    };

    extern std::unordered_map<SOCKET, Session*> g_SessionMap;
    extern univ_dev::ObjectFreeList<Session> g_SessionObjectPool;
    extern SRWLOCK g_SessionLock;
    Session* FindSession(SOCKET key);
    Session* CreateSession(SOCKET key, sockaddr_in clientaddr, HANDLE hIOCP);
    void ReleaseSession(SOCKET key);
}


#endif // !__ECHO_SERVER_SESSION_
