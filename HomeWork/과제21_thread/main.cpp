#include <Windows.h>
#include <process.h>
#include <iostream>



int g_Connect;
alignas(64)int g_Data;
bool g_Shutdown;
HANDLE g_EventArr[5];

unsigned __stdcall AcceptThread(void* param)
{
	printf("AcceptThread Created\n");
	int idx = *(int*)param;
	SetEvent(g_EventArr[idx]);
	//srand는 tls에 저장되는값이므로... 각 쓰레드마다 해줘야됨
	srand(time(nullptr)-1);
	int r;
	while (!g_Shutdown)
	{
		Sleep(rand() % 900 + 100);
		r = _InterlockedIncrement((unsigned int*)&g_Connect);
		//printf("Accept : %d\n", r);
	}
	printf("AcceptThread Stop\n");
	return 0;
}

unsigned __stdcall DisconnectThread(void* param)
{
	printf("DisconnectThread Created\n");
	//srand는 tls에 저장되는값이므로... 각 쓰레드마다 해줘야됨
	int idx = *(int*)param;
	SetEvent(g_EventArr[idx]);
	srand(time(nullptr)+1);
	int r;
	while (!g_Shutdown)
	{
		Sleep(rand() % 900 + 100);
		r = _InterlockedExchangeAdd((unsigned int*)&g_Connect, 0);
		if (r <= 0) continue;
		r = _InterlockedDecrement((unsigned int*)&g_Connect);
		//printf("Disconnect  : %d\n", r);
	}
	printf("DisconnectThread Stop\n");
	return 0;
}

unsigned __stdcall UpdateThread(void* param)
{
	printf("UpdateThread Created\n");
	int idx = *(int*)param;
	SetEvent(g_EventArr[idx]);
	int r;
	while (!g_Shutdown)
	{
		Sleep(10);
		r = _InterlockedIncrement((unsigned int*)&g_Data);
		if (r % 1000 == 0)
		{
			printf("Data : %d\n", r);
		}
	}
	printf("Update Thread Stop\n");
	return 0;
}


int main()
{
	HANDLE hThreadArr[5];
	int arr[5]{ 0,1,2,3,4 };
	int i = 0;
	for (int i = 0; i < 5; i++)
	{
		g_EventArr[i] = CreateEvent(nullptr, false, false, nullptr);
		if (g_EventArr[i] == nullptr) return -1;
	}
	for (; i < 3;)
		hThreadArr[i] = (HANDLE)_beginthreadex(nullptr, 0, UpdateThread, &arr[i++], 0, nullptr);
	hThreadArr[3] = (HANDLE)_beginthreadex(nullptr, 0, AcceptThread, &arr[i++], 0, nullptr);
	hThreadArr[4] = (HANDLE)_beginthreadex(nullptr, 0, DisconnectThread, &arr[i] , 0, nullptr);

	WaitForMultipleObjects(5, g_EventArr, true, INFINITE);
	printf("Thread Create End\n");


	for (int i = 0; i < 20; i++)
	{
		Sleep(1000);
		printf("g_Connect : %d\n", g_Connect);
	}
	g_Shutdown = true;
	printf("Wait For Thread...\n");
	WaitForMultipleObjects(5, hThreadArr, true, INFINITE);
	printf("All Thread Stoped\n");
	
	for (int i = 0; i < 5; i++)
		CloseHandle(g_EventArr[i]);

	return 0;
}