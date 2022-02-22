#include "profiler.h"
#include <stdio.h>
#include <cstring>
#include <ctime>

namespace univ_dev
{
	PROFILE_SAMPLE samples[SAMPLE_SIZE]{ 0 };
	SRWLOCK profileLock;

	void InitializeProfilerAndSamples()
	{
		InitializeSRWLock(&profileLock);
		memset(samples, 0, sizeof(samples));
	}

	void BeginProfiling(const char* name,DWORD threadID)
	{
		AcquireSRWLockExclusive(&profileLock);
		for (size_t i = 0; i < SAMPLE_SIZE; i++)
		{
			//찾았을때 있을경우
			if (samples[i].flag && strcmp(samples[i].profileName, name) == 0 && threadID == samples[i].threadID)
			{
				if (samples[i].startTime.QuadPart != 0)
				{
					int* ptr = nullptr;
					*ptr = 100;
				}
				QueryPerformanceCounter(&samples[i].startTime);
				break;
			}
			//여기부터는 없을경우
			else if (!samples[i].flag)
			{
				samples[i].flag = true;
				samples[i].threadID = threadID;
				strcpy_s(samples[i].profileName, name);
				samples[i].min[0] = MAXLONGLONG;
				samples[i].min[1] = MAXLONGLONG;
				samples[i].max[0] = MINLONGLONG;
				samples[i].max[1] = MINLONGLONG;
				samples[i].callCounts = 0;
				samples[i].totalTime = 0;
				QueryPerformanceCounter(&samples[i].startTime);
				break;
			}
		}
		ReleaseSRWLockExclusive(&profileLock);
	}

	void EndProfiling(const char* name,DWORD threadID)
	{
		AcquireSRWLockExclusive(&profileLock);
		for (size_t i = 0; i < SAMPLE_SIZE; i++)
		{
			//찾았을때 있을경우
			if (samples[i].flag && strcmp(samples[i].profileName, name) == 0 && threadID == samples[i].threadID)
			{
				LARGE_INTEGER endTime;
				QueryPerformanceCounter(&endTime);
				__int64 tempTime = endTime.QuadPart - samples[i].startTime.QuadPart;
				if (samples[i].min[0] > tempTime)
				{
					samples[i].min[0] = tempTime;
				}
				else if (samples[i].min[1] > tempTime)
				{
					samples[i].min[1] = tempTime;
				}
				if (samples[i].max[0] < tempTime)
				{
					samples[i].max[0] = tempTime;
				}
				else if (samples[i].max[1] < tempTime)
				{
					samples[i].max[1] = tempTime;
				}
				samples[i].totalTime += tempTime;
				samples[i].startTime.QuadPart = 0;

				samples[i].callCounts++;
				break;
			}
		}
		ReleaseSRWLockExclusive(&profileLock);
	}

	void ResetProfiling()
	{
		AcquireSRWLockExclusive(&profileLock);
		for (size_t i = 0; i < SAMPLE_SIZE; i++)
		{
			samples[i].callCounts = 0;
			samples[i].flag = false;
			samples[i].threadID = 0;
			samples[i].max[0] = MINLONGLONG;
			samples[i].max[1] = MINLONGLONG;
			samples[i].min[0] = MAXLONGLONG;
			samples[i].min[1] = MAXLONGLONG;
			memset(samples[i].profileName, 0, sizeof(samples[i].profileName));
			samples[i].startTime.QuadPart = 0;
			samples[i].totalTime = 0;
		}
		ReleaseSRWLockExclusive(&profileLock);
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
		//_itoa_s(t.tm_min, tempBuffer, 10);
		//strcat_s(fileName, tempBuffer);
		//strcat_s(fileName, "_");
		//_itoa_s(t.tm_sec, tempBuffer, 10);
		//strcat_s(fileName, tempBuffer);
		strcat_s(fileName, "Profile.csv");
		printf("%s\n", fileName);
		AcquireSRWLockExclusive(&profileLock);
		while (file == nullptr)
		{
			fopen_s(&file, fileName, "ab");
			printf("File is nullptr Try again\n");
		}
		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);
		fprintf(file, "name,threadid,total,average,min[0],min[1],max[0],max[1],callCount,단위 sec\n");
		for (size_t i = 0; i < SAMPLE_SIZE; i++)
		{
			if (samples[i].flag)
			{
				fprintf(file, "%s,%u,%llf,%llf,%llf,%llf,%llf,%llf,%lld\n",
					samples[i].profileName,
					samples[i].threadID,
					(double)samples[i].totalTime / freq.QuadPart,
					(double)samples[i].totalTime / freq.QuadPart / samples[i].callCounts,
					(double)samples[i].min[0] / freq.QuadPart,
					(double)samples[i].min[1] / freq.QuadPart,
					(double)samples[i].max[0] / freq.QuadPart,
					(double)samples[i].max[1] / freq.QuadPart,
					samples[i].callCounts);
			}
		}
		fclose(file);
		ReleaseSRWLockExclusive(&profileLock);
	}
}
