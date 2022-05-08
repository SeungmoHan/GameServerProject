#include <Windows.h>
#include <process.h>
#include <stdio.h>
#include "LockFreeMemoryPoolTLS.hpp"
#include "DumpClass.h"
#include "Profiler.h"

struct Test
{
	char c[1024];
};

univ_dev::LockFreeMemoryPoolTLS<Test> pool;


int g_ThreadCount;
int g_AllocSize;

DWORD begin;

bool stopFlag = false;

__int64 tps;
int hhhh = false;

unsigned __stdcall Thread(void* param)
{
	Test** outArr = new Test *[g_AllocSize];
	if (hhhh)
	{
		while (true)
		{
			for (int i = 0; i < g_AllocSize; i++)
			{
				univ_dev::Profiler alloc("Alloc");
				outArr[i] = pool.Alloc();
			}
			//for (int i = 0; i < 10; i++)
			//	SwitchToThread();

			for (int i = 0; i < g_AllocSize; i++)
			{
				univ_dev::Profiler free("Free");
				pool.Free(outArr[i]);
			}
		}
	}
	else
	{
		while (true)
		{
			for (int i = 0; i < g_AllocSize; i++)
			{
				univ_dev::Profiler newTest("New");
				outArr[i] = new Test();
			}
			//for (int i = 0; i < 10; i++)
			//	SwitchToThread();
			for (int i = 0; i < g_AllocSize; i++)
			{
				univ_dev::Profiler deleteTest("Delete");
				delete outArr[i];
			}
		}
	}

}


int main()
{
	printf("thread count : ");
	scanf_s("%d", &g_ThreadCount);
	printf("alloc size : ");
	scanf_s("%d", &g_AllocSize);
	printf("0 -> new delete \n1 -> memorypool\n");
	scanf_s("%d", &hhhh);

	Test** outArr = new Test * [g_AllocSize];
	//for (int i = 0; i < 10; i++)
	//	SwitchToThread();
	for (int i = 0; i < g_AllocSize; i++)
	{
		outArr[i] = pool.Alloc();
	}


	for (int i = 0; i < g_AllocSize; i++)
	{
		pool.Free(outArr[i]);
	}
	delete outArr;
	for (int i = 0; i < g_ThreadCount; i++)
		_beginthreadex(nullptr, 0, Thread, nullptr, 0, nullptr);

	while (true)
	{
		short f1Key = GetAsyncKeyState(VK_F1);
		short f2Key = GetAsyncKeyState(VK_F2);
		if (f1Key)
			stopFlag = !stopFlag;
		if (f2Key)
			univ_dev::SaveProfiling();
		__int64 ret =InterlockedExchange((unsigned long long*) & tps, 0);
		printf("excute time : %u\nthread count : %d\nalloc size %d\nqueue size : %d\n",begin, g_ThreadCount, g_AllocSize, pool.GetUseCount());
		Sleep(1000);
	}
}