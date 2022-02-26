#pragma once
#ifndef __RING_BUFFER_HEADER__
#define __RING_BUFFER_HEADER__
#define __UNIV_DEVELOPER_
#include <Windows.h>
#define CRASH() int*ptr=nullptr;\
*ptr=100

namespace univ_dev
{

	class RingBuffer
	{
	public:
		RingBuffer();
		RingBuffer(int bufferSize);
		~RingBuffer();
		//bool ReSize(int size);
		int GetBufferSize();

		int GetUseSize();
		int GetFreeSize();

		int DirectEnqueueSize();
		int DirectDequeueSize();

		int Enqueue(const char* buffer, int size);
		int Dequeue(char* pDest, int size);
		int Peek(char* pDest, int size);

		void MoveWritePtr(int size);
		void MoveReadPtr(int size);

		void ClearBuffer();
		int GetReadPtrPosition() { return readPointer - begin; }
		int GetWritePtrPosition() { return writePointer - begin; }
		char* GetWritePtr();
		char* GetReadPtr();
		char* GetBeginPtr();
		char* GetEndPtr();
		void Lock(bool shared = false);
		void Unlock(bool shared = false);

	private:
		void MoveTempPtr(int size, char** tempPtr);
		char* begin;
		char* end;
		char* readPointer;
		char* writePointer;
		SRWLOCK lock;
		int ringBufferSize;
	};
}




#endif