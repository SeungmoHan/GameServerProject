#pragma comment(lib,"ws2_32.lib")

#include <iostream>
#include <list>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "Message.h"
#include "Player.h"


#define MAP_WIDTH 81
#define MAP_HEIGHT 24

char gameBuffer[MAP_HEIGHT][MAP_WIDTH]{ 0 };

std::list<Player*> g_PlayerList;
Player* g_MyPlayer;
int g_MyID = -1;
SOCKET g_ClientSocket;

SOCKET CreateClientSocket(const char* ipStr);
void NetWorking();
void Update();
void Render();
void KeyProcess();

void Player::Render()
{
	gameBuffer[y][x] = '*';
}

int main()
{
	char ipStr[16]{"127.0.0.1" };
	scanf_s("%s", ipStr, sizeof(ipStr));
	g_ClientSocket = CreateClientSocket(ipStr);
	if (g_ClientSocket == INVALID_SOCKET) return -1;
	CONSOLE_CURSOR_INFO info;
	info.bVisible = false;
	info.dwSize = 1;
	SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);

	while (true)
	{
		Update();
	}
	
	closesocket(g_ClientSocket);
	WSACleanup();
}

SOCKET CreateClientSocket(const char* ipStr)
{
	WSADATA data;
	if (WSAStartup(MAKEWORD(2, 2), &data)) return INVALID_SOCKET;

	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
	{
		printf("socket() %d error\n", WSAGetLastError());
		WSACleanup();
		return INVALID_SOCKET;
	}

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(3000);
	inet_pton(AF_INET, ipStr, &addr.sin_addr);
	int conRet = connect(sock, (sockaddr*)&addr, sizeof(addr));
	if (conRet == SOCKET_ERROR)
	{
		printf("connect() %d error\n", WSAGetLastError());
		closesocket(sock);
		WSACleanup();
		return INVALID_SOCKET;
	}
	u_long mode = 1;
	ioctlsocket(sock, FIONBIO, &mode);
	return sock;
}

void Render()
{
	memset(gameBuffer, ' ', MAP_HEIGHT * MAP_WIDTH);
	auto iter = g_PlayerList.begin();
	for (; iter != g_PlayerList.end(); ++iter)
	{
		(*iter)->Render();
	}
	COORD coord{ 0,0 };
	for (; coord.Y < MAP_HEIGHT; coord.Y++)
	{
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
		gameBuffer[coord.Y][MAP_WIDTH - 1] = '\0';
		printf(gameBuffer[coord.Y]);
	}
}

void KeyProcess()
{
	short leftState = GetAsyncKeyState(VK_LEFT);
	short rightState = GetAsyncKeyState(VK_RIGHT);
	short upState = GetAsyncKeyState(VK_UP);
	short downState = GetAsyncKeyState(VK_DOWN);
	Player::Direction dir;

	if		(leftState && upState)		dir = Player::Direction::LU;
	else if (leftState && downState)	dir = Player::Direction::LD;
	else if (rightState && upState)		dir = Player::Direction::RU;
	else if (rightState && downState)	dir = Player::Direction::RD;
	else if (leftState)					dir = Player::Direction::LL;
	else if (rightState)				dir = Player::Direction::RR;
	else if (upState)					dir = Player::Direction::UU;
	else if (downState)					dir = Player::Direction::DD;
	else								dir = Player::Direction::NON;
	char sendBuffer[16]{ 0 };
	Header* header = (Header*)sendBuffer;
	header->type = MESSAGE_MOVE;
	MsgMove* moveMessage = (MsgMove*)(sendBuffer + 4);
	if (g_MyPlayer != nullptr)
	{
		if (dir != Player::Direction::NON)
		{
			g_MyPlayer->Move(dir);
			moveMessage->id = g_MyID;
			moveMessage->x = g_MyPlayer->GetXPos();
			moveMessage->y = g_MyPlayer->GetYPos();
			int sendRet = send(g_ClientSocket, sendBuffer, 16, 0);
			if (sendRet == SOCKET_ERROR)
			{
				int err = WSAGetLastError();
				if (err != WSAEWOULDBLOCK)
				{
					//진짜에러
					int err = WSAGetLastError();
					printf("send() error : %d", err);
					closesocket(g_ClientSocket);
					WSACleanup();
					int* ptr = nullptr;
					*ptr = 100;
				}
			}
		}

	}
}

void NetWorking()
{
	fd_set rSet;
	FD_ZERO(&rSet);
	FD_SET(g_ClientSocket, &rSet);
	timeval time{ 0,0 };
	int selectRet = select(0, &rSet, nullptr, nullptr, &time);
	if (selectRet == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		//printf("%d\n", error);
		if (error != WSAEWOULDBLOCK)
		{
			//진짜에러
			printf("select error %d\n", error);
			closesocket(g_ClientSocket);
			WSACleanup();
			int* ptr = nullptr;
			*ptr = 100;
			return;
		}
	}
	int recvRet;
	while (true)
	{
		char buffer[16]{ 0 };
		recvRet = recv(g_ClientSocket, buffer, 16, 0);
		if (recvRet == SOCKET_ERROR)
		{
			int err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK)
			{
				//더이상 받을게 없는 케이스
				break;
			}
			else
			{
				//진짜 에러
				printf("select error %d\n", err);
				closesocket(g_ClientSocket);
				WSACleanup();
				int* ptr = nullptr;
				*ptr = 100;
				break;
			}
		}
		//여기까지 왔으면 이제 문제가 없는 케이스.
		Header* pHeader = (Header*)buffer;
		switch (pHeader->type)
		{
		case MESSAGE_ID:
		{
			MsgID* msg = (MsgID*)(buffer + 4);
			g_MyID = msg->id;
			//printf("Message ID :");
			//printf("%d \n", msg->id);
			auto iter = g_PlayerList.begin();
			for (; iter != g_PlayerList.end(); ++iter)
			{
				Player* currentPlayer = *iter;
				if (currentPlayer->GetID() == g_MyID)
				{
					g_MyPlayer = currentPlayer;
					break;
				}
			}
			break;
		}
		case MESSAGE_CREATE_STAR:
		{
			MsgCreateStar* msg = (MsgCreateStar*)(buffer + 4);
			Player* newPlayer = new Player(msg->id, msg->x, msg->y);
			//printf("Message Create Star : ");
			//printf("Player id : %d, player x : %d, player y : %d\n", msg->id, msg->x, msg->y);
			g_PlayerList.push_back(newPlayer);
			if (g_MyPlayer == nullptr)
			{
				if (g_MyID == -1)break;
				auto iter = g_PlayerList.begin();
				for (; iter != g_PlayerList.end(); ++iter)
				{
					if ((*iter)->GetID() == g_MyID) g_MyPlayer = *iter;
				}
			}
			break;
		}
		case MESSAGE_DELETE_STAR:
		{
			MsgDeleteStar* msg = (MsgDeleteStar*)(buffer + 4);
			auto iter = g_PlayerList.begin();
			//printf("Message Delete Star : ");
			//printf("player id : %d\n", msg->id);
			for (; iter != g_PlayerList.end(); ++iter)
			{
				if ((*iter)->GetID() == msg->id)
				{
					delete* iter;
					g_PlayerList.erase(iter);
					break;
				}
			}
			break;
		}
		case MESSAGE_MOVE:
		{
			MsgMove* msg = (MsgMove*)(buffer + 4);
			auto iter = g_PlayerList.begin();
			//printf("MEssage Move : ");
			//printf("player id : %d, player x : %d, player y : %d\n", msg->id, msg->x, msg->y);
			for (; iter != g_PlayerList.end(); ++iter)
			{
				if ((*iter)->GetID() == msg->id)
				{
					if ((*iter)->GetID() == g_MyID) break;
					(*iter)->Move(msg->x, msg->y);
					break;
				}
			}
			break;
		}
		}
	}
}
void Update()
{
	NetWorking();
	KeyProcess();
	Render();
	Sleep(15);
}