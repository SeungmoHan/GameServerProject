#include "ScreenBuffer"
#include <iostream>
#include <Windows.h>
ScreenBuffer::ScreenBuffer()
{
	memset(screenBuffer, 0, bufferHeight * bufferWidth);
	CONSOLE_CURSOR_INFO stConsoleCursor;
	stConsoleCursor.bVisible = false;
	stConsoleCursor.dwSize = 1;
	SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &stConsoleCursor);
}
void ScreenBuffer::DrawSprite(int x, int y, char c)
{
	if ((y >= bufferHeight - 1 || y < 0) || (x >= bufferWidth - 1 || x < 0)) return;
	this->screenBuffer[y][x] = c;
}


void ScreenBuffer::DrawSprite(char buffer[bufferHeight][bufferWidth])
{
	memcpy_s(screenBuffer, bufferHeight * bufferWidth, buffer, bufferHeight * bufferWidth);
}

void ScreenBuffer::Flip()
{
	for (size_t i = 0; i < bufferHeight; i++)
	{
		if (i == 0 || i == bufferHeight - 1)
			memcpy_s(screenBuffer[i], bufferWidth, "################################################################################", bufferWidth);
		screenBuffer[i][0] = '#';
		screenBuffer[i][79] = '#';
	}
	for (size_t i = 0; i < bufferHeight; i++)
	{
		screenBuffer[i][80] = '\0';
		MoveCursor(0, i);
		printf("%s", screenBuffer[i]);
	}
	MoveCursor(0, 0);
	memset(screenBuffer, ' ', bufferHeight * bufferWidth);
}

ScreenBuffer* ScreenBuffer::GetInstance()
{
	return &buffer;
}

void ScreenBuffer::ClearScreen()
{
	DWORD dw;
	FillConsoleOutputCharacter(GetStdHandle(STD_OUTPUT_HANDLE), ' ', 100 * 100, { 0, 0 }, &dw);
}

void ScreenBuffer::MoveCursor(int x, int y)
{
	COORD stCoord;
	stCoord.X = x;
	stCoord.Y = y;
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), stCoord);
}

ScreenBuffer ScreenBuffer::buffer;