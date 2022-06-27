#pragma once
#ifndef __PROFILER_HEADER__
#define __PROFILER_HEADER__
#include <Windows.h>

#define PRO_BEGIN(arg) BeginProfiling(arg)
#define PRO_END(arg) EndProfiling(arg)

namespace univ_dev
{

	void InitProfile();
	void BeginProfiling(const char* name);
	void EndProfiling(const char* name);
	void SaveProfiling();
	void ResetProfiling();

	class Profiler
	{
	public:
		Profiler(const char* name) : name(name)
		{
			BeginProfiling(name);
		}
		~Profiler()
		{
			EndProfiling(name);
		}
	private:
		const char* name;
	};


	constexpr int SAMPLE_SIZE = 100;
	constexpr int THREAD_SIZE = 30;


	struct PROFILE_SAMPLE
	{
		DWORD _Flag;
		char _ProfileName[64];
		LARGE_INTEGER _StartTime;
		__int64 _TotalTime;
		__int64 _Min[2];
		__int64 _Max[2];
		__int64 _CallCounts;
	};

	struct THREAD_SAMPLE
	{
		DWORD threadID;
		PROFILE_SAMPLE profileSample[SAMPLE_SIZE];
	};


#endif // !__PROFILER_HEADER__
}

