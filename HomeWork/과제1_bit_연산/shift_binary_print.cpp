#include <stdio.h>

//����1. unsigned char ������ ���� ��Ʈ ������ ����ֱ�
int main()
{
	unsigned char input = 0;
	while (scanf_s("%d", (int*)&input))
	{
		printf("%d �� ���̳ʸ� ", input);
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