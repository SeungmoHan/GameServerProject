#include <Windows.h>
#include <iostream>
#include <synchapi.h>
#include <process.h>
#include <intrin.h>
using std::cout;
using std::endl;



int turn;

alignas(64)static volatile int flag0;
alignas(64)static volatile int flag1;

unsigned __int64 g_Count = 5000000;

alignas(64)static volatile unsigned __int64 g_RetCount;

unsigned __int64 g_InterLock_Thread1;
unsigned __int64 g_InterLock_Thread2;

int g_SetCount = 0;

unsigned __stdcall Thread1(void* param)
{
	unsigned __int64 i = g_Count;
	while (i > 0)
	{
		flag0 = true;
		turn = 0;
		//std::atomic_thread_fence(std::memory_order::memory_order_seq_cst);
		//_mm_mfence();
		//_ReadWriteBarrier();
		//__faststorefence();
		//_InterlockedOr(&g_InterLock_Thread1, 0);
		
		//int temp = flag1;
		//flag1 = temp;
		//while (temp == true && turn == 1)
		//	YieldProcessor();
		while (flag1 == true && turn == 0)
		{
			YieldProcessor();
			//printf("deadlock1\n");
		}
		i--;
		g_RetCount++;
		flag0 = false;
	}

	return 0;
}

unsigned __stdcall Thread2(void* param)
{
	unsigned __int64 i = g_Count;
	while (i > 0)
	{
		flag1 = true;
		turn = 1;
		std::atomic_thread_fence(std::memory_order::memory_order_seq_cst);
		//_mm_mfence();
		//_ReadWriteBarrier();
		//__faststorefence();
		//_InterlockedOr(&g_InterLock_Thread2, 0);

		//int temp = flag0;
		//flag0 = temp;
		//while (temp == true && turn == 1)
			//YieldProcessor();
		while (flag0 == true && turn == 1)
		{
			YieldProcessor();
			//printf("deadlock2\n");
		}
		i--;
		g_RetCount++;
		flag1 = false;
	}

	return 0;
}

HANDLE hThread[10];

HANDLE g_Event;

unsigned __stdcall ThreadFunction(void* param)
{
	while (true)
	{
		WaitForSingleObject(g_Event, INFINITE);

		printf("SetCount : %d\tthread id : %u\n", g_SetCount, GetCurrentThreadId());
	}
}
#include <conio.h>
void Post()
{
	while (true)
	{
		
		if (_kbhit())
		{
			int key = _getch();
			if (toupper(key) == 'Q')
				return;
			if (toupper(key) == 'A')
			{
				SetEvent(g_Event);
				g_SetCount++;
			}
		}
	}
}

int main()
{
	
	g_Event = CreateEvent(nullptr, false, false, nullptr);

	//for (int i = 0; i < 10; i++)
	//{
	//	hThread[i] = (HANDLE)_beginthreadex(nullptr, 0, ThreadFunction, nullptr, 0, nullptr);
	//}
	hThread[1] = (HANDLE)_beginthreadex(nullptr, 0, Thread2, nullptr, 0, nullptr);
	hThread[0] = (HANDLE)_beginthreadex(nullptr, 0, Thread1, nullptr, 0, nullptr);

	printf("WaitForObjects...\n");
	//Post();
	WaitForMultipleObjects(2, hThread, true, INFINITE);


	printf("g_RetCount value : %llu", g_RetCount);

	return 0;
}