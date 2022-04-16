#pragma once
#ifndef __ECHO_SERVER_SESSION__
#define __ECHO_SERVER_SESSION__
#define __UNIV_DEVELOPER_

#include "CoreBase.h"
#include <unordered_map>
#include <list>
namespace univ_dev
{
    struct JobInfo
    {
        OVERLAPPED _Overlapped;
        DWORD _IsRecv;
        RingBuffer _RingBuffer;
        LockFreeQueue<Packet*> _PacketQueue;
    };


    struct Session
    {
        ULONGLONG _SessionID;
        JobInfo _SendJob;
        JobInfo _RecvJob;
        
        alignas(64) DWORD _SendBufferCount;
        Packet _SendPacketBuffer[200];

        SOCKET _Sock;
        ULONG _SessionIP;
        WCHAR _SessionIPStr[20];
        USHORT _SessionPort;
        CRITICAL_SECTION _Lock;
        alignas(64) DWORD _IOCounts;
        alignas(64) DWORD _IOFlag;

        //Debug Field
        SOCKET _LastSock;
        SOCKET _LastSessionID;
    };
}


#endif // !__ECHO_SERVER_SESSION_
