#include "ObjectStar.h"
#include <Windows.h>
#include <conio.h>
#include <iostream>



int main()
{
	BaseObject* objArray[30]{ nullptr };
	while (true)
	{
		
		if (_kbhit())
		{
			int type = _getch();
			int idx = 0;
			while (objArray[idx] != nullptr) idx++;
			switch (type)
			{
			case '1':
				objArray[idx] = new OneStar();
				break;
			case '2':
				objArray[idx] = new TwoStar();
				break;
			case '3':
				objArray[idx] = new ThreeStar();
				break;
			}
		}
		for (size_t i = 0; i < 30; i++)
		{
			if (objArray[i] != nullptr)
				objArray[i]->Action();
		}

		for (size_t i = 0; i < 30; i++)
		{
			if (objArray[i] != nullptr)
				objArray[i]->Draw();
			printf("\n");
		}

		for (size_t i = 0; i < 30; i++)
		{
			if (objArray[i] != nullptr && objArray[i]->RemoveFlag())
			{
				delete objArray[i];
				objArray[i] = nullptr;
			}
		}
		Sleep(200);
		system("cls");
	}

	return 0;
}