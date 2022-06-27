#pragma comment(lib,"ws2_32.lib")
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>



struct PacketHeader
{
	DWORD packetCode ;
	WCHAR name[32] ;
	WCHAR fileName[128] ;
};



int main()
{
	WSADATA data;
	if (WSAStartup(MAKEWORD(2, 2), &data)) return -1;

	SOCKET listenSocket= socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket == INVALID_SOCKET) return -2;

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(10010);
	addr.sin_addr.S_un.S_addr = INADDR_ANY;

	int retVal = bind(listenSocket, (sockaddr*)&addr, sizeof(addr));
	if (retVal == SOCKET_ERROR) return -3;

	retVal = listen(listenSocket, SOMAXCONN);
	if (retVal == SOCKET_ERROR) return -4;

	while (true)
	{
		sockaddr_in addr;
		int addrlen = sizeof(addr);
		SOCKET sock = accept(listenSocket, (sockaddr*)&addr, &addrlen);
		if (sock == INVALID_SOCKET) break;

		PacketHeader header;
		retVal = recv(sock, (char*)&header, sizeof(header), 0);
		if (header.packetCode != 0x11223344)
		{
			closesocket(sock);
			continue;
		}
		if (retVal == SOCKET_ERROR) break;
		FILE* f;
		fopen_s(&f, "tempFile.jpg", "wb");
		if (f == nullptr) break;
		while (true)
		{
			char buffer[1000]{ 0 };
			retVal = recv(sock, buffer, 1000, 0);
			if (retVal == 0) break;
			fwrite(buffer, retVal, 1, f);
			if (retVal != 1000) break;
		}
		int a = 0xdddddddd;
		send(sock, (const char*)&a, 4, 0);
		fclose(f);
	}

	closesocket(listenSocket);
	WSACleanup();
}