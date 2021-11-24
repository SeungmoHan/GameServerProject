#pragma once
#ifndef __PROFILE_INTERFACE_HEADER__
#define __PROFILE_INTERFACE_HEADER__
#define PROFILE
#include "profiler.h"


class Profiler
{
public:
	Profiler(const char* args) : name(args)
	{
		BEGIN_PROFILE(args);
	}
	void ResetProfile()
	{
		ResetProfile();
	}
	~Profiler()
	{
		END_PROFILE(name);
	}
private:
	const char* name;
};


#endif // !__PROFILE_INTERFACE_HEADER__
