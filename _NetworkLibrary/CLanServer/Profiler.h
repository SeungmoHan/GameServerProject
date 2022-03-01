#pragma once
#ifndef __PROFILER_HEADER__
#define __PROFILER_HEADER__
#define __UNIV_DEVELOPER_
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

	void BeginProfiling(const char* name, DWORD threadID);
	void EndProfiling(const char* name, DWORD threadID);
	void SaveProfiling();
	void ResetProfiling();
	void InitializeProfilerAndSamples();
	constexpr int SAMPLE_SIZE = 100;
	extern SRWLOCK g_ProfileLock;

	class Profiler
	{
	public:
		Profiler(const char* name, DWORD threadID) : _Name(name), _ThreadID(threadID)
		{
			BeginProfiling(name, threadID);
		}
		~Profiler()
		{
			EndProfiling(_Name, _ThreadID);
		}
	private:
		const char* _Name;
		DWORD _ThreadID;
	};

	struct PROFILE_SAMPLE
	{
		bool _Flag;
		DWORD _ThreadID;
		char _ProfileName[64];
		LARGE_INTEGER _StartTime;
		__int64 _TotalTime;
		__int64 _Min[2];
		__int64 _Max[2];
		__int64 _CallCounts;
	};

	extern PROFILE_SAMPLE samples[SAMPLE_SIZE];
#endif // !__PROFILER_HEADER__
}

