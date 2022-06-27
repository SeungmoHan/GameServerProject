#include <stdio.h>
#include <Windows.h>
#include <conio.h>
#include <time.h>



constexpr int timerSize = 9;
constexpr int timings[timerSize]{ 5,10,14,17,20,25,29,31,33 };
const char* hitString[5]{ "","Perfect","Good","NotGood","Bad"};
clock_t inputTiming[timerSize]{ 0 };
constexpr clock_t endTime = 35000;
int totalTime = 0;
int currentHitIdx = 0;

inline bool EndCondition()
{
	return totalTime < endTime&& currentHitIdx < 9;
}
inline bool KeyBoardHit();
void Render();

int main()
{

	while (totalTime < endTime && currentHitIdx < 9)
	{
		clock_t t = clock();
		totalTime = t;
		if (KeyBoardHit())
		{
			inputTiming[currentHitIdx++] = t;
		}
		Render();
	}
	for (size_t i = 0; i < timerSize; i++)
	{
		if (!inputTiming[i])
		{
			inputTiming[i] = 35000;
		}
	}
	Render();
}


inline bool KeyBoardHit()
{
	if (_kbhit())
	{
		_getch();
		return true;
	}
	return false;
}

void Render()
{
	system("cls");
	printf("%02d.%03d Sec \n", totalTime / 1000, totalTime % 1000);
	for (size_t i = 0; i < timerSize; i++)
	{
		bool hasHitTiming = true;
		const char* str = hitString[0];
		if (!inputTiming[i]) hasHitTiming = false;
		else if (abs(inputTiming[i] - (timings[i] * 1000)) <= 250) str = hitString[1];
		else if (abs(inputTiming[i] - (timings[i] * 1000)) <= 500) str = hitString[2];
		else if (abs(inputTiming[i] - (timings[i] * 1000)) <= 700) str = hitString[3];
		else str = hitString[4];
		if (hasHitTiming)
			printf("%02d Sec : %s <%02d.%03d>\n", timings[i], str, inputTiming[i] / 1000, inputTiming[i] % 1000);
		else
			printf("%02d Sec : %s\n", timings[i], str);
	}
}