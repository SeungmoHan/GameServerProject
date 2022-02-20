#include "RingBuffer.h"
#include <cstdlib>
#include <algorithm>
///__univ_developer_ring_buffer

namespace univ_dev
{
	RingBuffer::RingBuffer() : ringBufferSize(10000)
	{
		InitializeSRWLock(&lock);
		begin = (char*)malloc(ringBufferSize);
		end = begin + ringBufferSize;
		readPointer = writePointer = begin;
	}

	RingBuffer::RingBuffer(int bufferSize) : ringBufferSize(bufferSize)
	{
		InitializeSRWLock(&lock);
		begin = (char*)malloc(ringBufferSize);
		end = begin + ringBufferSize;
		readPointer = writePointer = begin;
	}

	RingBuffer::~RingBuffer()
	{
		free(begin);
	}
	int RingBuffer::GetBufferSize()
	{
		return (end - begin - 1);
	}

	int RingBuffer::GetUseSize()
	{
		if (writePointer >= readPointer)
			return (writePointer - readPointer);
		return (writePointer - begin) + (end - readPointer);
	}
	int RingBuffer::GetFreeSize()
	{
		if (writePointer >= readPointer)
			return (end - writePointer) + (readPointer - begin) - 1;
		return (readPointer - writePointer) - 1;
	}

	int RingBuffer::DirectEnqueueSize()
	{
		if (writePointer >= readPointer)
			return (end - writePointer);
		return (readPointer - writePointer) - 1;
	}

	int RingBuffer::DirectDequeueSize()
	{
		if (writePointer >= readPointer)
			return (writePointer - readPointer);
		return (end - readPointer);
	}

	int RingBuffer::Enqueue(const char* pSrc, int size)
	{
		int freeSize = GetFreeSize();
		int writePointerPos = GetWritePtrPosition();
		int readPointerPos = GetReadPtrPosition();
		if (GetFreeSize() < size) return 0;
		int cnt = 0;
		char* tempWritePtr = writePointer;
		while (cnt < size)
		{
			*tempWritePtr = *pSrc;
			MoveTempPtr(1, &tempWritePtr);
			pSrc++;
			cnt++;
		}
		MoveWritePtr(cnt);
		return cnt;
	}

	int RingBuffer::Dequeue(char* pDest, int size)
	{
		int useSize = GetUseSize();
		int writePointerPos = GetWritePtrPosition();
		int readPointerPos = GetReadPtrPosition();
		if (GetUseSize() < size) return 0;
		int cnt = 0;
		char* tempReadPtr = readPointer;
		while (cnt < size)
		{
			*pDest = *tempReadPtr;
			MoveTempPtr(1, &tempReadPtr);
			pDest++;
			cnt++;
		}
		MoveReadPtr(cnt);
		return cnt;

	}

	int RingBuffer::Peek(char* pDest, int size)
	{
		int useSize = GetUseSize();
		int writePointerPos = GetWritePtrPosition();
		int readPointerPos = GetReadPtrPosition();
		if (GetUseSize() < size) return 0;
		int cnt = 0;
		char* tempReadPtr = readPointer;
		while (cnt < size)
		{
			*pDest = *tempReadPtr;
			MoveTempPtr(1, &tempReadPtr);
			pDest++;
			cnt++;
		}
		return cnt;
	}

	void RingBuffer::MoveWritePtr(int size)
	{
		if (size < 0) return;
		char* newWritePointer = writePointer + size;
		if (newWritePointer >= end)
		{
			int overFlow = (newWritePointer - end);
			writePointer = begin + overFlow;
			return;
		}
		writePointer = newWritePointer;
	}


	void RingBuffer::MoveReadPtr(int size)
	{
		if (size < 0) return;
		char* newReadPointer = readPointer + size;
		if (newReadPointer >= end)
		{
			int overFlow = (newReadPointer - end);
			readPointer = begin + overFlow;
			return;
		}
		readPointer = newReadPointer;
	}

	void RingBuffer::ClearBuffer()
	{
		readPointer = writePointer = begin;
	}
	char* RingBuffer::GetWritePtr()
	{
		return writePointer;
	}

	char* RingBuffer::GetReadPtr()
	{
		return readPointer;
	}

	void RingBuffer::Lock(bool shared)
	{
		return shared ? AcquireSRWLockShared(&lock) : AcquireSRWLockExclusive(&lock);
	}

	void RingBuffer::Unlock(bool shared)
	{
		return shared ? ReleaseSRWLockShared(&lock) : ReleaseSRWLockExclusive(&lock);
	}

	void RingBuffer::MoveTempPtr(int size, char** tempPtr)
	{
		*tempPtr = *tempPtr + size;
		if (*tempPtr >= end)
		{
			int overFlow = (int)(*tempPtr - end);
			*tempPtr = begin + overFlow;
			return;
		}
	}
}
