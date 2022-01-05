#include "RingBuffer.h"
#include <exception>
#include <string>
#include <cstdlib>

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
		//memcpy_s(writePointer, size, buffer, size);
		//MoveRear(size);
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
	if (DirectDequeueSize() >= size)
	{
		while (readPointer != end && cnt < size)
		{
			*pDest = *readPointer;
			pDest++;
			readPointer++;
			cnt++;
		}
		//memcpy_s(pDest, size, readPointer, size);
		//MoveFront(size);
		return size;
	}
	char* pDestTemp = pDest;
	while (readPointer != end)
	{
		*pDestTemp = *readPointer;
		//*readPointer = 0xff;
		readPointer++;
		cnt++;
		pDestTemp++;
	}
	readPointer = begin;
	while (cnt < size)
	{
		*pDestTemp = *readPointer;
		//*readPointer = 0xff;
		readPointer++;
		cnt++;
		pDestTemp++;
	}
	//int directDequeueSize = DirectDequeueSize();
	//memcpy_s(pDestTemp, directDequeueSize, readPointer, directDequeueSize);
	//MoveFront(directDequeueSize);
	//int remainSize = size - directDequeueSize;
	//pDestTemp += directDequeueSize;
	//memcpy_s(pDestTemp, remainSize, readPointer, remainSize);
	//MoveFront(remainSize);
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

