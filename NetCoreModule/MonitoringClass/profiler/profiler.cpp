#include "profiler.h"
#include <stdio.h>
#include <cstring>
#include <ctime>

namespace univ_dev
{
	THREAD_SAMPLE samples[THREAD_SIZE];

	DWORD gtls_ProfilingIdx;
	DWORD g_CurrentSampleIdx;
	void InitProfile()
	{
		gtls_ProfilingIdx = TlsAlloc();
		g_CurrentSampleIdx = 0;

		ZeroMemory(samples, sizeof(THREAD_SAMPLE)*THREAD_SIZE);

	}
	void BeginProfiling(const char* name)
	{
		DWORD currentThreadIdx = (DWORD)TlsGetValue(gtls_ProfilingIdx);
		if (currentThreadIdx == 0)
		{
			//처음일때
			currentThreadIdx = InterlockedIncrement(&g_CurrentSampleIdx);
			samples[currentThreadIdx].threadID = GetCurrentThreadId();
			if (!TlsSetValue(gtls_ProfilingIdx, (LPVOID)currentThreadIdx))
			{
				int* ptr = nullptr;
				*ptr = 100;
			}
		}
		for (size_t i = 0; i < SAMPLE_SIZE; i++)
		{
			//찾았을때 있을경우
			if (GetCurrentThreadId() == samples[currentThreadIdx].threadID && samples[currentThreadIdx].profileSample[i].flag && strcmp(samples[currentThreadIdx].profileSample[i].profileName, name) == 0)
			{
				//if (samples[currentThreadIdx].profileSample[i].flag)
				//{
				//	if (strcmp(samples[currentThreadIdx].profileSample[i].profileName, name) == 0)
				//	{
						if (samples[currentThreadIdx].profileSample[i].startTime.QuadPart != 0)
						{
							int* ptr = nullptr;
							*ptr = 100;
						}
				//	}
				//}

				QueryPerformanceCounter(&samples[currentThreadIdx].profileSample[i].startTime);
				break;
			}
			//여기부터는 없을경우
			else if (!samples[currentThreadIdx].profileSample[i].flag)
			{
				samples[currentThreadIdx].profileSample[i].flag = true;
				strcpy_s(samples[currentThreadIdx].profileSample[i].profileName, name);
				samples[currentThreadIdx].profileSample[i].min[0] = MAXLONGLONG;
				samples[currentThreadIdx].profileSample[i].min[1] = MAXLONGLONG;
				samples[currentThreadIdx].profileSample[i].max[0] = MINLONGLONG;
				samples[currentThreadIdx].profileSample[i].max[1] = MINLONGLONG;
				samples[currentThreadIdx].profileSample[i].callCounts = 0;
				samples[currentThreadIdx].profileSample[i].totalTime = 0;
				QueryPerformanceCounter(&samples[currentThreadIdx].profileSample[i].startTime);
				break;
			}
		}
	}

	void EndProfiling(const char* name)
	{
		DWORD currentThreadIdx = (DWORD)TlsGetValue(gtls_ProfilingIdx);
		if (currentThreadIdx == 0)
		{
			int* ptr = nullptr;
			*ptr = 100;
			currentThreadIdx = InterlockedIncrement(&g_CurrentSampleIdx);
			samples[currentThreadIdx].threadID = GetCurrentThreadId();
			if (!TlsSetValue(gtls_ProfilingIdx, (LPVOID)currentThreadIdx))
			{
				int* ptr = nullptr;
				*ptr = 100;
			}
		}

		for (size_t i = 0; i < SAMPLE_SIZE; i++)
		{
			//찾았을때 있을경우
			if (samples[currentThreadIdx].threadID == GetCurrentThreadId() && 
				samples[currentThreadIdx].profileSample[i].flag &&
				strcmp(samples[currentThreadIdx].profileSample[i].profileName, name) == 0)
			{
				LARGE_INTEGER endTime;
				QueryPerformanceCounter(&endTime);
				__int64 tempTime = endTime.QuadPart - samples[currentThreadIdx].profileSample[i].startTime.QuadPart;
				if (samples[currentThreadIdx].profileSample[i].min[0] > tempTime)
				{
					samples[currentThreadIdx].profileSample[i].min[1] = samples[currentThreadIdx].profileSample[i].min[0];
					samples[currentThreadIdx].profileSample[i].min[0] = tempTime;
				}
				else if (samples[currentThreadIdx].profileSample[i].min[1] > tempTime)
				{
					samples[currentThreadIdx].profileSample[i].min[1] = tempTime;
				}
				if (samples[currentThreadIdx].profileSample[i].max[0] < tempTime)
				{
					samples[currentThreadIdx].profileSample[i].max[1] = samples[currentThreadIdx].profileSample[i].max[0];
					samples[currentThreadIdx].profileSample[i].max[0] = tempTime;
				}
				else if (samples[currentThreadIdx].profileSample[i].max[1] < tempTime)
				{
					samples[currentThreadIdx].profileSample[i].max[1] = tempTime;
				}
				samples[currentThreadIdx].profileSample[i].totalTime += tempTime;
				samples[currentThreadIdx].profileSample[i].startTime.QuadPart = 0;

				samples[currentThreadIdx].profileSample[i].callCounts++;
				break;
			}
		}
	}

	void ResetProfiling()
	{
		for (size_t i = 0; i < THREAD_SIZE; i++)
		{
			for (size_t j = 0; j < SAMPLE_SIZE; j++)
			{
				samples[i].profileSample[j].callCounts = 0;
				samples[i].profileSample[j].flag = false;
				samples[i].profileSample[j].max[0] = MINLONGLONG;
				samples[i].profileSample[j].max[1] = MINLONGLONG;
				samples[i].profileSample[j].min[0] = MAXLONGLONG;
				samples[i].profileSample[j].min[1] = MAXLONGLONG;
				memset(samples[i].profileSample[j].profileName, 0, sizeof(samples[i].profileSample[j].profileName));
				samples[i].profileSample[j].startTime.QuadPart = 0;
				samples[i].profileSample[j].totalTime = 0;
			}
		}
	}

	void SaveProfiling()
	{
		FILE* file = nullptr;
		time_t timer = time(nullptr);
		tm t;
		localtime_s(&t, &timer);
		char fileName[100]{ 0 };

		char tempBuffer[20]{ 0 };

		strcat_s(fileName, "ServerProfile\\");
		_itoa_s(t.tm_year + 1900, tempBuffer, 10);
		strcat_s(fileName, tempBuffer);
		strcat_s(fileName, "_");
		_itoa_s(t.tm_mon + 1, tempBuffer, 10);
		strcat_s(fileName, tempBuffer);
		strcat_s(fileName, "_");
		_itoa_s(t.tm_mday, tempBuffer, 10);
		strcat_s(fileName, tempBuffer);
		strcat_s(fileName, "_");
		_itoa_s(t.tm_hour + 1, tempBuffer, 10);
		strcat_s(fileName, tempBuffer);
		strcat_s(fileName, "_");
		_itoa_s(t.tm_min, tempBuffer, 10);
		strcat_s(fileName, tempBuffer);
		strcat_s(fileName, "_");
		_itoa_s(t.tm_sec, tempBuffer, 10);
		strcat_s(fileName, tempBuffer);
		strcat_s(fileName, "Profile.csv");
		printf(fileName);
		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);

		while (file == nullptr)
			fopen_s(&file, fileName, "wb");

		fprintf(file, "name,threadID,total,average,min[0],min[1],max[0],max[1],callCountExceptMinMax,단위 sec\n");
		for (size_t i = 1; i < THREAD_SIZE; i++)
		{
			for (size_t j = 0; j < SAMPLE_SIZE; j++)
			{
				if (samples[i].threadID == 0) continue;
				if (samples[i].profileSample[j].flag)
				{
					if (samples[i].profileSample[j].callCounts > 4)
					{
						double totalTime = ((double)((double)(samples[i].profileSample[j].totalTime) -
							((double)(samples[i].profileSample[j].min[0]) + 
								(double)(samples[i].profileSample[j].min[1]) + 
								(double)(samples[i].profileSample[j].max[0]) + 
								(double)(samples[i].profileSample[j].max[1]))) /
							freq.QuadPart);
						__int64 callCountsExceptMinMax = samples[i].profileSample[j].callCounts - 4;

						double min0 = (double)((double)samples[i].profileSample[j].min[0] / freq.QuadPart);
						double min1 = (double)((double)samples[i].profileSample[j].min[1] / freq.QuadPart);
						double max0 = (double)((double)samples[i].profileSample[j].max[0] / freq.QuadPart);
						double max1 = (double)((double)samples[i].profileSample[j].max[1] / freq.QuadPart);

						printf("---------------------------------------------------\n");
						printf("totalTime : %llf\n", totalTime);
						printf("totalCallCounts : %lld\n", callCountsExceptMinMax);
						printf("min1 : %llf\n", min0);
						printf("min2 : %llf\n", min1);
						printf("max1 : %llf\n", max0);
						printf("max2 : %llf\n", max1);
						printf("---------------------------------------------------\n");

						fprintf(file, "%s,%u,%.10llf,%.10llf,%.10llf,%.10llf,%.10llf,%.10llf,%.10lld\n",
							samples[i].profileSample[j].profileName,
							samples[i].threadID, totalTime, (double)(totalTime / (double)callCountsExceptMinMax),
							min0, min1 / freq.QuadPart, max0 / freq.QuadPart, max1 / freq.QuadPart,
							callCountsExceptMinMax);
						continue;
					}
					fprintf(file, "%s,%u,Not enough samples counts for profiling\n",
						samples[i].profileSample[j].profileName,
						samples[i].threadID);

				}
			}

		}
		fclose(file);
	}
}
