#include <stdio.h>
#include "profile_class.h"


int main()
{

	for (size_t i = 0; i < 10000; i++)
	{
		univ_dev::ProFiler f("A");
		Sleep(0);
	}
	for (size_t i = 0; i < 10; i++)
	{
		univ_dev::ProFiler f("B");
		Sleep(100);
	}
	univ_dev::SaveProfiling();
}