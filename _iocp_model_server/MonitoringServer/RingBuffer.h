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
		inline int GetBufferSize()
		{
			return (_End - _Begin - 1);
		}

		inline int GetUseSize()
		{
			if (_WritePointer >= _ReadPointer)
				return (_WritePointer - _ReadPointer);
			return (_WritePointer - _Begin) + (_End - _ReadPointer);
		}
		inline int GetFreeSize()
		{
			if (_WritePointer >= _ReadPointer)
				return (_End - _WritePointer) + (_ReadPointer - _Begin) - 1;
			return (_ReadPointer - _WritePointer) - 1;
		}

		inline int DirectEnqueueSize()
		{
			if (_WritePointer >= _ReadPointer)
				return (_End - _WritePointer);
			return (_ReadPointer - _WritePointer) - 1;
		}
		inline int DirectDequeueSize()
		{
			if (_WritePointer >= _ReadPointer)
				return (_WritePointer - _ReadPointer);
			return (_End - _ReadPointer);
		}

		int Enqueue(const char* buffer, int size);
		int Dequeue(char* pDest, int size);
		int Peek(char* pDest, int size);

		inline void MoveWritePtr(int size)
		{
			if (size < 0) return;
			char* newWritePointer = _WritePointer + size;
			if (newWritePointer >= _End)
			{
				int overFlow = (newWritePointer - _End);
				_WritePointer = _Begin + overFlow;
				return;
			}
			_WritePointer = newWritePointer;
		}
		inline void MoveReadPtr(int size)
		{
			if (size < 0) return;
			char* newReadPointer = _ReadPointer + size;
			if (newReadPointer >= _End)
			{
				int overFlow = (newReadPointer - _End);
				_ReadPointer = _Begin + overFlow;
				return;
			}
			_ReadPointer = newReadPointer;
		}

		inline void ClearBuffer()
		{
			_ReadPointer = _WritePointer = _Begin;
		}
		inline int GetReadPtrPosition() { return _ReadPointer - _Begin; }
		inline int GetWritePtrPosition() { return _WritePointer - _Begin; }
		inline char* GetWritePtr() {return _WritePointer;};
		inline char* GetReadPtr() { return _ReadPointer; };
		inline char* GetBeginPtr() { return _Begin; };
		inline char* GetEndPtr() { return _End; };
		void Lock(bool shared = false);
		void Unlock(bool shared = false);

	private:
		inline void MoveTempPtr(int size, char** tempPtr)
		{
			*tempPtr = *tempPtr + size;
			if (*tempPtr >= _End)
			{
				int overFlow = (int)(*tempPtr - _End);
				*tempPtr = _Begin + overFlow;
				return;
			}
		}
		//Read Only
		SRWLOCK _Lock;
		char* _Begin;
		char* _End;
		int _RingBufferSize;
		char* _ReadPointer;
		char* _WritePointer;
	};
}




#endif