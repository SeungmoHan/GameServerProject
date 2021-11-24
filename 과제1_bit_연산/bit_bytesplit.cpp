#include <stdio.h>


int main()
{
	const int bitJump = 8;
	int value = 0;
	int inputPos;
	int inputValue;
	while (true)
	{
		printf("��ġ (1~4) : ");
		scanf_s("%d", &inputPos);
		printf("�� [0~255] : ");
		scanf_s("%d", &inputValue);
		if (inputPos > 4 || inputPos < 1)
		{
			printf("��ġ ������ �ʰ��Ͽ����ϴ�.\n\n");
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
			printf("%d ��° ����Ʈ �� : %d\n", i + 1, (unsigned char)(value >> (i * bitJump)));
		}
		
		printf("\n��ü 4 ����Ʈ�� : 0x%x\n\n", (int*)value);
	}
}