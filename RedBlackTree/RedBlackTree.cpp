// PrintRedBlackTree.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//
#ifdef UNICODE
#pragma comment(linker, "/entry:wWinMainCRTStartup /subsystem:console")
#else
#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
#endif

#define __PRINT_WITH_WINAPI
#include <iostream>
#include <thread>
#include <time.h>
#include <conio.h>
#include "framework.h"
#include "RBtree.h"
#include "RedBlackTree.h"
#define MAX_LOADSTRING 100

// 전역 변수:
HINSTANCE hInst;                                // 현재 인스턴스입니다.
WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.
HWND g_hEditBox;

WCHAR g_EditString[MAX_LOADSTRING];


// 이 코드 모듈에 포함된 함수의 선언을 전달합니다:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
void InputProc();

univ_dev::RedBlackTree g_RedBlackTree;
int g_XBasePos = 0;
int g_YBasePos = 0;
HBRUSH g_RedBrush;
HBRUSH g_BlackBrush;
HWND g_hWnd;
bool g_InputWorking = true;
std::thread inputThread(InputProc);
bool endFlag = false;

// ... endflag가 true가되도 scanf_s에서 멍때리고있어서 키타입한번더 해줘야됨.
void InputProc()
{
    while (!endFlag)
    {
        int temp;
        scanf_s("%d", &temp);
        if (g_InputWorking)
        {
            g_RedBlackTree.Insert(temp);
            InvalidateRect(g_hWnd, nullptr, true);
        }
        else
        {
            g_RedBlackTree.Remove(temp);
            InvalidateRect(g_hWnd, nullptr, true);
        }
    }
}

#ifdef __PRINT_WITH_WINAPI
namespace univ_dev
{
    void RedBlackTree::Print(HDC hdc)
    {
        int depth = 0;
        int x = 30;
        WCHAR buffer[10]{ 0 };
        _itow_s(GetNodeCount(), buffer, 10);
        TextOut(hdc, 100, 500, buffer, wcslen(buffer));
        if (root == &m_NilNode) return;
        Print(root, depth, x, hdc);
    }

    void RedBlackTree::Print(Node* root, int& depth, int& x, HDC hdc)
    {
        if (root == &m_NilNode) return;
        depth++;
        if (root->pLeftNode != &m_NilNode)
            Print(root->pLeftNode, depth, x, hdc);

        if (root->pLeftNode != &m_NilNode)
        {
            MoveToEx(hdc, g_XBasePos + x + 15, g_YBasePos + (depth) * 30, nullptr);
            LineTo(hdc, g_XBasePos + x + 15 - ((GetRightChildCount(root->pLeftNode)+1) * 30), g_YBasePos + (depth + 1) * 30);
        }
        if (root->pRightNode != &m_NilNode)
        {
            MoveToEx(hdc, g_XBasePos + x + 15, g_YBasePos + (depth) * 30, nullptr);
            LineTo(hdc, g_XBasePos + x + 15 + ((GetLeftChildCount(root->pRightNode) + 1) * 30), g_YBasePos + (depth + 1) * 30);
        }

        if (root->eNodeColor == NodeColor::BLACK)
            SelectObject(hdc, g_BlackBrush);
        else SelectObject(hdc, g_RedBrush);

        WCHAR print[10]{ 0 };
        WCHAR temp[10]{ 0 };
        Ellipse(hdc, g_XBasePos + x, g_YBasePos + (depth * 30) - 15, g_XBasePos + x + 30, g_YBasePos + (depth * 30) + 15);
        x += 30;
        _itow_s(root->key, temp, 10);
        wcscat_s(print, temp);
        TextOut(hdc, g_XBasePos + x - 23, g_YBasePos + (depth * 30) - 7, print, wcslen(print));
        if (root->pRightNode != &m_NilNode)
            Print(root->pRightNode, depth, x, hdc);
        depth--;
    }
}
#endif
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    srand(time(nullptr));
    // TODO: 여기에 코드를 입력합니다.
    // 전역 문자열을 초기화합니다.
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_REDBLACKTREE, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 애플리케이션 초기화를 수행합니다:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_REDBLACKTREE));

    MSG msg;



    g_BlackBrush = CreateSolidBrush(RGB(0, 0, 0));
    g_RedBrush = CreateSolidBrush(RGB(255, 0, 0));
    endFlag = false;
    // 기본 메시지 루프입니다:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return (int)msg.wParam;
}



ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDC_REDBLACKTREE));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_REDBLACKTREE);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW | ES_AUTOHSCROLL | WS_BORDER,
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
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        g_RedBlackTree.Print(hdc);
        EndPaint(hWnd, &ps);
    }
    break;
    case WM_KEYDOWN:
    {
        switch (wParam)
        {
        case VK_F1:
        {
            g_RedBlackTree.Insert(rand() % 101);
            break;
        }
        case VK_F2:
        {
            g_InputWorking = true;
            break;
        }
        case VK_F3:
        {
            g_InputWorking = false;
            break;
        }

        case VK_F9:
        {
            g_RedBlackTree.Release();
            break;
        }
        case VK_LEFT:
        {
            g_XBasePos += 100;
            break;
        }
        case VK_RIGHT:
        {
            g_XBasePos -= 100;
            break;
        }
        case VK_UP:
        {
            g_YBasePos += 100;
            break;
        }
        case VK_DOWN:
        {
            g_YBasePos -= 100;
            break;
        }
        }
        InvalidateRect(hWnd, nullptr, true);
        break;
    }
    case WM_DESTROY:
        endFlag = true;
        inputThread.join();
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
