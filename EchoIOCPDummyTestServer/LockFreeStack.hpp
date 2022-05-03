#pragma once
#ifndef __LOCK_FREE_STACK__
#define __LOCK_FREE_STACK__
#define __UNIV_DEVELOPER_

#include <Windows.h>
#include "LockFreeMemoryPool.hpp"

#define TEST_CASE_SIZE 10000

namespace univ_dev
{
	template <typename T>
	class LockFreeStack
	{
	private:
		struct Node
		{
			T _Data;
			Node* _Next = nullptr;
		};
	public:
		LockFreeStack() : _Top(nullptr), _Size(0)/*, _Pool(new LockFreeMemoryPool<Node>())*/, _CheckCount(0) { }
		~LockFreeStack()
		{
			while (_Top != nullptr)
			{
				Node* next = _Top->_Next;
				_Pool.Free(_Top);
				_Top = next;
			}
			//delete _Pool;
		}
		int size() { return _Size; }
		int GetCapacityCount() { return _Pool.GetCapacityCount(); }
		int GetUseCount() { return _Pool.GetUseCount(); }
		void SetTotalPushPopCountZero()
		{
			InterlockedExchange(&this->tps, 0);
		}
		void push(T data)
		{
			Node* newNode = _Pool.Alloc();

			newNode->_Data = data;
			Node* currentTop = nullptr;
			LONG64 popCount = 0;
			alignas(16) LONG64 comp[2];

			DWORD loopCount = 0;

			while (true)
			{
				currentTop = this->_Top;
				popCount = this->_CheckCount;
				newNode->_Next = currentTop;

				if (InterlockedCompareExchange128((LONG64*)&this->_Top, comp[1] + 1, (LONG64)newNode, comp) == 1)
					break;
			}
			InterlockedIncrement((unsigned int*)&_Size);
		}
		bool pop(T& ret)
		{
			Node* currentTop = nullptr;
			Node* nextNode = nullptr;
			LONG64 popCount;
			alignas(16)LONG64 comp[2];

			while (true)
			{
				currentTop = this->_Top;
				if (this->isempty(currentTop))
				{
					CRASH();
					return false;
				}

				popCount = this->_CheckCount;
				nextNode = currentTop->_Next;
				comp[0] = (LONG64)currentTop;
				comp[1] = popCount;

				if (InterlockedCompareExchange128((LONG64*)&this->_Top, comp[1] + 1, (LONG64)nextNode, comp) == 1)
					break;
			}

			ret = currentTop->_Data;

			_Pool.Free(currentTop);
			InterlockedDecrement((unsigned int*)&_Size);
			return true;
		}
		bool isempty(Node* top)
		{
			return nullptr == top;
		}
	private:
		LockFreeMemoryPool<Node>	_Pool;
		alignas(64) Node* _Top;
		__int64						_CheckCount;
		alignas(64) int				_Size;
	};
}




#endif // !__LOCK_FREE_STACK__
