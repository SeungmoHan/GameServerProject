#include "RingBuffer.h"
#include <cstdlib>

///__univ_developer_ring_buffer

namespace univ_dev
{
	RingBuffer::RingBuffer() : ringBufferSize(10000)
	{
		begin = (char*)malloc(ringBufferSize);
		//begin = (char*)RBBufferFreeList.Alloc();
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
		delete begin;
	}
	//RingBuffer::~RingBuffer()
	//{
	//	RBBufferFreeList.Free((RBBuffer*)begin);
	//}
	//bool RingBuffer::ReSize(int size)
	//{
	//	if (GetUseSize() > size) return false;
	//	char* temp = (char*)malloc(size);
	//	if (temp == nullptr)
	//	{
	//		int* ptr = nullptr;
	//		*ptr = 100;
	//	}
	//	int peekRet = Peek(temp, GetUseSize());
	//	if (GetUseSize() != peekRet)
	//	{
	//		free(temp);
	//		return false;
	//	}
	//	free(begin);
	//	ringBufferSize = size;
	//	readPointer = begin = temp;
	//	end = begin + size;
	//	writePointer = begin + peekRet;
	//	return true;
	//}

	int RingBuffer::GetBufferSize()
	{
		return end - begin;
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
			return (end - writePointer) + (readPointer - begin);
		return readPointer - writePointer;
	}

	int RingBuffer::DirectEnqueueSize()
	{
		if (writePointer >= readPointer)
			return end - writePointer;
		return readPointer - writePointer;
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
			int overFlow = writePointer - end;
			writePointer = begin + overFlow;
		}
	}


	void RingBuffer::MoveReadPtr(int size)
	{
		if (size < 0) return;
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
