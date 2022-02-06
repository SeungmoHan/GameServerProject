#pragma once
#ifndef __RING_BUFFER_HEADER__
#define __RING_BUFFER_HEADER__
#define __UNIV_DEVELOPER_



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

		char* GetWritePtr();
		char* GetReadPtr();
	private:
		char* begin;
		char* end;
		char* readPointer;
		char* writePointer;

		int ringBufferSize;
	};
}

#endif







