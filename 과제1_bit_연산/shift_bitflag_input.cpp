#include <stdio.h>

int main()
{
	unsigned short value = 0;
	int input;
	int flag;
	int temp;
	while (true)
	{
		printf("비트 위치 : ");
		scanf_s("%d", &input);
		printf("OFF/ON [0,1] : ");
		scanf_s("%d", &flag);
		if (input > 16 || input < 1)
		{
			printf("비트 범위 초과\n");
			continue;
		}
		temp = 1;
		if (flag)
		{
			/*
			00000001 에서 input 한거를 <<하면 
			만약 input이 2라면 00000001 을 input-1칸 <<한거고
			그러면 00000010이니까 이걸 |=해주면 지금 있던 bit살고 그리고 켜야되는 비트 살고
			*/
			temp <<= input - 1;
			value |= temp;
		}
		else
		{
			/*
			11111111 에서 1 << input 한거 빼면
			만약 input이 1이면 11111111 에서 00000001 뺀거 &= 하는거니까
			11111110 &= 해줄거고 그러면 마지막 비트빼고는 다 살아남음
			이경우에는 걸러내야되는 케이스라서그럼
			*/
			temp = 0xffff - (temp << (input - 1));
			value &= temp;
		}
		temp = value;
		for (size_t i = 16; i >= 1; i--)
		{
			printf("%d 번 Bit : ", i);
			if (0x8000 & temp)
				printf("ON\n");
			else
				printf("OFF\n");
			temp <<= 1;
		}
		printf("\n");
	}
}