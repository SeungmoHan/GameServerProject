#pragma comment(lib,"ws2_32.lib")
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>


#define PORT_BEGIN 10001
#define PORT_END 10100


/// udp서버에서 방제목 찾아오기

int main()
{
    WSADATA data;
    if (WSAStartup(MAKEWORD(2, 2), &data)) return -1;

    SOCKET client = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (client == INVALID_SOCKET)
    {
        int err = WSAGetLastError();
        printf("socket() error : %d\n", err);
        return -2;
    }
    bool enable = true;
    timeval time{ 0 ,0 };
    time.tv_usec = 200000;
    setsockopt(client, SOL_SOCKET, SO_BROADCAST, (char*)&enable, sizeof(bool));
    setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (char*)&time, sizeof(timeval));
    //0xff 0xee 0xdd 0xaa 0x00 0x99 0x77 0x55 0x33 0x11
    char buffer[10]{ 0xff ,0xee,0xdd,0xaa,0x00,0x99,0x77,0x55,0x33,0x11 };
    WCHAR retBuffer[256]{ 0 };
    u_long wait = 1;
    ioctlsocket(client, FIONBIO, &wait);
    for (int i = PORT_BEGIN; i < PORT_END; i++)
    {
        fd_set rSet;
        FD_ZERO(&rSet);
        FD_SET(client, &rSet);
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.S_un.S_addr = htonl(INADDR_BROADCAST);
        addr.sin_port = htons(i);
        int sendRet = sendto(client, buffer, 10, 0, (sockaddr*)&addr, sizeof(addr));
        if (sendRet == SOCKET_ERROR)
        {
            int err = WSAGetLastError();
            if (err != WSAEWOULDBLOCK)
            {
                printf("error\n");
                break;
            }
            printf("wouldblock\n");
        }
        int addrlen = sizeof(addr);
        int selectRet = select(0, &rSet, nullptr, nullptr, &time);
        if (selectRet == SOCKET_ERROR)
        {
            int err = WSAGetLastError();
            printf("%d", err);
            return -3;
        }
        if (selectRet == 0)
        {
            printf(".");
            continue;
        }
        int recvRet = recvfrom(client, (char*)retBuffer, sizeof(WCHAR) * 256, 0, (sockaddr*)&addr, &addrlen);
        if (recvRet == SOCKET_ERROR)
        {
            int err = WSAGetLastError();
            if (err != WSAEWOULDBLOCK)
            {
                continue;
            }
            return -4;
        }
        else if (recvRet > 0)
        {
            printf("\nrecv Byte : %d\t", recvRet);
            retBuffer[recvRet / sizeof(WCHAR)] = 0;
            printf("IP : %d.%d.%d.%d\t", addr.sin_addr.S_un.S_un_b.s_b1, addr.sin_addr.S_un.S_un_b.s_b2, addr.sin_addr.S_un.S_un_b.s_b3, addr.sin_addr.S_un.S_un_b.s_b4);
            printf("port : %d\t", ntohs(addr.sin_port));
            wprintf(retBuffer);
            printf("\n");
        }

    }

}