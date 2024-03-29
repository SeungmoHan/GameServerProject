﻿// PathFinder.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//
//#ifdef UNICODE
//#pragma comment(linker, "/entry:wWinMainCRTStartup /subsystem:console")
//#else
//#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
//#endif
#include <windowsx.h>
#include "framework.h"
#include "PathFinder.h"
#include "TileMap.h"
#include "AStarPathFinder.h"
#include <iostream>
#include <algorithm>
#include "profiler.h"
#define MAX_LOADSTRING 100

// 전역 변수:
HINSTANCE hInst;                                // 현재 인스턴스입니다.
WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.

using PathNode = PathFinder::PathNode;


constexpr int g_InchPerTile = 24;



HWND g_hWnd;
TileMap g_TileMap;
// 이 코드 모듈에 포함된 함수의 선언을 전달합니다:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

PathFinder g_PathFinder;
std::list<PathFinder::PathNode> g_Path;
PathFinder::PathNode* g_PathNode;



//자 이제 클래스화 된걸 전역에 흩뿌릴겁니다...
//이렇게 해야지 한 루프에 한큐씩 돌릴수있거든요...

HDC primaryHDC;
HDC secondaryHDC;

HPEN g_WhitePen;
HPEN g_BlackPen;
HPEN g_YellowPen;

bool changeTileFlag;
bool isTileBlack;

HBRUSH g_BlackBrush;
HBRUSH g_WhiteBrush;
HBRUSH g_GrayBrush;
HBRUSH g_BlueBrush;
HBRUSH g_RedBrush;
HBRUSH g_GreenBrush;

int g_MouseXPos;
int g_MouseYPos;

int g_BeginXPoint;
int g_BeginYPoint;

int g_EndXPoint;
int g_EndYPoint;

bool g_OneQueueFlag;
clock_t beginTime;
clock_t endTime;

PathNode* lastRet;
void ClearPath(PathFinder::PathNode*& pathNode);
void PrintTile(TileMap& tile, HDC hdc);
void PrintTile(TileMap& tile, HDC hdc);
void DrawPath(PathFinder::PathNode* nodeList, HDC hdc);


void ClearPath(PathFinder::PathNode*& pathNode)
{
    PathFinder::PathNode* temp = pathNode;
    while (pathNode != nullptr)
    {
        pathNode = pathNode->parent;
        delete temp;
        temp = pathNode;
    }
    pathNode = nullptr;
}
void PrintTile(TileMap& tile ,HDC hdc)
{
    //system("cls");
    for (int i = 0; i < TileMap::TileMapSize::Height; i++)
    {
        for (int j = 0; j < TileMap::TileMapSize::Width; j++)
        {
            if (tile.blockTile[i][j])
            {
                SelectObject(hdc, g_BlackBrush);
                SelectObject(hdc, g_WhitePen);
            }
            else if(tile.openList[i][j])
            {
                SelectObject(hdc, g_GrayBrush);
                SelectObject(hdc, g_BlackPen);
            }
            else if (tile.closeList[i][j])
            {
                SelectObject(hdc, g_BlueBrush);
                SelectObject(hdc, g_WhitePen);
            }
            else
            {
                SelectObject(hdc, g_WhiteBrush);
                SelectObject(hdc, g_BlackPen);
            }
            Rectangle(hdc,
                (g_InchPerTile * j) - (g_InchPerTile / 2) + (g_InchPerTile / 2),
                (g_InchPerTile * i) - g_InchPerTile / 2 + g_InchPerTile / 2,
                (g_InchPerTile * j) + g_InchPerTile / 2 + g_InchPerTile / 2,
                (g_InchPerTile * i) + g_InchPerTile / 2 + g_InchPerTile / 2);
            ////printf("%d ", tile.blockTile[i][j]);
        }
        SelectObject(hdc, g_BlackPen);
        SelectObject(hdc, g_GreenBrush);
        Rectangle(hdc,
            g_BeginXPoint * g_InchPerTile - g_InchPerTile / 2 + g_InchPerTile / 2,
            g_BeginYPoint * g_InchPerTile - g_InchPerTile / 2 + g_InchPerTile / 2,
            g_BeginXPoint * g_InchPerTile + g_InchPerTile / 2 + g_InchPerTile / 2,
            g_BeginYPoint * g_InchPerTile + g_InchPerTile / 2 + g_InchPerTile / 2);
        SelectObject(hdc, g_RedBrush);
        Rectangle(hdc,
            g_EndXPoint * g_InchPerTile - g_InchPerTile / 2 + g_InchPerTile / 2,
            g_EndYPoint * g_InchPerTile - g_InchPerTile / 2 + g_InchPerTile / 2,
            g_EndXPoint * g_InchPerTile + g_InchPerTile / 2 + g_InchPerTile / 2,
            g_EndYPoint * g_InchPerTile + g_InchPerTile / 2 + g_InchPerTile / 2);
        ////printf("\n");
    }
    //SelectObject(hdc, g_GrayBrush);
    //Rectangle(hdc, (g_MouseXPos / 32) * 32 - 16 + 16, (g_MouseYPos / 32) * 32 - 16 + 16, (g_MouseXPos / 32) * 32 + 16 + 16, (g_MouseYPos / 32) * 32 + 16 + 16);
    ReleaseDC(g_hWnd, hdc);

}

void DrawPath(PathFinder::PathNode* nodeList,HDC hdc)
{
    if (nodeList == nullptr)return;
    //HDC hdc = GetDC(g_hWnd);
    SelectObject(hdc, g_YellowPen);
    
    PathFinder::PathNode* temp = nodeList;
    int i = 0;
    MoveToEx(hdc, temp->x * g_InchPerTile + g_InchPerTile/2, temp->y * g_InchPerTile + g_InchPerTile/2, nullptr);
    while (temp != nullptr)
    {
        LineTo(hdc, temp->x * g_InchPerTile + g_InchPerTile / 2, temp->y * g_InchPerTile + g_InchPerTile / 2);
        WCHAR buffer[10]{ 0 };
        _itow_s(i++, buffer, 10);
        TextOut(hdc, temp->x * g_InchPerTile + g_InchPerTile / 2, temp->y * g_InchPerTile + g_InchPerTile / 2, buffer, wcslen(buffer));
        temp = temp->parent;
    }
    return;
    //temp = nodeList;
    //while (nodeList != nullptr)
    //{
    //    nodeList = nodeList->parentList;
    //    delete temp;
    //    temp = nodeList;
    //}
}

void DrawPath(std::list<PathFinder::PathNode>& nodeList)
{
    if (nodeList.size() == 0) return;
    HDC hdc = GetDC(g_hWnd);
    SelectObject(hdc, g_YellowPen); 

    //printf("Draw Path : \n");
    auto nodeIter = nodeList.begin();
    MoveToEx(hdc, nodeIter->x * g_InchPerTile + g_InchPerTile/2, nodeIter->y * g_InchPerTile + g_InchPerTile/2, nullptr);
    for (; nodeIter != nodeList.end(); ++nodeIter)
    {
        LineTo(hdc, nodeIter->x * g_InchPerTile + g_InchPerTile/2, nodeIter->y * g_InchPerTile + g_InchPerTile/2);
        //printf("node : %d, %d\n", nodeIter->x, nodeIter->y);
    }
    ReleaseDC(g_hWnd, hdc);
    //g_PathFinder.RemoveFoundPath(node);
}





int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    // 전역 문자열을 초기화합니다.
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_PATHFINDER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 애플리케이션 초기화를 수행합니다:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    g_WhitePen = CreatePen(PS_SOLID, 0, RGB(255, 255, 255));
    g_BlackPen = CreatePen(PS_SOLID, 0, RGB(50, 50, 50));
    g_YellowPen = CreatePen(PS_SOLID, 5, RGB(255, 255, 0));

    g_BlackBrush = CreateSolidBrush(RGB(0, 0, 0));
    g_WhiteBrush = CreateSolidBrush(RGB(255, 255, 255));
    g_GrayBrush = CreateSolidBrush(RGB(128, 128, 128));
    g_GreenBrush = CreateSolidBrush(RGB(0, 128, 0));
    g_RedBrush = CreateSolidBrush(RGB(255, 0, 0));
    g_BlueBrush = CreateSolidBrush(RGB(0, 0, 127));
    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PATHFINDER));

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
            bool success;
            if (lastRet == nullptr)
            {
                success = g_PathFinder.GetNextPoint(g_TileMap, lastRet, g_BeginXPoint, g_EndXPoint, g_BeginYPoint, g_EndYPoint);
            }
            else
            {
                success = g_PathFinder.GetNextPoint(g_TileMap, lastRet, lastRet->x, g_EndXPoint, lastRet->y, g_EndYPoint);
                InvalidateRect(g_hWnd, nullptr, true);
            }
            if (success)
            {
                g_PathNode = lastRet;
                lastRet = nullptr;
                success = false;
            }
        }
    }

    return (int) msg.wParam;
}



//
//  함수: MyRegisterClass()
//
//  용도: 창 클래스를 등록합니다.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PATHFINDER));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_PATHFINDER);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   함수: InitInstance(HINSTANCE, int)
//
//   용도: 인스턴스 핸들을 저장하고 주 창을 만듭니다.
//
//   주석:
//
//        이 함수를 통해 인스턴스 핸들을 전역 변수에 저장하고
//        주 프로그램 창을 만든 다음 표시합니다.
//
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
   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  함수: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  용도: 주 창의 메시지를 처리합니다.
//
//  WM_COMMAND  - 애플리케이션 메뉴를 처리합니다.
//  WM_PAINT    - 주 창을 그립니다.
//  WM_DESTROY  - 종료 메시지를 게시하고 반환합니다.
//
//
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
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WM_MOUSEMOVE:
    {
        g_MouseXPos = GET_X_LPARAM(lParam);
        g_MouseYPos = GET_Y_LPARAM(lParam);
        if (changeTileFlag)
        {
            if (g_TileMap.BlockTile(g_MouseXPos / g_InchPerTile, g_MouseYPos / g_InchPerTile, !isTileBlack))
                InvalidateRect(g_hWnd, nullptr, true);
        }
        break;
    }
    case WM_LBUTTONDOWN:
    {
        if (!changeTileFlag)
        {
            isTileBlack = g_TileMap.IsBlocked(g_MouseXPos / g_InchPerTile, g_MouseYPos / g_InchPerTile);
            changeTileFlag = true;
        }
        if (g_TileMap.BlockTile(g_MouseXPos / g_InchPerTile, g_MouseYPos / g_InchPerTile, !isTileBlack))
            InvalidateRect(g_hWnd, nullptr, true);
        break;
    }
    case WM_LBUTTONUP:
    {
        changeTileFlag = false;
        InvalidateRect(g_hWnd, nullptr, true);
        break;
    }
    case WM_MBUTTONDOWN:
    {
        g_BeginXPoint = g_MouseXPos / g_InchPerTile;
        g_BeginYPoint = g_MouseYPos / g_InchPerTile;
        InvalidateRect(g_hWnd, nullptr, true);
        break;
    }
    case WM_RBUTTONDOWN:
    {
        g_EndXPoint = g_MouseXPos / g_InchPerTile;
        g_EndYPoint = g_MouseYPos / g_InchPerTile;
        InvalidateRect(g_hWnd, nullptr, true);
        break;
    }
    case WM_KEYDOWN:
    {
        switch (wParam)
        {
        case VK_F1:
            {
                if (g_PathFinder.StartPathFinding())
                {
                    ClearPath(g_PathNode);
                    ClearPath(lastRet);
                    g_TileMap.ResetCloseList();
                    g_TileMap.ResetOpenList();
                    //g_Path = g_PathFinder.FindPath(g_TileMap, g_BeginXPoint, g_EndXPoint, g_BeginYPoint, g_EndYPoint);
                    //g_PathNode = g_PathFinder.TempFindPath(g_TileMap, g_BeginXPoint, g_EndXPoint, g_BeginYPoint, g_EndYPoint);
                }
                break;
            }
            case VK_F3:
            {
                if (!g_PathFinder.IsFindingPath())
                {
                    ClearPath(g_PathNode);
                    ClearPath(lastRet);
                    g_OneQueueFlag = true;
                    beginTime = clock();
                    g_PathNode = g_PathFinder.FindPath(g_TileMap, g_BeginXPoint, g_EndXPoint, g_BeginYPoint, g_EndYPoint);
                    endTime = clock();
                    InvalidateRect(g_hWnd, nullptr, true);
                }
                break;
            }
            case VK_F4:
            {
                if (!g_PathFinder.IsFindingPath())
                {
                    //g_Path.clear();
                    ClearPath(g_PathNode);
                    ClearPath(lastRet);
                    g_TileMap.ResetCloseList();
                    g_TileMap.ResetOpenList();
                    InvalidateRect(g_hWnd, nullptr, true);
                }
                break;
            }
            case VK_F5:
            {
                if (!g_PathFinder.IsFindingPath())
                {
                    g_TileMap.ResetBlockTile();
                    g_TileMap.ResetCloseList();
                    g_TileMap.ResetOpenList();
                    ClearPath(g_PathNode);
                    ClearPath(lastRet);
                    InvalidateRect(g_hWnd, nullptr, true);
                }
                break;
            }
            case VK_F6:
            {
                SaveProfiling();
                break;
            }
        }
        break;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        HDC MemDC = CreateCompatibleDC(hdc);
        HBITMAP m_hbmpBuff; // Buffer DC HBITMAP 
        RECT rect; // 윈도우 크기 읽기
        GetWindowRect(g_hWnd, &rect);
        m_hbmpBuff = CreateCompatibleBitmap(hdc, rect.right,rect.bottom);
        SelectObject(MemDC, m_hbmpBuff);
        PatBlt(MemDC, 0, 0, rect.right, rect.bottom, WHITENESS);

        TextOut(MemDC, 60 * g_InchPerTile + 40, 150, L"■■■■불친절한 사용법 안내■■■■", 19);
        TextOut(MemDC, 60 * g_InchPerTile + 40, 200, L"1. 마우스 왼쪽 드래그 벽생성", 17);
        TextOut(MemDC, 60 * g_InchPerTile + 40, 250, L"2. 마우스 중간 버튼 누르기 StartPoint이동", 29);
        TextOut(MemDC, 60 * g_InchPerTile + 40, 300, L"3. 마우스 오른쪽 버튼 누르기 EndPoint이동", 28);
        TextOut(MemDC, 60 * g_InchPerTile + 40, 350, L"4. 키보드 F1 시작", 12);
        TextOut(MemDC, 60 * g_InchPerTile + 40, 400, L"5. 키보드 F3 한번에 찾기", 16);
        TextOut(MemDC, 60 * g_InchPerTile + 40, 450, L"6. 키보드 F4 Path 초기화", 18);
        TextOut(MemDC, 60 * g_InchPerTile + 40, 500, L"7. 키보드 F5 Path + 벽 초기화", 22);
        WCHAR buffer[50]{ 0 };
        WCHAR temp[10]{ 0 };
        _itow_s(g_OpenListSize, temp, 10);
        wcscat_s(buffer,  L"오픈리스트 사이즈 : ");
        wcscat_s(buffer, temp);
        TextOut(MemDC, 60 * g_InchPerTile + 40, 700, buffer, wcslen(buffer));
        if (g_OneQueueFlag)
        {
            g_OneQueueFlag = false;
            WCHAR buffer[50]{ 0 };
            WCHAR temp[10]{ 0 };
            _itow_s(endTime - beginTime, temp, 10);
            wcscat_s(buffer, L"걸린 시간 : ");
            wcscat_s(buffer, temp);
            wcscat_s(buffer, L"ms,   연산횟수 : ");
            _itow_s(g_OperationCounting, temp, 10);
            g_OperationCounting = 0;
            wcscat_s(buffer, temp);
            TextOut(MemDC, 60 * g_InchPerTile + 40, 600, buffer, wcslen(buffer));
        }
        TextOut(MemDC, 400, 40 * g_InchPerTile, L"경로에 숫자가 처음엔 거꾸로 나오다가 나중에 뒤집어집니다. 이상한거 아닙니다. 일부로 보기좋으라고 한번더 뒤집는겁니다.", 66);
        TextOut(MemDC, 500, 40 * g_InchPerTile + 20, L"PS 스파게티입니다(맛없습니다) 더블버퍼링 안했습니다 눈도 아픕니다", 37);
        PrintTile(g_TileMap, MemDC);
        DrawPath(lastRet,MemDC);
        DrawPath(g_PathNode,MemDC);
        DrawPath(lastRet, MemDC);
        DrawPath(g_PathNode, MemDC);
        BitBlt(hdc, 0, 0, rect.right, rect.bottom, MemDC, 0, 0, SRCCOPY);
        DeleteDC(MemDC);
        Sleep(5);
        DeleteBitmap(m_hbmpBuff);
        EndPaint(hWnd, &ps);
        break;
        
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
