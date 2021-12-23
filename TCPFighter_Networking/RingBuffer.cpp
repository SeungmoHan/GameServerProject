#include "RingBuffer.h"
#include <exception>
#include <string>
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
		return end - begin;
		//return end - begin - 1;
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
		//return (end - writePointer) + (readPointer - begin - 1);
		return readPointer - writePointer;
		//return readPointer - writePointer - 1;
	}

	int RingBuffer::DirectEnqueueSize()
	{
		if (writePointer >= readPointer)
			return end - writePointer;
		//return end - writePointer - 1;
		return readPointer - writePointer;
		//return readPointer - writePointer - 1;
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
		char* tempWrite = writePointer;
		if (DirectEnqueueSize() >= size)
		{
			while (tempWrite != end && cnt < size)
			{
				*tempWrite = *buffer;
				tempWrite++;
				buffer++;
				cnt++;
			}
			MoveRear(cnt);
			//memcpy_s(writePointer, size, buffer, size);
			//MoveRear(size);
			return cnt;
		}
		const char* tempBuffer = buffer;
		while (tempWrite != end)
		{
			*tempWrite = *tempBuffer;
			tempWrite++;
			cnt++;
			tempBuffer++;
		}
		tempWrite = begin;
		while (cnt < size)
		{
			*tempWrite = *tempBuffer;
			tempWrite++;
			cnt++;
			tempBuffer++;
		}
		MoveRear(cnt);
		//int directEnqueueSize = DirectEnqueueSize();
		//memcpy_s(writePointer, directEnqueueSize, temp, directEnqueueSize);
		//temp += directEnqueueSize;
		//MoveRear(directEnqueueSize);

		//int remainSize = size - directEnqueueSize;
		//memcpy_s(writePointer, remainSize, temp, remainSize);
		//MoveRear(remainSize);
		return size;
	}

	int RingBuffer::Dequeue(char* pDest, int size)
	{
		if (GetUseSize() < size) return 0;
		int cnt = 0;
		char* tempRead = readPointer;
		if (DirectDequeueSize() >= size)
		{
			while (tempRead != end && cnt < size)
			{
				*pDest = *tempRead;
				pDest++;
				tempRead++;
				cnt++;
			}
			MoveFront(cnt);
			return cnt;
		}
		char* pDestTemp = pDest;
		while (tempRead != end)
		{
			*pDestTemp = *tempRead;
			//*readPointer = 0xff;
			tempRead++;
			cnt++;
			pDestTemp++;
		}
		tempRead = begin;
		while (cnt < size)
		{
			*pDestTemp = *tempRead;
			//*readPointer = 0xff;
			tempRead++;
			cnt++;
			pDestTemp++;
		}
		MoveFront(cnt);
		return cnt;
	}

	int RingBuffer::Peek(char* pDest, int size)
	{
		if (GetUseSize() < size) return 0;
		int cnt = 0;
		char* originReadPointer = readPointer;
		int dirDequeueSize = DirectDequeueSize();
		if (dirDequeueSize >= size)
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
		MoveFront(0);
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

	void RingBuffer::MoveRearBegin()
	{
		writePointer = begin;
	}

	void RingBuffer::MoveFrontBegin()
	{
		readPointer = begin;
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
