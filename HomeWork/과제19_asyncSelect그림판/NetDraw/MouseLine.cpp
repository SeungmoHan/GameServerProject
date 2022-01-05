#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib,"ws2_32.lib")
// MouseLine.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//
#include <Windowsx.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "framework.h"
#include "MouseLine.h"
#include "RingBuffer.h"

struct Header
{
    unsigned short Len;
};
Header header;
struct DrawPacket
{
    int		startX;
    int		startY;
    int		endX;
    int		endY;
};

#define MAX_LOADSTRING 100
#define UM_NETWORK (WM_USER + 1)
// 전역 변수:
HINSTANCE hInst;                                // 현재 인스턴스입니다.
WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.
char IPString[16];
SOCKET g_Client;
HWND g_HWnd;
RingBuffer g_SendRingBuffer;
RingBuffer g_RecvRingBuffer;


// 이 코드 모듈에 포함된 함수의 선언을 전달합니다:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
SOCKET SetAsyncSelectSocket();
void ReadEvent();
void WriteEvent();
void SendPacket(char* buffer, int size);
bool bSendFlag;
bool ipTyped;
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MOUSELINE, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }
    g_Client = SetAsyncSelectSocket();
    if (g_Client == INVALID_SOCKET) return -1;
    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MOUSELINE));

    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}


ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MOUSELINE));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_MOUSELINE);
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
   g_HWnd = hWnd;
   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}




SOCKET SetAsyncSelectSocket()
{
    WSADATA data;
    if (WSAStartup(MAKEWORD(2, 2), &data)) return INVALID_SOCKET;

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET)
    {
        return INVALID_SOCKET;
    }
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(25000);
    HANDLE file = CreateFile(L"IP.txt", GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return INVALID_SOCKET;
    char ip[16]{ 0 };
    DWORD read;
    if (!ReadFile(file, &ip, 16, &read, nullptr))
    {
        return INVALID_SOCKET;
    }
    CloseHandle(file);
    inet_pton(AF_INET, ip, &addr.sin_addr);
    u_long blockingMode = 1;
    ioctlsocket(g_Client, FIONBIO, &blockingMode);
    int conRet = connect(sock, (sockaddr*)&addr, sizeof(addr));
    int conErr;
    if (conRet == SOCKET_ERROR)
    {
        conErr = WSAGetLastError();
        if (conErr != WSAEWOULDBLOCK)
            return INVALID_SOCKET;
    }
    linger linger;
    linger.l_onoff = 1;
    linger.l_linger = 0;
    setsockopt(sock, SOL_SOCKET, SO_LINGER, (const char*)&linger, sizeof(linger));
    WSAAsyncSelect(sock, g_HWnd, UM_NETWORK, FD_CONNECT | FD_CLOSE | FD_WRITE | FD_READ);
    return sock;
}

//  함수: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  용도: 주 창의 메시지를 처리합니다.
//
//  WM_COMMAND  - 애플리케이션 메뉴를 처리합니다.
//  WM_PAINT    - 주 창을 그립니다.
//  WM_DESTROY  - 종료 메시지를 게시하고 반환합니다.
bool lMouseButtonClicked;
int oldX, oldY;
unsigned char r, g, b;
HPEN pen;
HPEN oldPen;
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        HDC hdc = GetDC(hWnd);
        TextOut(hdc, 400, 400, L"HEllo world", 12);
        pen = CreatePen(PS_SOLID, rand() % 15, RGB(rand() % 255, rand() % 255, rand() % 255));
        oldPen = (HPEN)SelectObject(hdc, oldPen);
        ReleaseDC(hWnd, hdc);
        break;
    }
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        // 메뉴 선택을 구문 분석합니다:
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            closesocket(g_Client);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WM_LBUTTONDOWN:
    {
        lMouseButtonClicked = true;
    }
    break;
    case WM_LBUTTONUP:
    {
        lMouseButtonClicked = false;
    }
    break;
    case UM_NETWORK:
    {
        int err = WSAGETASYNCERROR(lParam);
        if (err == SOCKET_ERROR)
        {
            PostMessage(hWnd, WM_DESTROY, 0, 0);
            break;
        }
        switch (WSAGETSELECTEVENT(lParam))
        {
        case FD_CONNECT:
        {
            break;
        }
        case FD_CLOSE:
        {
            break;
        }
        case FD_WRITE:
        {
            bSendFlag = true;
            WriteEvent();
            break;
        }
        case FD_READ:
        {
            ReadEvent();
            break;
        }
        }
        break;
    }
    case WM_MOUSEMOVE:
    {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        if (lMouseButtonClicked)
        {
            //여기서 send
            Header header;
            DrawPacket packet;
            header.Len = 16;
            packet.startX = oldX;
            packet.startY = oldY;
            packet.endX = x;
            packet.endY = y;
            g_SendRingBuffer.Enqueue((const char*)&header, sizeof(header));
            g_SendRingBuffer.Enqueue((const char*)&packet, sizeof(packet));
            WriteEvent();
            //MoveToEx(hdc, oldX, oldY, nullptr);
            //LineTo(hdc, x, y);   
        }
        oldX = x;
        oldY = y;
    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        // TODO: 여기에 hdc를 사용하는 그리기 코드를 추가합니다...
        EndPaint(hWnd, &ps);
    }
        break;
    case WM_DESTROY:
        closesocket(g_Client);
        DeleteObject(pen);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int lastX = -1;
int lastY = -1;
void ReadEvent()
{
    char buffer[1000];
    int recvRet = recv(g_Client, buffer, 1000, 0);
    int retErr;
    if (recvRet == SOCKET_ERROR)
    {
        retErr = WSAGetLastError();
        if (retErr != WSAEWOULDBLOCK)
        {
            PostMessage(g_HWnd, WM_DESTROY, 0, 0);
            return;
        }
    }
    int eqRet = g_RecvRingBuffer.Enqueue(buffer, recvRet);
    if (eqRet == 0)
    {
        //recvRingBuffer가 가득찬거.
        //문제됨
        int* ptr = nullptr;
        *ptr = 134;
    }
    HDC hdc = GetDC(g_HWnd);
    while (true)
    {
        Header header;
        if (g_RecvRingBuffer.GetUseSize() < sizeof(DrawPacket)) break;
        int recvPeekRet = g_RecvRingBuffer.Peek((char*)&header, sizeof(Header));
        if (recvPeekRet == 0) break;
        g_RecvRingBuffer.MoveFront(sizeof(Header));
        DrawPacket packet;
        int recvDQRet = g_RecvRingBuffer.Dequeue((char*)&packet, sizeof(DrawPacket));
        if (recvDQRet == 0) break;
        MoveToEx(hdc, packet.startX, packet.startY, nullptr);
        LineTo(hdc, packet.endX, packet.endY);
    }
    ReleaseDC(g_HWnd, hdc);
    return;
}
void WriteEvent()
{
    if (!bSendFlag) return;
    while (true)
    {
        char buffer[1000]{ 0 };
        int sendPeekRet = g_SendRingBuffer.Peek(buffer, min(1000, g_SendRingBuffer.GetUseSize()));
        if (sendPeekRet == 0) break;
        int sendRet = 0;
        sendRet = send(g_Client, buffer, sendPeekRet, 0);
        if (sendRet == SOCKET_ERROR)
        {
            int sendErr = WSAGetLastError();
            if (sendErr != WSAEWOULDBLOCK)
            {
                //문제있음...
                PostMessage(g_HWnd, WM_DESTROY, 0, 0);
                break;
            }
            bSendFlag = false;
            break;
        }
        g_SendRingBuffer.MoveFront(sendRet);
    }
}
void SendPacket(char* buffer, int size)
{

}

// 정보 대화 상자의 메시지 처리기입니다.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
