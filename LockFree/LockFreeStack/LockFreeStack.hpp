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
		struct Test
		{
			DWORD _ThreadID;
			Node* _NewTop;
			Node* _CurrentTop;
			__int64 _PopCount;
			T _CurrentNodeData;
			bool isPush;
			int _BeginStackSize;
			int _EndStackSize;
			DWORD _LoopCount;
		};

		bool CAS(Node** destNode, Node* newNode, Node* originNode)
		{
			return originNode == InterlockedCompareExchangePointer((PVOID*)destNode, newNode, originNode);
		}

	public:
		LockFreeStack() : _Top(nullptr), _Size(0), _TestSample(new Test[TEST_CASE_SIZE + 1000]),_PopCount(0), _TestSampleIdx(0){ }

		int size() { return _Size; }
		int pool_capacity(){ return _Pool.GetCapacityCount(); }
		int pool_size() { return _Pool.GetUseCount(); }
		void push(T data)
		{
			if (_TestSampleIdx >= TEST_CASE_SIZE - 1)
				InterlockedExchange((unsigned long long*) & _TestSampleIdx, 0);
			int curIdx = InterlockedIncrement((unsigned long long*) & _TestSampleIdx);


			_TestSample[curIdx]._ThreadID = GetCurrentThreadId();
			_TestSample[curIdx].isPush= true;
			_TestSample[curIdx]._LoopCount = 0;
			_TestSample[curIdx]._CurrentNodeData = data;
			Node* newNode = _Pool.Alloc();
			newNode->_Data = data;
			Node* currentTop = nullptr;
			LONG64 popCount = 0;
			alignas(16) LONG64 comp[2];
			DWORD loopCount = 0;

			while (true)
			{
				currentTop = this->_Top;
				popCount = InterlockedIncrement((unsigned long long*) & this->_PopCount);
				_TestSample[curIdx]._BeginStackSize = _Size;
				_TestSample[curIdx]._CurrentTop = currentTop;
				_TestSample[curIdx]._PopCount = popCount;
				
				newNode->_Next = currentTop;
				comp[0] = (LONG64)currentTop;
				comp[1] = popCount;
				loopCount++;

				if (InterlockedCompareExchange128((LONG64*)&this->_Top, comp[1], (LONG64)newNode, comp) == 1)
					break;
			}
			_TestSample[curIdx]._LoopCount = loopCount;

			int endStackSize = InterlockedIncrement((unsigned int*)&_Size);
			_TestSample[curIdx]._NewTop = newNode;
			_TestSample[curIdx]._EndStackSize = endStackSize;

		}
		bool pop(T& ret)
		{
			if (_TestSampleIdx >= TEST_CASE_SIZE - 1)
				InterlockedExchange((unsigned long long*) & _TestSampleIdx, 0);
			int curIdx = InterlockedIncrement((unsigned long long*) & _TestSampleIdx);

			_TestSample[curIdx]._ThreadID = GetCurrentThreadId();
			_TestSample[curIdx]._LoopCount = 0;
			_TestSample[curIdx].isPush = false;

			Node* currentTop = nullptr;
			Node* nextNode = nullptr;
			LONG64 popCount;
			DWORD loopCount = 0;
			alignas(16)LONG64 comp[2];




			while (true)
			{
				_TestSample[curIdx]._BeginStackSize = _Size;
				currentTop = this->_Top;
				if (this->isempty(currentTop))
				{
					CRASH();
					return false;
				}
				popCount = InterlockedIncrement((unsigned long long*) & this->_PopCount);
				nextNode = currentTop->_Next;

				_TestSample[curIdx]._CurrentTop = currentTop;
				_TestSample[curIdx]._PopCount = popCount;
				loopCount++;


				comp[0] = (LONG64)currentTop;
				comp[1] = popCount;

				if (InterlockedCompareExchange128((LONG64*)&this->_Top, comp[1], (LONG64)nextNode, comp) == 1)
					break;
			}

			_TestSample[curIdx]._LoopCount = loopCount;
			_TestSample[curIdx]._NewTop = nextNode;

			ret = currentTop->_Data;
			_TestSample[curIdx]._CurrentNodeData = ret;
			_Pool.Free(currentTop);
			InterlockedDecrement((unsigned int*)&_Size);
			return true;
		}
		bool isempty(Node* top)
		{
			return nullptr == top;
		}
	private:
		alignas(16) Node* _Top;
		__int64 _PopCount;
		int _Size;
		LockFreeMemoryPool<Node> _Pool;

		DWORD _TestSampleIdx;
		Test* _TestSample;
	};
}




#endif // !__LOCK_FREE_STACK__
