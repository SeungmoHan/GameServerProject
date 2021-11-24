#include <stdio.h>
#include "profile_class.h"


int main()
{

	for (size_t i = 0; i < 10000; i++)
	{
		Profiler f("A");
		Sleep(0);
	}

	for (size_t i = 0; i < 10; i++)
	{
		Profiler f("B");
		Sleep(100);
	}

	SaveProfiling();

}