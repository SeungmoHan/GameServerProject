#pragma comment(lib ,"Winmm.lib")
#pragma comment(lib, "ws2_32.lib")
#include <WinSock2.h>
#include <process.h>
#include <time.h>
#include "Session.h"
#include <unordered_map>
#include <conio.h>
#include <stdio.h>
#define SERVERPORT 6000

#define dfECHO_PACKET_HEADER_LENGTH (int)sizeof(EchoPacketHeader)
#define dfNUM_OF_THREAD 16


struct EchoPacketHeader
{
    unsigned short payloadSize;
};


std::unordered_map<DWORD,int> g_ThreadId;

// 작업자 스레드 함수
unsigned __stdcall WorkerThread(void* arg);
unsigned __stdcall AcceptThread(void* param);

HANDLE g_hIOCP;
SOCKET g_ListenSocket;
HANDLE g_hThread[dfNUM_OF_THREAD];
HANDLE g_AcceptThread;
HANDLE g_LogThread;

SRWLOCK g_PacketPoolLock;

//비동기 send전환용 flag
bool g_SendZeroBuffer;

bool g_ShutDownFlag;


// debug용도
unsigned int g_SendSuccessCount;
unsigned int g_RecvSuccessCount;
unsigned int g_SendIOPendingCount;
unsigned int g_RecvIOPendingCount;
unsigned long long g_PacketPerSec;
unsigned long long g_TotalPacket;

unsigned __stdcall MoniteringThread(void* param)
{
    ULONGLONG beginTime = GetTickCount64();
    DWORD prev = timeGetTime();
    while (true)
    {
        DWORD cur = timeGetTime();
        ULONGLONG now = GetTickCount64();
        if (cur - prev < 1000)
        {
            if (_kbhit())
            {
                int key = _getch();
                key = tolower(key);
                if (key == 's' || key == 'q')
                {
                    univ_dev::SaveProfiling();
                    univ_dev::ResetProfiling();
                }
                if (key == 'q')
                {
                    g_ShutDownFlag = true;
                    closesocket(g_ListenSocket);
                    printf("Monitering Thread End\n");
                    return 0;
                }
            }
            Sleep(5);
            continue;
        }
        prev = cur;
        system("cls");
        unsigned long long r_TPS = InterlockedExchange(&g_PacketPerSec, 0);
        printf("SaveProfile = \'s\'\nQuitProgram = \'q\'\nExcuteTime : %llu\nTPS : %llu\nTotal Packet Process : %llu\ng_SendSuccessCount : %d\ng_SendIOPendingCount : %d\ng_RecvSuccessCount : %d\ng_RecvIOPendingCount : %d\nMEMORY_FREE_LIST\nPacketPool Capacity : %d\nPacketPool UseCount : %d\nSessionPool Capacity : %d\nSessionPool UseCount : %d\n", (now - beginTime) / 1000,r_TPS,g_TotalPacket,g_SendSuccessCount,g_SendIOPendingCount,g_RecvSuccessCount, g_RecvIOPendingCount, univ_dev::g_PacketObjectPool.GetCapacityCount(), univ_dev::g_PacketObjectPool.GetUseCount(), univ_dev::g_SessionObjectPool.GetCapacityCount(), univ_dev::g_SessionObjectPool.GetUseCount());
        int cnt = 0;
        for (auto iter = g_ThreadId.begin(); iter != g_ThreadId.end(); ++iter)
        {
            printf("Thread : %d , Count : %d\t\t\t", iter->first, iter->second);
            if (++cnt % 2 == 0)
                printf("\n");
        }
    }
}

void ServerCleanup()
{
    WSACleanup();
    CloseHandle(g_hIOCP);
    for (auto iter = univ_dev::g_SessionMap.begin(); iter != univ_dev::g_SessionMap.end(); ++iter)
    {
        closesocket(iter->second->sock);
        univ_dev::g_SessionObjectPool.Free(iter->second);
    }
    univ_dev::g_SessionMap.clear();
}

int main()
{
    FILE* MainThreadLogFile = nullptr;
    char logFileName[128]{ 0 };
    char tempStr[20]{ 0 };
    time_t currentTime = time(nullptr);
    tm time;
    localtime_s(&time, &currentTime);
    _itoa_s(time.tm_year + 1900, tempStr, 10);
    strcat_s(logFileName, tempStr);
    strcat_s(logFileName, "_");
    _itoa_s(time.tm_mon + 1, tempStr, 10);
    strcat_s(logFileName, tempStr);
    strcat_s(logFileName, "_");
    _itoa_s(time.tm_mday, tempStr, 10);
    strcat_s(logFileName, tempStr);
    strcat_s(logFileName, "_MainThreadLog.txt");

    
    while (MainThreadLogFile == nullptr)
    {
        printf("MainThreadLogFile == nullptr\n");
        fopen_s(&MainThreadLogFile, logFileName, "ab");
    }
    fprintf(MainThreadLogFile, "-------------------------------------------------------\n");
    fprintf(MainThreadLogFile, "-------------------------------------------------------\n");
    int retval;
    // 윈속 초기화
    WSADATA wsa;
    printf("WSAStartUp Begin\n");
    fprintf(MainThreadLogFile, "WSAStartUp Begin\n");
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;
    fprintf(MainThreadLogFile, "WSAStartUp End\n");
    printf("WSAStartUp End\n");
    fflush(MainThreadLogFile);
    // 입출력 완료 포트 생성


    printf("IOCP CreateBegin\n");
    fprintf(MainThreadLogFile, "IOCP CreateBegin\n");
    g_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    if (g_hIOCP == NULL) return 1;
    fprintf(MainThreadLogFile, "IOCP CreateEnd\n");
    printf("IOCP CreateEnd\n");
    fflush(MainThreadLogFile);

    printf("timeBeginPeriod, SRWLock Init, ProfilerSample Init Begin\n");
    fprintf(MainThreadLogFile, "timeBeginPeriod, SRWLock Init, ProfilerSample Init Begin\n");
    timeBeginPeriod(1);
    InitializeSRWLock(&univ_dev::g_SessionLock);
    InitializeSRWLock(&g_PacketPoolLock);
    univ_dev::InitializeProfilerAndSamples();
    g_ShutDownFlag = false;
    fprintf(MainThreadLogFile, "timeBeginPeriod, SRWLock Init, ProfilerSample Init End\n");
    printf("timeBeginPeriod, SRWLock Init, ProfilerSample Init End\n");
    fflush(MainThreadLogFile);


    printf("PrintThread CreateBegin\n");
    fprintf(MainThreadLogFile, "PrintThread CreateBegin\n");
    g_LogThread = (HANDLE)_beginthreadex(nullptr, 0, MoniteringThread, nullptr, 0, nullptr);
    if (g_LogThread == nullptr) return -1;
    fprintf(MainThreadLogFile, "PrintThread CreateEnd\n");
    printf("PrintThread CreateEnd\n");
    fflush(MainThreadLogFile);

    SYSTEM_INFO si;
    GetSystemInfo(&si);

    // 쓰레드풀 형성
    printf("ThreadPool CreateBegin\n");
    fprintf(MainThreadLogFile, "ThreadPool CreateBegin\n");
    for (int i = 0; i < (int)si.dwNumberOfProcessors * 2; i++) 
    {
        g_hThread[i] = (HANDLE)_beginthreadex(NULL, 0, WorkerThread, g_hIOCP, 0, NULL);
        if (g_hThread[i] == NULL)
        {
            fclose(MainThreadLogFile);
            return 1;
        }
    }
    fprintf(MainThreadLogFile, "ThreadPool CreateEnd\n");
    printf("ThreadPool CreateEnd\n");
    fflush(MainThreadLogFile);


    // socket()
    printf("listen socket CreateBegin\n");
    fprintf(MainThreadLogFile, "listen socket CreateBegin\n");
    g_ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_ListenSocket == INVALID_SOCKET)
    {
        int err = WSAGetLastError();
        printf("listen socket is INVALID_SOCKET : %d\n", err);
        fprintf(MainThreadLogFile, "listen socket is INVALID_SOCKET : %d\n", err);
        fclose(MainThreadLogFile);
        return -1;
    }
    fprintf(MainThreadLogFile, "listen socket CreateEnd\n");
    printf("listen socket CreateEnd\n");

    // bind()
    SOCKADDR_IN serveraddr;
    ZeroMemory(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(SERVERPORT);
    printf("Port : %d BindBegin\n", ntohs(serveraddr.sin_port));
    fprintf(MainThreadLogFile, "Port : %d BindBegin\n", ntohs(serveraddr.sin_port));
    retval = bind(g_ListenSocket, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
    if (retval == SOCKET_ERROR)
    {
        int err = WSAGetLastError();
        printf("bind retval is SOCKET_ERROR : %d\n", err);
        return -1;
    }
    fprintf(MainThreadLogFile, "Port : %d BindEnd\n", ntohs(serveraddr.sin_port));
    printf("Port : %d BindEnd\n", ntohs(serveraddr.sin_port));
    fflush(MainThreadLogFile);


    LINGER l;
    l.l_onoff = 1;
    l.l_linger = 0;
    printf("Linger Option Setting Begin\n");
    fprintf(MainThreadLogFile, "Linger Option Setting Begin\n");
    setsockopt(g_ListenSocket, SOL_SOCKET, SO_LINGER, (const char*)&l, sizeof(l));
    fprintf(MainThreadLogFile, "Linger Option Setting End\n");
    printf("Linger Option Setting End\n");
    fflush(MainThreadLogFile);

    // listen()
    printf("listen begin BACKLOG QUEUE Size : %d -> ReasonableValue 200\n", SOMAXCONN);
    fprintf(MainThreadLogFile, "listen begin BACKLOG QUEUE Size : %d -> ReasonableValue 200\n", SOMAXCONN);
    retval = listen(g_ListenSocket, SOMAXCONN);
    if (retval == SOCKET_ERROR) 
    {
        int err = WSAGetLastError();
        fprintf(MainThreadLogFile, "listen retval is SOCKET_ERROR : %d\n",err);
        printf("listen retval is SOCKET_ERROR : %d\n", err);
        fclose(MainThreadLogFile);
        return -1;
    }
    fprintf(MainThreadLogFile, "listen end BACKLOG QUEUE Size : %d -> ReasonableValue 200\n", SOMAXCONN);
    printf("listen end BACKLOG QUEUE Size : %d -> ReasonableValue 200\n", SOMAXCONN);
    fflush(MainThreadLogFile);

    g_SendZeroBuffer = true;
    if (g_SendZeroBuffer)
    {
        DWORD sendBufferSize = 0;
        printf("setsockopt -> Async Send Active Begin\n");
        fprintf(MainThreadLogFile, "setsockopt -> Async Send Active Begin\n");
        setsockopt(g_ListenSocket, SOL_SOCKET, SO_SNDBUF, (const char*)&sendBufferSize, sizeof(sendBufferSize));
        fprintf(MainThreadLogFile, "setsockopt -> Async Send Active End\n");
        printf("setsockopt -> Async Send Active End\n");
        fflush(MainThreadLogFile);
    }
    
    printf("AcceptThread Create Begin\n");
    fprintf(MainThreadLogFile, "AcceptThread Create Begin\n");
    g_AcceptThread = (HANDLE)_beginthreadex(nullptr, 0, AcceptThread, nullptr, 0, nullptr);
    if (g_AcceptThread == nullptr)
    {
        printf("AcceptThread is nullptr\n");
        fprintf(MainThreadLogFile, "AcceptThread is nullptr\n");
        fclose(MainThreadLogFile);
        return -1;
    }
    fprintf(MainThreadLogFile, "AcceptThread Create End\n");
    printf("AcceptThread Create End\n");
    fflush(MainThreadLogFile);


    HANDLE TotalRunningThread[dfNUM_OF_THREAD + 2];

    for (int i = 0; i < si.dwNumberOfProcessors * 2; i++)
        TotalRunningThread[i] = g_hThread[i];
    TotalRunningThread[si.dwNumberOfProcessors * 2] = g_AcceptThread;
    TotalRunningThread[si.dwNumberOfProcessors * 2 + 1] = g_LogThread;

    printf("WaitForMultipleObjects -> Begin\n");
    fprintf(MainThreadLogFile, "WaitForMultipleObjects -> Begin\n");
    fclose(MainThreadLogFile);
    MainThreadLogFile = nullptr;
    DWORD ret = WaitForMultipleObjects(si.dwNumberOfProcessors * 2 + 2, TotalRunningThread, true, INFINITE);
    if (ret == WAIT_TIMEOUT)
    {
        printf("Time out\n");
        system("pause");
    }
    else if (ret == WAIT_ABANDONED)
    {
        printf("wait abandoned");
        system("pause");
    }
    else if (ret == WAIT_FAILED)
    {
        printf("wait failed");
        int err = WSAGetLastError();
        printf("Error == %d\n", err);
        system("pause");
    }
    while (MainThreadLogFile == nullptr)
    {
        printf("MainThreadLogFile == nullptr\n");
        fopen_s(&MainThreadLogFile, logFileName, "ab");
    }
    
    printf("WaitForMultipleObjects returned -> All Thread Stopped -> MainThread Stop\n");
    fprintf(MainThreadLogFile, "WaitForMultipleObjects returned->All Thread Stopped->MainThread Stop\n");

    fprintf(MainThreadLogFile, "-------------------------------------------------------\n");
    fprintf(MainThreadLogFile, "-------------------------------------------------------\n");
    fclose(MainThreadLogFile);
    //사용했던 메모리 다 정리하고 스톱
    

    ServerCleanup();

    return 0;
}
unsigned __stdcall AcceptThread(void* param)
{
    SOCKET client_sock;
    SOCKADDR_IN clientaddr;
    int addrlen;
    DWORD recvbytes, flags;
    DWORD retval;

    while (true)
    {
        // accept()
        addrlen = sizeof(clientaddr);
        client_sock = accept(g_ListenSocket, (SOCKADDR*)&clientaddr, &addrlen);
        if (client_sock == INVALID_SOCKET)
        {
            int err = WSAGetLastError();
            if (err == WSAEINTR || err == WSAENOTSOCK || err == WSAEINVAL)
            {
                ULONG_PTR completionKey = (ULONG_PTR)0xffffffff;
                DWORD byteTransfered = 0;
                PostQueuedCompletionStatus(g_hIOCP, byteTransfered, completionKey, nullptr);
                printf("Accept Thread End\n");
                return 0;
            }
            printf("client_sock == INVALID_SOCKET : %d\n", err);
            continue;
        }

        univ_dev::Session* newSession = univ_dev::CreateSession(client_sock, clientaddr, g_hIOCP);
        WSABUF recvBuffer;
        recvBuffer.buf = newSession->recvJob.ringBuffer.GetWritePtr();
        recvBuffer.len = newSession->recvJob.ringBuffer.DirectEnqueueSize();
        // 비동기 입출력 시작
        flags = 0;
        InterlockedIncrement(&newSession->ioCounts);
        //printf("Begin Recv Call\n");
        retval = WSARecv(client_sock, &recvBuffer, 1, &recvbytes,
            &flags, &newSession->recvJob.overlapped, NULL);
        if (retval == 0)
            InterlockedIncrement(&g_RecvSuccessCount);
        if (retval == SOCKET_ERROR)
        {
            int err = WSAGetLastError();
            if (err != WSA_IO_PENDING)
            {
                InterlockedDecrement(&newSession->ioCounts);
                printf("WSARecv is SOCKET_ERROR and WSAGetLastError is not WSA_IO_PENDING : %d\n", err);
            }
            else
                InterlockedIncrement(&g_RecvIOPendingCount);
        }
    }
    return -1;
}
// 작업자 스레드 함수
unsigned __stdcall WorkerThread(void* arg)
{
    int retval;

    while (1) 
    {
        DWORD byteTransfered = 0;
        ULONG_PTR completionKey = 0;
        OVERLAPPED* over = nullptr;
        univ_dev::JobInfo* job = nullptr;
        univ_dev::Session* session = nullptr;
        retval = 0;
        retval = GetQueuedCompletionStatus(g_hIOCP, &byteTransfered, &completionKey, &over, INFINITE);
        g_ThreadId[GetCurrentThreadId()]++;
        if (over == nullptr)
        {
            printf("ThreadID : %d\nCompletionKey : %llu     Thread End\n", GetCurrentThreadId(), completionKey);
            if (completionKey == 0xffffffff)
            {
                ULONG_PTR completionKey = (ULONG_PTR)0xffffffff;
                DWORD byteTransfered = 0;
                PostQueuedCompletionStatus(g_hIOCP, byteTransfered, completionKey, nullptr);
                return 0;
            }
            printf("over is nullptr\n");
            int err = WSAGetLastError();
            printf("Error code : %d\n", err);
            system("pause");
            continue;
        }

        session = (univ_dev::Session*)completionKey;
        job = (univ_dev::JobInfo*)over;
        int sendRet = 0;
        int recvRet = 0;
        int processBytes = 0;
            //recv 완료처리라면
        if (retval != 0 && byteTransfered != 0 && job->isRecv)
        {
            DWORD threadID = GetCurrentThreadId();
            univ_dev::Profiler recvLoopProfile("recv loop", GetCurrentThreadId());
            //printf("RecvJob GQCS return\n");
            //완료된 사이즈만큼 writePointer를 증가시킨다.
            session->recvJob.ringBuffer.MoveWritePtr(byteTransfered);
            //링버퍼에 있는거 하나하나 전부다 직렬화 버퍼로 가져올거임.
            AcquireSRWLockExclusive(&g_PacketPoolLock);
            univ_dev::Packet* sendPacket = univ_dev::g_PacketObjectPool.Alloc();
            univ_dev::Packet* recvPacket = univ_dev::g_PacketObjectPool.Alloc();
            ReleaseSRWLockExclusive(&g_PacketPoolLock);
            int sendRet;
            int recvRet;
            int i = 0;
            while (true)
            {
                //printf("Cycle %d\n", ++i);
                //링버퍼의 현재 사이즈가 헤더의 길이보다 작으면 더이상 뽑을게 없다는 의미임.
                if (session->recvJob.ringBuffer.GetUseSize() < dfECHO_PACKET_HEADER_LENGTH)
                {
                    //printf("this point RingBuffer.GetUseSize() -> %d\n",session->recvJob.ringBuffer.GetUseSize());
                    break;
                }
                sendPacket->Clear();
                recvPacket->Clear();
                
                //헤더하나 만든뒤 피크해서 2바이트가 제대로 피크됬는지 확인
                EchoPacketHeader header;
                int pkRet1 = session->recvJob.ringBuffer.Peek((char*)&header, dfECHO_PACKET_HEADER_LENGTH);
                if (pkRet1 != dfECHO_PACKET_HEADER_LENGTH)
                {
                    printf("pkRet is not dfECHO_PACKET_HEADER_LENGTH : %d\n", pkRet1);
                    break;
                }
                //printf("payloadSize : %d\n", header.payloadSize);
                //recv링버퍼에서 피크 했을때 payloadSize랑 합한것보다 링버퍼 사이즈가 작으면 다음번으로 넘긴다.
                if (session->recvJob.ringBuffer.GetUseSize() < dfECHO_PACKET_HEADER_LENGTH + header.payloadSize)
                //if (session->recvJob.ringBuffer.GetUseSize() < dfECHO_PACKET_HEADER_LENGTH + 8)
                {
                    printf("job->ringBuffer.GetUseSize() < dfECHO_PACKET_HEADER_LENGTH + header.payloadSize :%d, %d\n",session->recvJob.ringBuffer.GetUseSize(), dfECHO_PACKET_HEADER_LENGTH + header.payloadSize);
                    break;
                }
                //recv 링버퍼에서 데이터를 꺼냈으니 ReadPointer를 증가시켜서 UseSize를 줄여야함.
                session->recvJob.ringBuffer.MoveReadPtr(pkRet1);
                //payloadSize만큼 recv 링버퍼에서 다시 꺼내온다
                int pkRet2 = session->recvJob.ringBuffer.Peek(recvPacket->GetWritePtr(), header.payloadSize);
                if (pkRet2 != header.payloadSize)
                {
                    printf("pkRet2 is not header.payloadSize : %d PKRET : %d\n", header.payloadSize, pkRet2);
                    break;
                }

                session->recvJob.ringBuffer.MoveReadPtr(pkRet2);
                recvPacket->MoveWritePtr(pkRet2);
                unsigned __int64 data = 15;
                (*recvPacket) >> data;

                (*sendPacket) << header.payloadSize << data;

                int eqRet = session->sendJob.ringBuffer.Enqueue(sendPacket->GetReadPtr(), sendPacket->GetBufferSize());
                if (eqRet != dfECHO_PACKET_HEADER_LENGTH + header.payloadSize)
                {
                    //printf("eqRet is not header.payloadSize + dfECHO_PACKET_HEADER_LENGTH : %d EQRET : %d\n", header.payloadSize + dfECHO_PACKET_HEADER_LENGTH, eqRet);
                    break;
                }
                processBytes += eqRet;
                InterlockedIncrement(&g_TotalPacket);
                InterlockedIncrement(&g_PacketPerSec);
            }
            sendPacket->Clear();
            recvPacket->Clear();
            AcquireSRWLockExclusive(&g_PacketPoolLock);
            univ_dev::g_PacketObjectPool.Free(sendPacket);
            univ_dev::g_PacketObjectPool.Free(recvPacket);
            ReleaseSRWLockExclusive(&g_PacketPoolLock);

            WSABUF sendWSABuf[2];
            sendWSABuf[0].buf = session->sendJob.ringBuffer.GetReadPtr();
            sendWSABuf[0].len = session->sendJob.ringBuffer.DirectDequeueSize();
            sendWSABuf[1].buf = session->sendJob.ringBuffer.GetBeginPtr();
            sendWSABuf[1].len = session->sendJob.ringBuffer.GetUseSize() - session->sendJob.ringBuffer.DirectDequeueSize();
            session->sendJob.ringBuffer.MoveReadPtr(processBytes);
            DWORD byteSents = 0;
            ZeroMemory(&session->sendJob.overlapped, sizeof(OVERLAPPED));
            //printf("        WSASend Call Processed Bytes : %d\n", processBytes);
            InterlockedIncrement(&session->ioCounts);
            bool sendFlag = _InterlockedExchange(&session->ioFlag, true);
            if (sendFlag == false)
            {
                sendRet = WSASend(session->sock, sendWSABuf, 2, &byteSents, 0, &session->sendJob.overlapped, nullptr);
                if (sendRet == 0)
                {
                    printf("Sync IO Success\n");
                    InterlockedIncrement(&g_SendSuccessCount);
                }
                if (sendRet == SOCKET_ERROR)
                {
                    int err = WSAGetLastError();
                    if (err != WSA_IO_PENDING)
                    {
                        if (err != 10054)
                            printf("WSASend(), err is not WSA_IO_PENDING : %d\n", err);
                        InterlockedDecrement(&session->ioCounts);
                    }
                    else
                        InterlockedIncrement(&g_SendIOPendingCount);
                }
                _InterlockedExchange(&session->ioFlag, false);
            }

            WSABUF recvWSABuf[2];
            recvWSABuf[0].buf = session->recvJob.ringBuffer.GetWritePtr();
            recvWSABuf[0].len = session->recvJob.ringBuffer.DirectEnqueueSize();
            recvWSABuf[1].buf = session->recvJob.ringBuffer.GetBeginPtr();
            recvWSABuf[1].len = session->recvJob.ringBuffer.GetFreeSize() - session->recvJob.ringBuffer.DirectEnqueueSize();
            
            
            if (recvWSABuf[1].len > session->recvJob.ringBuffer.GetBufferSize())
                recvWSABuf[1].len = 0;

            DWORD byteRecvs;
            DWORD flag = 0;
            ZeroMemory(&session->recvJob.overlapped, sizeof(OVERLAPPED));
            //printf("        WSARecv Call\n");
            InterlockedIncrement(&session->ioCounts);
            recvRet = WSARecv(session->sock, recvWSABuf, 2, &byteRecvs, &flag, &session->recvJob.overlapped, nullptr);
            if (recvRet == 0)
            {
                InterlockedIncrement(&g_RecvSuccessCount);
            }
            if (recvRet == SOCKET_ERROR)
            {
                int err = WSAGetLastError();
                if (err != WSA_IO_PENDING)
                {
                    if (err != 10054)
                        printf("WSARecv(), err is not WSA_IO_PENDING : %d\n", err);
                    InterlockedDecrement(&session->ioCounts);
                }
                else
                    InterlockedIncrement(&g_RecvIOPendingCount);
            }
        }
        else
        {
            //printf("SendJob GQCS return\n");
            //printf("SendByte Transfered %d\n", byteTransfered);
        }
        int ioCounts = InterlockedDecrement(&session->ioCounts);
        if (ioCounts == 0)
        {
            //printf("Session Release\n");
            univ_dev::ReleaseSession(session->sock);
        }
    }
    return -1;
}