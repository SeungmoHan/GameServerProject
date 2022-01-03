#pragma once
#pragma once
#ifndef __RING_BUFFER_HEADER__
#define __RING_BUFFER_HEADER__

namespace univ_dev
{
#define CRASH() int*ptr=nullptr;\
*ptr=100

	class RingBuffer
	{
	public:
		RingBuffer();
		RingBuffer(int bufferSize);

		bool ReSize(int size);
		int GetBufferSize();

		int GetUseSize();
		int GetFreeSize();

		int DirectEnqueueSize();
		int DirectDequeueSize();

		int Enqueue(const char* buffer, int size);
		int Dequeue(char* pDest, int size);
		int Peek(char* pDest, int size);

		void MoveRear(int size);
		void MoveFront(int size);
		void MoveRearBegin();
		void MoveFrontBegin();

		void ClearBuffer();

		char* GetWritePtr();
		char* GetReadPtr();

		char* GetBegin() { return begin; }
		char* GetEnd() { return end; }
	private:
		char* begin;
		char* end;
		char* readPointer;
		char* writePointer;

		int ringBufferSize;
	};
}



#endif