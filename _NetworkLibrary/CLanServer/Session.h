#pragma once
#ifndef __ECHO_SERVER_SESSION__
#define __ECHO_SERVER_SESSION__
#define __UNIV_DEVELOPER_

#include "CoreBase.h"
#include <unordered_map>

namespace univ_dev
{
    struct JobInfo
    {
        OVERLAPPED _Overlapped;
        DWORD _IsRecv;
        univ_dev::RingBuffer _RingBuffer;
    };


    struct Session
    {
        ULONGLONG _SessionID;
        JobInfo _SendJob;
        JobInfo _RecvJob;
        SOCKET _Sock;
        ULONG _SessionIP;
        WCHAR _SessionIPStr[20];
        USHORT _SessionPort;
        DWORD _IOFlag;
        DWORD _IOCounts;
        CRITICAL_SECTION _Lock;
    };
}


#endif // !__ECHO_SERVER_SESSION_
