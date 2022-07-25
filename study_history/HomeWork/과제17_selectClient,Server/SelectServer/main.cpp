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
	//accept �ѵڿ�
	sockaddr_in addr;
	int addrlen = sizeof(addr);
	SOCKET client = accept(g_ListenSocket, (sockaddr*)&addr, &addrlen);
	if (client == INVALID_SOCKET)
	{
		closesocket(client);
		return false;
	}
	//player ������ְ�
	Player* player = new Player();
	player->id = g_PlayerID++;
	player->x = 40;
	player->y = 10;
	player->clientSocket = client;

	//ù player���� id���� �˷��ص�
	MsgID idPacket;
	idPacket.header.type = MESSAGE_ID;
	idPacket.id = player->id;
	SendUnicast(player, (char*)&idPacket, sizeof(MsgID));

	//���鼭 �ٸ� ��� ������ ��Ŷ�� �����ڿ��� ������
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
	//�÷��̾ ���� ����Ʈ�� �����Ŀ�
	g_PlayerList.push_back(player);

	// ��� ������� �÷��̾� ������ ��Ŷ�� ������.
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
	//���۸� ����ڿ� ���ú긦 �ϰ�
	char buffer[16]{ 0 };
	int recvRet = recv(target->clientSocket, buffer, 16, 0);
	//������ �ִ��� �켱üũ (select���� ���µ� ���ٰ� ������ �ϴ� üũ)
	if (recvRet == SOCKET_ERROR)
	{
		//�����ڵ� Ȯ�ε� WSAEWOULDBLOCK�� �ƴ϶��
		int err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK)
		{
			//��¥ ������
			g_RemovePlayerList.emplace(target);
			return -1;
		}
		//������ ���ٴ� �ǹ�.
		//���� WOULDBLOCK�̶�� �Ѵٸ� ������ ���ٴ� �ǹ�
		return 0;
	}
	// ������� ������ �ϴ� ������ �ִٴ� �ǹ�.
	Header* header = (Header*)buffer;
	//����� ���� � �޽������� �а�
	switch (header->type)
	{
	case MESSAGE_MOVE:
	{
		//�� �޽����� ĳ���� �ؼ� ���
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
	//rSet�ʱ�ȭ �� �������� ���� set
	fd_set rSet;
	FD_ZERO(&rSet);
	FD_SET(g_ListenSocket, &rSet);

	//�״��� �÷��̾� ����Ʈ�� ���鼭 ���δ� set
	for (auto iter = g_PlayerList.begin(); iter != g_PlayerList.end(); ++iter)
		FD_SET((*iter)->clientSocket, &rSet);

	//select�� timeval�� 0���� �����ϰ� select����.
	TIMEVAL time;
	time.tv_usec = 0;
	time.tv_sec = 0;
	int selectRet = select(0, &rSet, nullptr, nullptr, &time);

	//listen socket�� set�� ���������� accept�� �ؾߵ�.
	if (FD_ISSET(g_ListenSocket, &rSet))
	{
		AcceptProc();
	}

	//������ �ϳ��� ���鼭 set�� �Ǿ��ִ��� Ȯ���� set�Ǿ������� recv�� �ؾߵ�.
	for (auto iter = g_PlayerList.begin(); iter != g_PlayerList.end(); ++iter)
	{
		if (FD_ISSET((*iter)->clientSocket, &rSet))
		{
			RecvProc(*iter);
		}
	}
	//������ ���� ��� �͵��� ��Ƽ� ���⼭ ó��.
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
	addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);// �ص����ص��׸�... ������ 0�� ����� 0

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