#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"Winmm.lib")

#define MMO_TCP_FIGHTER_PORT 11350
#define MAX_TOLERANCE_DISTANCE 50

#define MAX_MAP_HEIGHT 6400
#define MAX_MAP_WIDTH 6400
#define MIN_MAP_HEIGHT 0
#define MIN_MAP_WIDTH 0

#define VERTICAL_MOVE_SPEED 4
#define HORIZONTAL_MOVE_SPEED 6

constexpr int ATTACK1_DAMAGE = 1;
constexpr int ATTACK2_DAMAGE = 2;
constexpr int ATTACK3_DAMAGE = 3;


constexpr int ATTACK1_X_DISTANCE = 70;
constexpr int ATTACK1_Y_DISTANCE = 30;
constexpr int ATTACK2_X_DISTANCE = 80;
constexpr int ATTACK2_Y_DISTANCE = 30;
constexpr int ATTACK3_X_DISTANCE = 80;
constexpr int ATTACK3_Y_DISTANCE = 40;



#include "PacketFunction.h"
#include "Session.h"
#include "Player.h"
#include "profiler.h"
#include "SerializingBuffer.h"
#include <iostream>
#include <conio.h>
#include <list>
#include <set>

// 01.09[SUN]
// 큰 틀잡기.
// 세션, 플레이어, 섹터 구현
// 패킷생성 함수
// 01.10[MON]
// 패킷 처리함수 만들기
// 업데이트 로직 구현
// ~띵가띵가
// 01.14[FRI]
// Sector 체크 및 BroadcastNearBy함수 구현
// 01.15[SAT]
// frame말고 deltaTime으로 바꿔보기
// 01.16[SUN]
// sync packet 안보내려고 노력중...
// 
//





/// global variable
bool g_ShutDownFlag = false;
bool g_DummyMode = true;
bool serverLock = true;
bool g_SimpleStatusShoing = true;
std::set<univ_dev::Session*> g_SendOnSessionList;
std::set<univ_dev::Session*> g_DisconnectSession;

std::set<univ_dev::Player*> g_Sector[univ_dev::SECTOR_MAX_Y][univ_dev::SECTOR_MAX_X];


SOCKET g_ListenSocket = INVALID_SOCKET;

int g_RecvPerSec = 0;
int g_SendPerSec = 0;
int g_SyncPacketPerSec = 0;
int g_NetworkIOLoop = 0;
unsigned long long g_TotalSyncPacketSend = 0;
int g_SyncMoveStart = 0;
int g_SyncMoveStop = 0;
int g_SyncAttack1 = 0;
int g_SyncAttack2 = 0;
int g_SyncAttack3 = 0;


SOCKET NetworkInitial();
bool LoadData();

void NetworkIOProcess();
void Update();
void ControlServer();
void MonitorServerStatus();

void LogSyncInfo(const char* functionName, int functionLine, univ_dev::Player* player, short x, short y);
void LogDisconnectInfo(const char* functionName, int functionLine, univ_dev::Session* session, const char* reason);
void LogCriticalError(const char* functionName, int functionLine, const char* reason);


void PlayerUpdate(DWORD& deltaTime);
void SectorUpdate(univ_dev::Player* player);


void AcceptProc();
void ReadProc(univ_dev::Session* session);
void SendProc();
void SendProc(univ_dev::Session* session);
void DisconnectProc(univ_dev::Session* session);
void Disconnect();
void PushSendList(univ_dev::Session* session);
void SendUnicast(univ_dev::Session* targetSession, univ_dev::Packet& packet);
void SendBroadCast(univ_dev::Session* exSession, univ_dev::Packet& packet);
void SendBroadcastNearby(univ_dev::Session* session, univ_dev::Session* exSession, univ_dev::Packet& packet);
void SendInOneSector(int xIdx, int yIdx, univ_dev::Packet& packet, univ_dev::Session* exSession);
void RemoveSector(univ_dev::Player* player);
void AddSector(univ_dev::Player* player);



void NetworkCleanup();


void PacketProc(PacketHeader header, univ_dev::Session* session, univ_dev::Packet& packet);
void PacketProcMoveStart(PacketHeader header, univ_dev::Session* session, univ_dev::Packet& packet);
void PacketProcMoveStop(PacketHeader header, univ_dev::Session* session, univ_dev::Packet& packet);
void PacketProcAttack1(PacketHeader header, univ_dev::Session* session, univ_dev::Packet& packet);
void PacketProcAttack2(PacketHeader header, univ_dev::Session* session, univ_dev::Packet& packet);
void PacketProcAttack3(PacketHeader header, univ_dev::Session* session, univ_dev::Packet& packet);
void PacketProcEcho(PacketHeader header, univ_dev::Session* session, univ_dev::Packet& packet);

void GetNearBySector(univ_dev::Player* player, univ_dev::SectorAround& oldSectorAround, univ_dev::SectorAround& curSectorAround);

void LogCriticalError(const char* functionName, int functionLine, const char* reason)
{
	time_t unixTime = time(nullptr);
	tm date;
	localtime_s(&date, &unixTime);
	char fileName[100]{ 0 };
	char tempBuffer[20]{ 0 };
	_itoa_s(date.tm_year + 1900, tempBuffer, 10);
	strcat_s(fileName, tempBuffer);
	_itoa_s(date.tm_mon + 1, tempBuffer, 10);
	strcat_s(fileName, tempBuffer);
	_itoa_s(date.tm_mday, tempBuffer, 10);
	strcat_s(fileName, tempBuffer);
	strcat_s(fileName, "_log.txt");
	FILE* file = nullptr;
	while (file == nullptr)
		fopen_s(&file, fileName, "ab");
	fprintf(file, "Critical Error : %s::%d_%s\n", functionName, functionLine, reason);
	printf("Critical Error : %s::%d_%s\n", functionName, functionLine, reason);
	fclose(file);
}
void LogSyncInfo(const char* functionName, int functionLine, univ_dev::Player* player, short x, short y)
{
	time_t unixTime = time(nullptr);
	tm date;
	localtime_s(&date, &unixTime);
	char fileName[100]{ 0 };
	char tempBuffer[20]{ 0 };
	_itoa_s(date.tm_year + 1900, tempBuffer, 10);
	strcat_s(fileName, tempBuffer);
	_itoa_s(date.tm_mon + 1, tempBuffer, 10);
	strcat_s(fileName, tempBuffer);
	_itoa_s(date.tm_mday, tempBuffer, 10);
	strcat_s(fileName, tempBuffer);
	strcat_s(fileName, "_log.txt");
	FILE* file = nullptr;
	while (file == nullptr)
		fopen_s(&file, fileName, "ab");
	fprintf(file, "Sync Send SessionID : %d\t Server Pos : X %d, Y %d\t Client Pos : X %d, Y %d\n", player->sessionID, player->xPos, player->yPos, x, y);
	printf("Sync Send SessionID : %d\t Server Pos : X %d, Y %d\t Client Pos : X %d, Y %d\n", player->sessionID, player->xPos, player->yPos, x, y);
	fclose(file);
}
void LogDisconnectInfo(const char* functionName, int functionLine, univ_dev::Session* session, const char* reason)
{
	time_t unixTime = time(nullptr);
	tm date;
	localtime_s(&date, &unixTime);
	char fileName[100]{ 0 };
	char tempBuffer[20]{ 0 };
	_itoa_s(date.tm_year + 1900, tempBuffer, 10);
	strcat_s(fileName, tempBuffer);
	_itoa_s(date.tm_mon + 1, tempBuffer, 10);
	strcat_s(fileName, tempBuffer);
	_itoa_s(date.tm_mday, tempBuffer, 10);
	strcat_s(fileName, tempBuffer);
	strcat_s(fileName, "_log.txt");
	FILE* file = nullptr;
	while (file == nullptr)
		fopen_s(&file, fileName, "ab");
	fprintf(file, "Disconnect SessionID : %d\n%s::%d_%s\n", session->sessionID, functionName, functionLine, reason);
	printf("Disconnect SessionID : %d\n%s::%d_%s\n", session->sessionID, functionName, functionLine, reason);
	fclose(file);
}


void SendUnicast(univ_dev::Session* targetSession, univ_dev::Packet& packet)
{
	if (targetSession == nullptr)
	{
		LogCriticalError(__FUNCTION__, __LINE__, "targetSession is nullptr");
		int* ptr = nullptr;
		*ptr = 100;
		return;
	}

	//패킷의 버퍼사이즈보다 SendRingBuffer의 사이즈가 적다는건... 오래전부터 로직수행 불가능이었다는 의미임
	//왜냐면 링버퍼 이전에 내송신버퍼 그리고 상대측의 수신버퍼가 가득찼다는 의미기 때문이다.
	//내가 만약 라이브러리를 구축하는 입장이라면 SendRingBuffer의 사이즈를 늘려줬겠지만
	//서버입장에서 이유저 한명때문에 다른 유저들의 쾌적한 플레이를 망칠수도있다면...
	//그건 그냥 걸러내는게 맞음

	if (targetSession->SQ.GetFreeSize() < packet.GetBufferSize())
	{
		LogDisconnectInfo(__FUNCTION__, __LINE__, targetSession, "Ringbuffer's FreeSize is smaller than Packet's useSize");
		printf("SQ.FreeSize:%d\tpacket.BufferSize:%d\n", targetSession->SQ.GetFreeSize(), packet.GetBufferSize());
		DisconnectProc(targetSession);
		return;
	}
	targetSession->SQ.Enqueue(packet.GetReadPtr(), packet.GetBufferSize());
	g_SendPerSec++;
	PushSendList(targetSession);
}
void SendBroadCast(univ_dev::Session* exSession, univ_dev::Packet& packet)
{
	univ_dev::Session* currentSession;
	auto iter = univ_dev::g_SessionMap.begin();
	for (; iter != univ_dev::g_SessionMap.end(); ++iter)
	{
		currentSession = iter->second;
		if (currentSession == exSession) continue;
		SendUnicast(currentSession, packet);
	}
}
void SendBroadcastNearby(univ_dev::Session* session, univ_dev::Session* exSession, univ_dev::Packet& packet)
{
	//최대 섹터9개를 돌면서 모든 유저들에게 보내야됨
	auto playerIter = univ_dev::g_PlayerMap.find(session->sessionID);
	if (playerIter == univ_dev::g_PlayerMap.end())
	{
		LogCriticalError(__FUNCTION__, __LINE__, "playerIter is g_PlayerMap.end");
		int* ptr = nullptr;
		*ptr = 100;
		return;
	}
	univ_dev::Player* player = playerIter->second;

	//보내야 하는 섹터는 (curx - 1 ~ curx +1)(cury - 1 ~ cury + 1)까지 보내야됨
	int xIdx = player->curSector.x - 1;
	int yIdx = player->curSector.y - 1;

	for (int i = 0; i < 3; i++)
	{
		xIdx = player->curSector.x - 1;
		if (yIdx < 0 || yIdx >= univ_dev::SECTOR_MAX_Y)
		{
			yIdx++;
			continue;
		}
		for (int j = 0; j < 3; j++)
		{
			//인덱스가 -1이거나 최대값이면 안되니까 그부분은 뛰어넘고
			if (xIdx < 0 || xIdx >= univ_dev::SECTOR_MAX_X)
			{
				xIdx++;
				continue;
			}
			//섹터 -1 +1 부분 전부다 돌면서 SendUnicast해줘야됨
			for (auto iter = g_Sector[yIdx][xIdx].begin(); iter != g_Sector[yIdx][xIdx].end(); ++iter)
			{
				univ_dev::Player* currentPlayer = *iter;
				if (exSession != nullptr && currentPlayer->sessionID == exSession->sessionID) continue;
				SendUnicast(currentPlayer->pSession, packet);
			}
			xIdx++;
		}
		yIdx++;
	}
	//SendBroadCast(exSession, packet);
}
void SendInOneSector(int xIdx, int yIdx, univ_dev::Packet& packet, univ_dev::Session* exSession)
{
	auto iter = g_Sector[yIdx][xIdx].begin();
	for (; iter != g_Sector[yIdx][xIdx].end(); ++iter)
	{
		univ_dev::Player* currentPlayer = *iter;
		if (exSession == currentPlayer->pSession) continue;
		SendUnicast(currentPlayer->pSession, packet);
	}
}
void PushSendList(univ_dev::Session* session)
{
	if (session == nullptr)
	{
		LogCriticalError(__FUNCTION__, __LINE__, "session is nullptr");
		int* ptr = nullptr;
		*ptr = 100;
	}
	g_SendOnSessionList.emplace(session);
}



int main()
{

	timeBeginPeriod(1);
	if (!LoadData())
	{
		LogCriticalError(__FUNCTION__, __LINE__, "LoadData failed");
		return -1;
	}
	g_ListenSocket = NetworkInitial();
	if (g_ListenSocket == INVALID_SOCKET)
	{
		LogCriticalError(__FUNCTION__, __LINE__, "NetworkInitial Failed and g_ListenSocket is INVALID_SOCKET");
		return -2;
	}
	srand(time(nullptr));
	while (!g_ShutDownFlag)
	{
		NetworkIOProcess();

		Update();
		ControlServer();
		MonitorServerStatus();
	}

	NetworkCleanup();

	return 0;
}

void NetworkIOProcess()
{
	g_NetworkIOLoop++;
	fd_set  rSet, wSet;
	clock_t prev = clock();
	clock_t current;
	bool saveFileFlag = true;
	SOCKET currentCheckSession[63];
	int userCount = 0;
	timeval time;
	time.tv_sec = 0;
	time.tv_usec = 0;
	FD_ZERO(&rSet);
	//FD_ZERO(&wSet);
	auto iter = univ_dev::g_SessionMap.begin();
	while (true)
	{
		//univ_dev::Profiler profiler("WHILE_LOOP");
		FD_ZERO(&rSet);
		//FD_ZERO(&wSet);
		//ZeroMemory(currentCheckSession, 63 * sizeof(SOCKET));
		FD_SET(g_ListenSocket, &rSet);
		userCount = 0;
		for (; userCount < 63; userCount++)
		{
			if (iter == univ_dev::g_SessionMap.end()) break;
			FD_SET(iter->second->sock, &rSet);
			//FD_SET(iter->second->sock, &wSet);
			currentCheckSession[userCount] = iter->second->sock;
			++iter;
		}
		int numOfSignaledSocket = select(0, &rSet, nullptr, nullptr, &time);
		if (numOfSignaledSocket == SOCKET_ERROR)
		{
			//printf("error select : %d\n", WSAGetLastError());
			int err = WSAGetLastError();
			if (err != WSAEWOULDBLOCK)
			{
				LogCriticalError(__FUNCTION__, __LINE__, "select, numOfSignaledSocket is SOCKET_ERROR and err is not WSAEWOULDBLOCK");
				int* ptr = nullptr;
				*ptr = 100;
				return;
			}
		}

		if (FD_ISSET(g_ListenSocket, &rSet))
		{
			AcceptProc();
			//printf("Accept Proc()\n");
		}
		for (int i = 0; i < userCount; i++)
		{
			univ_dev::Session* currentSession;
			auto currentSessionIter = univ_dev::g_SessionMap.find(currentCheckSession[i]);
			if (currentSessionIter == univ_dev::g_SessionMap.end())
			{
				//문제가 있음
				LogCriticalError(__FUNCTION__, __LINE__, "currentSessionIter is g_SessionMap.end");
				int* ptr = nullptr;
				*ptr = 100;
				return;
			}
			currentSession = currentSessionIter->second;
			if (FD_ISSET(currentSession->sock, &rSet))
			{
				ReadProc(currentSession);
			}
			//if (FD_ISSET(currentSession->sock, &wSet))
			//	SendProc(currentSession);
		}
		if (iter == univ_dev::g_SessionMap.end())break;
	}
	SendProc();
	Disconnect();
}

void PacketProc(PacketHeader header, univ_dev::Session* session, univ_dev::Packet& packet)
{
	switch (header.packetType)
	{
	case dfPACKET_CS_MOVE_START:
	{
		PacketProcMoveStart(header, session, packet);
		break;
	}
	case dfPACKET_CS_MOVE_STOP:
	{
		PacketProcMoveStop(header, session, packet);
		break;
	}
	case dfPACKET_CS_ATTACK1:
	{
		PacketProcAttack1(header, session, packet);
		break;
	}
	case dfPACKET_CS_ATTACK2:
	{
		PacketProcAttack2(header, session, packet);
		break;
	}
	case dfPACKET_CS_ATTACK3:
	{
		PacketProcAttack3(header, session, packet);
		break;
	}
	case dfPACKET_CS_ECHO:
	{
		PacketProcEcho(header, session, packet);
		break;
	}
	default:
	{
		LogCriticalError(__FUNCTION__, __LINE__, "default case");
		break;
	}
	}
}
void PacketProcEcho(PacketHeader header, univ_dev::Session* session, univ_dev::Packet& packet)
{
	int time;
	packet >> time;

	univ_dev::Packet* echoPacket = univ_dev::g_PacketObjectPool.Alloc();
	echoPacket->Clear();

	univ_dev::MakePacketEcho(*echoPacket, time);
	SendUnicast(session, *echoPacket);

	echoPacket->Clear();
	univ_dev::g_PacketObjectPool.Free(echoPacket);
}

void PacketProcMoveStart(PacketHeader header, univ_dev::Session* session, univ_dev::Packet& packet)
{
	if (session == nullptr)
	{
		LogCriticalError(__FUNCTION__, __LINE__, "session is nullptr");
		int* ptr = nullptr;
		*ptr = 100;
		return;
	}
	if (header.code != 0x89)
	{
		LogDisconnectInfo(__FUNCTION__, __LINE__, session, "header code is not 0x89");
		DisconnectProc(session);
		return;
	}
	if (packet.GetBufferSize() != header.payloadSize)
	{
		LogDisconnectInfo(__FUNCTION__, __LINE__, session, "packet's use size is not same with header's payloadsize");
		DisconnectProc(session);
		return;
	}
	auto currentPlayerIter = univ_dev::g_PlayerMap.find(session->sessionID);
	if (currentPlayerIter == univ_dev::g_PlayerMap.end())
	{
		LogCriticalError(__FUNCTION__, __LINE__, "currentPlayerIter is g_PlayerMap.end");
		int* ptr = nullptr;
		*ptr = 100;
		return;
	}
	univ_dev::Player* currentPlayer = currentPlayerIter->second;

	BYTE moveDirection;
	short x;
	short y;
	packet >> moveDirection >> x >> y;


	if (std::abs((short)currentPlayer->xPos - x) > MAX_TOLERANCE_DISTANCE || std::abs((short)currentPlayer->yPos - y) > MAX_TOLERANCE_DISTANCE)
	{
		univ_dev::Packet* pSyncPacket = univ_dev::g_PacketObjectPool.Alloc();
		pSyncPacket->Clear();

		LogSyncInfo(__FUNCTION__, __LINE__, currentPlayer, x, y);

		x = currentPlayer->xPos;
		y = currentPlayer->yPos;
		univ_dev::MakePacketPositionSync(*pSyncPacket, currentPlayer->sessionID, x, y);
		//printf("Sync\n");
		g_SyncPacketPerSec++;
		g_SyncMoveStart++;
		SendUnicast(session, *pSyncPacket);
		pSyncPacket->Clear();
		univ_dev::g_PacketObjectPool.Free(pSyncPacket);
	}

	currentPlayer->action = moveDirection;
	currentPlayer->moveDirection = moveDirection;
	currentPlayer->xPos = x;
	currentPlayer->yPos = y;
	BYTE lookingDirection;
	switch (moveDirection)
	{
	case dfPACKET_MOVE_DIR_LD:
	case dfPACKET_MOVE_DIR_LL:
	case dfPACKET_MOVE_DIR_LU:
		lookingDirection = dfPACKET_MOVE_DIR_LL;
		break;
	case dfPACKET_MOVE_DIR_RU:
	case dfPACKET_MOVE_DIR_RD:
	case dfPACKET_MOVE_DIR_RR:
		lookingDirection = dfPACKET_MOVE_DIR_RR;
		break;
	default:
		lookingDirection = currentPlayer->direction;
		break;
	}

	currentPlayer->direction = lookingDirection;



	univ_dev::Packet* pMovePacket = univ_dev::g_PacketObjectPool.Alloc();
	pMovePacket->Clear();
	univ_dev::MakePacketPlayerMoveStart(*pMovePacket, currentPlayer->sessionID, moveDirection, currentPlayer->xPos, currentPlayer->yPos);
	SendBroadcastNearby(session, session, *pMovePacket);
	pMovePacket->Clear();
	univ_dev::g_PacketObjectPool.Free(pMovePacket);
}
void PacketProcMoveStop(PacketHeader header, univ_dev::Session* session, univ_dev::Packet& packet)
{
	if (session == nullptr)
	{
		LogCriticalError(__FUNCTION__, __LINE__, "session is nullptr");
		int* ptr = nullptr;
		*ptr = 100;
		return;
	}
	if (header.code != 0x89)
	{
		LogDisconnectInfo(__FUNCTION__, __LINE__, session, "header code is not 0x89");
		DisconnectProc(session);
		return;
	}
	if (packet.GetBufferSize() != header.payloadSize)
	{
		LogDisconnectInfo(__FUNCTION__, __LINE__, session, "packet's use size is not same with header's payloadsize");
		DisconnectProc(session);
		return;
	}
	auto currentPlayerIter = univ_dev::g_PlayerMap.find(session->sessionID);
	if (currentPlayerIter == univ_dev::g_PlayerMap.end())
	{
		LogCriticalError(__FUNCTION__, __LINE__, "currentPlayerIter is g_PlayerMap.end");
		int* ptr = nullptr;
		*ptr = 100;
		return;
	}
	univ_dev::Player* currentPlayer = currentPlayerIter->second;

	BYTE direction;
	short x;
	short y;
	packet >> direction >> x >> y;

	if (std::abs(currentPlayer->xPos - x) > MAX_TOLERANCE_DISTANCE || std::abs(currentPlayer->yPos - y) > MAX_TOLERANCE_DISTANCE)
	{
		univ_dev::Packet* pSyncPacket = univ_dev::g_PacketObjectPool.Alloc();
		pSyncPacket->Clear();

		LogSyncInfo(__FUNCTION__, __LINE__, currentPlayer, x, y);		x = currentPlayer->xPos;

		x = currentPlayer->xPos;
		y = currentPlayer->yPos;
		univ_dev::MakePacketPositionSync(*pSyncPacket, currentPlayer->sessionID, x, y);
		g_SyncPacketPerSec++;
		g_SyncMoveStop++;
		SendUnicast(session, *pSyncPacket);
		pSyncPacket->Clear();
		univ_dev::g_PacketObjectPool.Free(pSyncPacket);
	}

	currentPlayer->direction = direction;
	currentPlayer->action = 255;
	currentPlayer->moveDirection = 255;
	currentPlayer->xPos = x;
	currentPlayer->yPos = y;

	univ_dev::Packet* pMoveStopPacket = univ_dev::g_PacketObjectPool.Alloc();
	pMoveStopPacket->Clear();
	univ_dev::MakePacketPlayerMoveStop(*pMoveStopPacket, currentPlayer->sessionID, currentPlayer->direction, currentPlayer->xPos, currentPlayer->yPos);
	SendBroadcastNearby(session, session, *pMoveStopPacket);
	pMoveStopPacket->Clear();
	univ_dev::g_PacketObjectPool.Free(pMoveStopPacket);
}
void PacketProcAttack1(PacketHeader header, univ_dev::Session* session, univ_dev::Packet& packet)
{
	if (session == nullptr)
	{
		LogCriticalError(__FUNCTION__, __LINE__, "session is nullptr");
		int* ptr = nullptr;
		*ptr = 100;
		return;
	}
	if (header.code != 0x89)
	{
		LogDisconnectInfo(__FUNCTION__, __LINE__, session, "header code is not 0x89");
		DisconnectProc(session);
		return;
	}
	if (packet.GetBufferSize() != header.payloadSize)
	{
		LogDisconnectInfo(__FUNCTION__, __LINE__, session, "packet's use size is not same with header's payloadsize");
		DisconnectProc(session);
		return;
	}
	auto currentPlayerIter = univ_dev::g_PlayerMap.find(session->sessionID);
	if (currentPlayerIter == univ_dev::g_PlayerMap.end())
	{
		LogCriticalError(__FUNCTION__, __LINE__, "currentPlayerIter is g_PlayerMap.end");
		DisconnectProc(session);
		return;
	}

	BYTE direction;
	short x;
	short y;

	packet >> direction >> x >> y;

	univ_dev::Player* currentPlayer = currentPlayerIter->second;

	if (std::abs(currentPlayer->xPos - x) > MAX_TOLERANCE_DISTANCE || std::abs(currentPlayer->yPos - y) > MAX_TOLERANCE_DISTANCE)
	{
		univ_dev::Packet* pSyncPacket = univ_dev::g_PacketObjectPool.Alloc();
		pSyncPacket->Clear();

		LogSyncInfo(__FUNCTION__, __LINE__, currentPlayer, x, y);

		x = currentPlayer->xPos;
		y = currentPlayer->yPos;
		univ_dev::MakePacketPositionSync(*pSyncPacket, currentPlayer->sessionID, x, y);
		//printf("Sync\n");
		g_SyncPacketPerSec++;
		g_SyncAttack1++;
		SendUnicast(session, *pSyncPacket);
		//SendBroadcastNearby(session,nullptr, *pSyncPacket);
		pSyncPacket->Clear();
		univ_dev::g_PacketObjectPool.Free(pSyncPacket);
	}

	currentPlayer->direction = direction;
	currentPlayer->moveDirection = 255;
	currentPlayer->action = 255;
	currentPlayer->xPos = x;
	currentPlayer->yPos = y;

	univ_dev::Packet* pAttack1Packet = univ_dev::g_PacketObjectPool.Alloc();
	pAttack1Packet->Clear();
	univ_dev::MakePacketPlayerAttack1(*pAttack1Packet, currentPlayer->sessionID, currentPlayer->direction, currentPlayer->xPos, currentPlayer->yPos);
	SendBroadcastNearby(session, session, *pAttack1Packet);

	int xIdx = currentPlayer->curSector.x - 1;
	int yIdx = currentPlayer->curSector.y - 1;
	for (int i = 0; i < 3; i++)
	{
		xIdx = currentPlayer->curSector.x - 1;
		if (yIdx < 0 || yIdx >= univ_dev::SECTOR_MAX_Y)
		{
			yIdx++;
			continue;
		}
		for (int j = 0; j < 3; j++)
		{
			if (xIdx < 0 || xIdx >= univ_dev::SECTOR_MAX_X)
			{
				xIdx++;
				continue;
			}
			for (auto iter = g_Sector[yIdx][xIdx].begin();
				iter != g_Sector[yIdx][xIdx].end(); ++iter)
			{
				univ_dev::Player* hitPlayer = *iter;
				if (hitPlayer == currentPlayer) continue;
				if (currentPlayer->direction == dfPACKET_MOVE_DIR_LL)
				{
					if (currentPlayer->xPos < hitPlayer->xPos) continue;
				}
				else
				{
					if (currentPlayer->xPos > hitPlayer->xPos) continue;
				}
				if (std::abs(hitPlayer->xPos - currentPlayer->xPos) > ATTACK1_X_DISTANCE) continue;
				if (std::abs(hitPlayer->yPos - currentPlayer->yPos) > ATTACK1_Y_DISTANCE) continue;

				univ_dev::Packet* hitPacket = univ_dev::g_PacketObjectPool.Alloc();
				hitPacket->Clear();
				hitPlayer->HP -= ATTACK1_DAMAGE;
				if (hitPlayer->HP < 0)
					hitPlayer->HP = 0;
				univ_dev::MakePacketPlayerHitDamage(*hitPacket, currentPlayer->sessionID, hitPlayer->sessionID, hitPlayer->HP);
				SendBroadcastNearby(hitPlayer->pSession, nullptr, *hitPacket);
				hitPacket->Clear();
				univ_dev::g_PacketObjectPool.Free(hitPacket);
			}
			xIdx++;
		}
		yIdx++;
	}
	pAttack1Packet->Clear();
	univ_dev::g_PacketObjectPool.Free(pAttack1Packet);
}
void PacketProcAttack2(PacketHeader header, univ_dev::Session* session, univ_dev::Packet& packet)
{
	if (session == nullptr)
	{
		LogCriticalError(__FUNCTION__, __LINE__, "session is nullptr");
		int* ptr = nullptr;
		*ptr = 100;
		return;
	}
	if (header.code != 0x89)
	{
		LogDisconnectInfo(__FUNCTION__, __LINE__, session, "header code is not 0x89");
		DisconnectProc(session);
		return;
	}
	if (packet.GetBufferSize() != header.payloadSize)
	{
		LogDisconnectInfo(__FUNCTION__, __LINE__, session, "packet's use size is not same with header's payloadsize");
		DisconnectProc(session);
		return;
	}
	auto currentPlayerIter = univ_dev::g_PlayerMap.find(session->sessionID);
	if (currentPlayerIter == univ_dev::g_PlayerMap.end())
	{
		LogCriticalError(__FUNCTION__, __LINE__, "currentPlayerIter is g_PlayerMap.end");
		DisconnectProc(session);
		return;
	}

	BYTE direction;
	short x;
	short y;

	packet >> direction >> x >> y;

	univ_dev::Player* currentPlayer = currentPlayerIter->second;

	if (std::abs(currentPlayer->xPos - x) > MAX_TOLERANCE_DISTANCE || std::abs(currentPlayer->yPos - y) > MAX_TOLERANCE_DISTANCE)
	{
		univ_dev::Packet* pSyncPacket = univ_dev::g_PacketObjectPool.Alloc();
		pSyncPacket->Clear();

		LogSyncInfo(__FUNCTION__, __LINE__, currentPlayer, x, y);

		x = currentPlayer->xPos;
		y = currentPlayer->yPos;
		univ_dev::MakePacketPositionSync(*pSyncPacket, currentPlayer->sessionID, x, y);
		//printf("Sync\n");
		g_SyncPacketPerSec++;
		g_SyncAttack2++;
		SendUnicast(session, *pSyncPacket);
		//SendBroadcastNearby(session,nullptr , *pSyncPacket);
		pSyncPacket->Clear();
		univ_dev::g_PacketObjectPool.Free(pSyncPacket);
	}

	currentPlayer->direction = direction;
	currentPlayer->moveDirection = 255;
	currentPlayer->action = 255;
	currentPlayer->xPos = x;
	currentPlayer->yPos = y;

	univ_dev::Packet* pAttack2Packet = univ_dev::g_PacketObjectPool.Alloc();
	pAttack2Packet->Clear();
	univ_dev::MakePacketPlayerAttack2(*pAttack2Packet, currentPlayer->sessionID, currentPlayer->direction, currentPlayer->xPos, currentPlayer->yPos);
	SendBroadcastNearby(session, session, *pAttack2Packet);
	int xIdx = currentPlayer->curSector.x - 1;
	int yIdx = currentPlayer->curSector.y - 1;
	for (int i = 0; i < 3; i++)
	{
		xIdx = currentPlayer->curSector.x - 1;
		if (yIdx < 0 || yIdx >= univ_dev::SECTOR_MAX_Y)
		{
			yIdx++;
			continue;
		}
		for (int j = 0; j < 3; j++)
		{
			if (xIdx < 0 || xIdx >= univ_dev::SECTOR_MAX_X)
			{
				xIdx++;
				continue;
			}
			for (auto iter = g_Sector[yIdx][xIdx].begin();
				iter != g_Sector[yIdx][xIdx].end(); ++iter)
			{
				univ_dev::Player* hitPlayer = *iter;
				if (hitPlayer == currentPlayer) continue;
				if (currentPlayer->direction == dfPACKET_MOVE_DIR_LL)
				{
					if (currentPlayer->xPos < hitPlayer->xPos) continue;
				}
				else
				{
					if (currentPlayer->xPos > hitPlayer->xPos) continue;
				}
				if (std::abs(hitPlayer->xPos - currentPlayer->xPos) > ATTACK2_X_DISTANCE) continue;
				if (std::abs(hitPlayer->yPos - currentPlayer->yPos) > ATTACK2_Y_DISTANCE) continue;

				univ_dev::Packet* hitPacket = univ_dev::g_PacketObjectPool.Alloc();
				hitPacket->Clear();
				hitPlayer->HP -= ATTACK2_DAMAGE;
				if (hitPlayer->HP < 0)
					hitPlayer->HP = 0;
				univ_dev::MakePacketPlayerHitDamage(*hitPacket, currentPlayer->sessionID, hitPlayer->sessionID, hitPlayer->HP);
				SendBroadcastNearby(hitPlayer->pSession, nullptr, *hitPacket);
				hitPacket->Clear();
				univ_dev::g_PacketObjectPool.Free(hitPacket);
			}
			xIdx++;
		}
		yIdx++;
	}

	pAttack2Packet->Clear();
	univ_dev::g_PacketObjectPool.Free(pAttack2Packet);
}
void PacketProcAttack3(PacketHeader header, univ_dev::Session* session, univ_dev::Packet& packet)
{
	if (session == nullptr)
	{
		LogCriticalError(__FUNCTION__, __LINE__, "session is nullptr");
		int* ptr = nullptr;
		*ptr = 100;
		return;
	}
	if (header.code != 0x89)
	{
		LogDisconnectInfo(__FUNCTION__, __LINE__, session, "header code is not 0x89");
		DisconnectProc(session);
		return;
	}
	if (packet.GetBufferSize() != header.payloadSize)
	{
		LogDisconnectInfo(__FUNCTION__, __LINE__, session, "packet's use size is not same with header's payloadsize");
		DisconnectProc(session);
		return;
	}
	auto currentPlayerIter = univ_dev::g_PlayerMap.find(session->sessionID);
	if (currentPlayerIter == univ_dev::g_PlayerMap.end())
	{
		LogCriticalError(__FUNCTION__, __LINE__, "currentPlayerIter is g_PlayerMap.end");
		DisconnectProc(session);
		return;
	}

	BYTE direction;
	short x;
	short y;

	packet >> direction >> x >> y;

	univ_dev::Player* currentPlayer = currentPlayerIter->second;

	if (std::abs(currentPlayer->xPos - x) > MAX_TOLERANCE_DISTANCE || std::abs(currentPlayer->yPos - y) > MAX_TOLERANCE_DISTANCE)
	{
		univ_dev::Packet* pSyncPacket = univ_dev::g_PacketObjectPool.Alloc();
		pSyncPacket->Clear();

		LogSyncInfo(__FUNCTION__, __LINE__, currentPlayer, x, y);

		x = currentPlayer->xPos;
		y = currentPlayer->yPos;
		univ_dev::MakePacketPositionSync(*pSyncPacket, currentPlayer->sessionID, x, y);
		g_SyncPacketPerSec++;
		g_SyncAttack3++;
		SendUnicast(session, *pSyncPacket);
		pSyncPacket->Clear();
		univ_dev::g_PacketObjectPool.Free(pSyncPacket);
	}

	currentPlayer->direction = direction;
	currentPlayer->moveDirection = 255;
	currentPlayer->action = 255;
	currentPlayer->xPos = x;
	currentPlayer->yPos = y;

	univ_dev::Packet* pAttack3Packet = univ_dev::g_PacketObjectPool.Alloc();
	pAttack3Packet->Clear();
	univ_dev::MakePacketPlayerAttack3(*pAttack3Packet, currentPlayer->sessionID, currentPlayer->direction, currentPlayer->xPos, currentPlayer->yPos);
	SendBroadcastNearby(session, session, *pAttack3Packet);
	// 여기까지 공격 해라는 판정이고 맞았으면 맞았다에대한 판정도 해줘야됨
	int xIdx = currentPlayer->curSector.x - 1;
	int yIdx = currentPlayer->curSector.y - 1;
	for (int i = 0; i < 3; i++)
	{
		xIdx = currentPlayer->curSector.x - 1;
		if (yIdx < 0 || yIdx >= univ_dev::SECTOR_MAX_Y)
		{
			yIdx++;
			continue;
		}
		for (int j = 0; j < 3; j++)
		{
			if (xIdx < 0 || xIdx >= univ_dev::SECTOR_MAX_X)
			{
				xIdx++;
				continue;
			}
			for (auto iter = g_Sector[yIdx][xIdx].begin();
				iter != g_Sector[yIdx][xIdx].end(); ++iter)
			{
				univ_dev::Player* hitPlayer = *iter;
				if (hitPlayer == currentPlayer) continue;
				if (currentPlayer->direction == dfPACKET_MOVE_DIR_LL)
				{
					if (currentPlayer->xPos < hitPlayer->xPos) continue;
				}
				else
				{
					if (currentPlayer->xPos > hitPlayer->xPos) continue;
				}
				if (std::abs(hitPlayer->xPos - currentPlayer->xPos) > ATTACK3_X_DISTANCE) continue;
				if (std::abs(hitPlayer->yPos - currentPlayer->yPos) > ATTACK3_Y_DISTANCE) continue;

				univ_dev::Packet* hitPacket = univ_dev::g_PacketObjectPool.Alloc();
				hitPacket->Clear();
				hitPlayer->HP -= ATTACK3_DAMAGE;
				if (hitPlayer->HP < 0)
					hitPlayer->HP = 0;
				univ_dev::MakePacketPlayerHitDamage(*hitPacket, currentPlayer->sessionID, hitPlayer->sessionID, hitPlayer->HP);
				SendBroadcastNearby(hitPlayer->pSession, nullptr, *hitPacket);
				hitPacket->Clear();
				univ_dev::g_PacketObjectPool.Free(hitPacket);
			}
			xIdx++;
		}
		yIdx++;
	}

	pAttack3Packet->Clear();
	univ_dev::g_PacketObjectPool.Free(pAttack3Packet);
}


DWORD old = timeGetTime();
DWORD cur;
DWORD sec = timeGetTime();
DWORD deltaTime = 0;
int update;
void Update()
{
	cur = timeGetTime();
	deltaTime += cur - old;
	old = cur;

	if (deltaTime < 40) return;
	update++;
	PlayerUpdate(deltaTime);
}

bool MoveCheck(short xPos, short yPos)
{
	if (xPos > MAX_MAP_WIDTH) return false;
	else if (xPos < MIN_MAP_WIDTH) return false;
	else if (yPos > MAX_MAP_HEIGHT) return false;
	else if (yPos < MIN_MAP_WIDTH) return false;

	return true;
}

void PlayerUpdate(DWORD& deltaTime)
{
	DWORD currentTime = timeGetTime();
	while (deltaTime >= 40)
	{
		deltaTime -= 40;
		for (auto iter = univ_dev::g_PlayerMap.begin(); iter != univ_dev::g_PlayerMap.end(); ++iter)
		{
			univ_dev::Player* currentPlayer = iter->second;
			if (!g_DummyMode)
			{
				if (currentPlayer->HP <= 0)
				{
					LogDisconnectInfo(__FUNCTION__, __LINE__, currentPlayer->pSession, "currentPlayer's HP is zero");
					DisconnectProc(currentPlayer->pSession);
					continue;
				}
			}
			if (currentTime - currentPlayer->pSession->lastRecvTime > univ_dev::RECV_TIMEOUT)
			{
				LogDisconnectInfo(__FUNCTION__, __LINE__, currentPlayer->pSession, "RECV_TIMEOUT");
				DisconnectProc(currentPlayer->pSession);
				continue;
			}
			switch (currentPlayer->moveDirection)
			{
			case dfPACKET_MOVE_DIR_LL:
				if (MoveCheck(currentPlayer->xPos - HORIZONTAL_MOVE_SPEED, currentPlayer->yPos))
				{
					currentPlayer->xPos -= HORIZONTAL_MOVE_SPEED;
				}
				break;
			case dfPACKET_MOVE_DIR_LU:
				if (MoveCheck(currentPlayer->xPos - HORIZONTAL_MOVE_SPEED, currentPlayer->yPos - VERTICAL_MOVE_SPEED))
				{
					currentPlayer->xPos -= HORIZONTAL_MOVE_SPEED;
					currentPlayer->yPos -= VERTICAL_MOVE_SPEED;
				}
				break;
			case dfPACKET_MOVE_DIR_UU:
				if (MoveCheck((int)currentPlayer->xPos, currentPlayer->yPos - VERTICAL_MOVE_SPEED))
				{
					currentPlayer->yPos -= VERTICAL_MOVE_SPEED;
				}
				break;
			case dfPACKET_MOVE_DIR_RU:
				if (MoveCheck(currentPlayer->xPos + HORIZONTAL_MOVE_SPEED, currentPlayer->yPos - VERTICAL_MOVE_SPEED))
				{
					currentPlayer->xPos += HORIZONTAL_MOVE_SPEED;
					currentPlayer->yPos -= VERTICAL_MOVE_SPEED;
				}
				break;
			case dfPACKET_MOVE_DIR_RR:
				if (MoveCheck(currentPlayer->xPos + HORIZONTAL_MOVE_SPEED, currentPlayer->yPos))
				{
					currentPlayer->xPos += HORIZONTAL_MOVE_SPEED;
				}
				break;
			case dfPACKET_MOVE_DIR_RD:
				if (MoveCheck(currentPlayer->xPos + HORIZONTAL_MOVE_SPEED, currentPlayer->yPos + VERTICAL_MOVE_SPEED))
				{
					currentPlayer->xPos += HORIZONTAL_MOVE_SPEED;
					currentPlayer->yPos += VERTICAL_MOVE_SPEED;
				}
				break;
			case dfPACKET_MOVE_DIR_DD:
				if (MoveCheck(currentPlayer->xPos, currentPlayer->yPos + VERTICAL_MOVE_SPEED))
				{
					currentPlayer->yPos += VERTICAL_MOVE_SPEED;
				}
				break;
			case dfPACKET_MOVE_DIR_LD:
				if (MoveCheck(currentPlayer->xPos - HORIZONTAL_MOVE_SPEED, currentPlayer->yPos + VERTICAL_MOVE_SPEED))
				{
					currentPlayer->xPos -= HORIZONTAL_MOVE_SPEED;
					currentPlayer->yPos += VERTICAL_MOVE_SPEED;
				}
				break;
			default:
				continue;
			}
			SectorUpdate(currentPlayer);
		}
	}
}

void GetUpdatedSectorAround(univ_dev::Player* player, univ_dev::SectorAround& removeSector, univ_dev::SectorAround& addSector)
{
	univ_dev::SectorAround oldSectorAround;
	univ_dev::SectorAround curSectorAround;
	oldSectorAround.sectorCount = 0;
	curSectorAround.sectorCount = 0;
	GetNearBySector(player, oldSectorAround, curSectorAround);

	bool find = false;
	for (int i = 0; i < oldSectorAround.sectorCount; i++)
	{
		find = false;
		for (int j = 0; j < curSectorAround.sectorCount; j++)
		{
			if (oldSectorAround.aroundSector[i].x == curSectorAround.aroundSector[j].x &&
				oldSectorAround.aroundSector[i].y == curSectorAround.aroundSector[j].y)
			{
				find = true;
				break;
			}
		}
		if (find == false)
			removeSector.aroundSector[removeSector.sectorCount++] = oldSectorAround.aroundSector[i];
	}
	for (int i = 0; i < curSectorAround.sectorCount; i++)
	{
		find = false;
		for (int j = 0; j < oldSectorAround.sectorCount; j++)
		{
			if (oldSectorAround.aroundSector[j].x == curSectorAround.aroundSector[i].x &&
				oldSectorAround.aroundSector[j].y == curSectorAround.aroundSector[i].y)
			{
				find = true;
				break;
			}
		}
		if (find == false)
			addSector.aroundSector[addSector.sectorCount++] = curSectorAround.aroundSector[i];
	}
}

void SectorUpdate(univ_dev::Player* player)
{
	bool sectorChanged = false;

	//old sector 백업
	int oldX = player->oldSector.x;
	int oldY = player->oldSector.y;
	player->oldSector.x = player->curSector.x;
	player->oldSector.y = player->curSector.y;
	player->curSector.x = player->xPos / univ_dev::SECTOR_WIDTH;
	player->curSector.y = player->yPos / univ_dev::SECTOR_WIDTH;
	if (player->curSector.x >= univ_dev::SECTOR_MAX_X)
		player->curSector.x = univ_dev::SECTOR_MAX_X - 1;
	if (player->curSector.y >= univ_dev::SECTOR_MAX_Y)
		player->curSector.y = univ_dev::SECTOR_MAX_Y - 1;
	if (player->curSector.x < 0)
		player->curSector.x = 0;
	if (player->curSector.y < 0)
		player->curSector.y = 0;


	if (player->curSector.x != player->oldSector.x || player->curSector.y != player->oldSector.y)
		sectorChanged = true;

	if (sectorChanged)
	{
		RemoveSector(player);
		AddSector(player);
		//old around sector를 가져옴
		//cur around sector를 가져옴

		// 여기서 넣어야 되는 섹터 주변에 다 넣고
		// 빼야되는 섹터 주변에 다 빼야됨.

		univ_dev::SectorAround removeSectorAround;
		univ_dev::SectorAround addSectorAround;
		removeSectorAround.sectorCount = addSectorAround.sectorCount = 0;
		GetUpdatedSectorAround(player, removeSectorAround, addSectorAround);

		//old around sector.count + cur around sector.count번 반복
		//동일한거면 하지 않고 지나감
		//다른거면 remove->around에다가 넣음 old sector around[y][x]를 집어넣음
				//old around sector.count + cur around sector.count번 반복
		univ_dev::Packet* pRemovePacket = univ_dev::g_PacketObjectPool.Alloc();
		univ_dev::Packet* pCreatePacket = univ_dev::g_PacketObjectPool.Alloc();
		univ_dev::Packet* pRemoveOtherPlayerPacket = univ_dev::g_PacketObjectPool.Alloc();
		univ_dev::Packet* pCreateOtherPlayerPacket = univ_dev::g_PacketObjectPool.Alloc();
		univ_dev::Packet* pMovePacket = univ_dev::g_PacketObjectPool.Alloc();
		pRemovePacket->Clear();
		pCreatePacket->Clear();
		pRemoveOtherPlayerPacket->Clear();
		pCreateOtherPlayerPacket->Clear();
		pMovePacket->Clear();

		univ_dev::MakePacketDeletePlayer(*pRemovePacket, player->sessionID);
		univ_dev::MakePacketCreatePlayer(*pCreatePacket, player->sessionID, player->direction, player->xPos, player->yPos, player->HP);

		for (int i = 0; i < removeSectorAround.sectorCount; i++)
		{
			int xIdx = removeSectorAround.aroundSector[i].x;
			int yIdx = removeSectorAround.aroundSector[i].y;
			auto iter = g_Sector[yIdx][xIdx].begin();
			for (; iter != g_Sector[yIdx][xIdx].end(); ++iter)
			{
				univ_dev::Player* tempPlayer = *iter;
				if (tempPlayer->sessionID == player->sessionID)continue;
				univ_dev::MakePacketDeletePlayer(*pRemoveOtherPlayerPacket, tempPlayer->sessionID);
				SendUnicast(player->pSession, *pRemoveOtherPlayerPacket);
			}
			SendInOneSector(xIdx, yIdx, *pRemovePacket, player->pSession);
			pRemoveOtherPlayerPacket->Clear();
		}
		for (int j = 0; j < addSectorAround.sectorCount; j++)
		{
			int xIdx = addSectorAround.aroundSector[j].x;
			int yIdx = addSectorAround.aroundSector[j].y;
			auto iter = g_Sector[yIdx][xIdx].begin();
			for (; iter != g_Sector[yIdx][xIdx].end(); ++iter)
			{
				auto currentPlayer = *iter;
				if (currentPlayer->sessionID == player->sessionID) continue;
				univ_dev::MakePacketCreatePlayer(*pCreateOtherPlayerPacket, currentPlayer->sessionID, currentPlayer->direction, currentPlayer->xPos, currentPlayer->yPos, currentPlayer->HP);

				SendUnicast(player->pSession, *pCreateOtherPlayerPacket);
				SendUnicast(currentPlayer->pSession, *pCreatePacket);
				pCreateOtherPlayerPacket->Clear();
				switch (currentPlayer->action)
				{
				case dfPACKET_MOVE_DIR_LL:
				case dfPACKET_MOVE_DIR_LU:
				case dfPACKET_MOVE_DIR_UU:
				case dfPACKET_MOVE_DIR_RU:
				case dfPACKET_MOVE_DIR_RR:
				case dfPACKET_MOVE_DIR_RD:
				case dfPACKET_MOVE_DIR_DD:
				case dfPACKET_MOVE_DIR_LD:
				{
					univ_dev::MakePacketPlayerMoveStart(*pMovePacket, currentPlayer->sessionID, currentPlayer->action, currentPlayer->xPos, currentPlayer->yPos);
					SendUnicast(player->pSession, *pMovePacket);
					pMovePacket->Clear();
					break;
				}
				}
				switch (player->action)
				{
				case dfPACKET_MOVE_DIR_LL:
				case dfPACKET_MOVE_DIR_LU:
				case dfPACKET_MOVE_DIR_UU:
				case dfPACKET_MOVE_DIR_RU:
				case dfPACKET_MOVE_DIR_RR:
				case dfPACKET_MOVE_DIR_RD:
				case dfPACKET_MOVE_DIR_DD:
				case dfPACKET_MOVE_DIR_LD:
				{
					univ_dev::MakePacketPlayerMoveStart(*pMovePacket, player->sessionID, player->action, player->xPos, player->yPos);
					SendUnicast(currentPlayer->pSession, *pMovePacket);
					pMovePacket->Clear();
					break;
				}
				}
			}
		}
		pRemovePacket->Clear();
		pCreatePacket->Clear();
		pRemoveOtherPlayerPacket->Clear();
		pCreateOtherPlayerPacket->Clear();
		pMovePacket->Clear();
		univ_dev::g_PacketObjectPool.Free(pMovePacket);
		univ_dev::g_PacketObjectPool.Free(pRemovePacket);
		univ_dev::g_PacketObjectPool.Free(pCreatePacket);
		univ_dev::g_PacketObjectPool.Free(pRemoveOtherPlayerPacket);
		univ_dev::g_PacketObjectPool.Free(pCreateOtherPlayerPacket);
	}
}
void GetNearBySector(univ_dev::Player* player, univ_dev::SectorAround& oldSectorAround, univ_dev::SectorAround& curSectorAround)
{
	int oldBeginSectorX = player->oldSector.x - 1;
	int oldBeginSectorY = player->oldSector.y - 1;

	int curBeginSectorX = player->curSector.x - 1;
	int curBeginSectorY = player->curSector.y - 1;
	oldSectorAround.sectorCount = 0;
	curSectorAround.sectorCount = 0;

	for (int i = 0; i < 3; i++)
	{
		oldBeginSectorX = player->oldSector.x - 1;
		if (oldBeginSectorY >= univ_dev::SECTOR_MAX_Y || oldBeginSectorY < 0)
		{
			oldBeginSectorY++;
			continue;
		}
		for (int j = 0; j < 3; j++)
		{
			if (oldBeginSectorX < 0 || oldBeginSectorX >= univ_dev::SECTOR_MAX_X)
			{
				oldBeginSectorX++;
				continue;
			}
			oldSectorAround.aroundSector[oldSectorAround.sectorCount].x = oldBeginSectorX;
			oldSectorAround.aroundSector[oldSectorAround.sectorCount].y = oldBeginSectorY;
			oldSectorAround.sectorCount++;
			oldBeginSectorX++;
		}
		oldBeginSectorY++;
	}

	for (int i = 0; i < 3; i++)
	{
		curBeginSectorX = player->curSector.x - 1;
		if (curBeginSectorY >= univ_dev::SECTOR_MAX_Y || curBeginSectorY < 0)
		{
			curBeginSectorY++;
			continue;
		}
		for (int j = 0; j < 3; j++)
		{
			if (curBeginSectorX < 0 || curBeginSectorX >= univ_dev::SECTOR_MAX_X)
			{
				curBeginSectorX++;
				continue;
			}
			curSectorAround.aroundSector[curSectorAround.sectorCount].x = curBeginSectorX;
			curSectorAround.aroundSector[curSectorAround.sectorCount].y = curBeginSectorY;
			curSectorAround.sectorCount++;
			curBeginSectorX++;
		}
		curBeginSectorY++;
	}
}

void RemoveSector(univ_dev::Player* player)
{
	auto iter = g_Sector[player->oldSector.y][player->oldSector.x].find(player);
	if (iter == g_Sector[player->oldSector.y][player->oldSector.x].end())
	{
		int* ptr = nullptr;
		*ptr = 100;
		return;
	}
	g_Sector[player->oldSector.y][player->oldSector.x].erase(player);
}
void AddSector(univ_dev::Player* player)
{
	g_Sector[player->curSector.y][player->curSector.x].emplace(player);
}
void ControlServer()
{
	if (_kbhit())
	{
		int key = _getch();
		if (!isalpha(key)) return;
		key = toupper(key);
		if (key == 'L')
		{
			serverLock = true;
			printf("Server Locked\n");
		}
		else if (toupper(key) == 'U')
		{
			serverLock = false;
			printf("Server Unlocked\n");
		}
		else if (toupper(key) == 'Q' && !serverLock)
		{
			g_ShutDownFlag = true;
			printf("ServerOff\n");
		}
		else if (toupper(key) == 'D' && !serverLock)
		{
			g_DummyMode = !g_DummyMode;
			g_DummyMode ? printf("Dummy Mode On\n") : printf("Dummy Mode Off\n");
		}
		else if (toupper(key) == 'S' && !serverLock)
		{
			g_SimpleStatusShoing = !g_SimpleStatusShoing;
			g_SimpleStatusShoing ? printf("Simple Showing Mode On\n") : printf("Simple Showing Mode Off\n");
		}
	}
}


void MonitorServerStatus()
{
	clock_t now = clock();
	if (cur - sec > 1000)
	{
		if (update < 25)
		{
			time_t unixTime = time(nullptr);
			tm date;
			localtime_s(&date, &unixTime);
			char fileName[100]{ 0 };
			char tempBuffer[20]{ 0 };
			_itoa_s(date.tm_year + 1900, tempBuffer, 10);
			strcat_s(fileName, tempBuffer);
			_itoa_s(date.tm_mon + 1, tempBuffer, 10);
			strcat_s(fileName, tempBuffer);
			_itoa_s(date.tm_mday, tempBuffer, 10);
			strcat_s(fileName, tempBuffer);
			strcat_s(fileName, "_log.txt");
			FILE* file = nullptr;
			while (file == nullptr)
				fopen_s(&file, fileName, "ab");
			fprintf(file, "Update Frame : %d\tIO Frame : %d\tExcute Time : %d\n", update, g_NetworkIOLoop, now / 1000);
			fclose(file);
			if (g_SimpleStatusShoing)
				printf("Update Frame : %d\tIO Frame : %d\tExcute Time : %d\n", update, g_NetworkIOLoop, now / 1000);
		}
		if (!g_SimpleStatusShoing)
		{
			system("cls");
			g_TotalSyncPacketSend += g_SyncPacketPerSec;
			printf("-----------------------------------------\n");
			printf("EXECUTE_TIME : %u sec\n", now / 1000);
			printf("CUR - SEC : %u\n", cur - sec);
			printf("%d_UPDATE_CALLED \n", update);
			if (serverLock) printf("SERVER_LOCKED\n");
			else printf("SERVER_UNLOCKED\n");
			printf("NET_IO_PER_SEC : %d\n", g_NetworkIOLoop);
			printf("RECV_PER_SEC : %d\n", g_RecvPerSec);
			printf("SEND_PER_SEC : %d\n", g_SendPerSec);
			printf("SYNC_PACKET_SEND_PER_SEC : %d\n", g_SyncPacketPerSec);
			printf("TOTAL_SYNC_PACKET_SEND : %llu\n", g_TotalSyncPacketSend);
			printf("SYNC_MOVE_START : %d\n", g_SyncMoveStart);
			printf("SYNC_MOVE_STOP : %d\n", g_SyncMoveStop);
			printf("SYNC_ATTACK1 : %d\n", g_SyncAttack1);
			printf("SYNC_ATTACK2 : %d\n", g_SyncAttack2);
			printf("SYNC_ATTACK3 : %d\n", g_SyncAttack3);

			printf("\nMEMORY OBJECT POOL\n");
			printf("PLAYER_POOL_CAPACITY : %d\n", univ_dev::g_PlayerObjectPool.GetCapacityCount());
			printf("PLAYER_POOL_USE_COUNT : %d\n", univ_dev::g_PlayerObjectPool.GetUseCount());
			printf("SESSION_POOL_CAPACITY : %d\n", univ_dev::g_SessionObjectPool.GetCapacityCount());
			printf("SESSION_POOL_USE_COUNT : %d\n", univ_dev::g_SessionObjectPool.GetUseCount());
			printf("PACKET_POOL_CAPACITY : %d\n", univ_dev::g_PacketObjectPool.GetCapacityCount());
			printf("PACKET_POOL_USE_COUNT : %d\n", univ_dev::g_PacketObjectPool.GetUseCount());
			g_SendPerSec = g_RecvPerSec = g_SyncPacketPerSec = g_NetworkIOLoop = 0;
			printf("-----------------------------------------\n\n");
		}
		g_NetworkIOLoop = update = 0;
		sec = cur;
	}
}
void DisconnectProc(univ_dev::Session* session)
{
	g_DisconnectSession.emplace(session);

	auto removePlayerIter = univ_dev::g_PlayerMap.find(session->sessionID);
	if (removePlayerIter == univ_dev::g_PlayerMap.end()) return;

	univ_dev::Player* removePlayer = removePlayerIter->second;
	auto iter = g_SendOnSessionList.find(session);
	if (iter != g_SendOnSessionList.end())
		g_SendOnSessionList.erase(session);
	g_Sector[removePlayer->curSector.y][removePlayer->curSector.x].erase(removePlayer);
	univ_dev::Packet* pRemovePacket = univ_dev::g_PacketObjectPool.Alloc();
	pRemovePacket->Clear();
	univ_dev::MakePacketDeletePlayer(*pRemovePacket, removePlayer->sessionID);
	SendBroadcastNearby(session, session, *pRemovePacket);
	pRemovePacket->Clear();
	univ_dev::g_PacketObjectPool.Free(pRemovePacket);
}


void Disconnect()
{
	// 1. 세션 연결종료
	for (auto iter = g_DisconnectSession.begin(); iter != g_DisconnectSession.end(); ++iter)
	{
		univ_dev::Session* removeSession = *iter;
		auto removePlayerIter = univ_dev::g_PlayerMap.find(removeSession->sessionID);
		if (removePlayerIter == univ_dev::g_PlayerMap.end()) return;

		univ_dev::Player* removePlayer = removePlayerIter->second;
		univ_dev::g_SessionMap.erase(removeSession->sock);
		univ_dev::g_PlayerMap.erase(removePlayer->sessionID);
		closesocket(removeSession->sock);
		univ_dev::g_SessionObjectPool.Free(removeSession);
		univ_dev::g_PlayerObjectPool.Free(removePlayer);
	}
	g_DisconnectSession.clear();
}


void AcceptProc()
{
	sockaddr_in addr;
	int addrlen = sizeof(addr);
	SOCKET sock = accept(g_ListenSocket, (sockaddr*)&addr, &addrlen);
	if (sock == INVALID_SOCKET) return;

	univ_dev::Session* newSession = univ_dev::CreateSession(sock);
	univ_dev::Player* newPlayer = univ_dev::CreatePlayer(newSession);


	AddSector(newPlayer);

	univ_dev::Packet* pPacket = univ_dev::g_PacketObjectPool.Alloc();
	univ_dev::Packet* pCreateOtherPacket = univ_dev::g_PacketObjectPool.Alloc();
	pPacket->Clear();
	pCreateOtherPacket->Clear();
	univ_dev::MakePacketCreateNewPlayer(*pPacket, newPlayer->sessionID, newPlayer->moveDirection, newPlayer->xPos, newPlayer->yPos, newPlayer->HP);

	int xIdx = newPlayer->curSector.x - 1;
	int yIdx = newPlayer->curSector.y - 1;
	for (int i = 0; i < 3; i++)
	{
		xIdx = newPlayer->curSector.x - 1;
		if (yIdx < 0 || yIdx >= univ_dev::SECTOR_MAX_Y)
		{
			yIdx++;
			continue;
		}
		for (int j = 0; j < 3; j++)
		{
			if (xIdx < 0 || xIdx >= univ_dev::SECTOR_MAX_X)
			{
				xIdx++;
				continue;
			}
			for (auto iter = g_Sector[yIdx][xIdx].begin(); iter != g_Sector[yIdx][xIdx].end(); ++iter)
			{
				univ_dev::Player* currentPlayer = *iter;
				if (currentPlayer == newPlayer)continue;
				univ_dev::MakePacketCreatePlayer(*pCreateOtherPacket, currentPlayer->sessionID, currentPlayer->direction, currentPlayer->xPos, currentPlayer->yPos, currentPlayer->HP);
				SendUnicast(newSession, *pCreateOtherPacket);
				pCreateOtherPacket->Clear();
			}
			xIdx++;
		}
		yIdx++;
	}

	SendUnicast(newSession, *pPacket);

	univ_dev::Packet* pBroadcastPacket = univ_dev::g_PacketObjectPool.Alloc();
	pBroadcastPacket->Clear();
	univ_dev::MakePacketCreatePlayer(*pBroadcastPacket, newPlayer->sessionID, newPlayer->moveDirection, newPlayer->xPos, newPlayer->yPos, newPlayer->HP);
	SendBroadcastNearby(newSession, newSession, *pBroadcastPacket);

	pBroadcastPacket->Clear();
	pPacket->Clear();
	pCreateOtherPacket->Clear();
	univ_dev::g_PacketObjectPool.Free(pPacket);
	univ_dev::g_PacketObjectPool.Free(pCreateOtherPacket);
	univ_dev::g_PacketObjectPool.Free(pBroadcastPacket);
}

void ReadProc(univ_dev::Session* session)
{
	int recvBytes = recv(session->sock, session->RQ.GetWritePtr(), session->RQ.DirectEnqueueSize(), 0);
	if (!recvBytes)
	{
		//disconnect
		DisconnectProc(session);
		return;
	}
	if (recvBytes == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK)
		{
			//진짜문제있음.
			if (err != 10054)
				LogCriticalError(__FUNCTION__, __LINE__, "recvBytes is SOCKET_ERROR and err is not WSAEWOULDBLOCK OR 10054");
			DisconnectProc(session);
			return;
		}
	}
	session->RQ.MoveWritePtr(recvBytes);
	session->lastRecvTime = timeGetTime();
	univ_dev::Packet* pPacket = univ_dev::g_PacketObjectPool.Alloc();
	while (true)
	{
		PacketHeader header;
		int rqUseSize = session->RQ.GetUseSize();
		if (rqUseSize < sizeof(header)) break;
		int headerPeekRet = session->RQ.Peek((char*)&header, sizeof(header));
		if (header.code != 0x89)
		{
			//잘못된 클라이언트는 삭제해버려야지
			LogDisconnectInfo(__FUNCTION__, __LINE__, session, "Header code is not 0x89");
			DisconnectProc(session);
			return;
		}
		if (rqUseSize < sizeof(header) + header.payloadSize) break;

		session->RQ.MoveReadPtr(sizeof(header));
		int directDQSize = session->RQ.DirectDequeueSize();
		int pkPeekRet = session->RQ.Peek(pPacket->GetWritePtr(), min(header.payloadSize, directDQSize));

		session->RQ.MoveReadPtr(pkPeekRet);
		int moveSize = pPacket->MoveWritePtr(header.payloadSize);
		g_RecvPerSec++;
		PacketProc(header, session, *pPacket);
		pPacket->Clear();
	}
	univ_dev::g_PacketObjectPool.Free(pPacket);
	int useSize = session->RQ.GetUseSize();
	if (useSize == 0)
		session->RQ.ClearBuffer();
}


void SendProc(univ_dev::Session* session)
{
	int useSize = session->SQ.GetUseSize();
	if (useSize == 0) return;
	int sendRet = send(session->sock, session->SQ.GetReadPtr(), session->SQ.DirectDequeueSize(), 0);

	int secondSendRet = 0;
	if (sendRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK)
		{
			if (err != 10054)
				LogDisconnectInfo(__FUNCTION__, __LINE__, session, "sendRet1 is SOCKET_ERROR and err is not WSAEWOULDBLOCK or 10054");
			DisconnectProc(session);
			return;
		}
	}
	if (useSize != sendRet)
	{
		secondSendRet = send(session->sock, session->SQ.GetReadPtr(), session->SQ.DirectDequeueSize(), 0);
		if (secondSendRet == SOCKET_ERROR)
		{
			int err = WSAGetLastError();
			if (err != WSAEWOULDBLOCK)
			{
				if (err != 10054)
					LogDisconnectInfo(__FUNCTION__, __LINE__, session, "sendRet2 is SOCKET_ERROR and err is not WSAEWOULDBLOCK or 10054");
				DisconnectProc(session);
				return;
			}
		}
	}
	if (useSize != sendRet + secondSendRet)
	{
		LogDisconnectInfo(__FUNCTION__, __LINE__, session, "useSize is not same with sendRet1 + sendRet2");
		DisconnectProc(session);
		return;
	}
	if (session->SQ.GetUseSize() == 0)
		session->SQ.ClearBuffer();
}


void SendProc()
{
	for (auto iter = g_SendOnSessionList.begin(); iter != g_SendOnSessionList.end(); ++iter)
	{
		//printf("SendProc()\n");
		univ_dev::Session* currentSession = *iter;
		if (univ_dev::g_SessionMap.find(currentSession->sock) == univ_dev::g_SessionMap.end()) continue;
		int useSize = currentSession->SQ.GetUseSize();
		int sendRet = send(currentSession->sock, currentSession->SQ.GetReadPtr(), currentSession->SQ.DirectDequeueSize(), 0);
		//g_SendPerSec++;
		int secondSendRet = 0;
		if (sendRet == SOCKET_ERROR)
		{
			int err = WSAGetLastError();
			if (err != WSAEWOULDBLOCK)
			{
				if (err != 10054)
					LogDisconnectInfo(__FUNCTION__, __LINE__, currentSession, "sendRet1 is SOCKET_ERROR and err is not WSAEWOULDBLOCK or 10054");
				DisconnectProc(currentSession);
				continue;
			}
		}
		currentSession->SQ.MoveReadPtr(sendRet);
		if (useSize != sendRet)
		{
			secondSendRet = send(currentSession->sock, currentSession->SQ.GetReadPtr(), currentSession->SQ.DirectDequeueSize(), 0);
			//g_SendPerSec++;
			if (secondSendRet == SOCKET_ERROR)
			{
				int err = WSAGetLastError();
				if (err != WSAEWOULDBLOCK)
				{
					if (err != 10054)
						LogDisconnectInfo(__FUNCTION__, __LINE__, currentSession, "sendRet2 is SOCKET_ERROR and err is not WSAEWOULDBLOCK or 10054");
					DisconnectProc(currentSession);
					continue;
				}
			}
		}
		if (useSize != sendRet + secondSendRet)
		{
			LogDisconnectInfo(__FUNCTION__, __LINE__, currentSession, "useSize is not same with sendRet1 + sendRet2");
			DisconnectProc(currentSession);
			continue;
		}
		if (currentSession->SQ.GetUseSize() == 0)
			currentSession->SQ.ClearBuffer();
	}
	g_SendOnSessionList.clear();
}


bool LoadData()
{
	//원래라면 DB에서 게임 데이터들을 읽거나 아니면...
	//서버를 위한 데이터를 읽어오는 준비를 해야됨.
	return true;
}


SOCKET NetworkInitial()
{
	WSADATA data;
	if (WSAStartup(MAKEWORD(2, 2), &data))
		return INVALID_SOCKET;
	printf("WSAStartup clear\n");

	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
	{
		WSACleanup();
		return INVALID_SOCKET;
	}
	printf("socket clear\n");

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(MMO_TCP_FIGHTER_PORT);
	addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	int bindRet = bind(sock, (sockaddr*)&addr, sizeof(addr));
	if (bindRet == SOCKET_ERROR)
	{
		closesocket(sock);
		WSACleanup();
		return INVALID_SOCKET;
	}
	printf("bind clear\n");

	int listenRet = listen(sock, SOMAXCONN_HINT(1000));
	if (listenRet == SOCKET_ERROR)
	{
		closesocket(sock);
		WSACleanup();
		return INVALID_SOCKET;
	}
	printf("listen clear\n");

	linger l;
	l.l_onoff = 1;
	l.l_linger = 0;
	setsockopt(sock, SOL_SOCKET, SO_LINGER, (const char*)&l, sizeof(l));
	printf("time wait off\n");


	u_long blockingMode = 0;
	ioctlsocket(sock, FIONBIO, &blockingMode);
	printf("non blocking mode on\n");

	return sock;
}



void NetworkCleanup()
{
	for (auto iter = univ_dev::g_PlayerMap.begin(); iter != univ_dev::g_PlayerMap.end(); ++iter)
	{
		univ_dev::g_PlayerObjectPool.Free(iter->second);
	}
	for (auto iter = univ_dev::g_SessionMap.begin(); iter != univ_dev::g_SessionMap.end(); ++iter)
	{
		univ_dev::Session* currentSession = iter->second;
		closesocket(currentSession->sock);
		univ_dev::g_SessionObjectPool.Free(currentSession);
	}

	univ_dev::g_PlayerMap.clear();
	univ_dev::g_SessionMap.clear();

	closesocket(g_ListenSocket);
	WSACleanup();
	return;
}