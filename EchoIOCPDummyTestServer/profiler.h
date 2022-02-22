#pragma once
#ifndef __PROFILER_HEADER__
#define __PROFILER_HEADER__
#include <Windows.h>

#ifdef PROFILE

#define BEGIN_PROFILE(arg) BeginProfiling(arg)
#define END_PROFILE(arg) EndProfiling(arg)

#else
#define BEGIN_PROFILE(arg)
#define END_PROFILE(arg)

#endif 
namespace univ_dev
{

	void BeginProfiling(const char* name,DWORD threadID);
	void EndProfiling(const char* name,DWORD threadID);
	void SaveProfiling();
	void ResetProfiling();
	void InitializeProfilerAndSamples();
	constexpr int SAMPLE_SIZE = 100;
	extern SRWLOCK profileLock;

	class Profiler
	{
	public:
		Profiler(const char* name,DWORD threadID) : name(name), threadID(threadID)
		{
			BeginProfiling(name,threadID);
		}
		~Profiler()
		{
			EndProfiling(name, threadID);
		}
	private:
		const char* name;
		DWORD threadID;
	};

	struct PROFILE_SAMPLE
	{
		bool flag;
		DWORD threadID;
		char profileName[64];
		LARGE_INTEGER startTime;
		__int64 totalTime;
		__int64 min[2];
		__int64 max[2];
		__int64 callCounts;
	};

	extern PROFILE_SAMPLE samples[SAMPLE_SIZE];
#endif // !__PROFILER_HEADER__
}

