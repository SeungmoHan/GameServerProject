#include "RingBuffer.h"
#include <cstdlib>
#include <algorithm>
///__univ_developer_ring_buffer

namespace univ_dev
{
	RingBuffer::RingBuffer() : _RingBufferSize(10000)
	{
		InitializeSRWLock(&_Lock);
		_Begin = (char*)malloc(_RingBufferSize);
		_End = _Begin + _RingBufferSize;
		_ReadPointer = _WritePointer = _Begin;
	}

	RingBuffer::RingBuffer(int bufferSize) : _RingBufferSize(bufferSize)
	{
		InitializeSRWLock(&_Lock);
		_Begin = (char*)malloc(_RingBufferSize);
		_End = _Begin + _RingBufferSize;
		_ReadPointer = _WritePointer = _Begin;
	}

	RingBuffer::~RingBuffer()
	{
		free(_Begin);
	}
	//int RingBuffer::GetBufferSize()
	//{
	//	return (_End - _Begin - 1);
	//}

	//int RingBuffer::GetUseSize()
	//{
	//	if (_WritePointer >= _ReadPointer)
	//		return (_WritePointer - _ReadPointer);
	//	return (_WritePointer - _Begin) + (_End - _ReadPointer);
	//}

	//int RingBuffer::GetFreeSize()
	//{
	//	if (_WritePointer >= _ReadPointer)
	//		return (_End - _WritePointer) + (_ReadPointer - _Begin) - 1;
	//	return (_ReadPointer - _WritePointer) - 1;
	//}

	//int RingBuffer::DirectEnqueueSize()
	//{
	//	if (_WritePointer >= _ReadPointer)
	//		return (_End - _WritePointer);
	//	return (_ReadPointer - _WritePointer) - 1;
	//}

	//int RingBuffer::DirectDequeueSize()
	//{
	//	if (_WritePointer >= _ReadPointer)
	//		return (_WritePointer - _ReadPointer);
	//	return (_End - _ReadPointer);
	//}

	int RingBuffer::Enqueue(const char* pSrc, int size)
	{
		int freeSize = GetFreeSize();
		int writePointerPos = GetWritePtrPosition();
		int readPointerPos = GetReadPtrPosition();
		if (GetFreeSize() < size) return 0;
		int cnt = 0;
		char* tempWritePtr = _WritePointer;
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
		char* tempReadPtr = _ReadPointer;
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
		char* tempReadPtr = _ReadPointer;
		while (cnt < size)
		{
			*pDest = *tempReadPtr;
			MoveTempPtr(1, &tempReadPtr);
			pDest++;
			cnt++;
		}
		return cnt;
	}

	//void RingBuffer::MoveWritePtr(int size)
	//{
	//	if (size < 0) return;
	//	char* newWritePointer = _WritePointer + size;
	//	if (newWritePointer >= _End)
	//	{
	//		int overFlow = (newWritePointer - _End);
	//		_WritePointer = _Begin + overFlow;
	//		return;
	//	}
	//	_WritePointer = newWritePointer;
	//}


	//void RingBuffer::MoveReadPtr(int size)
	//{
	//	if (size < 0) return;
	//	char* newReadPointer = _ReadPointer + size;
	//	if (newReadPointer >= _End)
	//	{
	//		int overFlow = (newReadPointer - _End);
	//		_ReadPointer = _Begin + overFlow;
	//		return;
	//	}
	//	_ReadPointer = newReadPointer;
	//}

	//void RingBuffer::ClearBuffer()
	//{
	//	_ReadPointer = _WritePointer = _Begin;
	//}

	//char* RingBuffer::GetWritePtr()
	//{
	//	return _WritePointer;
	//}

	//char* RingBuffer::GetReadPtr()
	//{
	//	return _ReadPointer;
	//}

	//char* RingBuffer::GetBeginPtr()
	//{
	//	return _Begin;
	//}

	//char* RingBuffer::GetEndPtr()
	//{
	//	return _End;
	//}

	void RingBuffer::Lock(bool shared)
	{
		return shared ? AcquireSRWLockShared(&_Lock) : AcquireSRWLockExclusive(&_Lock);
	}

	void RingBuffer::Unlock(bool shared)
	{
		return shared ? ReleaseSRWLockShared(&_Lock) : ReleaseSRWLockExclusive(&_Lock);
	}

	//void RingBuffer::MoveTempPtr(int size, char** tempPtr)
	//{
	//	*tempPtr = *tempPtr + size;
	//	if (*tempPtr >= _End)
	//	{
	//		int overFlow = (int)(*tempPtr - _End);
	//		*tempPtr = _Begin + overFlow;
	//		return;
	//	}
	//}
}
