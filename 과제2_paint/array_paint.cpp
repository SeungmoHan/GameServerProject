#include <iostream>
#include <Windows.h>


#define WIDTH_LENGTH 20
#define HEIGHT_LENGTH 9



void Render(char buffer[][WIDTH_LENGTH])
{
	system("cls");
	for (size_t i = 0; i < HEIGHT_LENGTH; i++)
	{
		printf(" %s \n", buffer[i]);
	}
}

void PaintToArray(char buffer[][WIDTH_LENGTH],int x, int y, int to)
{
	if (x >= WIDTH_LENGTH || x < 0) return;
	if (y >= HEIGHT_LENGTH || y < 0) return;
	do
	{

	} while (0);
	if (buffer[y][x] == '0')
		buffer[y][x] = to;
	else
		return;
	Render(buffer);
	PaintToArray(buffer, x - 1, y, to);
	PaintToArray(buffer, x + 1, y, to);
	PaintToArray(buffer, x, y - 1, to);
	PaintToArray(buffer, x, y + 1, to);
}



int main()
{
	
	char original_paint[HEIGHT_LENGTH][WIDTH_LENGTH]{
		 "00000/000/000/0000"
		,"000 000 000 000 00"
		,"0                0"
		,"0/00   00000  0000"
		,"000 0  00000  0//0"
		,"0/  0 0000000 0000" 
		,"0/ 00 ///////    0"
		," 0    000000000000"
		," 00000000000   000"
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
		PaintToArray(buffer, 0, 0, '*');
		Render(buffer);
		Sleep(2000);
	}
}