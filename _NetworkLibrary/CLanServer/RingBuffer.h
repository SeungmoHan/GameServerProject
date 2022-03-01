#pragma once
#ifndef __RING_BUFFER_HEADER__
#define __RING_BUFFER_HEADER__
#define __UNIV_DEVELOPER_

#include <Windows.h>


namespace univ_dev
{

	class RingBuffer
	{
	public:
		RingBuffer();
		RingBuffer(int bufferSize);
		~RingBuffer();
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
		int GetReadPtrPosition() { return _ReadPointer - _Begin; }
		int GetWritePtrPosition() { return _WritePointer - _Begin; }
		char* GetWritePtr();
		char* GetReadPtr();
		char* GetBeginPtr();
		char* GetEndPtr();
		void Lock(bool shared = false);
		void Unlock(bool shared = false);

	private:
		void MoveTempPtr(int size, char** tempPtr);
		char* _Begin;
		char* _End;
		char* _ReadPointer;
		char* _WritePointer;
		SRWLOCK _Lock;
		int _RingBufferSize;
	};
}




#endif