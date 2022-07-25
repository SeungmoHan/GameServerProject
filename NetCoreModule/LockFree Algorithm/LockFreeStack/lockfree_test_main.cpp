#pragma comment(lib ,"Winmm.lib")
#include"LockFreeStack.hpp"
#include <stdio.h>
#include <process.h>
#include <time.h>
#include <sys/locking.h>

unsigned long long g_ThreadCount = 4;
unsigned long long g_AllocSize = 500;

struct temp
{
	int a;
	double b;
	__int64 c;
};

class TestClass
{
	int a;
	DWORD b;
	__int64 c;
	char d;
	void* e[2];
	temp t;
};

alignas(512) univ_dev::LockFreeStack<int> stack;
alignas(512) univ_dev::LockFreeMemoryPool<TestClass> lf;





unsigned __stdcall Thread(void* param)
{
	int r;
	int i = 0;
	TestClass** arr = new TestClass * [g_AllocSize];
	while (true)
	{
		ZeroMemory(arr, sizeof(TestClass*) * g_AllocSize);
		//for (int i = 0; i < 10; i++)
		//	SwitchToThread();
		for (int i = 0; i < g_AllocSize; i++)
			arr[i] = lf.Alloc();
		for (int i = 0; i < g_AllocSize; i++)
		{
			if (!stack.pop(r)) CRASH();
			if (r != 0xffff5555)CRASH();
		}
		//for (int i = 0; i < 10; i++)
		//	SwitchToThread();
		for (int i = 0; i < g_AllocSize; i++)
			if (!lf.Free(arr[i]))CRASH();
		for (int i = 0; i < g_AllocSize; i++)
		{
			stack.push(0xffff5555);
		}
	}
}


int main()
{
	timeBeginPeriod(1);
	printf("Thread Count : ");
	scanf_s("%llu", &g_ThreadCount);
	printf("Alloc size : ");
	scanf_s("%llu", &g_AllocSize);
	TestClass** ptr = new TestClass * [g_AllocSize * g_ThreadCount];

	for (int i = 0; i < g_AllocSize * g_ThreadCount; i++)
	{
		stack.push(0xffff5555);
		ptr[i] = lf.Alloc();
		if (i == 4998)
		{
			int a = 0;
			a++;
		}
		if (ptr[i] == nullptr) CRASH();
	}
	int a = 0;
	new int[100];
	a++;
	for (int i = 0; i < g_AllocSize * g_ThreadCount; i++)
	{
		int r;
		//if (!stack.pop(r)) CRASH();
		if (!lf.Free(ptr[i]))CRASH();
		if (ptr[i] == nullptr) CRASH();
	}

	for (int i = 0; i < g_ThreadCount; i++)
	{
		_beginthreadex(nullptr, 0, Thread, nullptr, 0, nullptr);
	}

	DWORD beginTime = timeGetTime();
	while (true)
	{
		Sleep(1000);
		printf("------------------------------------------------------------\n");
		printf("stack size :                            %d\n", stack.size());
		printf("stack -> lfpool size :                  %d\n", stack.pool_size());
		printf("stack -> lfpool capacity :              %d\n", stack.pool_capacity());
		printf("LockFreeMemoryPool GetCapacityCount :	%d\n", lf.GetCapacityCount());
		printf("LockFreeMemoryPool GetUseCount :        %d\n", lf.GetUseCount());
		printf("excute time :                           %d\n", (timeGetTime() - beginTime) / 1000);
		printf("------------------------------------------------------------\n");
	}


	Sleep(INFINITE);
}