#pragma comment(lib,"ws2_32.lib")
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <locale>
#include <ctime>
#define PROFILE
#include "profiler.h"
#include "SerializingBuffer.h"
#include "ObjectFreeList.hpp"
#include "Protocol.h"
#include "Session.h"
#include "room.h"

///로비 방번호 0
#define LOBBY_ROOM_NUMBER 0



// 전역 변수
///------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
/// 서버 기능에 필요한 전역 변수

/// 들어와있는 모든 세션들
std::unordered_map<DWORD, univ_dev::Session*> g_SessionMap;
/// 생성되어있는 모든 방들
std::unordered_map<DWORD, univ_dev::Room*> g_RoomMap;

/// send해야 하는 세션을 여기에 모아서 한번에 처리
std::unordered_set<univ_dev::Session*> g_SendOnSessionList;

/// 사용중인 이름 검색용도
std::unordered_set<std::wstring> g_UsedNameSet;
/// 사용중인 방 이름 검색 용도
std::unordered_set<std::wstring> g_UsedRoomNameSet;

/// 루프에서 연결종료된 세션을 여기에 모아서 한번에 처리
std::unordered_set<univ_dev::Session*> g_DisconnectSession;


/// 오브젝트풀
/// 세션에 대한 오브젝트풀
univ_dev::ObjectFreeList<univ_dev::Session> g_SessionPool;
/// 방에 대한 오브젝트 풀
univ_dev::ObjectFreeList<univ_dev::Room> g_RoomPool;
/// 패킷에 대한 오브젝트 풀
univ_dev::ObjectFreeList<univ_dev::Packet> g_PacketPool;

/// 현재 방번호
unsigned int g_RoomNumber = 0;
/// 현재 클라이언트 번호
unsigned int g_ClientNumber = 1;
/// 전역 리슨소켓
SOCKET g_ListenSocket;
///------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

///------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
/// 프로파일링을 위한 변수

/// 시작 시간 나중에 프로파일링에 평균낼때 사용
clock_t g_beginTimer;



/// 1초당 ...
/// accept호출수
unsigned int g_AcceptPerSec;
/// recv호출한 세션수(recv호출 수 아님)
unsigned int g_RecvPerSec;
/// send호출한 세션수(send호출 수 아님)
unsigned int g_SendPerSec;
/// connect 된 세션수 이건 어떻게 구해야되는지 모르겠음...
unsigned int g_ConnectPerSec;
/// connect 실패 세션수 이건 어떻게 구해야되는지 모르겠음...
unsigned int g_ConnectFailPerSec;
/// Disconnect된 세션수
unsigned int g_DisconnectCount;

/// 프로세스 가동 후 총 ...
/// accept 횟수
unsigned long long g_CumerativeAcceptCount;
/// recv한 세션수(recv 횟수 아님)
unsigned long long g_CumerativeRecvCount;
/// send한 세션수(send 횟수 아님)
unsigned long long g_CumerativeSendCount;
/// connect 요청한 세션 수
unsigned long long g_CumerativeConnectCount;
/// connect 실패한 세션 수
unsigned long long g_CumerativeConnectFailCount;
/// Disconnect된 세션수
unsigned long long g_CumerativeDisconnectCount;

/// 프로세스 가동 후...
/// 로그인 아니라서 Disconnect 된 횟수
unsigned long long g_Disconnect_IsNotLogined;
/// 로비인데 방나가기 패킷이 와서 Disconnect 된 횟수
unsigned long long g_Disconnect_NotLobbyLeaveRoom;
/// 방이 아닌데 채팅 패킷이 와서 Disconnect 된 횟수
unsigned long long g_Disconnect_NotInRoomChatting;
/// 세션 이름이 동일해서 Disconnect 된 횟수
unsigned long long g_Disconnect_SessionNameDuplicate;
/// 7000명 이후에 들어와서 Disconnect 된 세션수
unsigned long long g_Disconnect_SessionOver7000;
/// accept에서 INVALID_SOCKET이 나와서 없어진 횟수
unsigned long long g_Disconnect_AcceptErrorInvalidSocket;
/// send Error인데 EWOULDBLOCK이 아니라 Disconnect 된 횟수
unsigned long long g_Disconnect_SendErrIsNotBLOCK;
/// 링버퍼의 문제로 Disconnect 된 횟수
unsigned long long g_Disconnect_RingBufferProb;
/// RST 시그널이 도착해서 Disconnect된 횟수(정상종료)
unsigned long long g_Disconnect_RSTSignal;
/// FIN 시그널이 도착해서 Disconnect된 횟수(올 수 없음) <- linger on + time = 0이니까.
unsigned long long g_Disconnect_FINSignal;
/// recv 에러인데 EWOULDBLOCK이 아니라 Disconnect 된 횟수
unsigned long long g_Disconnect_RECV_ERR_IsNotBlock;
/// Packet Code가 잘못 와서 Disconnect 된 횟수
unsigned long long g_Disconnect_PacketCodeError;
/// Check Sum이 잘못되어서 Disconnect된 횟수
unsigned long long g_Disconnect_CheckSumError;
/// Packet Proc에서 Default로 떨어져 Disconnect 된 횟수(Packet Type의 문제)
unsigned long long g_Disconnect_DefaultCase;
///------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

///------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
/// 프로파일링 함수 선언
/// Profiling Function Declare
void PrintLog();
void SaveServerStatus(clock_t current, clock_t g_beginTimer);
///------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


///------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
/// 체크섬 함수 선언
/// CheckSum Function Declare
int MakeCheckSum(WORD msgType,WORD payloadSize,univ_dev::Packet& packet);
///------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

///------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
/// Send 함수 선언
/// Send Function Declare
void SendUnicast(univ_dev::Session* targetSession, univ_dev::Packet& packet);
void SendBroadCast(univ_dev::Packet& packet);
void SendMulticast(univ_dev::Room* targetRoom, univ_dev::Session* exSession, univ_dev::Packet& packet);
void PushSendList(univ_dev::Session* session);
///------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


///------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
/// 방관련 함수 선언
/// Room Function Declare
void EnterRoom(univ_dev::Room* room, univ_dev::Session* session);
void LeaveRoom(univ_dev::Room* room, univ_dev::Session* session);
void RemoveRoom(univ_dev::Room* room);
void CreateRoom(DWORD roomNumber, const WCHAR* roomName, WORD roomNameByte);
///------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------



///------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
/// 패킷 프로시져 선언
/// Packet Procedure Declare
void PacketProcRequireLogin(univ_dev::Session* session, univ_dev::Packet& packet, const st_PACKET_HEADER & header);
void PacketProcRequireRoomList(univ_dev::Session* session, univ_dev::Packet& packet, const st_PACKET_HEADER & header);
void PacketProcRequireMakeRoom(univ_dev::Session* session, univ_dev::Packet& packet, const st_PACKET_HEADER & header);
void PacketProcRequireEnterRoom(univ_dev::Session* session, univ_dev::Packet& packet, const st_PACKET_HEADER& header);
void PacketProcRequireLeaveRoom(univ_dev::Session* session, univ_dev::Packet& packet, const st_PACKET_HEADER& header);
void PacketProcRequireChat(univ_dev::Session* session, univ_dev::Packet& packet, const st_PACKET_HEADER& header);
void PacketProcRequireStressEchoTest(univ_dev::Session* session, univ_dev::Packet& packet, const st_PACKET_HEADER& header);
///------------------------------------------------------------------------------------------------------------------------


///------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
/// 패킷 생성 함수 선언
/// Make Packet Function Declare
void MakePacketResponseLogin(univ_dev::Packet& packet,  BYTE result, DWORD userID);
void MakePacketResponseRoomList(univ_dev::Packet& packet, WORD numOfRoom);
void MakePacketResponseMakeRoom(univ_dev::Packet& packet, BYTE result, DWORD roomNumber, WORD roomNameByte,const WCHAR* roomName);
void MakePacketResponseEnterRoom(univ_dev::Packet& packet, BYTE result, univ_dev::Room* room);
void MakePacketResponseEnterOtherPerson(univ_dev::Packet& packet, univ_dev::Session* otherUser,univ_dev::Room* room);
void MakePacketResponseChat(univ_dev::Packet& packet, univ_dev::Session* session, DWORD userNo, WORD msgByte, const WCHAR* message);
void MakePacketResponseLeaveRoom(univ_dev::Packet& packet, univ_dev::Session* session, DWORD userNo);
void MakePacketResponseRemoveRoom(univ_dev::Packet& packet, DWORD roomNo);
void MakePacketResponseStressEchoTest(univ_dev::Packet& sendPacket, WORD msgByte, const WCHAR* message, const st_PACKET_HEADER& header);
///------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


///------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
/// 메인 로직 선언
/// Main Logic Function Declare
SOCKET NetworkInit(unsigned short port = dfNETWORK_PORT);
void AcceptProc();
void ReadProc(univ_dev::Session* session);
void DisconnectProc(univ_dev::Session* session);
void Disconnect();
void PacketProc(st_PACKET_HEADER header, univ_dev::Session* session,univ_dev::Packet& packet);
void SendProc();
///------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------



/// main function
int main()
{
	setlocale(LC_ALL, "");
	g_beginTimer = clock();
	g_ListenSocket = NetworkInit();
	if (g_ListenSocket == INVALID_SOCKET)
	{
		//printf("SetListenSocket() error \n");
		return -1;
	}
	//printf("Server Open...\n");
	timeval time;
	time.tv_sec = 0;
	time.tv_usec = 0;
	
	//univ_dev::Room* lobby = new univ_dev::Room();
	univ_dev::Room* lobby = g_RoomPool.Alloc();

	lobby->roomNumber = g_RoomNumber++;
	wcscpy_s(lobby->roomName, L"lobby");
	g_RoomMap.emplace(std::make_pair(lobby->roomNumber, lobby));

	fd_set  rSet,wSet;
	clock_t prev = clock();
	clock_t current;
	bool saveFileFlag = true;
	DWORD currentCheckSession[64];
	int userCount = 0;
	while (true)
	{
		current = clock();
		if (current - prev > 1000)
		{
			system("cls");
			PrintLog();
			prev = current;
			saveFileFlag = true;
		}
		short ret = GetAsyncKeyState(VK_F1);
		if (ret && saveFileFlag)
		{
			SaveServerStatus(current,g_beginTimer);
			univ_dev::SaveProfiling();
			saveFileFlag = false;
		}
		/// select polling 방식의 서버.
		//여기부터 루틴 시작
		//fd_set 하나 만듬 + 초기화
		FD_ZERO(&rSet);
		FD_ZERO(&wSet);
		// 리슨소켓 집어넣고
		auto iter = g_SessionMap.begin();
		// 반복문 g_SessionList의 사이즈만큼 반복
		while (true)
		{
			univ_dev::Profiler profiler("WHILE_LOOP");
			FD_ZERO(&rSet);
			ZeroMemory(currentCheckSession, 64 * sizeof(DWORD));
			FD_SET(g_ListenSocket, &rSet);
			FD_SET(g_ListenSocket, &wSet);
			userCount = 0;
			for (; userCount < 63; userCount++)
			{
				if (iter == g_SessionMap.end()) break;
				FD_SET(iter->second->sock, &rSet);
				FD_SET(iter->second->sock, &wSet);
				currentCheckSession[userCount] = iter->second->sessionNo;
				++iter;
			}
			int numOfSignaledSocket = select(0, &rSet, nullptr, nullptr, &time);
			if (numOfSignaledSocket == SOCKET_ERROR)
			{
				//printf("error select : %d\n", WSAGetLastError());
			}

			if (FD_ISSET(g_ListenSocket, &rSet))
			{
				AcceptProc();
			}
			for (int i = 0; i< userCount; i++)
			{
				univ_dev::Session* currentSession;
				auto currentSessionIter = g_SessionMap.find(currentCheckSession[i]);
				if (currentSessionIter == g_SessionMap.end())
				{
					//문제가 있음
					int* ptr = nullptr;
					*ptr = 100;
					return -1;
				}
				currentSession = currentSessionIter->second;
				if (FD_ISSET(currentSession->sock, &rSet))
				{
					ReadProc(currentSession);
				}
			}
			if (iter == g_SessionMap.end())break;
		}
		SendProc();
		Disconnect();
	}
	return 0;
}


///------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
/// 방 관련 함수 정의
/// Room Function Define
void EnterRoom(univ_dev::Room* room, univ_dev::Session* session)
{
	if (room == nullptr || session == nullptr)
	{
		int* ptr = nullptr;
		*ptr = 100;
		return;
	}
	g_RoomMap.at(0)->participants.erase(session);
	session->currentRoomNumber = room->roomNumber;
	room->participants.emplace(session);
}
void LeaveRoom(univ_dev::Room* room, univ_dev::Session* session)
{
	if (session == nullptr ||room == nullptr)
	{
		//큰문제가 있는거지..?
		int* ptr = nullptr;
		*ptr = 100;
		return;
	}
	room->participants.erase(session);
	session->currentRoomNumber = LOBBY_ROOM_NUMBER;
	g_RoomMap.at(0)->participants.emplace(session);
}
void RemoveRoom(univ_dev::Room* room)
{
	if (room == nullptr)
	{
		int* ptr = nullptr;
		*ptr = 100;
		return;
	}
	g_RoomMap.erase(room->roomNumber);
	g_UsedRoomNameSet.erase(room->roomName);
	g_RoomPool.Free(room);
	//delete room;
}
void CreateRoom(DWORD roomNumber, const WCHAR* roomName, WORD roomNameByte)
{
	//univ_dev::Room* newRoom = new univ_dev::Room();
	univ_dev::Room* newRoom = g_RoomPool.Alloc();
	newRoom->roomNumber = roomNumber;
	memset(newRoom->roomName, 0, ROOM_NAME_MAX_LEN * sizeof(WCHAR));
	memcpy_s(newRoom->roomName, roomNameByte, roomName, roomNameByte);

	g_RoomMap.emplace(std::make_pair(newRoom->roomNumber, newRoom));
	g_UsedRoomNameSet.emplace(newRoom->roomName);
}
///------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------



///------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
/// 패킷 프로시져 함수 정의
/// Packet Procedure Define
void PacketProcRequireLogin(univ_dev::Session* session, univ_dev::Packet& packet, const st_PACKET_HEADER & header)
{
	WCHAR nickNameBuffer[15]{ 0 };
	memcpy_s(nickNameBuffer, sizeof(WCHAR) * 15, packet.GetReadPtr(), header.wPayloadSize);
	//wprintf(L"packet.GetReadPtr() : %s\n", (WCHAR*)packet.GetReadPtr());
	BYTE result = 1;
	DWORD userID = -1;
	if(g_UsedNameSet.find(nickNameBuffer) != g_UsedNameSet.end())
	{
		result = 2; 
		g_Disconnect_SessionNameDuplicate++;
	}
	else if(g_SessionMap.size() > 7000)
	{
	 	result = 3;
		g_Disconnect_SessionOver7000++;
	}
	if (result == 1)
	{
		memset(session->sessionNickName, 0, sizeof(session->sessionNickName));
		memcpy_s(session->sessionNickName, dfNICK_MAX_LEN * sizeof(WCHAR), nickNameBuffer, header.wPayloadSize);
		//wprintf(L"nick name buffer : %s\n", nickNameBuffer);
		//wprintf(L"session nick name : %s\n", session->sessionNickName);
		g_UsedNameSet.emplace(nickNameBuffer);
		session->logined = true;
		userID = session->sessionNo;
		session->currentRoomNumber = 0;
		g_RoomMap.at(session->currentRoomNumber)->participants.emplace(session);
	}
	else
	{
		DisconnectProc(session);
	}

	//닉네임 넣고나서 SQ에 새로 패킷담아줘야되는데... 그 패킷안에는 1 + 4byte 들어감
	//5바이트만큼 SQ에 안들어가면 그 클라이언트는 버려야됨 이미 오래전부터 로직수행 불가능상태임.
	if (session->SQ.GetFreeSize() < df_RES_LOGIN_SIZE)
	{
		DisconnectProc(session);
		return;
	}


	univ_dev::Packet* pSendPacket = g_PacketPool.Alloc();
	pSendPacket->Clear();
	MakePacketResponseLogin(*pSendPacket, result, userID);
	SendUnicast(session, *pSendPacket);
	pSendPacket->Clear();
	g_PacketPool.Free(pSendPacket);
}
void PacketProcRequireRoomList(univ_dev::Session* session, univ_dev::Packet& packet, const st_PACKET_HEADER & header)
{
	if (!session->logined)
	{
		g_Disconnect_IsNotLogined++;
		DisconnectProc(session);
		return;
	}
	WORD numOfRooms = g_RoomMap.size();
	univ_dev::Packet* pSendPacket = g_PacketPool.Alloc();
	pSendPacket->Clear();
	MakePacketResponseRoomList(*pSendPacket, numOfRooms - 1);

	//printf("sendPacket.GetBufferSize() : %d\n", sendPacket.GetBufferSize());
	SendUnicast(session, *pSendPacket);
	pSendPacket->Clear();
	g_PacketPool.Free(pSendPacket);
	//session->SQ.Enqueue(pSendPacket->GetReadPtr(), pSendPacket->GetBufferSize());
	//PushSendList(session);
}
void PacketProcRequireMakeRoom(univ_dev::Session* session, univ_dev::Packet& packet, const st_PACKET_HEADER & header)
{
	if (!session->logined)
	{
		g_Disconnect_IsNotLogined++;
		DisconnectProc(session);
		return;
	}
	BYTE result = 1;
	WCHAR tempRoomName[ROOM_NAME_MAX_LEN]{ 0 };
	WORD roomNameBytes;
	DWORD roomNumber = g_RoomNumber;
	packet >> roomNameBytes;
	memcpy_s(tempRoomName, ROOM_NAME_MAX_LEN * sizeof(WCHAR), packet.GetReadPtr(), roomNameBytes);
	if (g_UsedRoomNameSet.find(tempRoomName) != g_UsedRoomNameSet.end())
	{
		result = 2;
		roomNumber = -1;
		roomNameBytes = 0;
	}
	else if (g_RoomMap.size() > 256)
	{
		result = 3;
		roomNumber = -1;
		roomNameBytes = 0;
	}
	if (result == 1)
	{
		CreateRoom(g_RoomNumber++, (WCHAR*)packet.GetReadPtr(), roomNameBytes);
		//univ_dev::Room* newRoom = new univ_dev::Room();
		//newRoom->roomNumber = g_RoomNumber++;
		//memset(newRoom->roomName, 0, ROOM_NAME_MAX_LEN * sizeof(WCHAR));
		//memcpy_s(newRoom->roomName, ROOM_NAME_MAX_LEN * sizeof(WCHAR), packet.GetReadPtr(), roomNameBytes);
		//g_RoomMap.emplace(std::make_pair(newRoom->roomNumber, newRoom));
		//g_UsedRoomNameSet.emplace(newRoom->roomName);
	}
	univ_dev::Packet* pSendPacket = g_PacketPool.Alloc();
	pSendPacket->Clear();
	MakePacketResponseMakeRoom(*pSendPacket, result, roomNumber, roomNameBytes, tempRoomName);

	SendBroadCast(*pSendPacket);
	pSendPacket->Clear();
	g_PacketPool.Free(pSendPacket);
	//for (auto iter = g_SessionMap.begin(); iter != g_SessionMap.end(); ++iter)
	//{
	//	univ_dev::Session* currentSession = iter->second;
	//	int eqRet = currentSession->SQ.Enqueue(sendPacket.GetReadPtr(), sendPacket.GetBufferSize());
	//	PushSendList(session);
	//}
}
void PacketProcRequireEnterRoom(univ_dev::Session* session, univ_dev::Packet& packet, const st_PACKET_HEADER& header)
{
	if (!session->logined)
	{
		g_Disconnect_IsNotLogined++;
		DisconnectProc(session);
		return;
	}
	else if (session->currentRoomNumber != 0) return;
	DWORD roomNumber;
	packet >> roomNumber;
	BYTE result = 1;
	auto iter = g_RoomMap.find(roomNumber);
	if (iter == g_RoomMap.end()) result = 2;
	else if (iter->second->participants.size() > 256) result = 3;

	univ_dev::Room* hopeToEnterRoom = nullptr;
	if (result == 1)
	{
		hopeToEnterRoom = iter->second;
		//g_RoomMap.at(0)->participants.erase(session);
		//hopeToEnterRoom->participants.emplace(session);
		EnterRoom(hopeToEnterRoom, session);
	}
	if (hopeToEnterRoom != nullptr)
	{
		session->currentRoomNumber = hopeToEnterRoom->roomNumber;
		univ_dev::Packet enterOtherPersonPacket;
		MakePacketResponseEnterOtherPerson(enterOtherPersonPacket, session, hopeToEnterRoom);
		SendMulticast(hopeToEnterRoom, session, enterOtherPersonPacket);
		//그방의 사람들에게 메시지를 보내줘야됨.
	}

	univ_dev::Packet* pSendPacket = g_PacketPool.Alloc();
	pSendPacket->Clear();
	MakePacketResponseEnterRoom(*pSendPacket, result, hopeToEnterRoom);
	SendUnicast(session, *pSendPacket);
	pSendPacket->Clear();
	g_PacketPool.Free(pSendPacket);
}
void PacketProcRequireLeaveRoom(univ_dev::Session* session, univ_dev::Packet& packet, const st_PACKET_HEADER& header)
{
	if (!session->logined)
	{
		g_Disconnect_IsNotLogined++;
		DisconnectProc(session);
		return;
	}
	else if (session->currentRoomNumber == LOBBY_ROOM_NUMBER)
	{
		g_Disconnect_NotLobbyLeaveRoom++;
		DisconnectProc(session);
		return;
	}
	auto iter = g_RoomMap.find(session->currentRoomNumber);
	if (iter == g_RoomMap.end())
	{
		//? 이사람은... 잘못된 방에 있었던 것인가...
		int* ptr = nullptr;
		*ptr = 100;
		return;
	}
	univ_dev::Room* currentRoom = iter->second;
	univ_dev::Packet* pSendPacket = g_PacketPool.Alloc();
	pSendPacket->Clear();
	LeaveRoom(currentRoom, session);
	//currentRoom->participants.erase(session);
	//g_RoomMap.at(0)->participants.emplace(session);
	//session->currentRoomNumber = LOBBY_ROOM_NUMBER;
	MakePacketResponseLeaveRoom(*pSendPacket, session, session->sessionNo);

	SendUnicast(session, *pSendPacket);
	SendMulticast(currentRoom, session, *pSendPacket);
	pSendPacket->Clear();
	g_PacketPool.Free(pSendPacket);

	if (currentRoom->roomNumber != 0 && currentRoom->participants.size() > 0)return;
	univ_dev::Packet* pRemoveRoomPacket = g_PacketPool.Alloc();
	pRemoveRoomPacket->Clear();
	MakePacketResponseRemoveRoom(*pRemoveRoomPacket, currentRoom->roomNumber);
	SendBroadCast(*pRemoveRoomPacket);
	RemoveRoom(currentRoom);
	pRemoveRoomPacket->Clear();
	g_PacketPool.Free(pRemoveRoomPacket);
}
void PacketProcRequireChat(univ_dev::Session* session, univ_dev::Packet& packet, const st_PACKET_HEADER& header)
{
	if (!session->logined)
	{
		g_Disconnect_IsNotLogined++;
		DisconnectProc(session);
		return;
	}
	else if (session->currentRoomNumber <= 0)
	{
		g_Disconnect_NotInRoomChatting++;
		DisconnectProc(session);
		return;
	}
	univ_dev::Room* currentRoom = nullptr;
	WORD messageSize = 0;
	packet >> messageSize;
	
	auto iter = g_RoomMap.find(session->currentRoomNumber);
	if (iter == g_RoomMap.end())
	{
		//존나 critical한 문제임
		int* ptr = nullptr;
		*ptr = 100;
		return;
	}
	currentRoom = iter->second;
	univ_dev::Packet* pSendPacket = g_PacketPool.Alloc();
	pSendPacket->Clear();
	MakePacketResponseChat(*pSendPacket, session, session->sessionNo, messageSize, (WCHAR*)packet.GetReadPtr());

	SendMulticast(currentRoom, session, *pSendPacket);
	pSendPacket->Clear();
	g_PacketPool.Free(pSendPacket);
}
void PacketProcRequireStressEchoTest(univ_dev::Session* session, univ_dev::Packet& packet, const st_PACKET_HEADER& header)
{
	WORD msgByte;
	packet >> msgByte;
	WCHAR* message = (WCHAR*)packet.GetReadPtr();
	//printf("%d\n", msgByte);
	univ_dev::Packet* pSendPacket = g_PacketPool.Alloc();
	pSendPacket->Clear();
	MakePacketResponseStressEchoTest(*pSendPacket, msgByte, message, header);

	SendUnicast(session, *pSendPacket);
	pSendPacket->Clear();
	g_PacketPool.Free(pSendPacket);
}
///------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------



///------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
/// 패킷 생성 함수 정의
/// Make Packet Function Define
void MakePacketResponseLogin(univ_dev::Packet& packet, BYTE result, DWORD userID)
{
	st_PACKET_HEADER header;
	header.byCode = 0x89;
	header.byCheckSum = 0;
	header.wMsgType = df_RES_LOGIN;
	header.wPayloadSize = df_RES_LOGIN_SIZE;

	//int checkSum = header.wMsgType + result;
	//char* tempPtr = (char*)&userID;
	//for (int i = 0; i < sizeof(DWORD); i++)
	//	checkSum += *(tempPtr + i);
	//header.byCheckSum = checkSum % 256;
	packet << header.byCode << header.byCheckSum << header.wMsgType << header.wPayloadSize << result << userID;

	WORD payloadSize = packet.GetBufferSize() - (sizeof(header.byCode) + sizeof(header.byCheckSum) + sizeof(header.wMsgType) + sizeof(header.wPayloadSize));
	char* checkSumPtr = packet.GetReadPtr();
	checkSumPtr++;
	*checkSumPtr = MakeCheckSum(header.wMsgType, payloadSize, packet);
	checkSumPtr += sizeof(header.byCheckSum) + sizeof(header.wMsgType);
	WORD* payloadPtr = (WORD*)checkSumPtr;
	*payloadPtr = payloadSize;

}
void MakePacketResponseRoomList(univ_dev::Packet& packet, WORD numOfRoom)
{
	st_PACKET_HEADER header;
	header.byCode = 0x89;
	header.byCheckSum = 0;
	header.wMsgType = df_RES_ROOM_LIST;
	header.wPayloadSize = 0;

	packet << header.byCode << header.byCheckSum << header.wMsgType << header.wPayloadSize << numOfRoom;
	//8
	//printf("numOfRoom : %d\n", numOfRoom);
	//char* checkSumPtr = packet.GetReadPtr();
	//checkSumPtr++;
	//char* tempPtr = packet.GetReadPtr();
	//tempPtr += sizeof(st_PACKET_HEADER);
	for (auto iter = g_RoomMap.begin(); iter != g_RoomMap.end(); ++iter)
	{
		univ_dev::Room* currentRoom = iter->second;
		if (currentRoom->roomNumber == 0) continue;
		packet << currentRoom->roomNumber;
		//printf("roomNumber : %d\n", currentRoom->roomNumber);
		WORD roomNameByte = wcslen(currentRoom->roomName) * sizeof(WCHAR);
		//printf("roomNameByte : %d\n", roomNameByte);
		packet << roomNameByte;
		memcpy_s(packet.GetWritePtr(), roomNameByte, currentRoom->roomName, roomNameByte);
		//wprintf(L"currentRoom->roomName : %s\n", currentRoom->roomName);
		//printf("\n");
		packet.MoveWritePtr(roomNameByte);
		BYTE numOfRoomParticipant = currentRoom->participants.size();
		packet << numOfRoomParticipant;
		for (auto iter = currentRoom->participants.begin(); iter != currentRoom->participants.end(); ++iter)
		{
			univ_dev::Session* currentSession = *iter;
			int nickNameByte = wcslen(currentSession->sessionNickName) * sizeof(WCHAR);
			memcpy_s(packet.GetWritePtr(), nickNameByte, currentSession->sessionNickName, nickNameByte);
			packet.MoveWritePtr(nickNameByte);
		}
	}
	WORD payloadSize = packet.GetBufferSize() - (sizeof(header.byCode) + sizeof(header.byCheckSum) + sizeof(header.wMsgType) + sizeof(header.wPayloadSize));
	char* checkSumPtr = packet.GetReadPtr();
	checkSumPtr++;
	*checkSumPtr = MakeCheckSum(header.wMsgType, payloadSize, packet);
	checkSumPtr += sizeof(header.byCheckSum) + sizeof(header.wMsgType);
	WORD* payloadPtr = (WORD*)checkSumPtr;
	*payloadPtr = payloadSize;
	//WORD payloadSize = packet.GetBufferSize() - (sizeof(header.byCode) + sizeof(header.byCheckSum) + sizeof(header.wMsgType) + sizeof(header.wPayloadSize));
	//int checkSum = header.wMsgType;
	//for (int i = 0; i < payloadSize; i++)
	//	checkSum += *(tempPtr + i);
	//*checkSumPtr = (checkSum % 256);
	//char* tempPayloadPtr = checkSumPtr + (sizeof(header.byCheckSum) + sizeof(header.wMsgType));
	//WORD* payloadPtr = (WORD*)tempPayloadPtr;
	//*payloadPtr = payloadSize;
}
void MakePacketResponseMakeRoom(univ_dev::Packet& packet, BYTE result, DWORD roomNumber, WORD roomNameByte, const WCHAR* roomName)
{
	st_PACKET_HEADER header;
	header.byCode = 0x89;
	header.byCheckSum = 0;
	header.wMsgType = df_RES_ROOM_CREATE;
	header.wPayloadSize = sizeof(BYTE) + sizeof(DWORD) + sizeof(WORD) + roomNameByte;

	//int checkSum = df_RES_ROOM_CREATE + result;
	//const char* tempPtr = (const char*)&roomNumber;
	//for (int i = 0; i < sizeof(DWORD); i++)
	//	checkSum += *(tempPtr + i);
	//tempPtr = (const char*)&roomNameByte;
	//for (int i = 0; i < sizeof(WORD); i++)
	//	checkSum += *(tempPtr + i);
	//tempPtr = (const char*)roomName;
	//for (int i = 0; i < roomNameByte; i++)
	//	checkSum += *((char*)roomName + i);
	//header.byCheckSum = checkSum % 256;
	//printf("send checksum %d\n", header.byCheckSum);
	packet << header.byCode << header.byCheckSum << header.wMsgType << header.wPayloadSize << result << roomNumber << roomNameByte;
	memcpy_s(packet.GetWritePtr(), roomNameByte, roomName, roomNameByte);
	packet.MoveWritePtr(roomNameByte);
	WORD payloadSize = packet.GetBufferSize() - (sizeof(header.byCode) + sizeof(header.byCheckSum) + sizeof(header.wMsgType) + sizeof(header.wPayloadSize));
	char* checkSumPtr = packet.GetReadPtr();
	checkSumPtr++;
	*checkSumPtr = MakeCheckSum(header.wMsgType, payloadSize, packet);
	checkSumPtr += sizeof(header.byCheckSum) + sizeof(header.wMsgType);
	WORD* payloadPtr = (WORD*)checkSumPtr;
	*payloadPtr = payloadSize;
}
void MakePacketResponseEnterRoom(univ_dev::Packet& packet, BYTE result, univ_dev::Room* room)
{
	st_PACKET_HEADER header;
	header.byCode = 0x89;
	header.byCheckSum = 0;
	header.wMsgType = df_RES_ROOM_ENTER;
	header.wPayloadSize = 0;

	packet << header.byCode << header.byCheckSum << header.wMsgType << header.wPayloadSize << result;
	if (result != 1) return;

	packet << room->roomNumber;
	WORD roomNameByte = wcslen(room->roomName) * sizeof(WCHAR);
	packet << roomNameByte;
	memcpy_s(packet.GetWritePtr(), roomNameByte, room->roomName, roomNameByte);
	packet.MoveWritePtr(roomNameByte);
	BYTE numOfParticipant = room->participants.size();
	packet << numOfParticipant;

	for (auto userIter = room->participants.begin(); userIter != room->participants.end(); ++userIter)
	{
		univ_dev::Session* currentUser = *userIter;
		memcpy_s(packet.GetWritePtr(), dfNICK_MAX_LEN * sizeof(WCHAR), currentUser->sessionNickName, dfNICK_MAX_LEN * sizeof(WCHAR));
		packet.MoveWritePtr(dfNICK_MAX_LEN * sizeof(WCHAR));
		packet << currentUser->sessionNo;
	}
	WORD payloadSize = packet.GetBufferSize() - (sizeof(header.byCode) + sizeof(header.byCheckSum) + sizeof(header.wMsgType) + sizeof(header.wPayloadSize));
	char* checkSumPtr = packet.GetReadPtr();
	checkSumPtr++;
	*checkSumPtr = MakeCheckSum(header.wMsgType, payloadSize, packet);
	checkSumPtr += sizeof(header.byCheckSum) + sizeof(header.wMsgType);
	WORD* payloadPtr = (WORD*)checkSumPtr;
	*payloadPtr = payloadSize;
}
void MakePacketResponseEnterOtherPerson(univ_dev::Packet& packet, univ_dev::Session* otherUser, univ_dev::Room* room)
{
	st_PACKET_HEADER header;
	header.byCode = dfPACKET_CODE;
	header.byCheckSum = 0;
	header.wMsgType = df_RES_USER_ENTER;
	header.wPayloadSize = 0;

	packet << header.byCode << header.byCheckSum << header.wMsgType << header.wPayloadSize;

	memcpy_s(packet.GetWritePtr(), dfNICK_MAX_LEN * sizeof(WCHAR), otherUser->sessionNickName, dfNICK_MAX_LEN * sizeof(WCHAR));
	packet.MoveWritePtr(dfNICK_MAX_LEN * sizeof(WCHAR));
	packet << otherUser->sessionNo;


	WORD payloadSize = packet.GetBufferSize() - (sizeof(header.byCode) + sizeof(header.byCheckSum) + sizeof(header.wMsgType) + sizeof(header.wPayloadSize));
	char* checkSumPtr = packet.GetReadPtr();
	checkSumPtr++;
	*checkSumPtr = MakeCheckSum(header.wMsgType, payloadSize, packet);
	checkSumPtr += sizeof(header.byCheckSum) + sizeof(header.wMsgType);
	WORD* payloadPtr = (WORD*)checkSumPtr;
	*payloadPtr = payloadSize;
}
void MakePacketResponseLeaveRoom(univ_dev::Packet& packet, univ_dev::Session* session, DWORD userNo)
{
	st_PACKET_HEADER header;
	header.byCode = dfPACKET_CODE;
	header.byCheckSum = 0;
	header.wMsgType = df_RES_ROOM_LEAVE;
	header.wPayloadSize = 0;

	packet << header.byCode << header.byCheckSum << header.wMsgType << header.wPayloadSize << userNo;

	WORD payloadSize = packet.GetBufferSize() - (sizeof(header.byCode) + sizeof(header.byCheckSum) + sizeof(header.wMsgType) + sizeof(header.wPayloadSize));
	char* checkSumPtr = packet.GetReadPtr();
	checkSumPtr++;
	*checkSumPtr = MakeCheckSum(header.wMsgType, payloadSize, packet);
	checkSumPtr += sizeof(header.byCheckSum) + sizeof(header.wMsgType);
	WORD* payloadPtr = (WORD*)checkSumPtr;
	*payloadPtr = payloadSize;
}
void MakePacketResponseRemoveRoom(univ_dev::Packet& packet, DWORD roomNo)
{
	st_PACKET_HEADER header;
	header.byCode = dfPACKET_CODE;
	header.byCheckSum = 0;
	header.wMsgType = df_RES_ROOM_DELETE;
	header.wPayloadSize = 0;

	packet << header.byCode << header.byCheckSum << header.wMsgType << header.wPayloadSize << roomNo;

	WORD payloadSize = packet.GetBufferSize() - (sizeof(header.byCode) + sizeof(header.byCheckSum) + sizeof(header.wMsgType) + sizeof(header.wPayloadSize));
	char* checkSumPtr = packet.GetReadPtr();
	checkSumPtr++;
	*checkSumPtr = MakeCheckSum(header.wMsgType, payloadSize, packet);
	checkSumPtr += sizeof(header.byCheckSum) + sizeof(header.wMsgType);
	WORD* payloadPtr = (WORD*)checkSumPtr;
	*payloadPtr = payloadSize;
}
void MakePacketResponseChat(univ_dev::Packet& packet, univ_dev::Session* session, DWORD userNo, WORD msgByte, const WCHAR* message)
{
	st_PACKET_HEADER header;
	header.byCode = dfPACKET_CODE;
	header.byCheckSum = 0;
	header.wMsgType = df_RES_CHAT;
	header.wPayloadSize = 0;

	//wprintf(L"user name : %s\n", session->sessionNickName);
	//wprintf(L"user number : %d\n", session->sessionNo);
	//message[msgByte / 2] = L'\0';
	//wprintf(L"MakePacketResponseChat ::::: %s\n", message);
	packet << header.byCode << header.byCheckSum << header.wMsgType << header.wPayloadSize << userNo << msgByte;

	memcpy_s(packet.GetWritePtr(), msgByte, message, msgByte);
	packet.MoveWritePtr(msgByte);

	WORD payloadSize = packet.GetBufferSize() - (sizeof(header.byCode) + sizeof(header.byCheckSum) + sizeof(header.wMsgType) + sizeof(header.wPayloadSize));
	char* checkSumPtr = packet.GetReadPtr();
	checkSumPtr++;
	*checkSumPtr = MakeCheckSum(header.wMsgType, payloadSize, packet);
	checkSumPtr += sizeof(header.byCheckSum) + sizeof(header.wMsgType);
	WORD* payloadPtr = (WORD*)checkSumPtr;
	*payloadPtr = payloadSize;
}
void MakePacketResponseStressEchoTest(univ_dev::Packet& packet, WORD msgByte, const WCHAR* message, const st_PACKET_HEADER& header)
{
	st_PACKET_HEADER sendHeader;
	sendHeader.byCode = dfPACKET_CODE;
	sendHeader.byCheckSum = (header.byCheckSum + 1) % 256;
	sendHeader.wMsgType = df_RES_STRESS_ECHO;
	sendHeader.wPayloadSize = header.wPayloadSize;

	packet << sendHeader.byCode << sendHeader.byCheckSum << sendHeader.wMsgType << sendHeader.wPayloadSize << msgByte;
	memcpy_s(packet.GetWritePtr(), msgByte, message, msgByte);
	packet.MoveWritePtr(msgByte);
	WORD payloadSize = packet.GetBufferSize() - (sizeof(header.byCode) + sizeof(header.byCheckSum) + sizeof(header.wMsgType) + sizeof(header.wPayloadSize));
	char* checkSumPtr = packet.GetReadPtr();
	checkSumPtr++;
	int cs  = MakeCheckSum(sendHeader.wMsgType, payloadSize, packet);
	*checkSumPtr = cs;
	if (cs != header.byCheckSum + 1)
	{
		int a = 10;
		a++;
	}
	checkSumPtr += sizeof(header.byCheckSum) + sizeof(header.wMsgType);
	WORD* payloadPtr = (WORD*)checkSumPtr;
	*payloadPtr = payloadSize;
}
///------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


///------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
/// Send함수 정의
/// Send Function Define
void SendUnicast(univ_dev::Session* targetSession, univ_dev::Packet& packet)
{
	if (targetSession == nullptr)
	{
		//printf("SendUnicast Error : Target is nullptr\n");
		int* ptr = nullptr;
		*ptr = 10;
		return;
	}
	targetSession->SQ.Enqueue(packet.GetReadPtr(), packet.GetBufferSize());
	PushSendList(targetSession);
}
void SendBroadCast(univ_dev::Packet& packet)
{
	univ_dev::Session* currentSession;
	auto iter = g_SessionMap.begin();
	for (; iter != g_SessionMap.end(); ++iter)
	{
		currentSession = iter->second;
		SendUnicast(currentSession, packet);
	}
}
void SendMulticast(univ_dev::Room* targetRoom, univ_dev::Session* exSession, univ_dev::Packet& packet)
{
	univ_dev::Session* currentSession;
	auto iter = targetRoom->participants.begin();
	//printf("current room number : %d\n", targetRoom->roomNumber);
	//wprintf(L"current room name : %s", targetRoom->roomName);
	for (; iter != targetRoom->participants.end(); ++iter)
	{
		currentSession = *iter;
		//printf("current session number : %d\n", currentSession->sessionNo);
		//wprintf(L"current session name : %s\n", currentSession->sessionNickName);
		if (currentSession->sessionNo != exSession->sessionNo)
			SendUnicast(currentSession, packet);
	}
}
void PushSendList(univ_dev::Session* session)
{
	g_SendOnSessionList.emplace(session);
}
///------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


///------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
/// 연결 종료 함수 정의
/// Disconnect Function Define
void DisconnectProc(univ_dev::Session* session)
{
	g_DisconnectSession.emplace(session);
}
void Disconnect()
{
	for (auto iter = g_DisconnectSession.begin(); iter != g_DisconnectSession.end(); ++iter)
	{
		auto removeIter = g_SessionMap.find((*iter)->sessionNo);
		if (removeIter != g_SessionMap.end())
		{
			univ_dev::Session* removeSession = removeIter->second;
			if (removeSession->currentRoomNumber != -1 && removeSession->currentRoomNumber != 0)
			{
				univ_dev::Room* currentRoom;
				auto iter = g_RoomMap.find(removeSession->currentRoomNumber);
				if (iter == g_RoomMap.end())
				{
					//이건... 큰문제다 방금까지 방이 있었는데 갑자기 방이없어졌다?
					int* ptr = nullptr;
					*ptr = 100;
					return;
				}
				currentRoom = iter->second;
				univ_dev::Packet* pPacket = g_PacketPool.Alloc();
				pPacket->Clear();
				MakePacketResponseLeaveRoom(*pPacket, removeSession, removeSession->sessionNo);
				SendMulticast(currentRoom, removeSession, *pPacket);
				pPacket->Clear();
				g_PacketPool.Free(pPacket);
				currentRoom->participants.erase(removeSession);

				if (currentRoom->participants.size() <= 0)
				{
					univ_dev::Packet* pRemoveRoomPacket = g_PacketPool.Alloc();
					pRemoveRoomPacket->Clear();
					MakePacketResponseRemoveRoom(*pRemoveRoomPacket, currentRoom->roomNumber);
					SendBroadCast(*pRemoveRoomPacket);
					pRemoveRoomPacket->Clear();
					g_PacketPool.Free(pRemoveRoomPacket);
				}
			}
			closesocket(removeSession->sock);
			removeSession->SQ.ClearBuffer();
			removeSession->RQ.ClearBuffer();

			int sessionNumber = removeSession->sessionNo;
			g_SessionMap.erase(sessionNumber);
			g_UsedNameSet.erase(removeSession->sessionNickName);
			g_SessionPool.Free(removeSession);
			g_DisconnectCount++;
			//printf("client disconnect...\n");
		}
	}
	g_DisconnectSession.clear();
}
///------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

///------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
/// 체크섬 생성 함수 정의
/// Check Sum Function Define
int MakeCheckSum(WORD msgType, WORD payloadSize, univ_dev::Packet& packet)
{
	int checkSum = msgType;

	char* tempPtr = packet.GetReadPtr();
	tempPtr += sizeof(st_PACKET_HEADER);

	for (int i = 0; i < payloadSize; i++)
		checkSum += *(tempPtr + i);
	return checkSum % 256;
///------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
}
///------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

///------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
/// 기본 프로시져 정의
/// Basic Procedure Define
void SendProc()
{
	for (auto iter = g_SendOnSessionList.begin(); iter != g_SendOnSessionList.end(); ++iter)
	{
		univ_dev::Session* currentSession = *iter;
		int useSize = currentSession->SQ.GetUseSize();
		int sendRet = send(currentSession->sock, currentSession->SQ.GetReadPtr(), currentSession->SQ.DirectDequeueSize(), 0);
		int secondSendRet = 0;
		if (sendRet == SOCKET_ERROR)
		{
			int err = WSAGetLastError();
			if (err != WSAEWOULDBLOCK)
			{
				g_Disconnect_SendErrIsNotBLOCK++;
				DisconnectProc(currentSession);
				continue;
			}
		}
		currentSession->SQ.MoveReadPtr(sendRet);
		if (useSize != sendRet)
		{
			secondSendRet = send(currentSession->sock, currentSession->SQ.GetReadPtr(), currentSession->SQ.DirectDequeueSize(), 0);
			if (sendRet == SOCKET_ERROR)
			{
				int err = WSAGetLastError();
				if (err != WSAEWOULDBLOCK)
				{
					g_Disconnect_SendErrIsNotBLOCK++;
					DisconnectProc(currentSession);
					continue;
				}
			}
		}
		if (useSize != sendRet + secondSendRet)
		{
			g_Disconnect_RingBufferProb++;
			DisconnectProc(currentSession);
			continue;
		}
		if (currentSession->SQ.GetUseSize() == 0)
			currentSession->SQ.ClearBuffer();
		g_SendPerSec++;
	}
	g_SendOnSessionList.clear();
}
void ReadProc(univ_dev::Session* session)
{
	int recvBytes = recv(session->sock, session->RQ.GetWritePtr(), session->RQ.DirectEnqueueSize(), 0);
	if (!recvBytes)
	{
		//disconnect

		DisconnectProc(session);
		//g_DisconnectSession.emplace(session);
		return;
	}
	if (recvBytes == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK)
		{
			//진짜문제있음.
			g_Disconnect_RECV_ERR_IsNotBlock++;
			g_Disconnect_RSTSignal++;
			DisconnectProc(session);
			return;
		}
	}
	session->RQ.MoveWritePtr(recvBytes);

	while (true)
	{
		st_PACKET_HEADER header;
		int rqUseSize = session->RQ.GetUseSize();
		if (rqUseSize < sizeof(st_PACKET_HEADER)) break;
		int headerPeekRet = session->RQ.Peek((char*)&header, sizeof(st_PACKET_HEADER));
		if (header.byCode != 0x89)
		{
			//잘못된 클라이언트는 삭제해버려야지
			g_Disconnect_PacketCodeError++;
			DisconnectProc(session);
			return;
		}
		if (rqUseSize < sizeof(header) + header.wPayloadSize) break;

		session->RQ.MoveReadPtr(sizeof(header));
		univ_dev::Packet* pPacket = g_PacketPool.Alloc();
		pPacket->Clear();
		int directDQSize = session->RQ.DirectDequeueSize();
		int pkPeekRet = session->RQ.Peek(pPacket->GetWritePtr(), min(header.wPayloadSize, directDQSize));
		//int tempPeekRet = 0;
		//if (pkPeekRet < header.wPayloadSize)
		//{
		//	session->RQ.MoveReadPtr(pkPeekRet);
		//	tempPeekRet = session->RQ.Peek(packet.GetWritePtr(), header.wPayloadSize - pkPeekRet);
		//	if (pkPeekRet + tempPeekRet != header.wPayloadSize)
		//	{
		//		int* ptr = nullptr;
		//		*ptr = 100;
		//	}
		//}
		session->RQ.MoveReadPtr(pkPeekRet);
		int moveSize = pPacket->MoveWritePtr(header.wPayloadSize);
		//printf("payload size : %d\n", header.wPayloadSize);
		char* ptr = pPacket->GetReadPtr();
		DWORD checkSum = header.wMsgType;
		for (int i = 0; i < header.wPayloadSize; i++)
			checkSum += *(ptr + i);
		//printf("recv checkSum %d\n", checkSum % 256);
		if (checkSum % 256 != header.byCheckSum)
		{
			//printf("wrong checksum");
			g_Disconnect_CheckSumError++;
			DisconnectProc(session);
			return;
		}

		PacketProc(header, session, *pPacket);
		pPacket->Clear();
		g_PacketPool.Free(pPacket);
	}
	int useSize = session->RQ.GetUseSize();
	//printf("----------current use size : %d\n", useSize);
	if (useSize == 0)
		session->RQ.ClearBuffer();
}
void AcceptProc()
{
	sockaddr_in addr;
	int addrlen = sizeof(addr);
	SOCKET sock = accept(g_ListenSocket, (sockaddr*)&addr, &addrlen);
	if (sock == INVALID_SOCKET)
	{
		//printf("client connect failed\n");
		g_Disconnect_AcceptErrorInvalidSocket++;
		return;
	}
	//univ_dev::Session* newSession = new	univ_dev::Session();
	univ_dev::Session* newSession = g_SessionPool.Alloc();

	newSession->sessionNo = g_ClientNumber++;
	newSession->sock = sock;
	newSession->RQ.ClearBuffer();
	newSession->SQ.ClearBuffer();
	newSession->logined = false;
	newSession->currentRoomNumber = -1;
	memset(newSession->sessionNickName, 0, dfNICK_MAX_LEN * sizeof(WCHAR));
	g_SessionMap.emplace(std::make_pair(newSession->sessionNo, newSession));
	//printf("Accept - %d.%d.%d.%d:%d [UserNO:%d]\n", addr.sin_addr.S_un.S_un_b.s_b1,
	//	addr.sin_addr.S_un.S_un_b.s_b2,
	//	addr.sin_addr.S_un.S_un_b.s_b3,
	//	addr.sin_addr.S_un.S_un_b.s_b4,
	//	ntohs(addr.sin_port), newSession->sessionNo);
	g_AcceptPerSec++;
}
void PacketProc(st_PACKET_HEADER header,univ_dev::Session* session ,univ_dev::Packet& packet)
{
	g_RecvPerSec++;
	switch (header.wMsgType)
	{
		case df_REQ_LOGIN:
		{
			PacketProcRequireLogin(session, packet, header);
			break;
		}
		case df_REQ_ROOM_LIST:
		{
			PacketProcRequireRoomList(session, packet, header);
			break;
		}
		case df_REQ_ROOM_CREATE:
		{
			PacketProcRequireMakeRoom(session, packet, header);
			break;
		}
		case df_REQ_ROOM_ENTER:
		{
			PacketProcRequireEnterRoom(session, packet, header);
			break;
		}
		case df_REQ_CHAT:
		{
			PacketProcRequireChat(session, packet, header);
			break;
		}
		case df_REQ_ROOM_LEAVE:
		{
			PacketProcRequireLeaveRoom(session, packet, header);
			break;
		}
		case df_REQ_STRESS_ECHO:
		{
			PacketProcRequireStressEchoTest(session, packet, header);
			break;
		}
		default:
		{
			//printf("default case...?\n");
			g_Disconnect_DefaultCase++;
			DisconnectProc(session);
			break;
		}
	}
}
///------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


///------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
/// 네트워크 초기화 함수 정의
/// Network Initializing Function Define
SOCKET NetworkInit(unsigned short port)
{
	WSADATA data;
	if (WSAStartup(MAKEWORD(2, 2), &data)) return INVALID_SOCKET;

	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
	{
		WSACleanup();
		return INVALID_SOCKET;
	}

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	int bindRet = bind(sock, (sockaddr*)&addr, sizeof(addr));
	if (bindRet == SOCKET_ERROR)
	{
		closesocket(sock);
		WSACleanup();
		return INVALID_SOCKET;
	}

	int listenRet = listen(sock, SOMAXCONN_HINT(1000));
	if (listenRet == SOCKET_ERROR)
	{
		closesocket(sock);
		WSACleanup();
		return INVALID_SOCKET;
	}

	linger l;
	l.l_onoff = 1;
	l.l_linger = 0;
	setsockopt(sock, SOL_SOCKET, SO_LINGER, (const char*)&l, sizeof(l));
	u_long blockingMode = 1;
	ioctlsocket(sock, FIONBIO, &blockingMode);
	return sock;
}
///------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


///------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
/// 프로파일링용 함수 정의
/// Profiling Function Define
void SaveServerStatus(clock_t current, clock_t g_beginTimer)
{
	FILE* file;

	time_t timer = time(nullptr);
	tm t;
	localtime_s(&t, &timer);
	char fileName[100]{ 0 };
	
	char tempBuffer[20]{ 0 };
	
	strcat_s(fileName, "ServerStatus\\");
	_itoa_s(t.tm_year + 1900, tempBuffer, 10);
	strcat_s(fileName, tempBuffer);
	strcat_s(fileName, "_");
	_itoa_s(t.tm_mon + 1, tempBuffer, 10);
	strcat_s(fileName, tempBuffer);
	strcat_s(fileName, "_");
	_itoa_s(t.tm_mday, tempBuffer, 10);
	strcat_s(fileName, tempBuffer);
	strcat_s(fileName, "_");
	_itoa_s(t.tm_hour + 1, tempBuffer, 10);
	strcat_s(fileName, tempBuffer);
	strcat_s(fileName, "_");
	_itoa_s(t.tm_min, tempBuffer, 10);
	strcat_s(fileName, tempBuffer);
	strcat_s(fileName, "_");
	_itoa_s(t.tm_sec, tempBuffer, 10);
	strcat_s(fileName, tempBuffer);
	strcat_s(fileName, "_ChatServerStatus.csv");

	printf(fileName);
	fopen_s(&file, fileName, "wb");
	if (file == nullptr) return;

	fprintf(file, "Current User Count,%lld\n", g_SessionMap.size());
	fprintf(file, "Accept Per Sec,%u\n", g_AcceptPerSec);
	fprintf(file, "Recv Per sec,%u\n", g_RecvPerSec);
	fprintf(file, "Send Per sec,%u\n", g_SendPerSec);
	fprintf(file, "Connect Per sec,%u\n", g_ConnectPerSec);
	fprintf(file, "Connect Fail Per sec,%u\n", g_ConnectFailPerSec);
	fprintf(file, "Disconnect Per sec,%u\n\n\n", g_DisconnectCount);

	clock_t time = current - g_beginTimer;
	time /= 1000;
	fprintf(file, "AVR User Count,%llf\n", (double)g_SessionMap.size() / (double)time);
	fprintf(file, "AVR Accept Per Sec,%llf\n", (double)g_CumerativeAcceptCount / (double)time);
	fprintf(file, "AVR Recv Per Sec,%llf\n", (double)g_CumerativeRecvCount / (double)time);
	fprintf(file, "AVR Send Per Sec,%llf\n", (double)g_CumerativeSendCount / (double)time);
	fprintf(file, "AVR Connect Per Sec,%llf\n", (double)g_CumerativeConnectCount / (double)time);
	fprintf(file, "AVR Connect Fail Per Sec,%llf\n", (double)g_CumerativeConnectFailCount / (double)time);
	fprintf(file, "AVR Disconnect Per Sec,%llf\n\n\n", (double)g_CumerativeDisconnectCount / (double)time);

	fprintf(file, "TOTAL User Count,%lld\n", g_SessionMap.size());
	fprintf(file, "TOTAL Accept Per Sec,%llu\n", g_CumerativeAcceptCount);
	fprintf(file, "TOTAL Recv Per Sec,%llu\n", g_CumerativeRecvCount);
	fprintf(file, "TOTAL Send Per Sec,%llu\n", g_CumerativeSendCount);
	fprintf(file, "TOTAL Connect Per Sec,%llu\n", g_CumerativeConnectCount);
	fprintf(file, "TOTAL Connect Fail Per Sec,%llu\n", g_CumerativeConnectFailCount);
	fprintf(file, "TOTAL Disconnect Per Sec,%llu\n\n\n", g_CumerativeDisconnectCount);


	fprintf(file, "CONTENT ERROR\n");
	fprintf(file, "DISCONNECT_IS_NOT_LOGIN_COUNT,%llu\n", g_Disconnect_IsNotLogined);
	fprintf(file, "DISCONNECT_NOT_LOBBY_LEAVE_ROOM,%llu\n", g_Disconnect_NotLobbyLeaveRoom);
	fprintf(file, "DISCONNECT_NOT_IN_ROOM_CHAT,%llu\n", g_Disconnect_NotInRoomChatting);
	fprintf(file, "DISCONNECT_SESSION_NAME_DUPLICATE,%llu\n", g_Disconnect_SessionNameDuplicate);
	fprintf(file, "DISCONNECT_SESSION_COUNTS_OVER_7000,%llu\n\n", g_Disconnect_SessionOver7000);
	fprintf(file, "DISCONNECT_CHECK_DEFAULT_CASE,%llu\n", g_Disconnect_DefaultCase); 
	fprintf(file, "NETWORK ERROR\n"); 
	fprintf(file, "DISCONNECT_ACCEPT_ERROR_INVALID_SOCKET,%llu\n", g_Disconnect_AcceptErrorInvalidSocket);
	fprintf(file, "DISCONNECT_SEND_ERROR_IS_NOT_BLOCK,%llu\n", g_Disconnect_SendErrIsNotBLOCK);
	fprintf(file, "DISCONNECT_RING_BUFFER_PROBLEM,%llu\n", g_Disconnect_RingBufferProb);
	fprintf(file, "DISCONNECT_DISCONNECT_RST_SIGNAL,%llu\n", g_Disconnect_RSTSignal);
	fprintf(file, "DISCONNECT_DISCONNECT_FIN_SIGNAL,%llu\n", g_Disconnect_FINSignal);
	fprintf(file, "DISCONNECT_RECV_ERROR_IS_NOT_BLOCK,%llu\n", g_Disconnect_RECV_ERR_IsNotBlock);
	fprintf(file, "DISCONNECT_PACKET_CODE_ERROR,%llu\n", g_Disconnect_PacketCodeError);
	fprintf(file, "DISCONNECT_CHECK_SUM_ERROR,%llu\n", g_Disconnect_CheckSumError);

	fprintf(file, "MEMORY FREE LIST\n");
	fprintf(file, "ROOM_POOL_CAPACITY,%d\n", g_RoomPool.GetCapacityCount());
	fprintf(file, "ROOM_POOL_USE_COUNT,%d\n", g_RoomPool.GetUseCount());
	fprintf(file, "SESSION_POOL_CAPACITY,%d\n", g_SessionPool.GetCapacityCount());
	fprintf(file, "SESSION_POOL_CAPACITY,%d\n", g_SessionPool.GetUseCount());
	fprintf(file, "RINB_BUFFER_POOL_CAPACITY,%d\n", g_SessionPool.GetCapacityCount() * 2);
	fprintf(file, "RINB_BUFFER_POOL_USE_COUNT,%d\n", g_SessionPool.GetCapacityCount() * 2);
	fprintf(file, "PACKET_BUFFER_POOL_CAPACITY,%d\n", g_PacketPool.GetCapacityCount());
	fprintf(file, "PACKET_BUFFER_POOL_USE_COUNT,%d\n", g_PacketPool.GetUseCount());



	fclose(file);
}
void PrintLog()
{
	printf("\n-----------------------------\n");
	printf("Current User Count : %lld\n", g_SessionMap.size());
	printf("Accept Per Sec : %u\n", g_AcceptPerSec);
	printf("Recv Per sec : %u\n", g_RecvPerSec);
	printf("Send Per sec : %u\n", g_SendPerSec);
	printf("Connect Per sec : %u\n", g_ConnectPerSec);
	printf("Connect Fail Per sec : %u\n", g_ConnectFailPerSec);
	printf("Disconnect Per sec : %u\n\n", g_DisconnectCount);

	printf("MEMORY FREE LIST \n");
	printf("Room Memory Free List Capacity : %d\n", g_RoomPool.GetCapacityCount());
	printf("Room Memory Free List Use Count : %d\n", g_RoomPool.GetUseCount());
	printf("Session Memory Free List Capacity : %d\n", g_SessionPool.GetCapacityCount());
	printf("Session Memory Free List Use Count : %d\n", g_SessionPool.GetUseCount());
	printf("Ring Buffer Memory Free List Capacity : %d\n", g_SessionPool.GetCapacityCount() * 2);
	printf("Ring Buffer Memory Free List Use Count : %d\n", g_SessionPool.GetUseCount() * 2);
	printf("Packet Memory Free List Capacity : %d\n", g_PacketPool.GetCapacityCount());
	printf("Packet Memory Free List Use Count : %d\n", g_PacketPool.GetUseCount());
	printf("-----------------------------\n");
	g_CumerativeAcceptCount += g_AcceptPerSec;
	g_CumerativeRecvCount += g_RecvPerSec;
	g_CumerativeSendCount += g_SendPerSec;
	g_CumerativeConnectCount += g_ConnectPerSec;
	g_CumerativeConnectFailCount += g_ConnectFailPerSec;
	g_CumerativeDisconnectCount += g_DisconnectCount;
	g_AcceptPerSec = g_RecvPerSec = g_SendPerSec = g_ConnectPerSec = g_ConnectFailPerSec = g_DisconnectCount = 0;
}
///------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------