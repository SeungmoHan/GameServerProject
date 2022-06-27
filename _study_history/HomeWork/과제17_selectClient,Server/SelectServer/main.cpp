#pragma comment(lib,"ws2_32.lib")
#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <list>
#include <set>
#define MESSAGE_ID 0
#define MESSAGE_CREATE_STAR 1
#define MESSAGE_DELETE_STAR 2
#define MESSAGE_MOVE 3
#define MAP_HEIGHT 24
#define MAP_WIDTH 81
struct Player
{
	int id;
	int x;
	int y;
	SOCKET clientSocket;
};

struct Header
{
	int type;
};
struct MsgID
{
	Header header;
	int id;
private:
	int not_used[2];
};
struct MsgCreateStar
{
	Header header;
	int id;
	int x;
	int y;
};
struct MsgDeleteStar
{
	Header header;
	int id;
private:
	int not_used[2];
};
struct MsgMove
{
	Header header;
	int id;
	int x;
	int y;
};

int g_PlayerID = 0;


char g_MapBuffer[MAP_HEIGHT][MAP_WIDTH]{ 0 };

SOCKET g_ListenSocket;
std::list<Player*> g_PlayerList;
std::set<Player*> g_RemovePlayerList;

SOCKET SetListenSocket();
int SendUnicast(Player* target, char* buffer, int size);
bool SendBroadcast(Player* exTarget, char* buffer, int size);
bool AcceptProc();
bool Disconnect();
void NetWorking();
int RecvProc(Player* target);

void Render();

int frame;
int g_FramePerSec;
clock_t begin, end;
int main()
{
	CONSOLE_CURSOR_INFO info;
	info.bVisible = 0;
	info.dwSize = 1;
	SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
	g_ListenSocket = SetListenSocket();
	begin = end = clock();
	while (true)
	{
		NetWorking();
		Render();

		frame++;
		end = clock();
		if (end - begin >= 1000)
		{
			g_FramePerSec = frame;
			frame = 0;
			begin = end;
		}
	}
}

void Render()
{
	memset(g_MapBuffer, ' ', MAP_HEIGHT * MAP_WIDTH);
	for (auto iter = g_PlayerList.begin(); iter != g_PlayerList.end(); ++iter)
	{
		Player* temp = *iter;
		g_MapBuffer[temp->y][temp->x] = '*';
	}
	COORD pos{ 0,0 };
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
	printf("Connect Client : %d                                        ", (int)g_PlayerList.size());
	pos.Y++;
	for (int i = 0; i < MAP_HEIGHT; i++)
	{
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
		g_MapBuffer[i][MAP_WIDTH - 1] = '\0';
		printf(g_MapBuffer[i]);
		pos.Y++;
	}
}

bool Disconnect()
{
	for (auto iter = g_RemovePlayerList.begin(); iter != g_RemovePlayerList.end(); ++iter)
	{
		Player* temp = *iter;
		g_PlayerList.erase(std::find(g_PlayerList.begin(), g_PlayerList.end(), temp));
		closesocket(temp->clientSocket);
		MsgDeleteStar packet;
		packet.header.type = MESSAGE_DELETE_STAR;
		packet.id = temp->id;
		SendBroadcast(temp, (char*)&packet, sizeof(packet));
		delete* iter;
	}
	g_RemovePlayerList.clear();
	return true;
}

int SendUnicast(Player* target, char* buffer, int size)
{
	int idPacketSendRet = send(target->clientSocket, buffer, sizeof(MsgID), 0);
	if (idPacketSendRet == SOCKET_ERROR)
	{
		g_RemovePlayerList.emplace(target);
		return SOCKET_ERROR;
	}
	return idPacketSendRet;
}
bool SendBroadcast(Player* exTarget, char* buffer, int size)
{
	bool flag = true;
	for (auto iter = g_PlayerList.begin(); iter != g_PlayerList.end(); ++iter)
	{
		if ((*iter) == exTarget) continue;
		if (!SendUnicast((*iter), buffer, size)) flag = false;
	}
	return flag;
}
bool AcceptProc()
{
	//printf("Start Accept\n");
	//accept 한뒤에
	sockaddr_in addr;
	int addrlen = sizeof(addr);
	SOCKET client = accept(g_ListenSocket, (sockaddr*)&addr, &addrlen);
	if (client == INVALID_SOCKET)
	{
		closesocket(client);
		return false;
	}
	//player 만들어주고
	Player* player = new Player();
	player->id = g_PlayerID++;
	player->x = 40;
	player->y = 10;
	player->clientSocket = client;

	//첫 player에게 id정보 알려준뒤
	MsgID idPacket;
	idPacket.header.type = MESSAGE_ID;
	idPacket.id = player->id;
	SendUnicast(player, (char*)&idPacket, sizeof(MsgID));

	//돌면서 다른 모든 별생성 패킷을 접속자에게 보내고
	for (auto iter = g_PlayerList.begin(); iter != g_PlayerList.end(); ++iter)
	{
		Player* temp = (*iter);
		MsgCreateStar createOtherPacket;
		createOtherPacket.header.type = MESSAGE_CREATE_STAR;
		createOtherPacket.id = temp->id;
		createOtherPacket.x = temp->x;
		createOtherPacket.y = temp->y;
		SendUnicast(player, (char*)&createOtherPacket, sizeof(MsgCreateStar));
	}
	//플레이어를 이제 리스트에 넣은후에
	g_PlayerList.push_back(player);

	// 모든 사람에게 플레이어 별생성 패킷을 보낸다.
	MsgCreateStar createPacket;
	createPacket.header.type = MESSAGE_CREATE_STAR;
	createPacket.id = player->id;
	createPacket.x = player->x;
	createPacket.y = player->y;
	SendBroadcast(nullptr, (char*)&createPacket, sizeof(MsgCreateStar));
	//printf("Accept Clear\n");
	return true;
}

int RecvProc(Player* target)
{
	//printf("RecvProc Start\n");
	//버퍼를 만든뒤에 리시브를 하고
	char buffer[16]{ 0 };
	int recvRet = recv(target->clientSocket, buffer, 16, 0);
	//에러가 있는지 우선체크 (select에서 나온뒤 없다고 할지라도 일단 체크)
	if (recvRet == SOCKET_ERROR)
	{
		//에러코드 확인뒤 WSAEWOULDBLOCK이 아니라면
		int err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK)
		{
			//진짜 에러임
			g_RemovePlayerList.emplace(target);
			return -1;
		}
		//받을게 없다는 의미.
		//만약 WOULDBLOCK이라고 한다면 받을게 없다는 의미
		return 0;
	}
	// 여기까지 왔으면 일단 받을건 있다는 의미.
	Header* header = (Header*)buffer;
	//헤더를 통해 어떤 메시지인지 읽고
	switch (header->type)
	{
	case MESSAGE_MOVE:
	{
		//그 메시지로 캐스팅 해서 사용
		MsgMove* packet = (MsgMove*)buffer;
		if (target->id != packet->id) break;
		target->x = packet->x;
		target->y = packet->y;
		SendBroadcast(target, buffer, 16);
		break;
	}
	}

	//printf("RecvProc Clear\n");
	return recvRet;
}


void NetWorking()
{
	//rSet초기화 후 리슨소켓 먼저 set
	fd_set rSet;
	FD_ZERO(&rSet);
	FD_SET(g_ListenSocket, &rSet);

	//그다음 플레이어 리스트를 돌면서 전부다 set
	for (auto iter = g_PlayerList.begin(); iter != g_PlayerList.end(); ++iter)
		FD_SET((*iter)->clientSocket, &rSet);

	//select의 timeval을 0으로 설정하고 select를함.
	TIMEVAL time;
	time.tv_usec = 0;
	time.tv_sec = 0;
	int selectRet = select(0, &rSet, nullptr, nullptr, &time);

	//listen socket이 set에 남아있으면 accept를 해야됨.
	if (FD_ISSET(g_ListenSocket, &rSet))
	{
		AcceptProc();
	}

	//나머지 하나씩 돌면서 set이 되어있는지 확인후 set되어있으면 recv를 해야됨.
	for (auto iter = g_PlayerList.begin(); iter != g_PlayerList.end(); ++iter)
	{
		if (FD_ISSET((*iter)->clientSocket, &rSet))
		{
			RecvProc(*iter);
		}
	}
	//문제가 생긴 모든 것들을 모아서 여기서 처리.
	Disconnect();
}




SOCKET SetListenSocket()
{
	WSADATA data;
	if (WSAStartup(MAKEWORD(2, 2), &data)) return INVALID_SOCKET;

	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET)
	{
		printf("socket() error %d\n", WSAGetLastError());
		return INVALID_SOCKET;
	}

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(3000);
	addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);// 해도안해도그만... 어차피 0은 뒤집어도 0

	int bindRet = bind(listenSocket, (sockaddr*)&addr, sizeof(addr));
	if (bindRet == SOCKET_ERROR)
	{
		printf("bind() error %dn\n", WSAGetLastError());
		return INVALID_SOCKET;
	}
	u_long nonBlockingOption = 1;
	ioctlsocket(listenSocket, FIONBIO, &nonBlockingOption);

	LINGER linger;
	linger.l_onoff = 1;
	linger.l_linger = 0;

	setsockopt(listenSocket, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));

	int listenRet = listen(listenSocket, SOMAXCONN);
	if (listenRet == SOCKET_ERROR)
	{
		printf("listen() error %d\n", WSAGetLastError());
		return INVALID_SOCKET;
	}

	return listenSocket;
}