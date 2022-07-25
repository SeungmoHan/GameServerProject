#pragma comment(lib,"Winmm.lib")

#include "RingBuffer.h"
#include <iostream>
#include <string>
#include <list>
#include <Windows.h>
#include <thread>
#include <conio.h>
#include "SerializingBuffer.h"

#define dfTYPE_ADD_STR		0
#define dfTYPE_DEL_STR		1
#define dfTYPE_PRINT_LIST	2
#define dfTYPE_QUIT		3	

#define MAX_THREAD_COUNT 20
#define CUR_THREAD_COUNT 7
struct MSG_Header
{
	short type;
	short strlen;
};


int seed = 1;
std::list<std::wstring> g_StringList;
univ_dev::RingBuffer g_RingBuffer(50000);
std::wstring STR(L"PROCADEMY");

SRWLOCK g_Lock;
HANDLE hEvent[2];
HANDLE h_Threads[MAX_THREAD_COUNT];

unsigned int g_WorkerThreadTPS;

unsigned int g_AddProcess;
unsigned int g_DelProcess;
unsigned int g_PrintProcess;

unsigned int g_NumOfPosting;
unsigned int numOfRunningThread;

bool g_ShutDownFlag = false;

char fileNames[MAX_THREAD_COUNT][20]{ "Log1.txt","Log2.txt","Log3.txt","Log4.txt","Log5.txt" ,"Log6.txt" ,"Log7.txt" ,"Log8.txt" ,"Log9.txt" ,"Log10.txt"
,"Log11.txt" ,"Log12.txt" ,"Log13.txt" ,"Log14.txt" ,"Log15.txt" ,"Log16.txt" ,"Log17.txt" ,"Log18.txt" ,"Log19.txt" ,"Log20.txt" };


char str[]{ "zhelloworlddontdwellonthepastbelieveinyourselffollowyourheartseizethedayyouonlyliveoncepasyisjustpasthelloworldprocademy" };
univ_dev::RingBuffer buffer(129);

int idx;

std::wstring backupStr = L"";

unsigned __stdcall EnqueueThread(void* param)
{
	//Sleep(10000);
	srand(seed - 1);
	while (true)
	{
		//int randEnqueueSize = (rand() % 120) + 1;
		int randEnqueueSize = 120;

		int ret = buffer.Enqueue(str, randEnqueueSize);

		//int nowStep = idx + randEnqueueSize;
		//int overFlow = max(0, nowStep - 120);
		//nowStep %= 120;
		//if (overFlow == 0)
		//{
		//	//buffer.Lock();
		//	int ret = buffer.Enqueue(str + idx, randEnqueueSize);
		//	//buffer.Unlock();
		//	if (ret != 0)
		//	{
		//		idx += randEnqueueSize;
		//		idx %= 120;
		//		if (idx != nowStep)
		//		{
		//			printf("enqueue1 : step is diffrent, nowstep :%d \t idx :%d\n", nowStep, idx);
		//		}
		//	}
		//}
		//else
		//{
		//	//buffer.Lock();
		//	//int ret = buffer.Enqueue(str + idx, 120 - idx);
		//	//if (ret == 0)
		//	//{
		//	//	//buffer.Unlock();
		//	//	continue;
		//	//}
		//	//idx += ret;
		//	//idx %= 120;
		//	//ret = buffer.Enqueue(str + idx, overFlow);
		//	////buffer.Unlock();
		//	//if (ret != 0)
		//	//{
		//	//	idx += overFlow;
		//	//	idx %= 120;
		//	//	if (idx != nowStep)
		//	//	{
		//	//		printf("enqueue2 : step is diffrent, nowstep :%d \t idx :%d\n", nowStep, idx);
		//	//	}
		//	//}
		//}
	}
	return 0;
}
unsigned __stdcall DequeueThread(void* param)
{
	//Sleep(10000);
	srand(seed + 1);
	while (true)
	{
		char tempBuffer[200]{ 0 };
		char peekTempBuffer[200]{ 0 };
		//int randDequeueSize = (rand() % 100) + 1;
		//int randDequeueSize = 22;
		//int randDequeueSize = (rand() % 70) + 30;
		int randDequeueSize = (rand() % 120) + 1;
		//buffer.Lock();
		//int peekRet = buffer.Peek(peekTempBuffer, randDequeueSize);
		int dqRet = buffer.Dequeue(tempBuffer, randDequeueSize);
		//buffer.Unlock();
		//int cmpRet = memcmp(peekTempBuffer, tempBuffer, randDequeueSize);
		//if (cmpRet != 0 || peekRet != dqRet)
		//{
		//	FILE* file = nullptr;
		//	while (file == nullptr)
		//		fopen_s(&file, "MemcmpResult.txt", "ab");
		//	fprintf(file, "------------------------------\n");
		//	fprintf(file, "peekRet : %d\ndqRet : %d\n", peekRet, dqRet);
		//	fprintf(file, "peekTempBuffer : ");
		//	fprintf(file, peekTempBuffer);
		//	fprintf(file, "\ntempBuffer");
		//	fprintf(file, tempBuffer);
		//	fprintf(file, "\n------------------------------\n");
		//	fclose(file);
		//	int* ptr = nullptr;
		//	*ptr = 100;
		//}
		if (dqRet)
		{
			tempBuffer[0] = toupper(tempBuffer[0]);
			tempBuffer[dqRet - 1] = toupper(tempBuffer[dqRet - 1]);
			printf(tempBuffer);
		}
	}
	return 0;
}

#define _Test1
//#define _Test2

int lastFirstReadBytes[CUR_THREAD_COUNT]{ 65535,65535,65535,65535,65535,65535,65535 };
int lastSecondReadBytes[CUR_THREAD_COUNT]{ 65535,65535,65535,65535,65535,65535,65535 };

unsigned __stdcall WorkerThread(void* param)
{
	_InterlockedIncrement(&numOfRunningThread);
	int a;
	srand((int)&a);
	int idx = *(int*)param;
	printf("%d Start\n", GetCurrentThreadId());
	DWORD prev = timeGetTime();
	Sleep(500);
	while (true)
	{
		MSG_Header header{ ~0,~0 };
		std::wstring str;
		if (g_ShutDownFlag || g_RingBuffer.GetUseSize() >= sizeof(header)) SetEvent(hEvent[0]);
		WaitForSingleObject(hEvent[0], INFINITE);

		g_RingBuffer.Lock();
		int firstDQRet = 0, secondDQRet = 0;
		int writePointerPos1 = g_RingBuffer.GetWritePtrPosition();
		int readPointerPos1 = g_RingBuffer.GetReadPtrPosition();
		int bufferSize1 = g_RingBuffer.GetBufferSize();
		int freeSize1 = g_RingBuffer.GetFreeSize();
		int useSize1 = g_RingBuffer.GetUseSize();
		firstDQRet = g_RingBuffer.Peek((char*)&header, sizeof(header));
		if (firstDQRet != sizeof(header))
		{
			g_RingBuffer.Unlock();
			//SetEvent(hEvent[0]);
			continue;
		}
		g_RingBuffer.MoveReadPtr(firstDQRet);
		_InterlockedIncrement(&g_WorkerThreadTPS);

		WCHAR tempStr[20]{ 0 };
		if (header.type == dfTYPE_ADD_STR)
		{
			secondDQRet = g_RingBuffer.Peek((char*)tempStr, header.strlen * sizeof(WCHAR));
			if (secondDQRet == 0) 
			{
				int* ptr = nullptr;
				*ptr = 100;
			}
			g_RingBuffer.MoveReadPtr(secondDQRet);
			str = tempStr;
		}
		int writePointerPos2 = g_RingBuffer.GetWritePtrPosition();
		int readPointerPos2 = g_RingBuffer.GetReadPtrPosition();
		int bufferSize2 = g_RingBuffer.GetBufferSize();
		int freeSize2 = g_RingBuffer.GetFreeSize();
		int useSize2 = g_RingBuffer.GetUseSize();

		if (header.type == dfTYPE_ADD_STR)
			lastSecondReadBytes[idx] = secondDQRet;
		lastFirstReadBytes[idx] = firstDQRet;
		g_RingBuffer.Unlock();
		if (!g_ShutDownFlag && header.type == dfTYPE_QUIT)
		{
			printf("quit packet\n");
		}
		if ((unsigned long long)firstDQRet + (unsigned long long)secondDQRet != sizeof(header) + str.length() * sizeof(WCHAR))
		{
			writePointerPos1, readPointerPos1, bufferSize1, freeSize1, useSize1;
			writePointerPos2, readPointerPos2, bufferSize2, freeSize2, useSize2;
			printf("Error: firstDQRet + secondDQRet != sizeof header + str.length() * sizeof WCHAR\n");
			int* ptr = nullptr;
			*ptr = 100;
		}
		switch (header.type)
		{
		case dfTYPE_ADD_STR:
		{
			AcquireSRWLockExclusive(&g_Lock);
			g_AddProcess++;
			g_StringList.push_back(str);
			ReleaseSRWLockExclusive(&g_Lock);
			break;
		}
		case dfTYPE_DEL_STR:
		{
			if (g_StringList.empty()) continue;
			AcquireSRWLockExclusive(&g_Lock);
			g_DelProcess++;
			if (g_StringList.empty())
			{
				ReleaseSRWLockExclusive(&g_Lock);
				continue;
			}
			auto iter = g_StringList.begin();
			iter->clear();
			g_StringList.erase(g_StringList.begin());
			ReleaseSRWLockExclusive(&g_Lock);
			break;
		}
		case dfTYPE_PRINT_LIST:
		{
			if (g_StringList.empty()) continue;
			AcquireSRWLockExclusive(&g_Lock);
			g_PrintProcess++;
			int c = backupStr.capacity();
			int s = backupStr.size();
			int l = backupStr.length();
			for (auto iter = g_StringList.begin(); iter != g_StringList.end(); ++iter)
			{
				backupStr += L"[";
				backupStr += *iter;
				backupStr += L"]";
			}
			backupStr += L'\n';

			if (backupStr.length() >= 1000000)
			{
				FILE* file = nullptr;
				while (file == nullptr)
					fopen_s(&file, fileNames[0], "ab");
				fwrite(backupStr.c_str(), sizeof(char), backupStr.length(), file);
				backupStr = L"";
				fclose(file);
			}
			ReleaseSRWLockExclusive(&g_Lock);
			break;
		}
		case dfTYPE_QUIT:
		{
			int size = g_RingBuffer.GetUseSize();
			FILE* file = nullptr;
			while (file == nullptr)
				fopen_s(&file, "Log.txt", "ab");
			fprintf(file, "%d Quit\n", GetCurrentThreadId());
			printf("%d Quit\n", GetCurrentThreadId());
			fclose(file);
			_InterlockedDecrement(&numOfRunningThread);
			return 0;
		}
		}
	}
	return -1;
}

#ifdef _Test1

int main()
{
	timeBeginPeriod(1);
	HANDLE threads[2];
	threads[0] = (HANDLE)_beginthreadex(nullptr, 0, EnqueueThread, nullptr, 0, nullptr);
	threads[1] = (HANDLE)_beginthreadex(nullptr, 0, DequeueThread, nullptr, 0, nullptr);

	WaitForMultipleObjects(2, threads, true, INFINITE);
}
#endif

#ifdef _Test2

int main()
{
	timeBeginPeriod(1);
	//SRWLock 초기화
	InitializeSRWLock(&g_Lock);
	hEvent[0] = CreateEvent(nullptr, false, false, nullptr);
	//hEvent[1] = CreateEvent(nullptr, true, false, nullptr);
	if (hEvent[0] == nullptr) return -1;
	//if (hEvent[1] == nullptr) return -2;
	ResetEvent(hEvent[0]);
	//ResetEvent(hEvent[1]);
	//thread 생성
	int idx[MAX_THREAD_COUNT]{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19 };
	for (int i = 0; i < CUR_THREAD_COUNT; i++)
	{
		h_Threads[i] = (HANDLE)_beginthreadex(nullptr, 0, WorkerThread, &idx[i], 0, nullptr);
		if (h_Threads[i] == nullptr)
		{
			printf("Thread Create Failed :(\n");
			return -2;
		}
	}

	//thread pool 형성될때까지 Sleep
	DWORD prev = timeGetTime();
	Sleep(200);
	while (true)
	{
		DWORD cur = timeGetTime();

		//1초마다 출력
		if (cur - prev >= 1000)
		{
			prev = cur;
			int rTPS = _InterlockedExchange(&g_WorkerThreadTPS, (unsigned int)0);
			int rAddProcess = _InterlockedExchange(&g_AddProcess, 0);
			int rDelProcess = _InterlockedExchange(&g_DelProcess, 0);
			int rPrintProcess = _InterlockedExchange(&g_PrintProcess, 0);
			printf("----------------------------------------------------------------\n");
			printf("NumOfRunningWorkerThread : %d\n", numOfRunningThread);
			printf("WorkerThread TPS : %d\n", rTPS);
			printf("WorkerThread Add : %d\n", rAddProcess);
			printf("WorkerThread Del : %d\n", rDelProcess);
			printf("WorkerThread Print : %d\n", rPrintProcess);
			printf("MainThread TPS : %d\n", g_NumOfPosting);
			printf("g_RingBuffer::GetUseSize : %d\n", g_RingBuffer.GetUseSize());
			printf("g_RingBuffer::GetFreeSize : %d\n", g_RingBuffer.GetFreeSize());
			printf("g_RingBuffer::WritePointerPosition : %d\n", g_RingBuffer.GetWritePtrPosition());
			printf("g_RingBuffer::ReadPointerPosition : %d\n", g_RingBuffer.GetReadPtrPosition());
			printf("g_StringList.size : %llu\n", g_StringList.size());
			printf("----------------------------------------------------------------\n");
			g_NumOfPosting = 0;
		}

		MSG_Header header;
		if (g_RingBuffer.GetUseSize() >= sizeof(header)) SetEvent(hEvent[0]);
		if (g_ShutDownFlag)
		{
			if (/*!numOfRunningThread || */g_RingBuffer.GetUseSize() == 0) break;
			continue;
		}
		header.type = rand() % 3;
		header.strlen = 65535;
		std::wstring tempStr = L"";
		if (_kbhit())
		{
			int key = _getch();
			if (key == ' ')
			{
				printf("space key input\n");
				header.type = dfTYPE_QUIT;
				header.strlen = (1 << 10) + (1 << 2);
				//g_RingBuffer.Lock();
				for (int i = 0; i < CUR_THREAD_COUNT; i++)
				{
					g_NumOfPosting++;
					//g_RingBuffer.Lock();
					int ret = g_RingBuffer.Enqueue((char*)&header, sizeof(header));
					//g_RingBuffer.Unlock();
					if (ret == 0)
					{
						SetEvent(hEvent[0]);
						i--;
					}
					else printf("quit packet enqueued\n");
				}
				//g_RingBuffer.Unlock();
				g_ShutDownFlag = true;
			}
			continue;
		}
		
		if (header.type == dfTYPE_ADD_STR)
		{
			tempStr = STR.substr(0, (rand() % (STR.size() - 1)) + 1);
			header.strlen = tempStr.size();
			if (header.strlen == 0)
			{
				int* ptr = nullptr;
				*ptr = 100;
			}
		}
		if (g_RingBuffer.GetFreeSize() < tempStr.length() * sizeof(WCHAR) + sizeof(header))
		{
			SetEvent(hEvent[0]);
			continue;
		}
		g_NumOfPosting++;
		
		univ_dev::Packet packet;
		//g_RingBuffer.Lock();
		int firstEQRet = 0, secondEQRet = 0;
		//firstEQRet = g_RingBuffer.Enqueue((char*)&header, sizeof(header));
		packet << header.type << header.strlen;
		//if (firstEQRet == 0)
		//{
		//	int a = 10;
		//	a++;
		//}
		if (header.type == dfTYPE_ADD_STR)
		{
			//secondEQRet = g_RingBuffer.Enqueue((char*)tempStr.c_str(), tempStr.size() * sizeof(WCHAR));
			memcpy_s(packet.GetWritePtr(), tempStr.size() * sizeof(WCHAR), tempStr.c_str(), tempStr.size() * sizeof(WCHAR));
			packet.MoveWritePtr(tempStr.size() * sizeof(WCHAR));
		}
		//g_RingBuffer.Lock();
		firstEQRet = g_RingBuffer.Enqueue(packet.GetReadPtr(), packet.GetBufferSize());
		//g_RingBuffer.Unlock();
		if ((unsigned long long)firstEQRet /*+ (unsigned long long)secondEQRet*/ != sizeof(header) + tempStr.length() * sizeof(WCHAR))
		{
			FILE* file = nullptr;
			while (file == nullptr)
				fopen_s(&file, "Log.txt", "ab");
			fwprintf_s(file, L"EQRet : %d\n", firstEQRet + secondEQRet);
			fwprintf_s(file, L"sizeof(header) : %llu\n", sizeof(header));
			fwprintf_s(file, L"tempStr.length() * sizeof(char) : %lld\n", tempStr.length() * sizeof(WCHAR));
			fwprintf_s(file, L"Old RingBuffer BufferSize : %d\n", g_RingBuffer.GetBufferSize());
			fwprintf_s(file, L"Old RingBuffer UseSize : %d\n", g_RingBuffer.GetUseSize());
			fwprintf_s(file, L"Old RingBuffer FreeSize : %d\n", g_RingBuffer.GetFreeSize());
			fwprintf_s(file, L"RingBuffer BufferSize : %d\n", g_RingBuffer.GetBufferSize());
			fwprintf_s(file, L"RingBuffer UseSize : %d\n", g_RingBuffer.GetUseSize());
			fwprintf_s(file, L"RingBuffer FreeSize : %d\n", g_RingBuffer.GetFreeSize());
			fclose(file);
			int* ptr = nullptr;
			*ptr = 100;
		}
		//for (int i = 0; i < 100; i++)
		//	SwitchToThread();
	}
	WaitForMultipleObjects(CUR_THREAD_COUNT, h_Threads, true, INFINITE);
	
	for (int i = 0; i < CUR_THREAD_COUNT; i++)
	{
		DWORD exitCode;
		GetExitCodeThread(h_Threads[i], &exitCode);
		if (exitCode != 0)
			printf("ExitCode is not 0 thread : %u", (DWORD)h_Threads[i]);
	}
	printf("\n\nEnd Of MainThread\n");
	printf("RingBuffer GetBufferSize : %d\n", g_RingBuffer.GetBufferSize());
	printf("RingBuffer GetUseSize : %d\n", g_RingBuffer.GetUseSize());
	printf("RingBuffer GetFreeSize : %d\n", g_RingBuffer.GetFreeSize());
	printf("g_StringList.size : %llu\n", g_StringList.size());

	return 0;
}

#endif