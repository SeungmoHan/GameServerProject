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
			return (this->_End - this->_Begin - 1);
		}

		inline int GetUseSize()
		{
			if (this->_WritePointer >= this->_ReadPointer)
				return (this->_WritePointer - this->_ReadPointer);
			return (this->_WritePointer - this->_Begin) + (this->_End - this->_ReadPointer);
		}
		inline int GetFreeSize()
		{
			if (this->_WritePointer >= this->_ReadPointer)
				return (this->_End - this->_WritePointer) + (this->_ReadPointer - this->_Begin) - 1;
			return (this->_ReadPointer - this->_WritePointer) - 1;
		}

		inline int DirectEnqueueSize()
		{
			if (this->_WritePointer >= this->_ReadPointer)
				return (this->_End - this->_WritePointer);
			return (this->_ReadPointer - this->_WritePointer) - 1;
		}
		inline int DirectDequeueSize()
		{
			if (this->_WritePointer >= this->_ReadPointer)
				return (this->_WritePointer - this->_ReadPointer);
			return (this->_End - this->_ReadPointer);
		}

		int Enqueue(const char* buffer, int size);
		int Dequeue(char* pDest, int size);
		int Peek(char* pDest, int size);

		inline void MoveWritePtr(int size)
		{
			if (size < 0) return;
			char* newWritePointer = this->_WritePointer + size;
			if (newWritePointer >= _End)
			{
				int overFlow = (newWritePointer - this->_End);
				this->_WritePointer = this->_Begin + overFlow;
				return;
			}
			this->_WritePointer = newWritePointer;
		}
		inline void MoveReadPtr(int size)
		{
			if (size < 0) return;
			char* newReadPointer = this->_ReadPointer + size;
			if (newReadPointer >= this->_End)
			{
				int overFlow = (newReadPointer - this->_End);
				this->_ReadPointer = this->_Begin + overFlow;
				return;
			}
			this->_ReadPointer = newReadPointer;
		}

		inline void ClearBuffer()
		{
			this->_ReadPointer = this->_WritePointer = this->_Begin;
		}
		inline int GetReadPtrPosition() { return this->_ReadPointer - this->_Begin; }
		inline int GetWritePtrPosition() { return this->_WritePointer - this->_Begin; }
		inline char* GetWritePtr() {return this->_WritePointer;};
		inline char* GetReadPtr() { return this->_ReadPointer; };
		inline char* GetBeginPtr() { return this->_Begin; };
		inline char* GetEndPtr() { return this->_End; };
		void Lock(bool shared = false);
		void Unlock(bool shared = false);

	private:
		inline void MoveTempPtr(int size, char** tempPtr)
		{
			*tempPtr = *tempPtr + size;
			if (*tempPtr >= this->_End)
			{
				int overFlow = (int)(*tempPtr - this->_End);
				*tempPtr = this->_Begin + overFlow;
				return;
			}
		}
		//Read Only
		SRWLOCK _Lock;
		char*	_Begin;
		char*	_End;
		int	_RingBufferSize;
		char*	_ReadPointer;
		char*	_WritePointer;
	};
}




#endif