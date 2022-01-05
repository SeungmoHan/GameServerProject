#pragma comment(lib,"ws2_32.lib")
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>



int main()
{
	WSADATA data;
	if (WSAStartup(MAKEWORD(2, 2), &data)) return -1;

	SOCKET client;
	int idx = 0;
	while (true)
	{
		client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (client == INVALID_SOCKET)
		{
			return -2;
		}
		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(8000);
		inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
		int retVal = connect(client, (sockaddr*)&addr, sizeof(addr));
		if (retVal == SOCKET_ERROR)
		{
			int error = WSAGetLastError();
			printf("%d\n", error);
			WCHAR buffer[100]{ 0 };
			FormatMessage(0, nullptr, error, LANG_NEUTRAL, buffer, 100, nullptr);
			wprintf(buffer);
			break;
		}

		closesocket(client);
		idx++;
	}
	printf("%d client connect", idx);
	while (true);
	return 0;
}