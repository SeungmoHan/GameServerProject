#include "LockFreeQueue.hpp"
#include "DumpClass.h"
#include <stdio.h>
#include <process.h>
int g_ThreadCount;
int g_AllocSize;


univ_dev::LockFreeQueue<int> queue;

DWORD begin;


unsigned __stdcall Thread(void* param)
{
	int ret;
	int cycle = 0;
	int* outArr = new int[g_AllocSize];
	while (true)
	{
		//for (int i = 0; i < 10; i++)
		//	SwitchToThread();
		for (int i = 0; i < g_AllocSize; i++)
		{
			ret = 0;
			if (!queue.dequeue(ret))continue;/* CRASH();*/
			if (ret != 0xffff5555) CRASH();
			outArr[i] = ret + 1;
		}
		//for (int i = 0; i < 10; i++)
		//	SwitchToThread();

		for (int i = 0; i < g_AllocSize; i++)
		{
			if (outArr[i] != 0xffff5555 + 1) CRASH();
			queue.enqueue(outArr[i] - 1);
		}
		//printf("Cycle over : %d\n",++cycle);
	}
}

int main()
{
	printf("thread count : ");
	scanf_s("%d", &g_ThreadCount);	
	printf("alloc size : ");
	scanf_s("%d", &g_AllocSize);

	for (int i = 0; i < g_AllocSize * g_ThreadCount; i++)
	{
		queue.enqueue(0xffff5555);
	}
	
	for (int i = 0; i < g_ThreadCount; i++)
	{
		_beginthreadex(nullptr, 0, Thread, nullptr, 0, nullptr);
	}

	while (true)
	{
		printf("excute time : %u\nthread count : %d\nalloc size %d\nqueue size : %d\nmemory pool size : %d\n memory pool capaicity : %d\nTPS : %lld\n",begin, g_ThreadCount, g_AllocSize, queue.size(), queue.pool_size(), queue.pool_capacity(), queue.GetCountAndSetCountZero());
		//printf("excute time : %u\nthread count : %d\nalloc size %d\nqueue size : %d\n",begin, g_ThreadCount, g_AllocSize, queue.size());
		Sleep(1000);
		begin++;
	}
}