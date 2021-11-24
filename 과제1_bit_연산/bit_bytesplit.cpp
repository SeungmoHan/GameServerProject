#include <stdio.h>


int main()
{
	const int bitJump = 8;
	int value = 0;
	int inputPos;
	int inputValue;
	while (true)
	{
		printf("위치 (1~4) : ");
		scanf_s("%d", &inputPos);
		printf("값 [0~255] : ");
		scanf_s("%d", &inputValue);
		if (inputPos > 4 || inputPos < 1)
		{
			printf("위치 범위를 초과하였습니다.\n\n");
			continue;
		}
		int bitMask = 0;
		for (size_t i = 4; i >= 1; i--)
		{
			bitMask <<= bitJump;
			if (inputPos == i)
				bitMask += 0x0;
			else
				bitMask += 0xFF;
		}
		value &= bitMask;
		value += (inputValue << (bitJump * (inputPos - 1))) & ~bitMask;
		for (size_t i = 0; i < 4; i++)
		{
			printf("%d 번째 바이트 값 : %d\n", i + 1, (unsigned char)(value >> (i * bitJump)));
		}
		
		printf("\n전체 4 바이트값 : 0x%x\n\n", (int*)value);
	}
}