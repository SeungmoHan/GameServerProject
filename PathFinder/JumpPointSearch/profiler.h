#pragma once
#ifndef __PROFILER_HEADER__
#define __PROFILER_HEADER__
#include <Windows.h>

#define __UNIV_DEV_PROFILE
#ifdef __UNIV_DEV_PROFILE

#define BEGIN_PROFILE(arg) univ_dev::BeginProfiling(arg)
#define END_PROFILE(arg) univ_dev::EndProfiling(arg)

#else
#define BEGIN_PROFILE(arg)
#define END_PROFILE(arg)

#endif 

namespace univ_dev
{
	constexpr int SAMPLE_SIZE = 20;
	
	struct PROFILE_SAMPLE
	{
		bool flag;
		char profileName[64];
		LARGE_INTEGER startTime;
		__int64 totalTime;
		__int64 min[2];
		__int64 max[2];
		__int64 callCounts;
	};


	void BeginProfiling(const char* name);
	void EndProfiling(const char* name);
	void SaveProfiling();
	void ResetProfiling();

	extern PROFILE_SAMPLE samples[SAMPLE_SIZE];
	class ProFiler
	{
	public:
		ProFiler(const char* name) :name(name)
		{
			univ_dev::BeginProfiling(name);
		}
		~ProFiler()
		{
			univ_dev::EndProfiling(name);
		}
	private:
		const char* name;
	};
}
#endif // !__PROFILER_HEADER__
