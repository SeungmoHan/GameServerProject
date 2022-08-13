#pragma once
#ifndef __ECHO_SERVER_SESSION__
#define __ECHO_SERVER_SESSION__
#define __UNIV_DEVELOPER_

#include "CoreBase.h"
#include <unordered_map>
#include <list>

#define SESSION_SEND_PACKER_BUFFER_SIZE 200
namespace univ_dev
{
    struct OverlappedEx
    {
        OVERLAPPED _Overlapped{ 0 };
        DWORD _IsRecv = false;
    };


    struct Session
    {
        Session()
            : _SessionID(0), _SendBufferCount(0), _SendPacketBuffer{ 0 }, _Sock(0x80000000), _SessionIP(0),_Available(0),
            _SessionPort(0), _SessionIPStr{ 0 }, _IOCounts(0x80000000), _IOFlag(0), _TimeOutTimer(0), _ThreadBlockIdx(-1){};
        //Read Only
        DWORD                   _Available;
        volatile ULONGLONG      _SessionID;
        volatile SOCKET         _Sock;
        ULONG                   _SessionIP;
        WCHAR                   _SessionIPStr[20];
        USHORT                  _SessionPort;

        alignas(64) DWORD       _ThreadBlockIdx;

        alignas(64)
        OverlappedEx            _RecvJob;
        DWORD                   _TimeOutTimer;
        RingBuffer              _RingBuffer;
        alignas(64)
            OverlappedEx            _SendJob;
        Packet* _SendPacketBuffer[SESSION_SEND_PACKER_BUFFER_SIZE];
        LockFreeQueue<Packet*>  _SendPacketQueue;
        alignas(64) DWORD       _SendBufferCount;
        alignas(64) DWORD       _IOCounts;
        alignas(64) DWORD       _IOFlag;
    };
}


#endif // !__ECHO_SERVER_SESSION_
