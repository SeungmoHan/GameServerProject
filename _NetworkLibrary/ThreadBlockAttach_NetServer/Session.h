#pragma once
#ifndef __ECHO_SERVER_SESSION__
#define __ECHO_SERVER_SESSION__
#define __UNIV_DEVELOPER_

#include "CoreBase.h"

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
            : _SessionID(0), _SendBufferCount(0), _SendPacketBuffer{ 0 }, _Sock(0x80000000), _SessionIP(0), _Available{ false }, 
            _SessionPort(0), _SessionIPStr{ 0 }, _IOCounts(0x80000000), _IOFlag(0), _TimeOutTimer(0), _ThreadBlockIdx(-1){};
        //Read Only
        ULONGLONG               _SessionID;
        SOCKET                  _Sock;
        DWORD                   _Available;
        ULONG                   _SessionIP;
        WCHAR                   _SessionIPStr[20];
        USHORT                  _SessionPort;

        DWORD       _ThreadBlockIdx;

        alignas(64)
        OverlappedEx            _RecvJob;
        DWORD                   _TimeOutTimer;
        RingBuffer              _RingBuffer;
        
        alignas(64)
        OverlappedEx            _SendJob;
        Packet*                 _SendPacketBuffer[SESSION_SEND_PACKER_BUFFER_SIZE];
        LockFreeQueue<Packet*>  _SendPacketQueue;
        DWORD                   _SendBufferCount;
        
        alignas(64) DWORD       _IOCounts;
        alignas(64) DWORD       _IOFlag;
    };
}


#endif // !__ECHO_SERVER_SESSION_
