#include "RingBuffer.h"
#include <iostream>
#include <vector>
#include <Windows.h>
using std::cout;
using std::endl;
using std::vector;
struct Header
{
	int type;
};
struct Msg0
{
	Header type;
	int a;
	char b;
	short c;
	__int64 d;
	int* e;
	bool operator == (const Msg0& other)
	{
		return a == other.a && b == other.b && c == other.c && d == other.d && e == other.e;
	}
	bool operator !=(const Msg0& other)
	{
		return a != other.a || b != other.b || c != other.c || d != other.d || e != other.e;
	}
};
struct Msg1
{
	Header type;
	char a;
	short b;
	short c;
	char d;
	int e;
	bool operator == (const Msg1& other)
	{
		return a == other.a && b == other.b && c == other.c && d == other.d && e == other.e;
	}
	bool operator !=(const Msg1& other)
	{
		return a != other.a || b != other.b || c != other.c || d != other.d || e != other.e;
	}
};
struct Msg2
{
	Header type;
	__int64 a;
	__int64 b;
	__int64 c;
	__int64 d;
	__int64 e;
	bool operator == (const Msg2& other)
	{
		return a == other.a && b == other.b && c == other.c && d == other.d && e == other.e;
	}
	bool operator !=(const Msg2& other)
	{
		return a != other.a || b != other.b || c != other.c || d != other.d || e != other.e;
	}
};
struct Msg3
{
	Header type;
	char a;
	char b;
	short c;
	short d;
	short e;
	bool operator == (const Msg3& other)
	{
		return a == other.a && b == other.b && c == other.c && d == other.d && e == other.e;
	}
	bool operator !=(const Msg3& other)
	{
		return a != other.a || b != other.b || c != other.c || d != other.d || e != other.e;
	}
};

struct Test
{
	int a;
	char b;
	short c;
	__int64 d;
	int* e;
	bool operator == (const Test& other)
	{
		return a == other.a && b == other.b && c == other.c && d == other.d && e == other.e;
	}
	bool operator !=(const Test& other)
	{
		return a != other.a || b != other.b || c != other.c || d != other.d || e != other.e;
	}
};

int main()
{
	univ_dev::RingBuffer buffer(480);
	char buf[1000]{ 0 };

	char sstr[]{ "123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890" };
	//buffer.Enqueue(sstr, 399);
	//buffer.Dequeue(buf, 300);

	//cout << "BUFFER SIZE :        \t" << buffer.GetBufferSize() << endl;
	//cout << "FREE   SIZE:         \t" << buffer.GetFreeSize() << endl;
	//cout << "USE    SIZE        : \t" << buffer.GetUseSize() << endl;
	//cout << "DIRECT DEQUEUE SIZE: \t" << buffer.DirectDequeueSize() << endl;
	//cout << "DIRECT ENQUEUE SIZE: \t" << buffer.DirectEnqueueSize() << endl;
	//cout << (int)buffer.GetWritePtr() << endl;
	//cout << (int)buffer.GetReadPtr() << endl;
	//cout << endl << endl;


	//buffer.Enqueue(sstr, 300);
	//buffer.Dequeue(buf, 250);
	//cout <<std::dec<< "BUFFER SIZE :        \t" << buffer.GetBufferSize() << endl;
	//cout << "FREE   SIZE:         \t" << buffer.GetFreeSize() << endl;
	//cout << "USE    SIZE        : \t" << buffer.GetUseSize() << endl;
	//cout << "DIRECT DEQUEUE SIZE: \t" << buffer.DirectDequeueSize() << endl;
	//cout << "DIRECT ENQUEUE SIZE: \t" << buffer.DirectEnqueueSize() << endl;
	//cout << (int)buffer.GetWritePtr() << endl;
	//cout << (int)buffer.GetReadPtr() << endl;
	//cout << endl << endl;



	//buffer.Dequeue(buf, 50);
	//cout << std::dec << "BUFFER SIZE :        \t" << buffer.GetBufferSize() << endl;
	//cout << "FREE   SIZE:         \t" << buffer.GetFreeSize() << endl;
	//cout << "USE    SIZE        : \t" << buffer.GetUseSize() << endl;
	//cout << "DIRECT DEQUEUE SIZE: \t" << buffer.DirectDequeueSize() << endl;
	//cout << "DIRECT ENQUEUE SIZE: \t" << buffer.DirectEnqueueSize() << endl;
	//cout << (int)buffer.GetWritePtr() << endl;
	//cout << (int)buffer.GetReadPtr() << endl;
	//cout << endl << endl;

	//buffer.Enqueue(sstr, 300);
	//buffer.Dequeue(buf, 300);

	//cout << std::dec << "BUFFER SIZE :        \t" << buffer.GetBufferSize() << endl;
	//cout << "FREE   SIZE:         \t" << buffer.GetFreeSize() << endl;
	//cout << "USE    SIZE        : \t" << buffer.GetUseSize() << endl;
	//cout << "DIRECT DEQUEUE SIZE: \t" << buffer.DirectDequeueSize() << endl;
	//cout << "DIRECT ENQUEUE SIZE: \t" << buffer.DirectEnqueueSize() << endl;
	//cout << (int)buffer.GetWritePtr() << endl;
	//cout << (int)buffer.GetReadPtr() << endl;
	//cout << endl << endl;





	//return 0;
	//임의로 Test객체 넣었다 빼는거 테스트
	int seed = 2;
	srand(seed);
	char str[]{ "1abcdefgh 2abcdefgh 3abcdefgh 4abcdefgh 5abcdefgh 6abcdefgh 7abcdefgh 8abcdefgh 9abcdefgh 0abcdefgh 1abcdefgh 2abcdefgh0" };
	clock_t beginTime;
	clock_t currentTime = beginTime = clock();
	char* temp = str;
	int idx = 0;
	int IDX = 1;
	while (true)
	{
		while (true)
		{
			//cout << "idx : " << idx << endl;
			//cout << "buffer.DirectDequeueSize()" << buffer.DirectDequeueSize() << endl;
			//cout << "buffer.DirectEnqueueSize()" << buffer.DirectEnqueueSize() << endl;
			//cout << "buffer.GetBufferSize()" << buffer.GetBufferSize() << endl;
			//cout << "buffer.GetFreeSize()" << buffer.GetFreeSize() << endl;
			//cout << "buffer.GetUseSize()" << buffer.GetUseSize() << endl;
			//cout << "buffer text : " << buffer.GetReadPtr() << endl;
			//cout << endl << endl;
			//system("pause");
			//특정 바이트만큼 문자열 넣고 특정 바이트만큼 문자열 빼기
			time_t now = time(nullptr);
			if (now % 1200 == 0)
			{
				//system("pause");
				srand(++seed);
			}
			currentTime = clock();
			if (currentTime - beginTime >= 500)
			{
				int randValue = (rand() % 50) + 200;
				buffer.ReSize(randValue);
				//printf("buffer Size Changed : %d\n", randValue);
				//printf("buffer size : %d\n\n", buffer.GetBufferSize());
				Sleep(100);
				//printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
				beginTime = currentTime = clock();
			}
			int randomEnqueueSize = (rand() % 40) + 1;
			int eqRet;
			bool enqueueFlag = buffer.GetFreeSize() >= randomEnqueueSize;
			if (enqueueFlag)
			{
				if (120 <= idx + randomEnqueueSize)
				{
					int firstEnqueueSize = 120 - idx;
					int secondEnqueueSize = randomEnqueueSize - firstEnqueueSize;
					eqRet = buffer.Enqueue(temp + idx, firstEnqueueSize);
					if (eqRet != firstEnqueueSize)
					{
						cout << idx << endl;
						cout << "eqRet : " << eqRet << endl;
						cout << "randomEnqueueSize : " << randomEnqueueSize << endl;
						cout << "first Enqueue Size : " << firstEnqueueSize << endl;
						cout << "second Enqueue Size : " << secondEnqueueSize << endl;
						cout << "buffer.GetFreeSize() : " << buffer.GetFreeSize() << endl;
						return -123;
					}
					idx = 0;
					eqRet = buffer.Enqueue(temp + idx, secondEnqueueSize);
					if (eqRet != secondEnqueueSize)
					{
						cout << idx << endl;
						cout <<"eqRet : "<< eqRet << endl;
						cout << "randomEnqueueSize : " << randomEnqueueSize << endl;
						cout << "first Enqueue Size : " << firstEnqueueSize << endl;
						cout << "second Enqueue Size : " << secondEnqueueSize << endl;
						cout << " buffer.GetFreeSize() : " << buffer.GetFreeSize() << endl;
						return -321;
					}
					idx += secondEnqueueSize;
					//printf("random enqueue size : %d\t", randomEnqueueSize);
					//printf("eqret : %d\n", eqRet);
				}
				else
				{
					eqRet = buffer.Enqueue(temp + idx, randomEnqueueSize);
					//printf("random enqueue size : %d\t", randomEnqueueSize);
					//printf("eqret : %d\t\t", eqRet);
					idx += eqRet;
				}
			}

			char tempBuffer[1050]{ 0 };
			int randomDequeueSize = (rand() % 100) + 100;
			int dqRet = buffer.Dequeue(tempBuffer, randomDequeueSize);
			//printf("random dequeue size : %d\t", randomDequeueSize);
			//printf("dqret : %d\t", dqRet);
			if (dqRet > 0)
			{
				printf("%s", tempBuffer);
			}
			Sleep(1);

			//////////////진짜 링버퍼처럼 구현
			////////////std::vector<Msg0> msg1Vec, msg1RetVec;
			////////////std::vector<Msg1> msg2Vec, msg2RetVec;
			////////////std::vector<Msg2> msg3Vec, msg3RetVec;
			////////////std::vector<Msg3> msg4Vec, msg4RetVec;
			////////////int loopCount = rand() % 1000;
			////////////printf("loop count : %d\n", loopCount);
			////////////for (int i = 0; i < 4; i++)
			////////////{
			////////////	int randNum = rand() % 4;
			////////////	printf("%d case \t", randNum);
			////////////	switch (randNum)
			////////////	{
			////////////	case 0:
			////////////	{
			////////////		Msg0 packet;
			////////////		packet.type.type = 0;
			////////////		packet.a = (rand() % 10) + 5;
			////////////		packet.b = (rand() % 10) + 10;
			////////////		packet.c = (rand() % 10) + 15;
			////////////		packet.d = (rand() % 10) + (__int64)20;
			////////////		packet.e = (int*)(rand() % 10 + (__int64)0xff);
			////////////		buffer.Enqueue((const char*)&packet, sizeof(Msg0));
			////////////		msg1Vec.push_back(packet);
			////////////		break;
			////////////	}
			////////////	case 1:
			////////////	{
			////////////		Msg1 packet;
			////////////		packet.type.type = 1;
			////////////		packet.a = (rand() % 10) + 5;
			////////////		packet.b = (rand() % 10) + 10;
			////////////		packet.c = (rand() % 10) + 15;
			////////////		packet.d = (rand() % 10) + (__int64)20;
			////////////		packet.e = (rand() % 10 + (__int64)0xff);
			////////////		buffer.Enqueue((const char*)&packet, sizeof(Msg1));
			////////////		msg2Vec.push_back(packet);
			////////////		break;
			////////////	}
			////////////	case 2:
			////////////	{
			////////////		Msg2 packet;
			////////////		packet.type.type = 2;
			////////////		packet.a = (rand() % 10) + (__int64)5;
			////////////		packet.b = (rand() % 10) + (__int64)10;
			////////////		packet.c = (rand() % 10) + (__int64)15;
			////////////		packet.d = (rand() % 10) + (__int64)20;
			////////////		packet.e = (rand() % 10 + (__int64)0xff);
			////////////		buffer.Enqueue((const char*)&packet, sizeof(Msg2));
			////////////		msg3Vec.push_back(packet);
			////////////		break;
			////////////	}
			////////////	case 3:
			////////////	{
			////////////		Msg3 packet;
			////////////		packet.type.type = 3;
			////////////		packet.a = (rand() % 10) + 5;
			////////////		packet.b = (rand() % 10) + 10;
			////////////		packet.c = (rand() % 10) + 15;
			////////////		packet.d = (rand() % 10) + (__int64)20;
			////////////		packet.e = (rand() % 10 + (short)30);
			////////////		buffer.Enqueue((const char*)&packet, sizeof(Msg3));
			////////////		msg4Vec.push_back(packet);
			////////////		break;
			////////////	}
			////////////	}
			////////////}
			//////////////printf("Use Size : %d\n", buffer.GetUseSize());
			//////////////printf("Free Size : %d\n", buffer.GetFreeSize());
			//////////////printf("all packet size : %llu\n", sizeof(Msg1) + sizeof(Msg2) + sizeof(Msg3) + sizeof(Msg0));
			//////////////printf("sizeof(header) : %llu\n", sizeof(Header));
			////////////while (true)
			////////////{
			////////////	Header header;
			////////////	cout << sizeof(Msg3) << endl;
			////////////	if (buffer.GetUseSize() < (int)sizeof(Msg2))
			////////////	{
			////////////		//printf("%d\n", __LINE__);
			////////////		break;
			////////////	}
			////////////	int peekRet = buffer.Peek((char*)&header, sizeof(Header));
			////////////	if (peekRet == 0)
			////////////	{
			////////////		//printf("%d\n", __LINE__);
			////////////		break;
			////////////	}
			////////////	printf("type : %d\n", header.type);
			////////////	switch (header.type)
			////////////	{
			////////////	case 0:
			////////////	{
			////////////		//printf("packet case 0\n");
			////////////		if (buffer.GetUseSize() < sizeof(Msg0))
			////////////		{
			////////////			//printf("%d\n", __LINE__);
			////////////			break;
			////////////		}
			////////////		Msg0 packet;
			////////////		//printf("packet size : %llu\n", sizeof(packet));
			////////////		int dqRet = buffer.Dequeue((char*)&packet, sizeof(Msg0));
			////////////		if (dqRet == 0)
			////////////		{
			////////////			//printf("%d\n", __LINE__);
			////////////			break;
			////////////		}
			////////////		msg1RetVec.push_back(packet);
			////////////		break;
			////////////	}
			////////////	case 1:
			////////////	{
			////////////		//printf("packet case 1\n");
			////////////		if (buffer.GetUseSize() < sizeof(Msg1))
			////////////		{
			////////////			//printf("%d\n", __LINE__);
			////////////			break;
			////////////		}
			////////////		Msg1 packet;
			////////////		//printf("packet size : %llu", sizeof(packet));
			////////////		int dqRet = buffer.Dequeue((char*)&packet, sizeof(Msg1));
			////////////		if (dqRet == 0)
			////////////		{
			////////////			//printf("%d\n", __LINE__);
			////////////			break;
			////////////		}
			////////////		msg2RetVec.push_back(packet);
			////////////		break;
			////////////	}
			////////////	case 2:
			////////////	{
			////////////		//printf("packet case 2\n");
			////////////		if (buffer.GetUseSize() < sizeof(Msg2))
			////////////		{
			////////////			//printf("%d\n", __LINE__);
			////////////			break;
			////////////		}
			////////////		Msg2 packet;
			////////////		printf("packet size : %llu", sizeof(packet));
			////////////		int dqRet = buffer.Dequeue((char*)&packet, sizeof(Msg2));
			////////////		printf("dqRet : %d", dqRet);
			////////////		if (dqRet == 0)
			////////////		{
			////////////			//printf("%d\n", __LINE__);
			////////////			break;
			////////////		}
			////////////		msg3RetVec.push_back(packet);
			////////////		break;
			////////////	}
			////////////	case 3:
			////////////	{
			////////////		//printf("packet case 3\n");
			////////////		if (buffer.GetUseSize() < sizeof(Msg3))
			////////////		{
			////////////			//printf("%d\n", __LINE__);
			////////////			break;
			////////////		}
			////////////		Msg3 packet;
			////////////		//printf("packet size : %llu", sizeof(packet));
			////////////		int dqRet = buffer.Dequeue((char*)&packet, sizeof(Msg3));
			////////////		if (dqRet == 0)
			////////////		{
			////////////			//printf("%d\n", __LINE__);
			////////////			break;
			////////////		}
			////////////		msg4RetVec.push_back(packet);
			////////////		break;
			////////////	}
			////////////	}
			////////////	printf("GetUseSize() -> %d\n", buffer.GetUseSize());
			////////////}
			//////////////printf("v1 size : %llu, v1r size : %llu\n", msg1Vec.size(), msg1RetVec.size());
			//////////////printf("v2 size : %llu, v2r size : %llu\n", msg2Vec.size(), msg2RetVec.size());
			//////////////printf("v3 size : %llu, v3r size : %llu\n", msg3Vec.size(), msg3RetVec.size());
			//////////////printf("v4 size : %llu, v4r size : %llu\n\n", msg4Vec.size(), msg4RetVec.size());
			////////////if (msg1Vec.size() != msg1RetVec.size())
			////////////{
			////////////	printf("v1 size : %llu, v1r size : %llu\n", msg1Vec.size(), msg1RetVec.size());
			////////////	return 1;
			////////////}
			////////////if (msg2Vec.size() != msg2RetVec.size())
			////////////{
			////////////	printf("v2 size : %llu, v2r size : %llu\n", msg2Vec.size(), msg2RetVec.size());
			////////////	return 2;
			////////////}
			////////////if (msg3Vec.size() != msg3RetVec.size())
			////////////{
			////////////	printf("v3 size : %llu, v3r size : %llu\n", msg3Vec.size(), msg3RetVec.size());
			////////////	return 3;
			////////////}
			////////////if (msg4Vec.size() != msg4RetVec.size())
			////////////{
			////////////	printf("v4 size : %llu, v4r size : %llu\n", msg4Vec.size(), msg4RetVec.size());
			////////////	return 4;
			////////////}

			////////////for (int i = 0; i < msg1Vec.size(); i++)
			////////////	if (msg1Vec[i] != msg1RetVec[i]) return -1;
			////////////for (int i = 0; i < msg2Vec.size(); i++)
			////////////	if (msg2Vec[i] != msg2RetVec[i]) return -2;
			////////////for (int i = 0; i < msg3Vec.size(); i++)
			////////////	if (msg3Vec[i] != msg3RetVec[i]) return -3;
			////////////for (int i = 0; i < msg4Vec.size(); i++)
			////////////	if (msg4Vec[i] != msg4RetVec[i]) return -4;

			////////	std::vector<Test> tv, rv;
			////////	int enqueueRandomTime = 10000;
			////////	int cnt = 0;
			////////	Test t{ (rand()) % 5,(rand() % 5) + 5,(rand() % 5) + 10,4,(int*)rand() };
			////////	for (int i = 0; i < enqueueRandomTime; i++)
			////////	{
			////////		t.a = rand() % 5;
			////////		t.b = (rand() % 5) + 10;
			////////		t.c = (rand() % 5) + 15;
			////////		t.d = (rand() % 5) + 20;
			////////		t.e = (int*)(rand());
			////////		tv.push_back(t);
			////////		int enqueueSize = buffer.Enqueue((const char*)&t, sizeof(Test));
			////////		printf("EnqueueSize : %d\n", enqueueSize);
			////////		if (enqueueSize == 0) break;
			////////		cnt++;
			////////	}
			////////	Test t2{ 0 };
			////////	int dqCount = 0;
			////////	while (true)
			////////	{
			////////		//printf("freeSize : %d\t",buffer.GetFreeSize());
			////////		int dequeueSize = buffer.Dequeue((char*)&t2, sizeof(Test));
			////////		printf("Dequeue Size : %d\n", dequeueSize);
			////////		if (dequeueSize == 0)
			////////		{
			////////			break;
			////////		}
			////////		dqCount++;
			////////		rv.push_back(t2);
			////////	}
			////////	printf("eqCnt : %d\n", cnt);
			////////	printf("dqCount : %d\n", dqCount);
			////////	for (int i = 0; i < cnt; i++)
			////////	{
			////////		if (tv[i] != rv[i])
			////////		{
			////////			printf("T1.a : %d\t\t\t\t T2.a : %d\n", tv[i].a, rv[i].a);
			////////			printf("T1.a : %d\t\t\t\t T2.b : %d\n", tv[i].b, rv[i].b);
			////////			printf("T1.a : %d\t\t\t\t T2.c : %d\n", tv[i].c, rv[i].c);
			////////			printf("T1.a : %lld\t\t\t\t T2.d : %lld\n", tv[i].d, rv[i].d);
			////////			printf("T1.a : %x\t\t\t\t T2.e : %x\n", tv[i].e, rv[i].e);

			////////			printf("i : %d\n", i);
			////////			printf("fail");
			////////			return 0;
			////////		}
			////////	}

		}
	}

}