#pragma comment(lib,"ws2_32.lib")
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <iostream>

/// <backlog queue counting>
/// listen side
int main()
{
	WSADATA data;
	if (WSAStartup(MAKEWORD(2, 2), &data)) return -1;

	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET)
	{
		return -2;
	}
	printf("socket()\n");
	sockaddr_in addr;
	ZeroMemory(&addr, sizeof(addr));
	addr.sin_addr.S_un.S_addr = INADDR_ANY;
	addr.sin_port = htons(8000);
	addr.sin_family = AF_INET;
	int retVal = bind(listenSocket, (sockaddr*)&addr, sizeof(addr));
	if (retVal == SOCKET_ERROR)
	{
		return -3;
	}
	printf("bind()\n");
	
	retVal = listen(listenSocket, SOMAXCONN_HINT(65535));
	SOMAXCONN_HINT(10);
	if (retVal == SOCKET_ERROR)
	{
		return -4;
	}
	printf("listen()\n");
	while (true);

	return 0;
}
/// </backlog queue counting>