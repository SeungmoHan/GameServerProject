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
        OVERLAPPED _Overlapped{ 0 };
        DWORD _IsRecv = false; 
    };


    struct Session
    {
        Session() : _SessionID(0), _SendBufferCount(0), _SendPacketBuffer{ 0 }, _Sock(0), _SessionIP(0), _SessionPort(0), _SessionIPStr{ 0 }, _IOCounts(0), _IOFlag(0), _LastSock(0), _LastSessionID(0) {}
        ULONGLONG _SessionID;
        JobInfo _SendJob;
        JobInfo _RecvJob;
        
        RingBuffer _RingBuffer;
        alignas(64) DWORD _SendBufferCount;
        Packet* _SendPacketBuffer[200];
        LockFreeQueue<Packet*> _SendPacketQueue;

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
