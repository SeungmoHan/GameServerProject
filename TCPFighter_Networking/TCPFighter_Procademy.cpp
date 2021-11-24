//#pragma comment(linker, "/entry:wWinMainCRTStartup /subsystem:console")
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"imm32.lib")
#pragma comment(lib,"Winmm.lib")

// TCPFighter_Procademy.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//

#include "framework.h"
#include "TCPFighter_Procademy.h"
#include <vector>
#include <time.h>
#include <timeapi.h>
#include <WS2tcpip.h>
#include <WinSock2.h>
#include "PlayerObject.h"
#include "MakePacketMessage.h"
#include "RingBuffer.h"

#define MAX_LOADSTRING 100


///
/// done  -> 
///         1 ScreenDib 클래스 구현 완료
///         2 SpriteDib 클래스 구현 완료
///         3 파일 제대로 읽는지 확인 완료
///         4 그림이 찍히는지 확인
///         1 BaseObject 만들기
///         2 Player : BaseObject 만들기
///         3 Effect : BaseObject 만들기
///         3-1 Effect : 그림자 + hp 게이지 생기는거 없애기
///         4 FrameSkip 만들기 -> 50frame 맞추기 위해 연산하는 클래스(전역함수로 둬도 ㄱㅊ)
///         5. 네트워크 연결
///         6. 서버는 있고... 프로토콜 구현
///         7. 패킷 생성 프로시져 만들기
///         8. 패킷 분석 프로시져 만들기
///         9. 패킷 프로시저들 형태 통일(해시로 함수포인터로 넘어갈거라서그럼)
///         10. 클라이언트 자체 애니메이션 고장 수정
///         11. 키 꾹눌렀을때 계속 미친듯이 send수정
///         12. 11수정후 키 꾹눌렀을때 EndFrame이 되어도 재전송 하지않음
///         13. recv send에 링버퍼 포인터 바로 박아넣기
///         13-1 그로인한 오류 수정
///         14. serializing buffer packet적용
///         14-1 그로인한 오류 수정
/// 
/// to do ->
/// 





// 전역 변수:
HINSTANCE hInst;                                // 현재 인스턴스입니다.
WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.
HIMC g_hOldIMC;
HWND g_hWnd;

SOCKET g_ClientSocket;

/// 이번 프레임이 20ms가 넘어버리면 프레임 스킵
/// 오버되는 시간 저장할 공간. <- 축적형태로 가야됨
/// 초당 총 루프 반복횟수 프레임수 저장할 공간. v
/// 초당 총 로직 반복횟수 프레임수 저장할 공간. v
/// 1초 측정 타이머 v
std::vector<BaseObject*> g_ObjectList;
PlayerObject* gp_Player;
DWORD g_AllFramePerSec;
DWORD g_LogicFramePerSec;
static bool g_isActive;
bool bSendFlag;
RingBuffer g_RecvRingBuffer;
RingBuffer g_SendRingBuffer;


/// </전역 변수>

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);



// Networking Procedure
SOCKET InitialNetwork();
bool ReadEvent();
bool WriteEvent();
bool NetWorkProc(DWORD lParam, DWORD wParam);
void PacketProc(BYTE packetType, char* packet);
void PacketProc(BYTE packetType, Packet& packet);

void PacketProcCreateMyPlayer(Packet& rawPacket);
void PacketProcCreateOtherPlayer(Packet& rawPacket);
void PacketProcDeletePlayer(Packet& rawPacket);
void PacketProcMoveStart(Packet& rawPacket);
void PacketProcMoveStop(Packet& rawPacket);
void PacketProcAttack1(Packet& rawPacket);
void PacketProcAttack2(Packet& rawPacket);
void PacketProcAttack3(Packet& rawPacket);
void PacketProcDamage(Packet& rawPacket);

void PacketProcCreateMyPlayer(char* packet);
void PacketProcCreateOtherPlayer(char* packet);
void PacketProcDeletePlayer(char* packet);
void PacketProcMoveStart(char* packet);
void PacketProcMoveStop(char* packet);
void PacketProcAttack1(char* packet);
void PacketProcAttack2(char* packet);
void PacketProcAttack3(char* packet);
void PacketProcDamage(char* packet);

// GameLogic Procedure
bool InitialGame();
void Update();
void Render();
void LayerOrdering();
void PrintFrameInfo();





int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    timeBeginPeriod(1);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_TCPFIGHTERPROCADEMY, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }
    g_ClientSocket = InitialNetwork();
    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TCPFIGHTERPROCADEMY));

    MSG msg;

    PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) 
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else 
        {
            //game loop
            Update();
        }
    }


    timeEndPeriod(1);

    closesocket(g_ClientSocket);
    WSACleanup();

    return (int) msg.wParam;
}

clock_t g_1SecTimer = 0;



ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TCPFIGHTERPROCADEMY));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_TCPFIGHTERPROCADEMY);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   } 
   g_hWnd = hWnd;
   ShowWindow(hWnd, SW_SHOW);
   UpdateWindow(hWnd);
   SetFocus(hWnd);
   RECT windowRect;
   windowRect.top = 0;
   windowRect.left = 0;
   windowRect.right = 640;
   windowRect.bottom = 480;

   g_hOldIMC = ImmAssociateContext(hWnd, nullptr);
   AdjustWindowRectEx(&windowRect, GetWindowLong(g_hWnd, GWL_STYLE), GetMenu(g_hWnd) != NULL, GetWindowLong(hWnd, GWL_EXSTYLE));

   int x = (GetSystemMetrics(SM_CXSCREEN) / 2) - 640 / 2;
   int y = (GetSystemMetrics(SM_CYSCREEN) / 2) - 480 / 2;

   MoveWindow(g_hWnd, x, y, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, true);

   return TRUE;
}

//  함수: WndProc(HWND, UINT, WPARAM, LPARAM)

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        if (InitialGame() == false)
            PostMessage(g_hWnd, WM_DESTROY, 0, 0);
        break;
    }
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 메뉴 선택을 구문 분석합니다:
            switch (wmId)
            {
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case UM_NETWORK:
    {
        int err = WSAGETSELECTERROR(lParam);
        if (err == SOCKET_ERROR)
        {
            MessageBox(g_hWnd, L"WSAGETSELECTERROR", L"끝났지롱~", MB_OK);
            PostMessage(g_hWnd, WM_DESTROY, 0, 0);
            break;
        }
        NetWorkProc(lParam,wParam);
    }
    case WM_ACTIVATEAPP:
        {
            bool activeFlag = LOWORD(wParam);
            if (activeFlag ==WA_INACTIVE) g_isActive = false;
            else g_isActive = true;
            break;
        }
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
            break;
        }
    case WM_DESTROY:
        //printf("player id : %d\n",gp_Player->GetObjectID());
        //printf("player x : %d\n", gp_Player->GetCurrentX());
        //printf("player y : %d\n", gp_Player->GetCurrentY());
        //printf("player dir : %d\n", gp_Player->GetDirection());
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
    return 0;
}

// NetWork Functions begin
SOCKET InitialNetwork()
{
    WSADATA data;
    if (WSAStartup(MAKEWORD(2, 2), &data)) return INVALID_SOCKET;

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) return INVALID_SOCKET;

    HANDLE file = CreateFile(L"IP.txt", GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return INVALID_SOCKET;
    char ip[16]{ 0 };
    DWORD read;
    if (!ReadFile(file, &ip, 16, &read, nullptr))
    {
        return INVALID_SOCKET;
    }
    CloseHandle(file);

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5000);
    inet_pton(AF_INET, ip, &addr.sin_addr);
    u_long blockingMode = 1;
    ioctlsocket(sock, FIONBIO, &blockingMode);
    int conRet = connect(sock, (sockaddr*)&addr, sizeof(addr));
    int conErr;
    if (conRet == SOCKET_ERROR)
    {
        conErr = WSAGetLastError();
        if(conErr != WSAEWOULDBLOCK)
            return INVALID_SOCKET;
    }
    linger timeOut;
    timeOut.l_onoff = 1;
    timeOut.l_linger = 0;
    setsockopt(sock, SOL_SOCKET, SO_LINGER, (const char*)&timeOut, sizeof(timeOut));

    WSAAsyncSelect(sock, g_hWnd, UM_NETWORK, FD_CONNECT | FD_CLOSE | FD_WRITE | FD_READ);
    g_ClientSocket = sock;
    return sock;
}
bool NetWorkProc(DWORD lParam, DWORD wParam)
{
    switch (WSAGETSELECTEVENT(lParam))
    {
        case FD_CONNECT:return true;
        case FD_CLOSE:
        {
            MessageBox(g_hWnd, L"FD_CLOSE", L"끝났지롱~", MB_OK);
            PostMessage(g_hWnd, WM_DESTROY, 0, 0);
            return false;
        }
        case FD_WRITE:
        {
            bSendFlag = true;
            return WriteEvent();
        }
        case FD_READ:
        {
            return ReadEvent();
        }
    }
    return false;
}
bool ReadEvent()
{
    int recvRet;
    int recvErr;
    //char recvBuffer[1024];

    //recvRet = recv(g_ClientSocket, recvBuffer, 1024, 0);
    recvRet = recv(g_ClientSocket, g_RecvRingBuffer.GetWritePtr(), g_RecvRingBuffer.DirectEnqueueSize(), 0);
    if (recvRet == SOCKET_ERROR)
    {
        recvErr = WSAGetLastError();
        if (recvErr != WSAEWOULDBLOCK)
        {
            WCHAR buffer[10]{ 0 };
            _itow_s(recvRet, buffer, 10);
            MessageBox(g_hWnd, L"recv()", buffer, MB_OK);
            PostMessage(g_hWnd, WM_DESTROY, 0, 0);
            return false;
        }
        //WOULDBLOCK인 상황 받을게 더이상 없다고판단되면 로직수행하로가야됨
    }
    //printf("recv buffer -> dir Enqueue size : %d\t", g_RecvRingBuffer.DirectEnqueueSize());
    //printf("recvRet : %d\n", recvRet);
    g_RecvRingBuffer.MoveRear(recvRet);
    //int eqRet;
    //if (g_RecvRingBuffer.GetFreeSize() < 0) return false;
    //if (recvRet > 0) eqRet = g_RecvRingBuffer.Enqueue(recvBuffer, recvRet);
    //if (recvRet > 0 && eqRet != recvRet)
    //{
    //    int* ptr = nullptr;
    //    *ptr = 1234;
    //}
    
    while (true)
    {
        PacketHeader header;
        int size = g_RecvRingBuffer.GetUseSize();
        if (g_RecvRingBuffer.GetUseSize() < sizeof(PacketHeader)) break;
        int headerPeekRet = g_RecvRingBuffer.Peek((char*)&header, sizeof(PacketHeader));
        if (header.code != 0x89)
        {
            WCHAR buffer[10]{ 0 };
            _itow_s(header.code, buffer, 10);
            MessageBox(g_hWnd, L"header.code", buffer, MB_OK);
            PostMessage(g_hWnd, WM_DESTROY, 0, 0);
            return false;
        }
        if (g_RecvRingBuffer.GetUseSize() < header.payloadSize + sizeof(header)) break;
        //printf("header.code : %x\t\t", header.code);
        //printf("header.payloadSize : %d\t\t", header.payloadSize);
        //printf("header.packetType : %d\n", header.packetType);
        //printf("g_RecvRingBuffer.GetUseSize() : %d\n", g_RecvRingBuffer.GetUseSize());
        //printf("g_RecvRingBuffer.GetFreeSize() : %d\n", g_RecvRingBuffer.GetFreeSize());
        g_RecvRingBuffer.MoveFront(sizeof(PacketHeader));
        Packet packet;
        int pkRet = g_RecvRingBuffer.Peek(packet.GetBufferPtr(), header.payloadSize);
        if (pkRet != header.payloadSize) break;
        g_RecvRingBuffer.MoveFront(header.payloadSize);
        packet.MoveWritePtr(header.payloadSize);
        PacketProc(header.packetType, packet);
    }
    return true;
}
bool WriteEvent()
{
    if (!bSendFlag) return false;
    while (true)
    {
        //char buffer[1000]{ 0 };
        //int sendPeekRet = g_SendRingBuffer.Peek(buffer, min(1000, g_SendRingBuffer.GetUseSize()));
        //if (sendPeekRet == 0) break;
        int sendRet = 0;
        //sendRet = send(g_ClientSocket, buffer, sendPeekRet, 0);
        if (g_SendRingBuffer.GetUseSize() == 0) 
            break;
        sendRet = send(g_ClientSocket, g_SendRingBuffer.GetReadPtr(), g_SendRingBuffer.DirectDequeueSize(), 0);
        char* code = g_SendRingBuffer.GetReadPtr();

        //int directDequeueSize = g_SendRingBuffer.DirectDequeueSize();
        //int currentReadPointerIdx = g_SendRingBuffer.GetReadPtr() - g_SendRingBuffer.GetBegin();
        //void* currentReadPointerPos = g_SendRingBuffer.GetReadPtr();
        //void* currentWritePointerPos = g_SendRingBuffer.GetWritePtr();
        //int currentFreeSize = g_SendRingBuffer.GetFreeSize();
        //int currentUseSize = g_SendRingBuffer.GetUseSize();
        ////일단 남아있는 사이즈확인
        //printf("current byte code : 0x%x\n", *code);
        //printf("SendRet : %d\n", sendRet);
        //printf("g_SendRingBuffer.DirectDequeueSize() : %d\n", directDequeueSize);
        //printf("g_SendRingBuffer.current ReadPointer index  : %d\n", currentReadPointerIdx);
        //printf("g_SendRingBuffer.current ReadPointer Pos() : %p\n", currentReadPointerPos);
        //printf("g_SendRingBuffer.current GetWritePtr Pos() : %p\n", currentWritePointerPos);
        //printf("g_SendRingBuffer.GetFreeSize() : %d\n", currentFreeSize);
        //printf("g_SendRingBuffer.GetUseSize() : %d\n", currentUseSize);

        if (sendRet == SOCKET_ERROR)
        {
            int sendErr = WSAGetLastError();
            if (sendErr != WSAEWOULDBLOCK)
            {
                WCHAR buffer[10]{ 0 };
                _itow_s(sendErr, buffer, 10);
                MessageBox(g_hWnd, L"send()", buffer, MB_OK);
                PostMessage(g_hWnd, WM_DESTROY, 0, 0);
                return false;
            }
            bSendFlag = false;
            break;
        }
        else if (sendRet == 0)
        {
            break;
        }
        g_SendRingBuffer.MoveFront(sendRet);
    }
    return true;
}
// NetWork Functions End




// Packet Procedure Begin
void PacketProc(BYTE packetType, Packet& packet)
{
    //printf("get a message ! %d \n", packetType);
    switch (packetType)
    {
    case dfPACKET_SC_CREATE_MY_CHARACTER:
    {
        PacketProcCreateMyPlayer(packet);
        break;
    }
    case dfPACKET_SC_CREATE_OTHER_CHARACTER:
    {
        PacketProcCreateOtherPlayer(packet);
        break;
    }
    case dfPACKET_SC_DELETE_CHARACTER:
    {
        PacketProcDeletePlayer(packet);
        break;
    }
    case dfPACKET_SC_MOVE_START:
    {
        PacketProcMoveStart(packet);
        break;
    }
    case dfPACKET_SC_MOVE_STOP:
    {
        PacketProcMoveStop(packet);
        break;
    }
    case dfPACKET_SC_ATTACK1:
    {
        PacketProcAttack1(packet);
        break;
    }
    case dfPACKET_SC_ATTACK2:
    {
        PacketProcAttack2(packet);
        break;
    }
    case dfPACKET_SC_ATTACK3:
    {
        PacketProcAttack3(packet);
        break;
    }
    case dfPACKET_SC_DAMAGE:
    {
        PacketProcDamage(packet);
        break;
    }
    default:
    {
        WCHAR buffer[50]{ 0 };
        _itow_s(packetType, buffer, 10);
        wcscat_s(buffer, L" : PACKET NUMBER IS WRONG");
        MessageBox(g_hWnd, L"MISS PACKET TYPE", buffer, MB_OK);
        break;
    }
    }
}



void PacketProcCreateMyPlayer(char* rawPacket)
{
    SC_PacketCreateMyCharacter* packet = (SC_PacketCreateMyCharacter*)rawPacket;
    int spriteStart;
    int spriteEnd;
    int direction;
    if (packet->direction == ACTION_MOVE_LL)
    {
        spriteStart = (int)e_SPRITE::PLAYER_STAND_L01;
        spriteEnd = (int)e_SPRITE::PLAYER_STAND_L_MAX;
        direction = dfPACKET_MOVE_DIR_LL;
    }
    else
    {
        spriteStart = (int)e_SPRITE::PLAYER_STAND_R01;
        spriteEnd = (int)e_SPRITE::PLAYER_STAND_R_MAX;
        direction = dfPACKET_MOVE_DIR_RR;
    }
    PlayerObject* myPlayer = new PlayerObject(packet->x, packet->y, e_OBJECT_TYPE::TYPE_PLAYER, true, spriteStart, spriteEnd, packet->playerID, packet->HP);
    myPlayer->SetDirection(direction);
    myPlayer->SetAction(12345);
    myPlayer->SetActionStand();
    gp_Player = myPlayer;
    g_ObjectList.push_back(myPlayer);
}
void PacketProcCreateMyPlayer(Packet& rawPacket)
{
    unsigned int id;
    BYTE direction;
    unsigned short x;
    unsigned short y;
    BYTE hp;

    rawPacket >> id >> direction >> x >> y >> hp;
    int inputSpriteStart;
    int inputSpriteEnd;
    int inputDirection;
    if (direction == dfPACKET_MOVE_DIR_LL)
    {
        inputSpriteStart = (int)e_SPRITE::PLAYER_STAND_L01;
        inputSpriteEnd = (int)e_SPRITE::PLAYER_STAND_L_MAX;
        inputDirection = dfPACKET_MOVE_DIR_LL;
    }
    else
    {
        inputSpriteStart = (int)e_SPRITE::PLAYER_STAND_R01;
        inputSpriteEnd = (int)e_SPRITE::PLAYER_STAND_R_MAX;
        inputDirection = dfPACKET_MOVE_DIR_RR;
    }
    PlayerObject* myPlayer = new PlayerObject(x, y, e_OBJECT_TYPE::TYPE_PLAYER, true, inputSpriteStart, inputSpriteEnd, id, hp);
    myPlayer->SetDirection(inputDirection);
    myPlayer->SetAction(12345);
    myPlayer->SetActionStand();
    gp_Player = myPlayer;
    g_ObjectList.push_back(myPlayer);
}


void PacketProcCreateOtherPlayer(char* rawPacket)
{
    SC_PacketCreateMyCharacter* packet = (SC_PacketCreateMyCharacter*)rawPacket;
    int spriteStart;
    int spriteEnd;
    int direction;
    if (packet->direction == ACTION_MOVE_LL)
    {
        spriteStart = (int)e_SPRITE::PLAYER_STAND_L01;
        spriteEnd = (int)e_SPRITE::PLAYER_STAND_L_MAX;
        direction = dfPACKET_MOVE_DIR_LL;
    }
    else
    {
        spriteStart = (int)e_SPRITE::PLAYER_STAND_R01;
        spriteEnd = (int)e_SPRITE::PLAYER_STAND_R_MAX;
        direction = dfPACKET_MOVE_DIR_RR;
    }
    PlayerObject* newPlayer = new PlayerObject(packet->x, packet->y, e_OBJECT_TYPE::TYPE_PLAYER, false, spriteStart, spriteEnd, packet->playerID, packet->HP);
    newPlayer->SetDirection(direction);
    newPlayer->SetAction(12345);
    newPlayer->SetActionStand();
    g_ObjectList.push_back(newPlayer);
}
void PacketProcCreateOtherPlayer(Packet& rawPacket)
{
    unsigned int id;
    BYTE direction;
    unsigned short x;
    unsigned short y;
    BYTE hp;

    rawPacket >> id >> direction >> x >> y >> hp;
    int inputSpriteStart;
    int inputSpriteEnd;
    int inputDirection;
    if (direction == dfPACKET_MOVE_DIR_LL)
    {
        inputSpriteStart = (int)e_SPRITE::PLAYER_STAND_L01;
        inputSpriteEnd = (int)e_SPRITE::PLAYER_STAND_L_MAX;
        inputDirection = dfPACKET_MOVE_DIR_LL;
    }
    else
    {
        inputSpriteStart = (int)e_SPRITE::PLAYER_STAND_R01;
        inputSpriteEnd = (int)e_SPRITE::PLAYER_STAND_R_MAX;
        inputDirection = dfPACKET_MOVE_DIR_RR;
    }
    PlayerObject* myPlayer = new PlayerObject(x, y, e_OBJECT_TYPE::TYPE_PLAYER, false, inputSpriteStart, inputSpriteEnd, id, hp);
    myPlayer->SetDirection(inputDirection);
    myPlayer->SetAction(12345);
    myPlayer->SetActionStand();
    g_ObjectList.push_back(myPlayer);
}


void PacketProcDeletePlayer(char* rawPacket)
{
    SC_PacketDeleteCharacter* packet = (SC_PacketDeleteCharacter*)rawPacket;
    auto iter = g_ObjectList.begin();
    for (; iter != g_ObjectList.end();)
    {
        if ((*iter)->GetObjectID() == packet->playerID)
        {
            BaseObject* removePlayer = *iter;
            iter = g_ObjectList.erase(iter);
            delete removePlayer;
        }
        else
        {
            ++iter;
        }
    }
}
void PacketProcDeletePlayer(Packet& rawPacket)
{
    unsigned int playerID;

    rawPacket >> playerID;
    auto iter = g_ObjectList.begin();
    for (; iter != g_ObjectList.end();)
    {
        if ((*iter)->GetObjectID() == playerID)
        {
            BaseObject* removePlayer = *iter;
            iter = g_ObjectList.erase(iter);
            delete removePlayer;
        }
        else
        {
            ++iter;
        }
    }
}


void PacketProcMoveStart(char* rawPacket)
{
    SC_PacketMoveStart* packet = (SC_PacketMoveStart*)rawPacket;
    auto iter = g_ObjectList.begin();
    for (; iter != g_ObjectList.end(); ++iter)
    {
        if ((*iter)->GetObjectID() == packet->playerID)
        {
            PlayerObject* currentPlayer = (PlayerObject*)*iter;
            BYTE direction;
            if (packet->direction == dfPACKET_MOVE_DIR_DD || packet->direction == dfPACKET_MOVE_DIR_UU)
                direction = currentPlayer->GetDirection();
            else if (packet->direction == dfPACKET_MOVE_DIR_LU || packet->direction == dfPACKET_MOVE_DIR_LD)
                direction = dfPACKET_MOVE_DIR_LL;
            else if (packet->direction == dfPACKET_MOVE_DIR_RU || packet->direction == dfPACKET_MOVE_DIR_RD)
                direction = dfPACKET_MOVE_DIR_RR;
            else
                direction = packet->direction;
            currentPlayer->SetPosition(packet->x, packet->y);
            currentPlayer->SetDirection(direction);
            currentPlayer->SetAction(packet->direction);
            currentPlayer->SetActionMove();
            break;
        }
    }
}
void PacketProcMoveStart(Packet& rawPacket)
{
    unsigned int playerID;
    BYTE moveDirection;
    unsigned short x;
    unsigned short y;

    rawPacket >> playerID >> moveDirection >> x >> y;

    auto iter = g_ObjectList.begin();
    for (; iter != g_ObjectList.end(); ++iter)
    {
        if ((*iter)->GetObjectID() == playerID)
        {
            PlayerObject* currentPlayer = (PlayerObject*)*iter;
            BYTE direction;
            if (moveDirection == dfPACKET_MOVE_DIR_DD || moveDirection == dfPACKET_MOVE_DIR_UU)
                direction = currentPlayer->GetDirection();
            else if (moveDirection == dfPACKET_MOVE_DIR_LU || moveDirection == dfPACKET_MOVE_DIR_LD)
                direction = dfPACKET_MOVE_DIR_LL;
            else if (moveDirection == dfPACKET_MOVE_DIR_RU || moveDirection == dfPACKET_MOVE_DIR_RD)
                direction = dfPACKET_MOVE_DIR_RR;
            else
                direction = moveDirection;
            currentPlayer->SetPosition(x, y);
            currentPlayer->SetDirection(direction);
            currentPlayer->SetAction(moveDirection);
            currentPlayer->SetActionMove();
            break;
        }
    }
}


void PacketProcMoveStop(char* rawPacket)
{
    SC_PacketMoveStop* packet = (SC_PacketMoveStop*)rawPacket;
    auto iter = g_ObjectList.begin();
    for (; iter != g_ObjectList.end(); ++iter)
    {
        if ((*iter)->GetObjectID() == packet->playerID)
        {
            PlayerObject* currentPlayer = (PlayerObject*)*iter;
            currentPlayer->SetPosition(packet->x, packet->y);
            currentPlayer->SetDirection(packet->direction);
            currentPlayer->SetAction(12345);
            currentPlayer->SetActionStand();
            break;
        }
    }
}
void PacketProcMoveStop(Packet& rawPacket)
{
    unsigned int playerID;
    BYTE direction;
    unsigned short x;
    unsigned short y;

    rawPacket >> playerID >> direction >> x >> y;

    //printf("PlayerID -> %d\n", playerID);
    //printf("Direction -> %d\n", playerID);
    //printf("x -> %d\n", x);
    //printf("y -> %d\n", y);
    auto iter = g_ObjectList.begin();
    for (; iter != g_ObjectList.end(); ++iter)
    {
        if ((*iter)->GetObjectID() == playerID)
        {
            PlayerObject* currentPlayer = (PlayerObject*)*iter;
            currentPlayer->SetPosition(x, y);
            currentPlayer->SetDirection(direction);
            currentPlayer->SetAction(12345);
            currentPlayer->SetActionStand();
            break;
        }
    }
}


void PacketProcAttack1(char* rawPacket)
{
    SC_PacketAttack1* packet = (SC_PacketAttack1*)rawPacket;
    auto iter = g_ObjectList.begin();
    for (; iter != g_ObjectList.end(); ++iter)
    {
        if ((*iter)->GetObjectID() == packet->playerID)
        {
            PlayerObject* currentPlayer = (PlayerObject*)*iter;
            currentPlayer->SetPosition(packet->x, packet->y);
            currentPlayer->SetDirection(packet->direction);
            currentPlayer->SetActionAttack1();
            currentPlayer->SetAction(ACTION_ATTACK1);
            break;
        }
    }
}
void PacketProcAttack1(Packet& rawPacket)
{
    unsigned int playerID;
    BYTE direction;
    unsigned short x;
    unsigned short y;

    rawPacket >> playerID >> direction >> x >> y;
    auto iter = g_ObjectList.begin();
    for (; iter != g_ObjectList.end(); ++iter)
    {
        if ((*iter)->GetObjectID() == playerID)
        {
            PlayerObject* currentPlayer = (PlayerObject*)*iter;
            currentPlayer->SetPosition(x, y);
            currentPlayer->SetDirection(direction);
            currentPlayer->SetActionAttack1();
            currentPlayer->SetAction(ACTION_ATTACK1);
            break;
        }
    }
}


void PacketProcAttack2(char* rawPacket)
{
    SC_PacketAttack2* packet = (SC_PacketAttack2*)rawPacket;
    auto iter = g_ObjectList.begin();
    for (; iter != g_ObjectList.end(); ++iter)
    {
        if ((*iter)->GetObjectID() == packet->playerID)
        {
            PlayerObject* currentPlayer = (PlayerObject*)*iter;
            currentPlayer->SetPosition(packet->x, packet->y);
            currentPlayer->SetDirection(packet->direction);
            currentPlayer->SetActionAttack2();
            currentPlayer->SetAction(ACTION_ATTACK2);
            break;
        }
    }
}
void PacketProcAttack2(Packet& rawPacket)
{
    unsigned int playerID;
    BYTE direction;
    unsigned short x;
    unsigned short y;

    rawPacket >> playerID >> direction >> x >> y;

    auto iter = g_ObjectList.begin();
    for (; iter != g_ObjectList.end(); ++iter)
    {
        if ((*iter)->GetObjectID() == playerID)
        {
            PlayerObject* currentPlayer = (PlayerObject*)*iter;
            currentPlayer->SetPosition(x, y);
            currentPlayer->SetDirection(direction);
            currentPlayer->SetActionAttack2();
            currentPlayer->SetAction(ACTION_ATTACK2);
            break;
        }
    }
}


void PacketProcAttack3(char* rawPacket)
{
    SC_PacketAttack3* packet = (SC_PacketAttack3*)rawPacket;
    auto iter = g_ObjectList.begin();
    for (; iter != g_ObjectList.end(); ++iter)
    {
        if ((*iter)->GetObjectID() == packet->playerID)
        {
            PlayerObject* currentPlayer = (PlayerObject*)*iter;
            currentPlayer->SetPosition(packet->x, packet->y);
            currentPlayer->SetDirection(packet->direction);
            currentPlayer->SetActionAttack3();
            currentPlayer->SetAction(ACTION_ATTACK3);
            break;
        }
    }
}
void PacketProcAttack3(Packet& rawPacket)
{
    unsigned int playerID;
    BYTE direction;
    unsigned short x;
    unsigned short y;

    rawPacket >> playerID >> direction >> x >> y;

    auto iter = g_ObjectList.begin();
    for (; iter != g_ObjectList.end(); ++iter)
    {
        if ((*iter)->GetObjectID() == playerID)
        {
            PlayerObject* currentPlayer = (PlayerObject*)*iter;
            currentPlayer->SetPosition(x, y);
            currentPlayer->SetDirection(direction);
            currentPlayer->SetActionAttack3();
            currentPlayer->SetAction(ACTION_ATTACK3);
            break;
        }
    }
}


void PacketProcDamage(char* rawPacket)
{
    SC_PacketDamage* packet = (SC_PacketDamage*)rawPacket;
    auto iter = g_ObjectList.begin();
    PlayerObject* createEffectPlayer = nullptr;
    int originHP;
    for (; iter != g_ObjectList.end(); ++iter)
    {
        if ((*iter)->GetObjectType() == e_OBJECT_TYPE::TYPE_EFFECT) continue;
        if ((*iter)->GetObjectID() == packet->damagePlayerID)
        {
            PlayerObject* hitPlayer = (PlayerObject*)*iter;
            createEffectPlayer = hitPlayer;
            originHP = hitPlayer->GetHP();
            hitPlayer->SetHP(packet->damageHP);
            continue;
        }
        if ((*iter)->GetObjectID() == packet->attackPlayerID)
        {
            PlayerObject* attackPlayer = (PlayerObject*)*iter;
        }
    }
    if (createEffectPlayer != nullptr)
    {
        if(originHP - createEffectPlayer->GetHP() ==1)
        createEffectPlayer->CreateEffect(5, createEffectPlayer->GetCurrentX(), createEffectPlayer->GetCurrentY() -55);
        else if (originHP - createEffectPlayer->GetHP() == 2)
            createEffectPlayer->CreateEffect(8, createEffectPlayer->GetCurrentX(), createEffectPlayer->GetCurrentY() - 55);
        else createEffectPlayer->CreateEffect(10, createEffectPlayer->GetCurrentX(), createEffectPlayer->GetCurrentY() - 75);
    }
}
void PacketProcDamage(Packet& rawPacket)
{
    unsigned int attackPlayerID;
    unsigned int damagePlayerID;
    BYTE damageHP;

    rawPacket >> attackPlayerID >> damagePlayerID >> damageHP;

    auto iter = g_ObjectList.begin();
    PlayerObject* createEffectPlayer = nullptr;
    int originHP;
    for (; iter != g_ObjectList.end(); ++iter)
    {
        if ((*iter)->GetObjectType() == e_OBJECT_TYPE::TYPE_EFFECT) continue;
        if ((*iter)->GetObjectID() == damagePlayerID)
        {
            PlayerObject* hitPlayer = (PlayerObject*)*iter;
            createEffectPlayer = hitPlayer;
            originHP = hitPlayer->GetHP();
            hitPlayer->SetHP(damageHP);
            continue;
        }
        if ((*iter)->GetObjectID() == attackPlayerID)
        {
            PlayerObject* attackPlayer = (PlayerObject*)*iter;
        }
    }
    if (createEffectPlayer != nullptr)
    {
        if (originHP - createEffectPlayer->GetHP() == 1)
            createEffectPlayer->CreateEffect(5, createEffectPlayer->GetCurrentX(), createEffectPlayer->GetCurrentY() - 55);
        else if (originHP - createEffectPlayer->GetHP() == 2)
            createEffectPlayer->CreateEffect(8, createEffectPlayer->GetCurrentX(), createEffectPlayer->GetCurrentY() - 55);
        else createEffectPlayer->CreateEffect(10, createEffectPlayer->GetCurrentX(), createEffectPlayer->GetCurrentY() - 75);
    }
}


// Packet Procedure End




DWORD g_OldActionType;
bool g_EnqueueFlag;
void KeyProcess()
{
    if (gp_Player == nullptr || !g_isActive) return;
    DWORD action = -1;
    int direction = -1;
    SHORT leftKeyState = GetAsyncKeyState(VK_LEFT);
    SHORT rightKeyState = GetAsyncKeyState(VK_RIGHT);
    SHORT upKeyState = GetAsyncKeyState(VK_UP);
    SHORT downKeyState = GetAsyncKeyState(VK_DOWN);
    SHORT AkeyState = GetAsyncKeyState(0x41);
    SHORT SkeyState = GetAsyncKeyState(0x53);
    SHORT DkeyState = GetAsyncKeyState(0x44);

    g_EnqueueFlag = false;
    int actionType;


    if (AkeyState)
    {
        actionType = action = ACTION_ATTACK1;
        direction = gp_Player->GetDirection();
    }
    else if (SkeyState)
    {
        actionType = action = ACTION_ATTACK2;
        direction = gp_Player->GetDirection();
        
    }
    else if (DkeyState)
    {
        actionType = action = ACTION_ATTACK3;
        direction = gp_Player->GetDirection();
    }
    else if (leftKeyState && upKeyState)
    {
        action = ACTION_MOVE_LU;
        direction = ACTION_MOVE_LL;
        actionType = ACTION_MOVE_LL;
    }
    else if (leftKeyState && downKeyState)
    {
        action = ACTION_MOVE_LD;
        direction = ACTION_MOVE_LL;
        actionType = ACTION_MOVE_LL;
    }
    else if (rightKeyState && upKeyState)
    {
        action = ACTION_MOVE_RU;
        direction = ACTION_MOVE_RR;
        actionType = ACTION_MOVE_LL;
    }
    else if (rightKeyState && downKeyState)
    {
        action = ACTION_MOVE_RD;
        direction = ACTION_MOVE_RR;
        actionType = ACTION_MOVE_LL;
    }
    else if (rightKeyState)
    {
        action = ACTION_MOVE_RR;
        direction = ACTION_MOVE_RR;
        actionType = ACTION_MOVE_LL;
    }
    else if (leftKeyState)
    {
        action = ACTION_MOVE_LL;
        direction = ACTION_MOVE_LL;
        actionType = ACTION_MOVE_LL;
    }
    else if (upKeyState)
    {
        action = ACTION_MOVE_UU;
        actionType = ACTION_MOVE_LL;
        direction = gp_Player->GetDirection();
    }
    else if (downKeyState)
    {
        action = ACTION_MOVE_DD;
        actionType = ACTION_MOVE_LL;
        direction = gp_Player->GetDirection();
    }
    else
    {
        //if ((g_OldActionType == ACTION_ATTACK1 || g_OldActionType == ACTION_ATTACK2 || g_OldActionType == ACTION_ATTACK3) && !gp_Player->IsEndFrame())
        //{
        //    actionType = action = g_OldActionType;
        //}
        //else
        //{
            action = 12345;
            actionType = ACTION_MOVE_DD;
        //}
        direction = gp_Player->GetDirection();
    }
    //if ((g_OldActionType == ACTION_ATTACK1 || g_OldActionType == ACTION_ATTACK2 || g_OldActionType == ACTION_ATTACK3))
    //{
    //    if (!gp_Player->IsEndFrame()) return;
    //    g_OldActionType = actionType;
    //}

    gp_Player->InputActionProc(action, direction);
    DWORD playerOldAction = gp_Player->GetOldAction();
    DWORD playerCurrentAction = action;

    if (playerCurrentAction != playerOldAction)
    {
        g_EnqueueFlag = true;
    }
    else if (playerCurrentAction >= ACTION_ATTACK1 && playerCurrentAction <= ACTION_ATTACK3 && playerOldAction >= ACTION_ATTACK1 && playerOldAction <= ACTION_ATTACK3)
    {
        if (gp_Player->IsEndFrame())
        {
            actionType = ACTION_MOVE_DD;
            gp_Player->InputActionProc(12345, gp_Player->GetDirection());
            g_EnqueueFlag = true;
        }
    }

    if (g_EnqueueFlag)
    {
        PacketHeader header;
        bool sendFlag = false;
        int size;
        char* packetPointer = nullptr;
        Packet pk;
        switch (actionType)
        {
        case ACTION_ATTACK1:
        {
            if (actionType == g_OldActionType || (g_OldActionType == ACTION_ATTACK2 || g_OldActionType == ACTION_ATTACK3))
            {
                if (!gp_Player->IsEndFrame()) break;
            }
            MakePacketAttack1(pk, direction, gp_Player->GetCurrentX(), gp_Player->GetCurrentY());
            sendFlag = true;
            //CS_PacketAttack1 packet;
            //MakePacketAttack1(header, packet, direction, gp_Player->GetCurrentX(), gp_Player->GetCurrentY());
            //packetPointer = (char*)&packet;
            //size = sizeof(packet);
            break;
        }
        case ACTION_ATTACK2:
        {
            if (actionType == g_OldActionType || (g_OldActionType == ACTION_ATTACK1 || g_OldActionType == ACTION_ATTACK3))
            {
                if (!gp_Player->IsEndFrame()) break;
            }
            MakePacketAttack2(pk, direction, gp_Player->GetCurrentX(), gp_Player->GetCurrentY());
            sendFlag = true;
            //CS_PacketAttack2 packet;
            //MakePacketAttack2(header, packet, direction, gp_Player->GetCurrentX(), gp_Player->GetCurrentY());
            //packetPointer = (char*)&packet;
            //size = sizeof(packet);
            break;
        }
        case ACTION_ATTACK3:
        {
            if (actionType == g_OldActionType || (g_OldActionType == ACTION_ATTACK1 || g_OldActionType == ACTION_ATTACK2))
            {
                if (!gp_Player->IsEndFrame()) break;
            }
            MakePacketAttack3(pk, direction, gp_Player->GetCurrentX(), gp_Player->GetCurrentY());
            sendFlag = true;
            //CS_PacketAttack3 packet;
            //MakePacketAttack3(header, packet, direction, gp_Player->GetCurrentX(), gp_Player->GetCurrentY());
            //packetPointer = (char*)&packet;
            //size = sizeof(packet);
            break;
        }
        case ACTION_MOVE_LL:
        {
            if (action == gp_Player->GetOldAction()) break;
            if ((g_OldActionType == ACTION_ATTACK1 || g_OldActionType == ACTION_ATTACK2 || g_OldActionType == ACTION_ATTACK3))
            {
                if (!gp_Player->IsEndFrame()) break;
            }
            MakePacketStartMove(pk, action, gp_Player->GetCurrentX(), gp_Player->GetCurrentY());
            sendFlag = true;
            //CS_PacketMoveStart packet;
            //MakePacketStartMove(header, packet, action, gp_Player->GetCurrentX(), gp_Player->GetCurrentY());
            //packetPointer = (char*)&packet;
            //size = sizeof(packet);
            break;
        }
        case ACTION_MOVE_DD:
        {
            if (g_OldActionType == ACTION_MOVE_DD) break;
            if ((g_OldActionType == ACTION_ATTACK1 || g_OldActionType == ACTION_ATTACK2 || g_OldActionType == ACTION_ATTACK3))
            {
                {
                    if (!gp_Player->IsEndFrame()) break;
                }
            }
            MakePacketStopMove(pk, direction, gp_Player->GetCurrentX(), gp_Player->GetCurrentY());
            sendFlag = true;
            //CS_PacketMoveStop packet;
            //MakePacketStopMove(header, packet, direction, gp_Player->GetCurrentX(), gp_Player->GetCurrentY());
            //packetPointer = (char*)&packet;
            //size = sizeof(packet);
            break;
        }
        default:
            packetPointer = nullptr;
        }
        //if (packetPointer == nullptr) return;
        if (!sendFlag) return;
        g_OldActionType = actionType;
        g_SendRingBuffer.Enqueue(pk.GetReadPtr(), pk.GetBufferSize());
        //g_SendRingBuffer.Enqueue((const char*)&header, sizeof(PacketHeader));
        //g_SendRingBuffer.Enqueue((const char*)packetPointer, size);
        WriteEvent();
    }
}
void UpdateObject()
{
    for (__int64 i = 0; i < g_ObjectList.size(); i++)
    {
        g_ObjectList[i]->Update();
    }
}

void Render()
{
    BYTE* dibBuffer = g_ScreenDibBuffer.GetDibBuffer();
    int width = g_ScreenDibBuffer.GetWidth();
    int height = g_ScreenDibBuffer.GetHeight();
    int pitch = g_ScreenDibBuffer.GetPitch();
    g_SpriteDibBuffer.DrawSprite((int)e_SPRITE::MAP, 0, 0, dibBuffer, width, height, pitch);

    LayerOrdering();
    for (__int64 i = 0; i < g_ObjectList.size(); i++)
    {
        g_ObjectList[i]->Render(dibBuffer, width, height, pitch);
    }
    g_ScreenDibBuffer.DrawBuffer(g_hWnd);
}
bool InitialGame()
{
    //load on memory
//C:\Users\user\Desktop\Education\procademy\TCPFighter\TCPFighter_Procademy\Sprite_Data
    //Load Map
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::MAP, L"Sprite_Data\\_Map.bmp", 0, 0)) return false;
    //Load Left Standing
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_STAND_L01, L"Sprite_Data/Stand_L_01.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_STAND_L02, L"Sprite_Data/Stand_L_02.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_STAND_L03, L"Sprite_Data/Stand_L_03.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_STAND_L04, L"Sprite_Data/Stand_L_02.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_STAND_L05, L"Sprite_Data/Stand_L_01.bmp", 71, 90)) return false;
    //Load Right Standing
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_STAND_R01, L"Sprite_Data/Stand_R_01.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_STAND_R02, L"Sprite_Data/Stand_R_02.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_STAND_R03, L"Sprite_Data/Stand_R_03.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_STAND_R04, L"Sprite_Data/Stand_R_02.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_STAND_R05, L"Sprite_Data/Stand_R_01.bmp", 71, 90)) return false;
    //Load Left Move
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_MOVE_L01, L"Sprite_Data/Move_L_01.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_MOVE_L02, L"Sprite_Data/Move_L_02.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_MOVE_L03, L"Sprite_Data/Move_L_03.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_MOVE_L04, L"Sprite_Data/Move_L_04.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_MOVE_L05, L"Sprite_Data/Move_L_05.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_MOVE_L06, L"Sprite_Data/Move_L_06.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_MOVE_L07, L"Sprite_Data/Move_L_07.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_MOVE_L08, L"Sprite_Data/Move_L_08.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_MOVE_L09, L"Sprite_Data/Move_L_09.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_MOVE_L10, L"Sprite_Data/Move_L_10.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_MOVE_L11, L"Sprite_Data/Move_L_11.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_MOVE_L12, L"Sprite_Data/Move_L_12.bmp", 71, 90)) return false;
    //Load Right Move
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_MOVE_R01, L"Sprite_Data/Move_R_01.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_MOVE_R02, L"Sprite_Data/Move_R_02.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_MOVE_R03, L"Sprite_Data/Move_R_03.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_MOVE_R04, L"Sprite_Data/Move_R_04.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_MOVE_R05, L"Sprite_Data/Move_R_05.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_MOVE_R06, L"Sprite_Data/Move_R_06.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_MOVE_R07, L"Sprite_Data/Move_R_07.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_MOVE_R08, L"Sprite_Data/Move_R_08.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_MOVE_R09, L"Sprite_Data/Move_R_09.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_MOVE_R10, L"Sprite_Data/Move_R_10.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_MOVE_R11, L"Sprite_Data/Move_R_11.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_MOVE_R12, L"Sprite_Data/Move_R_12.bmp", 71, 90)) return false;
    //Load Left Attack1
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_ATTACK1_L01, L"Sprite_Data/Attack1_L_01.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_ATTACK1_L02, L"Sprite_Data/Attack1_L_02.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_ATTACK1_L03, L"Sprite_Data/Attack1_L_03.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_ATTACK1_L04, L"Sprite_Data/Attack1_L_04.bmp", 71, 90)) return false;
    //Load Right Attack1
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_ATTACK1_R01, L"Sprite_Data/Attack1_R_01.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_ATTACK1_R02, L"Sprite_Data/Attack1_R_02.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_ATTACK1_R03, L"Sprite_Data/Attack1_R_03.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_ATTACK1_R04, L"Sprite_Data/Attack1_R_04.bmp", 71, 90)) return false;
    //Load Left Attack2
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_ATTACK2_L01, L"Sprite_Data/Attack2_L_01.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_ATTACK2_L02, L"Sprite_Data/Attack2_L_02.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_ATTACK2_L03, L"Sprite_Data/Attack2_L_03.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_ATTACK2_L04, L"Sprite_Data/Attack2_L_04.bmp", 71, 90)) return false;
    //Load Right Attack2
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_ATTACK2_R01, L"Sprite_Data/Attack2_R_01.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_ATTACK2_R02, L"Sprite_Data/Attack2_R_02.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_ATTACK2_R03, L"Sprite_Data/Attack2_R_03.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_ATTACK2_R04, L"Sprite_Data/Attack2_R_04.bmp", 71, 90)) return false;
    //Load Left Attack3
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_ATTACK3_L01, L"Sprite_Data/Attack3_L_01.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_ATTACK3_L02, L"Sprite_Data/Attack3_L_02.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_ATTACK3_L03, L"Sprite_Data/Attack3_L_03.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_ATTACK3_L04, L"Sprite_Data/Attack3_L_04.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_ATTACK3_L05, L"Sprite_Data/Attack3_L_05.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_ATTACK3_L06, L"Sprite_Data/Attack3_L_06.bmp", 71, 90)) return false;
    //Load Right Attack3
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_ATTACK3_R01, L"Sprite_Data/Attack3_R_01.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_ATTACK3_R02, L"Sprite_Data/Attack3_R_02.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_ATTACK3_R03, L"Sprite_Data/Attack3_R_03.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_ATTACK3_R04, L"Sprite_Data/Attack3_R_04.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_ATTACK3_R05, L"Sprite_Data/Attack3_R_05.bmp", 71, 90)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::PLAYER_ATTACK3_R06, L"Sprite_Data/Attack3_R_06.bmp", 71, 90)) return false;
    //Load Effect Spark
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::EFFECT_SPARK_01, L"Sprite_Data/xSpark_1.bmp", 70, 70)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::EFFECT_SPARK_02, L"Sprite_Data/xSpark_2.bmp", 70, 70)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::EFFECT_SPARK_03, L"Sprite_Data/xSpark_3.bmp", 70, 70)) return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::EFFECT_SPARK_04, L"Sprite_Data/xSpark_4.bmp", 70, 70)) return false;
    //Load HPGuage, Shadow
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::GUAGE_HP, L"Sprite_Data/HPGuage.bmp", 35, -10))return false;
    if (!g_SpriteDibBuffer.LoadDibSprite((int)e_SPRITE::SHADOW, L"Sprite_Data/Shadow.bmp", 32, 4))return false;

    return true;
}

void LayerOrdering()
{
    if (g_ObjectList.size() <= 1) return;
    for (__int64 i = 0; i < g_ObjectList.size()-1; i++)
    {
        for (__int64 j = i; j < g_ObjectList.size()-1; j++)
        {
            if (g_ObjectList[j]->GetCurrentY() > g_ObjectList[j+1]->GetCurrentY())
                std::swap(g_ObjectList[j + 1], g_ObjectList[j]);

        }
    }
    for (__int64 i = 0; i < g_ObjectList.size()-1; i++)
    {
        for (__int64 j = i; j < g_ObjectList.size()-1; j++)
        {
            if (g_ObjectList[j]->GetObjectType() == e_OBJECT_TYPE::TYPE_EFFECT && g_ObjectList[j+1]->GetObjectType() != e_OBJECT_TYPE::TYPE_EFFECT)
                std::swap(g_ObjectList[j + 1], g_ObjectList[j]);
        }
    }
}


bool frameSkipFlag;
int cur = timeGetTime();
int old = timeGetTime();
int deltaTime;
int skipTime = 0;

int prev = timeGetTime();

void Update()
{
    if (g_isActive)
        KeyProcess();
    UpdateObject();
    g_LogicFramePerSec++;

    cur = timeGetTime();
    if (frameSkipFlag)
    {
        old = cur - (deltaTime - (20 - (cur - skipTime)));
        frameSkipFlag = false;
    }
    deltaTime = cur - old;

    if (deltaTime >= 40 == false)
    {
        Render();
        if (deltaTime < 20)
        {
            Sleep(20 - deltaTime);
        }
        old = cur - (deltaTime - 20);
        g_AllFramePerSec++;
    }
    else
    {
        frameSkipFlag = true;
        skipTime = cur;
    }

    int now = timeGetTime();
    g_1SecTimer += now - prev;
    prev = now;
    if (g_1SecTimer >= 1000)
    {
        PrintFrameInfo();
        g_1SecTimer = 0;
        g_AllFramePerSec = 0;
        g_LogicFramePerSec = 0;
    }
}


void PrintFrameInfo()
{
    WCHAR printBuffer[50]{ 0 };
    WCHAR logicFrameBuffer[10]{ 0 };
    WCHAR allFrameBuffer[10]{ 0 };
    _itow_s(g_LogicFramePerSec, logicFrameBuffer, 10);
    _itow_s(g_AllFramePerSec, allFrameBuffer, 10);
    wcscat_s(printBuffer, L"LOGIC FRAME : ");
    wcscat_s(printBuffer, logicFrameBuffer);
    wcscat_s(printBuffer, L"        ALL FRAME : ");
    wcscat_s(printBuffer, allFrameBuffer);
    SetWindowTextW(g_hWnd, printBuffer);
}