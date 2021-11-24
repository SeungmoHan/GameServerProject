#include <stdio.h>

int main()
{
	unsigned short value = 0;
	int input;
	int flag;
	int temp;
	while (true)
	{
		printf("��Ʈ ��ġ : ");
		scanf_s("%d", &input);
		printf("OFF/ON [0,1] : ");
		scanf_s("%d", &flag);
		if (input > 16 || input < 1)
		{
			printf("��Ʈ ���� �ʰ�\n");
			continue;
		}
		temp = 1;
		if (flag)
		{
			/*
			00000001 ���� input �ѰŸ� <<�ϸ� 
			���� input�� 2��� 00000001 �� input-1ĭ <<�ѰŰ�
			�׷��� 00000010�̴ϱ� �̰� |=���ָ� ���� �ִ� bit��� �׸��� �ѾߵǴ� ��Ʈ ���
			*/
			temp <<= input - 1;
			value |= temp;
		}
		else
		{
			/*
			11111111 ���� 1 << input �Ѱ� ����
			���� input�� 1�̸� 11111111 ���� 00000001 ���� &= �ϴ°Ŵϱ�
			11111110 &= ���ٰŰ� �׷��� ������ ��Ʈ����� �� ��Ƴ���
			�̰�쿡�� �ɷ����ߵǴ� ���̽��󼭱׷�
			*/
			temp = 0xffff - (temp << (input - 1));
			value &= temp;
		}
		temp = value;
		for (size_t i = 16; i >= 1; i--)
		{
			printf("%d �� Bit : ", i);
			if (0x8000 & temp)
				printf("ON\n");
			else
				printf("OFF\n");
			temp <<= 1;
		}
		printf("\n");
	}
}