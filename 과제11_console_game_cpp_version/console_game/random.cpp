#include <time.h>
#include <algorithm>
#include "random"

Random::Random(int numOfCases)
{
	cases.push_back(true);
	for (size_t i = 0; i < numOfCases -1; i++)
	{
		cases.push_back(false);
	}
	std::random_shuffle(cases.begin(), cases.end());
};

bool Random::GetNextRandomCase()
{
	int ret = cases[currentIdx++];
	if (currentIdx >= cases.size())
	{
		std::random_shuffle(cases.begin(), cases.end());
		currentIdx = 0;
	}
	return ret;
}
void Random::SetCases(int numOfCases)
{
	cases.clear();
	cases.push_back(true);
	for (size_t i = 0; i < numOfCases -1; i++)
	{
		cases.push_back(false);
	}
	std::random_shuffle(cases.begin(), cases.end());
}