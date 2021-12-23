#include "RingBuffer.h"
#include <cstdlib>

namespace univ_dev
{
	RingBuffer::RingBuffer() : ringBufferSize(10000)
	{
		begin = (char*)malloc(ringBufferSize);
		end = begin + ringBufferSize;
		readPointer = writePointer = begin;
	}

	RingBuffer::RingBuffer(int bufferSize) : ringBufferSize(bufferSize)
	{
		begin = (char*)malloc(ringBufferSize);
		end = begin + ringBufferSize;
		readPointer = writePointer = begin;
	}

	bool RingBuffer::ReSize(int size)
	{
		if (GetUseSize() > size) return false;
		char* temp = (char*)malloc(size);
		if (temp == nullptr)
		{
			int* ptr = nullptr;
			*ptr = 100;
		}
		int peekRet = Peek(temp, GetUseSize());
		if (GetUseSize() != peekRet)
		{
			free(temp);
			return false;
		}
		free(begin);
		ringBufferSize = size;
		readPointer = begin = temp;
		end = begin + size;
		writePointer = begin + peekRet;
		return true;
	}

	int RingBuffer::GetBufferSize()
	{
		return end - begin - 1;
	}

	int RingBuffer::GetUseSize()
	{
		if (writePointer >= readPointer)
			return writePointer - readPointer;
		return (writePointer - begin) + (end - readPointer);
	}

	int RingBuffer::GetFreeSize()
	{
		if (writePointer >= readPointer)
			return (end - writePointer) + (readPointer - begin - 1);
		return readPointer - writePointer - 1;
	}

	int RingBuffer::DirectEnqueueSize()
	{
		if (writePointer >= readPointer)
			return end - writePointer - 1;
		return readPointer - writePointer - 1;
	}

	int RingBuffer::DirectDequeueSize()
	{
		if (writePointer >= readPointer)
			return writePointer - readPointer;
		return end - readPointer;
	}

	int RingBuffer::Enqueue(const char* buffer, int size)
	{
		if (GetFreeSize() < size) return 0;
		int cnt = 0;
		if (DirectEnqueueSize() >= size)
		{
			while (writePointer != end && cnt < size)
			{
				*writePointer = *buffer;
				writePointer++;
				buffer++;
				cnt++;
			}
			return size;
		}
		const char* temp = buffer;
		while (writePointer != end)
		{
			*writePointer = *temp;
			writePointer++;
			cnt++;
			temp++;
		}
		writePointer = begin;

		while (cnt < size)
		{
			*writePointer = *temp;
			writePointer++;
			cnt++;
			temp++;
		}

		return size;
	}

	int RingBuffer::Dequeue(char* pDest, int size)
	{
		if (GetUseSize() < size) return 0;
		int cnt = 0;
		if (DirectDequeueSize() >= size)
		{
			while (readPointer != end && cnt < size)
			{
				*pDest = *readPointer;
				pDest++;
				readPointer++;
				cnt++;
			}
			return size;
		}
		char* pDestTemp = pDest;
		while (readPointer != end)
		{
			*pDestTemp = *readPointer;
			//*readPointer = 0xff;  // for debug
			readPointer++;
			cnt++;
			pDestTemp++;
		}
		readPointer = begin;
		while (cnt < size)
		{
			*pDestTemp = *readPointer;
			//*readPointer = 0xff;  // for debug
			readPointer++;
			cnt++;
			pDestTemp++;
		}
		return size;
	}

	int RingBuffer::Peek(char* pDest, int size)
	{
		if (GetUseSize() < size) return 0;
		int cnt = 0;
		char* originReadPointer = readPointer;
		if (DirectDequeueSize() >= size)
		{
			while (readPointer != end && cnt < size)
			{
				*pDest = *readPointer;
				pDest++;
				readPointer++;
				cnt++;
			}
			readPointer = originReadPointer;
			return size;
		}
		char* pDestTemp = pDest;
		while (readPointer != end)
		{
			*pDestTemp = *readPointer;
			readPointer++;
			cnt++;
			pDestTemp++;
		}
		readPointer = begin;
		while (cnt < size)
		{
			*pDestTemp = *readPointer;
			readPointer++;
			cnt++;
			pDestTemp++;
		}
		readPointer = originReadPointer;
		return size;
	}

	void RingBuffer::MoveRear(int size)
	{
		writePointer += size;
		if (writePointer >= end)
		{
			int overFlow = writePointer - end;
			writePointer = begin + overFlow;
		}
	}


	void RingBuffer::MoveFront(int size)
	{
		readPointer += size;
		if (readPointer >= end)
		{
			int overFlow = readPointer - end;
			readPointer = begin + overFlow;
		}
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


}
