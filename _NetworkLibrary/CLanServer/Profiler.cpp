#include "profiler.h"
#include <stdio.h>
#include <cstring>
#include <ctime>

namespace univ_dev
{
	PROFILE_SAMPLE samples[SAMPLE_SIZE]{ 0 };
	SRWLOCK g_ProfileLock;

	void InitializeProfilerAndSamples()
	{
		InitializeSRWLock(&g_ProfileLock);
		memset(samples, 0, sizeof(samples));
	}

	void BeginProfiling(const char* name, DWORD threadID)
	{
		AcquireSRWLockExclusive(&g_ProfileLock);
		for (size_t i = 0; i < SAMPLE_SIZE; i++)
		{
			//찾았을때 있을경우
			if (samples[i]._Flag && strcmp(samples[i]._ProfileName, name) == 0 && threadID == samples[i]._ThreadID)
			{
				if (samples[i]._StartTime.QuadPart != 0)
				{
					int* ptr = nullptr;
					*ptr = 100;
				}
				QueryPerformanceCounter(&samples[i]._StartTime);
				break;
			}
			//여기부터는 없을경우
			else if (!samples[i]._Flag)
			{
				samples[i]._Flag = true;
				samples[i]._ThreadID = threadID;
				strcpy_s(samples[i]._ProfileName, name);
				samples[i]._Min[0] = MAXLONGLONG;
				samples[i]._Min[1] = MAXLONGLONG;
				samples[i]._Max[0] = MINLONGLONG;
				samples[i]._Max[1] = MINLONGLONG;
				samples[i]._CallCounts = 0;
				samples[i]._TotalTime = 0;
				QueryPerformanceCounter(&samples[i]._StartTime);
				break;
			}
		}
		ReleaseSRWLockExclusive(&g_ProfileLock);
	}

	void EndProfiling(const char* name, DWORD threadID)
	{
		AcquireSRWLockExclusive(&g_ProfileLock);
		for (size_t i = 0; i < SAMPLE_SIZE; i++)
		{
			//찾았을때 있을경우
			if (samples[i]._Flag && strcmp(samples[i]._ProfileName, name) == 0 && threadID == samples[i]._ThreadID)
			{
				LARGE_INTEGER endTime;
				QueryPerformanceCounter(&endTime);
				__int64 tempTime = endTime.QuadPart - samples[i]._StartTime.QuadPart;
				if (samples[i]._Min[0] > tempTime)
				{
					samples[i]._Min[0] = tempTime;
				}
				else if (samples[i]._Min[1] > tempTime)
				{
					samples[i]._Min[1] = tempTime;
				}
				if (samples[i]._Max[0] < tempTime)
				{
					samples[i]._Max[0] = tempTime;
				}
				else if (samples[i]._Max[1] < tempTime)
				{
					samples[i]._Max[1] = tempTime;
				}
				samples[i]._TotalTime += tempTime;
				samples[i]._StartTime.QuadPart = 0;

				samples[i]._CallCounts++;
				break;
			}
		}
		ReleaseSRWLockExclusive(&g_ProfileLock);
	}

	void ResetProfiling()
	{
		AcquireSRWLockExclusive(&g_ProfileLock);
		for (size_t i = 0; i < SAMPLE_SIZE; i++)
		{
			samples[i]._CallCounts = 0;
			samples[i]._Flag = false;
			samples[i]._ThreadID = 0;
			samples[i]._Max[0] = MINLONGLONG;
			samples[i]._Max[1] = MINLONGLONG;
			samples[i]._Min[0] = MAXLONGLONG;
			samples[i]._Min[1] = MAXLONGLONG;
			memset(samples[i]._ProfileName, 0, sizeof(samples[i]._ProfileName));
			samples[i]._StartTime.QuadPart = 0;
			samples[i]._TotalTime = 0;
		}
		ReleaseSRWLockExclusive(&g_ProfileLock);
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
		_itoa_s(t.tm_hour, tempBuffer, 10);
		strcat_s(fileName, tempBuffer);
		strcat_s(fileName, "_");
		//_itoa_s(t.tm_min, tempBuffer, 10);
		//strcat_s(fileName, tempBuffer);
		//strcat_s(fileName, "_");
		//_itoa_s(t.tm_sec, tempBuffer, 10);
		//strcat_s(fileName, tempBuffer);
		strcat_s(fileName, "Profile.csv");
		printf("%s\n", fileName);
		AcquireSRWLockExclusive(&g_ProfileLock);
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
			if (samples[i]._Flag)
			{
				fprintf(file, "%s	,%u	,\'%.9f\',\'%.9f\',\'%.9f\',\'%.9f\',\'%.9f\',\'%.9f\',%lld\n",
					samples[i]._ProfileName,
					samples[i]._ThreadID,
					(double)samples[i]._TotalTime / freq.QuadPart,
					(double)samples[i]._TotalTime / freq.QuadPart / samples[i]._CallCounts,
					(double)samples[i]._Min[0] / freq.QuadPart,
					(double)samples[i]._Min[1] / freq.QuadPart,
					(double)samples[i]._Max[0] / freq.QuadPart,
					(double)samples[i]._Max[1] / freq.QuadPart,
					samples[i]._CallCounts);
			}
		}
		fclose(file);
		ReleaseSRWLockExclusive(&g_ProfileLock);
	}
}
