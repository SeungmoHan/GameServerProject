#include <stdio.h>
#include "packing.h"


int main()
{
	int packChoice;
	while (true)
	{
		printf("1 ��ŷ\n2 ����ŷ\n");
		scanf_s("%d", &packChoice);
		if (packChoice == 1)
		{
			if (Packing()) printf("��ŷ ����\n");
			else
			{
				printf("��ŷ ����\n���α׷� ����\n");
				return 0;
			}
		}
		if (packChoice == 2)
		{
			if (UnPacking()) printf("����ŷ ����\n");
			else
			{
				printf("����ŷ ����\n���α׷� ����\n");
				return 0;
			}
		}
	}
}