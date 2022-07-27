#pragma comment(lib,"imm32.lib")
#pragma comment(lib,"Winmm.lib")
// TCPFighter_Procademy.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//

#include "framework.h"
#include "TCPFighter_Procademy.h"
#include <vector>
#include <time.h>
#include "PlayerObject.h"
#include <timeapi.h>
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
///         
/// to do ->
///         1. 네트워크 연결
///         2. 서버는 있고... 프로토콜 구현 (웃긴다 서버는 있는데 L7프로토콜이 없어서 통신을 못하는 꼬라지라닠ㅋㅋㅋ)
///         3. 
/// 





// 전역 변수:
HINSTANCE hInst;                                // 현재 인스턴스입니다.
WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.
HIMC g_hOldIMC;
HWND g_hWnd;
/// 이번 프레임이 20ms가 넘어버리면 프레임 스킵
/// 오버되는 시간 저장할 공간. <- 축적형태로 가야됨
/// 초당 총 루프 반복횟수 프레임수 저장할 공간. v
/// 초당 총 로직 반복횟수 프레임수 저장할 공간. v
/// 1초 측정 타이머 v
univ_dev::PlayerObject* gp_Player;
DWORD g_AllFramePerSec;
DWORD g_LogicFramePerSec;
bool g_isActive;
/// </전역 변수>






ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

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
    InitialGame();
    
    

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_TCPFIGHTERPROCADEMY, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

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
    return (int) msg.wParam;
}

clock_t g_1SecTimer = 0;

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
    g_1SecTimer = 0;
    g_AllFramePerSec = 0;
    g_LogicFramePerSec = 0;
    SetWindowTextW(g_hWnd, printBuffer);
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
    case WM_ACTIVATEAPP:
        if (LOWORD(wParam) && LOWORD(lParam))
            g_isActive = true;
        else g_isActive = false;
        break;
    //case WM_ACTIVATE:
    //    {
    //        int wmId = LOWORD(wParam);
    //        if (wmId) g_isActive = true;
    //        else g_isActive = false;
    //    }
    //    break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}





void KeyProcess()
{
    DWORD action = 12345;
    int direction = -1;
    SHORT leftKeyState = GetAsyncKeyState(VK_LEFT);
    SHORT rightKeyState = GetAsyncKeyState(VK_RIGHT);
    SHORT upKeyState = GetAsyncKeyState(VK_UP);
    SHORT downKeyState = GetAsyncKeyState(VK_DOWN);
    SHORT AkeyState = GetAsyncKeyState(0x41);
    SHORT SkeyState = GetAsyncKeyState(0x53);
    SHORT DkeyState = GetAsyncKeyState(0x44);

    if (AkeyState) action = ACTION_ATTACK1;
    else if (SkeyState) action = ACTION_ATTACK2;
    else if (DkeyState) action = ACTION_ATTACK3;
    else if (leftKeyState && upKeyState)
    {
        action = ACTION_MOVE_LU;
        direction = ACTION_MOVE_LL;
    }
    else if (leftKeyState && downKeyState)
    {
        action = ACTION_MOVE_LD;
        direction = ACTION_MOVE_LL;
    }
    else if (rightKeyState && upKeyState)
    {
        action = ACTION_MOVE_RU;
        direction = ACTION_MOVE_RR;
    }
    else if (rightKeyState && downKeyState)
    {
        action = ACTION_MOVE_RD;
        direction = ACTION_MOVE_RR;
    }
    else if (rightKeyState)
    {
        action = ACTION_MOVE_RR;
        direction = ACTION_MOVE_RR;
    }
    else if (leftKeyState)
    {
        action = ACTION_MOVE_LL;
        direction = ACTION_MOVE_LL;
    }
    else if (upKeyState)
    {
        action = ACTION_MOVE_UU;
        direction = gp_Player->GetDirection();
    }
    else if (downKeyState)
    {
        action = ACTION_MOVE_DD;
        direction = gp_Player->GetDirection();
    }
    else
    {
        action = 12345;
        direction = gp_Player->GetDirection();
    }
    if (action == 12345) gp_Player->InputActionProc(12345, direction);
    else gp_Player->InputActionProc(action, direction);
}

void Action()
{
    for (__int64 i = 0; i < univ_dev::g_ObjectList.size(); i++)
    {
        univ_dev::g_ObjectList[i]->Update();
    }
}

void Render()
{
    BYTE* dibBuffer = univ_dev::g_ScreenDibBuffer.GetDibBuffer();
    int width = univ_dev::g_ScreenDibBuffer.GetWidth();
    int height = univ_dev::g_ScreenDibBuffer.GetHeight();
    int pitch = univ_dev::g_ScreenDibBuffer.GetPitch();
    univ_dev::g_SpriteDibBuffer.DrawSprite((int)univ_dev::e_SPRITE::MAP, 0, 0, dibBuffer, width, height, pitch);

    LayerOrdering();
    for (__int64 i = 0; i < univ_dev::g_ObjectList.size(); i++)
    {
        univ_dev::g_ObjectList[i]->Render(dibBuffer, width, height, pitch);
    }
    univ_dev::g_ScreenDibBuffer.DrawBuffer(g_hWnd);
}
bool InitialGame()
{
    //load on memory
//C:\Users\user\Desktop\Education\procademy\TCPFighter\TCPFighter_Procademy\Sprite_Data
    //Load Map
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::MAP, L"Sprite_Data\\_Map.bmp", 0, 0)) return false;
    //Load Left Standing
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_STAND_L01, L"Sprite_Data/Stand_L_01.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_STAND_L02, L"Sprite_Data/Stand_L_02.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_STAND_L03, L"Sprite_Data/Stand_L_03.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_STAND_L04, L"Sprite_Data/Stand_L_02.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_STAND_L05, L"Sprite_Data/Stand_L_01.bmp", 71, 90)) return false;
    //Load Right Standing
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_STAND_R01, L"Sprite_Data/Stand_R_01.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_STAND_R02, L"Sprite_Data/Stand_R_02.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_STAND_R03, L"Sprite_Data/Stand_R_03.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_STAND_R04, L"Sprite_Data/Stand_R_02.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_STAND_R05, L"Sprite_Data/Stand_R_01.bmp", 71, 90)) return false;
    //Load Left Move
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_MOVE_L01, L"Sprite_Data/Move_L_01.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_MOVE_L02, L"Sprite_Data/Move_L_02.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_MOVE_L03, L"Sprite_Data/Move_L_03.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_MOVE_L04, L"Sprite_Data/Move_L_04.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_MOVE_L05, L"Sprite_Data/Move_L_05.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_MOVE_L06, L"Sprite_Data/Move_L_06.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_MOVE_L07, L"Sprite_Data/Move_L_07.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_MOVE_L08, L"Sprite_Data/Move_L_08.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_MOVE_L09, L"Sprite_Data/Move_L_09.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_MOVE_L10, L"Sprite_Data/Move_L_10.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_MOVE_L11, L"Sprite_Data/Move_L_11.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_MOVE_L12, L"Sprite_Data/Move_L_12.bmp", 71, 90)) return false;
    //Load Right Move
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_MOVE_R01, L"Sprite_Data/Move_R_01.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_MOVE_R02, L"Sprite_Data/Move_R_02.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_MOVE_R03, L"Sprite_Data/Move_R_03.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_MOVE_R04, L"Sprite_Data/Move_R_04.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_MOVE_R05, L"Sprite_Data/Move_R_05.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_MOVE_R06, L"Sprite_Data/Move_R_06.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_MOVE_R07, L"Sprite_Data/Move_R_07.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_MOVE_R08, L"Sprite_Data/Move_R_08.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_MOVE_R09, L"Sprite_Data/Move_R_09.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_MOVE_R10, L"Sprite_Data/Move_R_10.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_MOVE_R11, L"Sprite_Data/Move_R_11.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_MOVE_R12, L"Sprite_Data/Move_R_12.bmp", 71, 90)) return false;
    //Load Left Attack1
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_ATTACK1_L01, L"Sprite_Data/Attack1_L_01.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_ATTACK1_L02, L"Sprite_Data/Attack1_L_02.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_ATTACK1_L03, L"Sprite_Data/Attack1_L_03.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_ATTACK1_L04, L"Sprite_Data/Attack1_L_04.bmp", 71, 90)) return false;
    //Load Right Attack1
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_ATTACK1_R01, L"Sprite_Data/Attack1_R_01.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_ATTACK1_R02, L"Sprite_Data/Attack1_R_02.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_ATTACK1_R03, L"Sprite_Data/Attack1_R_03.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_ATTACK1_R04, L"Sprite_Data/Attack1_R_04.bmp", 71, 90)) return false;
    //Load Left Attack2
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_ATTACK2_L01, L"Sprite_Data/Attack2_L_01.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_ATTACK2_L02, L"Sprite_Data/Attack2_L_02.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_ATTACK2_L03, L"Sprite_Data/Attack2_L_03.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_ATTACK2_L04, L"Sprite_Data/Attack2_L_04.bmp", 71, 90)) return false;
    //Load Right Attack2
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_ATTACK2_R01, L"Sprite_Data/Attack2_R_01.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_ATTACK2_R02, L"Sprite_Data/Attack2_R_02.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_ATTACK2_R03, L"Sprite_Data/Attack2_R_03.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_ATTACK2_R04, L"Sprite_Data/Attack2_R_04.bmp", 71, 90)) return false;
    //Load Left Attack3
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_ATTACK3_L01, L"Sprite_Data/Attack3_L_01.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_ATTACK3_L02, L"Sprite_Data/Attack3_L_02.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_ATTACK3_L03, L"Sprite_Data/Attack3_L_03.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_ATTACK3_L04, L"Sprite_Data/Attack3_L_04.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_ATTACK3_L05, L"Sprite_Data/Attack3_L_05.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_ATTACK3_L06, L"Sprite_Data/Attack3_L_06.bmp", 71, 90)) return false;
    //Load Right Attack3
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_ATTACK3_R01, L"Sprite_Data/Attack3_R_01.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_ATTACK3_R02, L"Sprite_Data/Attack3_R_02.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_ATTACK3_R03, L"Sprite_Data/Attack3_R_03.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_ATTACK3_R04, L"Sprite_Data/Attack3_R_04.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_ATTACK3_R05, L"Sprite_Data/Attack3_R_05.bmp", 71, 90)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::PLAYER_ATTACK3_R06, L"Sprite_Data/Attack3_R_06.bmp", 71, 90)) return false;
    //Load Effect Spark
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::EFFECT_SPARK_01, L"Sprite_Data/xSpark_1.bmp", 70, 70)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::EFFECT_SPARK_02, L"Sprite_Data/xSpark_2.bmp", 70, 70)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::EFFECT_SPARK_03, L"Sprite_Data/xSpark_3.bmp", 70, 70)) return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::EFFECT_SPARK_04, L"Sprite_Data/xSpark_4.bmp", 70, 70)) return false;
    //Load HPGuage, Shadow
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::GUAGE_HP, L"Sprite_Data/HPGuage.bmp", 35, -10))return false;
    if (!univ_dev::g_SpriteDibBuffer.LoadDibSprite((int)univ_dev::e_SPRITE::SHADOW, L"Sprite_Data/Shadow.bmp", 32, 4))return false;

    //PlayerObject* pPlayer = new PlayerObject(...);
    univ_dev::PlayerObject* p_Player = new univ_dev::PlayerObject(100, 100, e_OBJECT_TYPE::TYPE_PLAYER, true, (int)univ_dev::e_SPRITE::PLAYER_STAND_L01, (int)univ_dev::e_SPRITE::PLAYER_STAND_L_MAX);
    univ_dev::PlayerObject* otherPlayer = new univ_dev::PlayerObject(100, 200, e_OBJECT_TYPE::TYPE_PLAYER, false, (int)univ_dev::e_SPRITE::PLAYER_STAND_L01, (int)univ_dev::e_SPRITE::PLAYER_STAND_L_MAX);
    univ_dev::g_ObjectList.push_back((univ_dev::BaseObject*)(p_Player));
    univ_dev::g_ObjectList.push_back((univ_dev::BaseObject*)(otherPlayer));
    gp_Player = p_Player;

    /// network 연결되었을땐 서버에서 생성하라는 만큼 생성해주면됨.
    return true;
}

void LayerOrdering()
{
    for (__int64 i = 0; i < univ_dev::g_ObjectList.size()-1; i++)
    {
        for (__int64 j = i; j < univ_dev::g_ObjectList.size()-1; j++)
        {
            if (univ_dev::g_ObjectList[j]->GetCurrentY() > univ_dev::g_ObjectList[j+1]->GetCurrentY())
                std::swap(univ_dev::g_ObjectList[j + 1], univ_dev::g_ObjectList[j]);

        }
    }
    for (__int64 i = 0; i < univ_dev::g_ObjectList.size() - 1; i++)
    {
        for (__int64 j = i; j < univ_dev::g_ObjectList.size() - 1; j++)
        {
            if (univ_dev::g_ObjectList[j]->GetObjectType() == e_OBJECT_TYPE::TYPE_EFFECT && univ_dev::g_ObjectList[j+1]->GetObjectType() != e_OBJECT_TYPE::TYPE_EFFECT)
                std::swap(univ_dev::g_ObjectList[j + 1], univ_dev::g_ObjectList[j]);
        }
    }
}


bool frameSkipFlag;
WCHAR allFrameBuffer[10]{ 0 };
int cur = timeGetTime();
int old = timeGetTime();
int deltaTime;
int skipTime = 0;

int prev = timeGetTime();

void Update()
{
    //NetWorking();
    if (g_isActive)
        KeyProcess();
    Action();
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
    }
}
