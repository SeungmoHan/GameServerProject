#include <iostream>
#include "SerializingBuffer.h"



int main()
{
	univ_dev::Packet p;


	int a = 0x11111111;
	char b = 0x22;
	short c = 0x3333;
	double d = 0x4444444444444444;
	long e = 0x55555555;
	__int64 f = 0x6666666666666666;

	int retA_1;
	int retA_2;
	char retB;
	short retC;
	double retD;
	long retE;
	__int64 retF;
	p << a;
	p << a << b << c << d << e << f;

	p >> retA_1;
	p >> retA_2 >> retB >> retC >> retD >> retE >> retF;

	printf("retA_1 : 0x%p\n",retA_1);
	printf("retA_2 : 0x%p\n",retA_1);
	printf("retB : 0x%p\n",retB);
	printf("retC : 0x%p\n",retC);
	printf("retD : 0x%p\n",retD);
	printf("retE : 0x%p\n",retE);
	printf("retF : 0x%p\n",retF);


	printf("hello world\n");

}