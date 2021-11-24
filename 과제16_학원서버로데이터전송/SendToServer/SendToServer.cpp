#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32")
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>

struct PacketHeader
{
	DWORD packetCode;
	WCHAR name[32]{ 0 };
	WCHAR fileName[128]{ 0 };
	int fileSize;
};

bool DomainToIP(const WCHAR* domain, in_addr* pAddr)
{
	ADDRINFOW* pAddrInfo;
	sockaddr_in* pAddrIn;
	if (GetAddrInfo(domain, L"0", nullptr, &pAddrInfo) != 0)return false;
	pAddrIn = (SOCKADDR_IN*)pAddrInfo->ai_addr;
	*pAddr = pAddrIn->sin_addr;
	FreeAddrInfo(pAddrInfo);
	return true;
}


int main()
{
	FILE* f;
	fopen_s(&f, "Ãò.jpg", "rb");
	if (f == nullptr) return -1;
	fseek(f, 0, SEEK_END);
	int fileSize = ftell(f);
	rewind(f);

	char* buffer = new char[fileSize];
	fread_s(buffer, fileSize, fileSize, 1, f);
	fclose(f);

	WSADATA data;
	if (WSAStartup(MAKEWORD(2, 2), &data))return -12345;

	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
	{
		printf("%d\n", WSAGetLastError());
		return -2;
	}

	in_addr tempAddr;
	if (DomainToIP(L"procademyserver.iptime.org", &tempAddr) == false) return -3;

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr = tempAddr;
	//inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
	addr.sin_port = htons(10010);

	printf("before connect\n");
	int retVal = connect(sock, (sockaddr*)&addr, sizeof(addr));
	if (retVal == SOCKET_ERROR) return -4;

	PacketHeader header;
	header.packetCode = 0x11223344;
	header.fileSize = fileSize;
	wcscpy_s(header.name, L"ÇÑ½Â¸ð");
	wcscpy_s(header.fileName, L"Ãò");

	retVal = send(sock, (const char*)&header, sizeof(header), 0);
	if (retVal == SOCKET_ERROR) return -5;

	int i = 0;
	int idx = 0;
	int remainedFileSize = fileSize;
	int totalSendSize = 0;
	while (remainedFileSize > 0)
	{
		idx++;
		int sendSize;
		if (remainedFileSize >= 1000) sendSize = 1000;
		else sendSize = remainedFileSize;

		retVal = send(sock, buffer + i, sendSize, 0);
		if (retVal != sendSize)
		{
			printf("sendSize is not retVal		");
		}
		i += sendSize;
		remainedFileSize -= sendSize;
		totalSendSize += sendSize;
		printf("sendSize : %d , remainedSize : %d , totalSendSize : %d , idx : %d\n", sendSize, remainedFileSize, totalSendSize, idx);
	}
	printf("idx : %d\n", idx);
	char retFromServer[100]{ 0 };
	retVal = recv(sock, retFromServer, 100, 0);
	if (retVal == SOCKET_ERROR)
	{
		printf("%d\n", WSAGetLastError());
		delete[] buffer;
		closesocket(sock);
		printf("retval : %d , buffer : %x", retVal, *((int*)retFromServer));
		WSACleanup();
		return -6; WSAECONNRESET;
	}
	printf("retval : %d , buffer : %x", retVal, *((int*)retFromServer));
	delete[] buffer;
	closesocket(sock);
	WSACleanup();
}