#include <stdio.h>

//과제1. unsigned char 변수의 값을 비트 단위로 찍어주기
int main()
{
	unsigned char input = 0;
	while (scanf_s("%d", (int*)&input))
	{
		printf("%d 의 바이너리 ", input);
		for (size_t i = 0; i < 8; i++)
		{
			if (input & 0x80) // 128 or 0b10000000
				printf("1");
			else
				printf("0");
			input <<= 1;
		}
		printf("\n");
	}
}