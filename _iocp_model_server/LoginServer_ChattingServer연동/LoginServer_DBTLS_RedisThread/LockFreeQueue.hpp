#pragma once
#ifndef __LOCK_FREE_QUEUE__
#define __LOCK_FREE_QUEUE__
#define __UNIV_DEVELOPER_

#include "LockFreeMemoryPool.hpp"



namespace univ_dev
{

	template<typename T>
	class LockFreeQueue
	{
	private:
		struct Node
		{
			T _Value;
			Node* _Next;
		};
		struct cmpNode
		{
			Node* _Node;
			__int64 _Count;
		};

	public:

		LockFreeQueue() : _Size(0), _Head(nullptr), _Tail(nullptr), /*_Pool(new LockFreeMemoryPool<Node>),*/ _HeadCheckCount(0), _TailCheckCount(0)
		{
			_Tail = _Head = this->_Pool.Alloc();
		}
		~LockFreeQueue()
		{
			for (int i = 0; i < _Size; i++)
			{
				T ret;
				if (!dequeue(ret))
					CRASH();
			}
		}

		int size()
		{
			return _Size;
		}
		int GetUseCount()
		{
			return _Pool.GetUseCount();
		}
		int GetCapacityCount()
		{
			return _Pool.GetCapacityCount();
		}

		void enqueue(T val)
		{
			Node* newNode = this->_Pool.Alloc();
			newNode->_Value = val;
			newNode->_Next = nullptr;

			while (true)
			{
				cmpNode cmp;
				cmp._Node = this->_Tail;
				cmp._Count = this->_TailCheckCount;

				if (cmp._Node->_Next == nullptr)
				{
					if (InterlockedCompareExchangePointer((PVOID*)&this->_Tail->_Next, newNode, nullptr) == nullptr)
					{
						InterlockedCompareExchange128((LONG64*)&this->_Tail, cmp._Count + 1, (LONG64)newNode, (LONG64*)&cmp);
						break;
					}
				}
				else
				{
					InterlockedCompareExchange128((LONG64*)&this->_Tail, cmp._Count + 1, (LONG64)cmp._Node->_Next, (LONG64*)&cmp);
				}
			}
			InterlockedIncrement((unsigned long*)&this->_Size);
		}

		bool dequeue(T& ret)
		{
			int curSize = InterlockedDecrement((unsigned long*)&this->_Size);
			if (curSize < 0)
			{
				InterlockedIncrement((unsigned long*)&this->_Size);
				return false;
			}
			Node* currentHead = nullptr;
			Node* nextNode = nullptr;
			T retData;
			cmpNode cmpHead;
			while (true)
			{
				cmpHead._Node = this->_Head;
				cmpHead._Count = this->_HeadCheckCount;
				Node* headNext = cmpHead._Node->_Next;
				cmpNode cmpTail;
				cmpTail._Node = this->_Tail;
				cmpTail._Count = this->_TailCheckCount;
				
				if (cmpTail._Node->_Next != nullptr)
				{
					InterlockedCompareExchange128((LONG64*)&this->_Tail, cmpTail._Count + 1, (LONG64)cmpTail._Node->_Next, (LONG64*)&cmpTail);
					continue;
				}
				if (headNext == nullptr)
					continue;
				
				retData = headNext->_Value;
				if (InterlockedCompareExchange128((LONG64*)&this->_Head, cmpHead._Count + 1, (LONG64)this->_Head->_Next, (LONG64*)&cmpHead))
					break;
			}
			ret = retData;
			this->_Pool.Free(cmpHead._Node);
			return true;
		}

	private:
		LockFreeMemoryPool<Node>	_Pool;
		alignas(64) Node*			_Head;
		__int64						_HeadCheckCount;

		alignas(64) Node*			_Tail;
		__int64						_TailCheckCount;

		alignas(64) int				_Size;
	};
}



#endif // !__LOCK_FREE_QUEUE__