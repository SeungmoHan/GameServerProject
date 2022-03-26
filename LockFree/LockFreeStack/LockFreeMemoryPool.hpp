#pragma once
#ifndef __LOCK_FREE_MEMORY_POOL__
#define __LOCK_FREE_MEMORY_POOL__
#define __UNIV_DEVELOPER_
#pragma comment(lib,"Winmm.lib")

#include <cstdlib>
#include <new>
#include <Windows.h>
#include <stdio.h>
#include <time.h>


bool g_RunningFlag = true;
#define CRASH() do{g_RunningFlag = false;int*ptr =nullptr; *ptr =100;}while(0)
#define TEST_CASE_SIZE 10000

namespace univ_dev
{
	template <typename ObjectType>
	class LockFreeMemoryPool
	{
	private:
		//외부에서 접근불가능한 노드
		struct DataNode
		{
			LockFreeMemoryPool<ObjectType>* _UnderFlowGuard = nullptr;
			ObjectType _Object;
			LockFreeMemoryPool<ObjectType>* _OverFlowGuard = nullptr;
			DataNode* _NextNode = nullptr;
			DWORD _UseFlag = false;
		};

	private:
		bool CAS(DataNode** destNode, DataNode* newNode, DataNode* originNode)
		{
			return originNode == InterlockedCompareExchangePointer((PVOID*)destNode, newNode, originNode);
		}
	public:
		//생성자 그냥 alloc을 n번 호출한것과 동일한효과 그 이후에 추가되는것도 free list처럼
		LockFreeMemoryPool(bool placementNew = false) : _Capacity(0), _UseCount(0), _PlacementNew(placementNew), _FreeNode(nullptr), _PopCount(0)/*,testSample(new Test[TEST_CASE_SIZE+1000])*/
		{
		}
		virtual ~LockFreeMemoryPool()
		{

		};


		struct Test
		{
			DWORD Cases = 101010;
			DWORD threadID = 10101010;
			DataNode* currentTop = nullptr;
			DataNode* newTop = nullptr;
			ObjectType* pObj = nullptr;
			__int64 popCount = 0;
			LockFreeMemoryPool<ObjectType>* underFlowGuard = nullptr;
			LockFreeMemoryPool<ObjectType>* overFlowGuard = nullptr;
			DWORD useFlag = 1010;
		};
		ObjectType* Alloc()
		{
			DWORD curTime = timeGetTime();
			DataNode* currentTop = nullptr;
			DataNode* nextNode = nullptr;
			LONG64 popCount = 0;
			alignas(16) LONG64 comp[2];
			//pop의 역할
			while (true)
			{
				if (!g_RunningFlag)
				{
					Sleep(INFINITE);
				}
				currentTop = this->_FreeNode;
				if (currentTop == nullptr)
				{
					ObjectType* newObject = NewAlloc();
					return newObject;
				}
				popCount = InterlockedIncrement((unsigned long long*)&this->_PopCount);
				comp[0] = (LONG64)currentTop;
				comp[1] = popCount;
				nextNode = currentTop->_NextNode;

				if (InterlockedCompareExchange128((LONG64*)&_FreeNode, popCount, (LONG64)nextNode, comp) == 1)
					break;
			}
			//if (testIdx >= TEST_CASE_SIZE - 1)
			//	InterlockedExchange((unsigned long long*) & testIdx, 0);
			//int curIdx = InterlockedIncrement((unsigned long long*) & testIdx);

			//__faststorefence();
			//testSample[curIdx].Cases = 2;
			//testSample[curIdx].threadID = GetCurrentThreadId();
			//testSample[curIdx].currentTop = currentTop;
			//testSample[curIdx].newTop = nextNode;
			//testSample[curIdx].popCount = comp[1];
			//testSample[curIdx].overFlowGuard = currentTop->_OverFlowGuard;
			//testSample[curIdx].underFlowGuard = currentTop->_UnderFlowGuard;
			//testSample[curIdx].useFlag = currentTop->_UseFlag;
			//__faststorefence();

			if (currentTop->_OverFlowGuard != this) CRASH();
			if (currentTop->_UnderFlowGuard != this) CRASH();
			if (currentTop->_UseFlag != 0) CRASH();
			InterlockedIncrement(&currentTop->_UseFlag);
			//InterlockedExchange(&currentTop->_UseFlag, true);
			ObjectType* pObj = &currentTop->_Object;
			//testSample[curIdx].pObj = pObj;
			InterlockedIncrement((unsigned long*)&this->_UseCount);
			return pObj;
		}
		bool Free(ObjectType* pData)
		{
			//if (testIdx >= TEST_CASE_SIZE - 1)
			//	InterlockedExchange((unsigned long long*) & testIdx, 0);
			//int curIdx = InterlockedIncrement((unsigned long long*) & testIdx);
			DWORD curTime = timeGetTime();
			char* temp = (char*)pData;
			temp -= sizeof(LockFreeMemoryPool<ObjectType>*);
			DataNode* removeNode = (DataNode*)temp;
#if WIN32 + 0
			if (removeNode->_UnderFlowGuard != this)
			{
				temp -= sizeof(LockFreeMemoryPool<ObjectType>*);
				removeNode = (DataNode*)temp;
			}
#endif
			DWORD useFlag = removeNode->_UseFlag;
			if (removeNode->_OverFlowGuard != this) CRASH();
			if (removeNode->_UnderFlowGuard != this) CRASH();
			if (removeNode->_UseFlag == 0) CRASH();
			if (this->_UseCount < 0) CRASH();

			//testSample[curIdx].Cases = 3;
			//testSample[curIdx].threadID = GetCurrentThreadId();
			
			DataNode* currentTop = nullptr;
			ULONG64 popCount = 0;
			alignas(16) LONG64 comp[2];

			//push의 역할
			InterlockedDecrement(&removeNode->_UseFlag);
			//InterlockedExchange(&removeNode->_UseFlag, false);
			while (true)
			{
				if (!g_RunningFlag)
				{
					Sleep(INFINITE);
				}
				currentTop = this->_FreeNode;
				popCount = InterlockedIncrement((unsigned long long*)&this->_PopCount);


				comp[0] = (LONG64)currentTop;
				comp[1] = (LONG64)popCount;

				//__faststorefence();
				//testSample[curIdx].currentTop = currentTop;
				//testSample[curIdx].useFlag = removeNode->_UseFlag;
				//testSample[curIdx].newTop = removeNode;
				//testSample[curIdx].popCount = popCount;
				//testSample[curIdx].pObj = pData;
				//testSample[curIdx].overFlowGuard = removeNode->_OverFlowGuard;
				//testSample[curIdx].underFlowGuard = removeNode->_UnderFlowGuard;
				//__faststorefence();

				removeNode->_NextNode = currentTop;
				if (InterlockedCompareExchange128((LONG64*)&this->_FreeNode, comp[1], (LONG64)removeNode, (LONG64*)comp) == 1)
					break;
			}
			InterlockedDecrement((unsigned long*)&this->_UseCount);
			return true;
		}

		int GetCapacityCount() { return this->_Capacity; }
		int GetUseCount() { return this->_UseCount; }
	private:
		ObjectType* NewAlloc()
		{
			//if (testIdx >= TEST_CASE_SIZE - 1)
			//	InterlockedExchange((unsigned long long*) & testIdx, 0);
			//int curIdx = InterlockedIncrement((unsigned long long*) & testIdx);
			DWORD curTime = timeGetTime();
			DataNode* newNode = new DataNode();
			newNode->_UnderFlowGuard = this;
			newNode->_OverFlowGuard = this;
			newNode->_UseFlag++;
			newNode->_NextNode = nullptr;
			InterlockedIncrement((LONG*)&this->_UseCount);
			InterlockedIncrement((LONG*)&this->_Capacity);
			ObjectType* pObj = &newNode->_Object;

			//DataNode* curTop = this->_FreeNode;
			//testSample[curIdx].Cases = 1;
			//testSample[curIdx].threadID = GetCurrentThreadId();
			//testSample[curIdx].currentTop = curTop;
			//testSample[curIdx].newTop = curTop;
			//testSample[curIdx].useFlag = newNode->_UseFlag;
			//testSample[curIdx].overFlowGuard = newNode->_OverFlowGuard;
			//testSample[curIdx].underFlowGuard = newNode->_UnderFlowGuard;
			//testSample[curIdx].popCount = _PopCount;
			//testSample[curIdx].pObj = pObj;

			return pObj;
		}
		alignas(16)DataNode* _FreeNode;
		__int64 _PopCount;

		int _Capacity = 0;
		int _UseCount = 0;
		bool _PlacementNew = false;

		//Test* testSample = nullptr;
		//__int64 testIdx = 0;
	};
}




#endif // !__LOCK_FREE_MEMORY_POOL__
