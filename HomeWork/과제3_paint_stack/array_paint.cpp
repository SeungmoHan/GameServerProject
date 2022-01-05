#include <iostream>
#include <Windows.h>
#include "stack.hpp"
#include <vector>
#include <algorithm>

#define WIDTH_LENGTH 20
#define HEIGHT_LENGTH 10
using std::cout; using std::endl;
inline void Print(int x, int y)
{
	cout << "pos.x : " << x << "\t" << "pos.y" << y << endl;
}

struct Pos
{
	int x;
	int y;
};
void Render(char buffer[][WIDTH_LENGTH]);
std::vector<Pos> PaintToArray(char buffer[][WIDTH_LENGTH], Pos pos, int to);

int main()
{
	char original_paint[HEIGHT_LENGTH][WIDTH_LENGTH]
	{
		 " 00000/000/000/000 "
		," 000 000 000 000 0 "
		," 0               0 "
		," 0/00   00000  000 "
		," 000 0  00000  0/0 "
		," 0/  0 0000000 000 " 
		," 0/ 00 ///////   0 "
		,"  0    00000000000 "
		,"  00000000000   00 "
	};
	char buffer[HEIGHT_LENGTH][WIDTH_LENGTH];
	while (true)
	{
		for (size_t i = 0; i < HEIGHT_LENGTH; i++)
		{
			for (size_t j = 0; j < WIDTH_LENGTH; j++)
			{
				buffer[i][j] = original_paint[i][j];
			}
		}
		//logic
		std::vector<Pos> invalidPosFinder = PaintToArray(buffer, Pos{ 1,0}, '*');

		for (auto iter = invalidPosFinder.begin(); iter != invalidPosFinder.end(); iter++)
		{
			Print(iter->x, iter->y);
		}
		system("pause");
		Sleep(2000);
	}
}

void Render(char buffer[][WIDTH_LENGTH])
{
	system("cls");
	for (size_t i = 0; i < HEIGHT_LENGTH; i++)
	{
		printf(" %s \n", buffer[i]);
	}
}

std::vector<Pos> PaintToArray(char buffer[][WIDTH_LENGTH], Pos pos, int to)
{
	Stack<Pos> positions;
	std::vector<Pos> errorFinder;
	positions.Push(pos);
	while (positions.Pop(pos))
	{
		if (pos.x < 0 || pos.x >= WIDTH_LENGTH) continue;

		if (pos.y < 0 || pos.y >= HEIGHT_LENGTH) continue;
		if (buffer[pos.y][pos.x] == '0')
			buffer[pos.y][pos.x] = to;
		Render(buffer);
		errorFinder.push_back(pos);

		if (pos.y - 1 >= 0 && buffer[pos.y - 1][pos.x] == '0')
		{
			positions.Push(Pos{ pos.x,pos.y - 1 });
		}
		if (pos.x - 1 >= 0 && buffer[pos.y][pos.x - 1] == '0')
		{
			positions.Push(Pos{ pos.x - 1,pos.y });
		}
		if (pos.x + 1 < WIDTH_LENGTH && buffer[pos.y][pos.x + 1] == '0')
		{
			positions.Push(Pos{ pos.x + 1,pos.y });
		}
		if (pos.y + 1 < HEIGHT_LENGTH && buffer[pos.y + 1][pos.x] == '0')
		{
			positions.Push(Pos{ pos.x,pos.y + 1 });
		}
	}
	return errorFinder;
}