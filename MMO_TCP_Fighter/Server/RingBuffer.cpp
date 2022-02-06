#include "RingBuffer.h"
#include <cstdlib>
#include <algorithm>
///__univ_developer_ring_buffer

namespace univ_dev
{
	RingBuffer::RingBuffer() : ringBufferSize(20000)
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

	RingBuffer::~RingBuffer()
	{
		free(begin);
	}
	int RingBuffer::GetBufferSize()
	{
		return (int)(end - begin - 1);
	}

	int RingBuffer::GetUseSize()
	{
		if (writePointer >= readPointer)
			return (int)(writePointer - readPointer);
		return (int)(writePointer - begin) + (int)(end - readPointer);
	}
	int RingBuffer::GetFreeSize()
	{
		if (writePointer >= readPointer)
			return (int)(end - writePointer) + (int)(readPointer - begin) - 1;
		return (int)(readPointer - writePointer) - 1;
	}

	int RingBuffer::DirectEnqueueSize()
	{
		if (writePointer >= readPointer)
			return (int)(end - writePointer);
		return (int)(readPointer - writePointer) - 1;
	}

	int RingBuffer::DirectDequeueSize()
	{
		if (writePointer >= readPointer)
			return (int)(writePointer - readPointer);
		return (int)(end - readPointer);
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
			MoveWritePtr(0);
			return cnt;
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
		return cnt;
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
			MoveReadPtr(0);
			return cnt;
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
		return cnt;
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
			return cnt;
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
		return cnt;
	}

	void RingBuffer::MoveWritePtr(int size)
	{
		if (size < 0) return;
		writePointer += size;
		if (writePointer >= end)
		{
			int overFlow = (int)(writePointer - end);
			writePointer = begin + overFlow;
		}
	}


	void RingBuffer::MoveReadPtr(int size)
	{
		if (size < 0) return;
		readPointer += size;
		if (readPointer >= end)
		{
			int overFlow = (int)(readPointer - end);
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
