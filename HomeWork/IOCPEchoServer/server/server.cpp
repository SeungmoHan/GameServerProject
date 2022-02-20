#define _WINSOCK_DEPRECATED_NO_WARNINGS // �ֽ� VC++ ������ �� ��� ����
#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <process.h>
#include <unordered_map>
#include "RingBuffer.h"
#define SERVERPORT 9000

// ���� ���� ������ ���� ����ü

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

    unsigned long ip;
    unsigned short port;
};

std::unordered_map<SOCKET, Session*> g_SessionMap;

Session* CreateSession(SOCKET key,sockaddr_in addr);
void ReleaseSession(SOCKET key);

// �۾��� ������ �Լ�
DWORD WINAPI WorkerThread(LPVOID arg);
// ���� ��� �Լ�
void err_quit(const char* msg);
void err_display(const char* msg);
HANDLE hcp;

SOCKET g_ClientSocket;

unsigned int g_SendSuccessCount;
unsigned int g_RecvSuccessCount;

unsigned int g_SendIOPendingCount;
unsigned int g_RecvIOPendingCount;

bool g_SendZeroBuffer;


Session* FindSession(SOCKET key)
{
    auto iter = g_SessionMap.find(key);
    if (iter == g_SessionMap.end())
        return nullptr;
    return iter->second;
}

Session* CreateSession(SOCKET key,sockaddr_in clientaddr)
{
    Session* newSession = new Session();
    newSession->ip = clientaddr.sin_addr.S_un.S_addr;
    newSession->port = clientaddr.sin_port;
    newSession->ioCounts = 0;
    newSession->sock = key;
    newSession->recvJob.isRecv = true;
    newSession->sendJob.isRecv = false;
    ZeroMemory(&newSession->recvJob.overlapped, sizeof(OVERLAPPED));

    CreateIoCompletionPort((HANDLE)key, hcp, (ULONG_PTR)newSession, 0);
    g_SessionMap.emplace(std::make_pair(key, newSession));
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
    g_SessionMap.erase(key);
    delete removeSession;

    return;
}

unsigned __stdcall PrintThread(void* param)
{
    while (true)
    {
        Sleep(INFINITE);
        //Sleep(1000);
        system("cls");
        printf("g_SendSuccessCount : %d\n", g_SendSuccessCount);
        printf("g_SendIOPendingCount : %d\n", g_SendIOPendingCount);
        printf("g_RecvSuccessCount : %d\n", g_RecvSuccessCount);
        printf("g_RecvIOPendingCount : %d\n", g_RecvIOPendingCount);
    }
}

int main(int argc, char* argv[])
{
    int retval;
    // ���� �ʱ�ȭ
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

    // ����� �Ϸ� ��Ʈ ����
    hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    if (hcp == NULL) return 1;

    HANDLE tempThread = (HANDLE)_beginthreadex(nullptr, 0, PrintThread, nullptr, 0, nullptr);
    if (tempThread == nullptr) return -1;
    // CPU ���� Ȯ��
    SYSTEM_INFO si;
    GetSystemInfo(&si);

    // (CPU ���� * 2)���� �۾��� ������ ����
    HANDLE hThread;
    for (int i = 0; i < (int)si.dwNumberOfProcessors * 2; i++) {
        hThread = CreateThread(NULL, 0, WorkerThread, hcp, 0, NULL);
        if (hThread == NULL) return 1;
        CloseHandle(hThread);
    }

    // socket()
    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == INVALID_SOCKET) err_quit("socket()");

    // bind()
    SOCKADDR_IN serveraddr;
    ZeroMemory(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(SERVERPORT);
    retval = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
    if (retval == SOCKET_ERROR) err_quit("bind()");

    // listen()
    retval = listen(listen_sock, SOMAXCONN);
    if (retval == SOCKET_ERROR) err_quit("listen()");

    g_SendZeroBuffer = true;
    if (g_SendZeroBuffer)
    {
        DWORD sendBufferSize = 0;
        setsockopt(listen_sock, SOL_SOCKET, SO_SNDBUF, (const char*)&sendBufferSize, sizeof(sendBufferSize));
    }
    // ������ ��ſ� ����� ����
    SOCKET client_sock;
    SOCKADDR_IN clientaddr;
    int addrlen;
    DWORD recvbytes, flags;

    while (1) {
        // accept()
        addrlen = sizeof(clientaddr);
        client_sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);
        if (client_sock == INVALID_SOCKET) 
        {
            err_display("accept()");
            break;
        }
        printf("accept() return\n");
        printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
            inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

        // ���ϰ� ����� �Ϸ� ��Ʈ ����

        Session* newSession = CreateSession(client_sock,clientaddr);
        WSABUF recvBuffer;
        recvBuffer.buf = newSession->recvJob.ringBuffer.GetWritePtr();
        recvBuffer.len = newSession->recvJob.ringBuffer.DirectEnqueueSize();
        printf("recvWSABUF length : %u\n", recvBuffer.len);
        // �񵿱� ����� ����
        flags = 0;
        printf("WSARecv() call\n");
        InterlockedIncrement(&newSession->ioCounts);
        retval = WSARecv(client_sock, &recvBuffer, 1, &recvbytes,
            &flags, &newSession->recvJob.overlapped, NULL);
        if (retval == 0)
            InterlockedIncrement(&g_RecvSuccessCount);
        if (retval == SOCKET_ERROR) {
            if (WSAGetLastError() != ERROR_IO_PENDING)
                err_display("WSARecv()");
            else
                InterlockedIncrement(&g_RecvIOPendingCount);
        }
    }

    // ���� ����
    WSACleanup();
    return 0;
}

// �۾��� ������ �Լ�
DWORD WINAPI WorkerThread(LPVOID arg)
{
    int retval;
    HANDLE hcp = (HANDLE)arg;

    while (1) 
    {
        DWORD byteTransfered = 0;
        ULONG_PTR completionKey = 0;
        SOCKET clientSocket = 0;
        OVERLAPPED* over = nullptr;
        JobInfo* job = nullptr;
        Session* session = nullptr;
        retval = 0;
        retval = GetQueuedCompletionStatus(hcp, &byteTransfered, &completionKey, &over, INFINITE);
        printf("Thread %d Wake up\n", GetCurrentThreadId());
        if (over == nullptr)
        {
            printf("over is nullptr\n");
            int err = WSAGetLastError();
            printf("Error code : %d\n", err);
            continue;
        }
        session = (Session*)completionKey;
        int ioCounts = session->ioCounts;
        InterlockedDecrement(&session->ioCounts);
        //printf("------------------------------\n");
        //printf("retval != 0 : %d\n",retval);
        //printf("byteTramsfered : %d\n", byteTransfered);
        //printf("IO Count : %d\n", ioCounts);
        //printf("------------------------------\n");
        if (retval == 0)
        {
            printf("retval == 0\n");
            if (session->ioCounts == 0)
            {
                printf("ReleaseSession\n");
                closesocket(session->sock);
                ReleaseSession(session->sock);
                continue;
            }
            continue;
        }
        if (byteTransfered == 0)
        {
            printf("bytetransfered == 0\n");
            if (session->ioCounts == 0)
            {
                printf("ReleaseSession\n");
                closesocket(session->sock);
                ReleaseSession(session->sock);
                continue;
            }
            continue;
        }

        job = (JobInfo*)over;
        int sendRet;
        int recvRet;
        if (job->isRecv)
        {
            session->recvJob.ringBuffer.MoveWritePtr(byteTransfered);
            char buf[10000];
            int pkRet = session->recvJob.ringBuffer.Dequeue(buf, byteTransfered);
            buf[pkRet] = '\0';
            printf("%s : %d bytes\n", buf, byteTransfered);

            int eqRet = session->sendJob.ringBuffer.Enqueue(buf, byteTransfered);
            WSABUF buffer;
            buffer.buf = session->sendJob.ringBuffer.GetReadPtr();
            buffer.len = byteTransfered;
            ZeroMemory(&session->sendJob.overlapped, sizeof(OVERLAPPED));
            session->sendJob.isRecv = false;

            DWORD byteSents;
            InterlockedIncrement(&session->ioCounts);
            sendRet = WSASend(session->sock, &buffer, 1, &byteSents, 0, &session->sendJob.overlapped, nullptr);
            if (sendRet == SOCKET_ERROR)
            {
                int err = WSAGetLastError();
                if (err != WSA_IO_PENDING)
                {
                    printf("WSASend() -> ERR_Log err code : %d\n", err);
                    closesocket(session->sock);
                    InterlockedDecrement(&session->ioCounts);
                    continue;
                }
            }
        }
        else
        {
            //send �۾� ó��
            session->sendJob.ringBuffer.MoveReadPtr(byteTransfered);
            WSABUF buffer;
            buffer.buf = session->recvJob.ringBuffer.GetWritePtr();
            buffer.len = session->recvJob.ringBuffer.DirectEnqueueSize();
            ZeroMemory(&session->recvJob.overlapped, sizeof(OVERLAPPED));
            session->recvJob.isRecv = true;

            DWORD byteRecved;
            DWORD flag = 0;
            InterlockedIncrement(&session->ioCounts);
            recvRet = WSARecv(session->sock, &buffer, 1, &byteRecved, &flag, &session->recvJob.overlapped, nullptr);
            if (recvRet == SOCKET_ERROR)
            {
                int err = WSAGetLastError();
                if (err != WSA_IO_PENDING)
                {
                    printf("WSASend() -> ERR_Log err code : %d\n", err);
                    closesocket(session->sock);
                    InterlockedDecrement(&session->ioCounts);
                    continue;
                }
            }
        }
    }

    return 0;
}

// ���� �Լ� ���� ��� �� ����
void err_quit(const char* msg)
{
    LPVOID lpMsgBuf;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf, 0, NULL);
    MessageBoxA(NULL, (LPCSTR)lpMsgBuf, msg, MB_ICONERROR);
    LocalFree(lpMsgBuf);
    exit(1);
}

// ���� �Լ� ���� ���
void err_display(const char* msg)
{
    LPVOID lpMsgBuf;
    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&lpMsgBuf, 0, NULL);
    printf("[%s] %s", msg, (char*)lpMsgBuf);
    LocalFree(lpMsgBuf);
}