#include <stdio.h>
#include "packing.h"


int main()
{
	int packChoice;
	while (true)
	{
		printf("1 패킹\n2 언패킹\n");
		scanf_s("%d", &packChoice);
		if (packChoice == 1)
		{
			if (Packing()) printf("패킹 성공\n");
			else
			{
				printf("패킹 실패\n프로그램 종료\n");
				return 0;
			}
		}
		if (packChoice == 2)
		{
			if (UnPacking()) printf("언패킹 성공\n");
			else
			{
				printf("언패킹 실패\n프로그램 종료\n");
				return 0;
			}
		}
	}
}