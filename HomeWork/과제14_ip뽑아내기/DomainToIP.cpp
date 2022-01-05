#pragma comment(lib,"ws2_32.lib")
#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>




bool PrintIpFromDomain(const char* url)
{
	ADDRINFOA* pInfo;
	getaddrinfo(url, nullptr, nullptr, &pInfo);


	system("ipconfig /flushdns");
	if (pInfo == nullptr)
	{
		return false;
	}
	ADDRINFOA* temp = pInfo;
	printf("%s ip : \n", url);
	while (temp != nullptr)
	{
		sockaddr_in* addr = (sockaddr_in*)temp->ai_addr;
		printf("%d.%d.%d.%d\n", addr->sin_addr.S_un.S_un_b.s_b1, addr->sin_addr.S_un.S_un_b.s_b2, addr->sin_addr.S_un.S_un_b.s_b3, addr->sin_addr.S_un.S_un_b.s_b4);
		temp = temp->ai_next;
	}
	freeaddrinfo(pInfo);

	return true;
}
int main()
{

	WSADATA data;
	if (WSAStartup(MAKEWORD(2, 2), &data)) return -1;
	printf("SetUp Clear\n");

	char url[50]{ 0 };
	scanf_s("%s", url, sizeof(url));
	PrintIpFromDomain(url);
	WSACleanup();

	return 0;
}