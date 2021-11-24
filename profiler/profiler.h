#pragma once
#ifndef __PROFILER_HEADER__
#define __PROFILER_HEADER__
#include <Windows.h>
constexpr int SAMPLE_SIZE = 10;

#ifdef PROFILE

#define BEGIN_PROFILE(arg) BeginProfiling(arg)
#define END_PROFILE(arg) EndProfiling(arg)

#else
#define BEGIN_PROFILE(arg)
#define END_PROFILE(arg)

#endif 

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
extern PROFILE_SAMPLE samples[10];
#endif // !__PROFILER_HEADER__
