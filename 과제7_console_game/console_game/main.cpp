#include "Console.h"
#include "console_game.h"

int main()
{
	if (!GameInit()) return -1;

	while (true)
	{
		ClearBuffer();
		switch (GetCurrentState())
		{
		case State::START:
			Start();
			break;
		case State::PLAYING:
			Play();
			break;
		case State::END:
			End();
			break;
		case State::LOAD:
			Load();
			break;
		}
		Render();
		AdjustFrame();
	}
}