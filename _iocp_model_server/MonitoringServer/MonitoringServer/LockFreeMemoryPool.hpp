#pragma once
#ifndef __LOCK_FREE_MEMORY_POOL__
#define __LOCK_FREE_MEMORY_POOL__
#define __UNIV_DEVELOPER_

#include <cstdlib>
#include <new>
#include <Windows.h>


#define CRASH() do{int*ptr =nullptr; *ptr =100;}while(0)

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

	public:
		//생성자 그냥 alloc을 n번 호출한것과 동일한효과 그 이후에 추가되는것도 free list처럼
		LockFreeMemoryPool(bool placementNew = false) : _Capacity(0), _UseCount(0), _PlacementNew(placementNew), _FreeNode(nullptr), _CheckCount(0)
		{
		}
		virtual ~LockFreeMemoryPool()
		{
			while (this->_FreeNode != nullptr)
			{
				DataNode* curNode = this->_FreeNode;
				this->_FreeNode = _FreeNode->_NextNode;
				delete curNode;
			}
		};
		ObjectType* Alloc()
		{
			DataNode* currentTop = nullptr;
			DataNode* nextNode = nullptr;
			LONG64 popCount = 0;
			alignas(16) LONG64 comp[2];
			//pop의 역할
			while (true)
			{
				currentTop = this->_FreeNode;
				if (currentTop == nullptr)
				{
					ObjectType* newObject = NewAlloc();
					return newObject;
				}
				
				popCount = this->_CheckCount;

				comp[0] = (LONG64)currentTop;
				comp[1] = popCount;
				nextNode = currentTop->_NextNode;

				if (InterlockedCompareExchange128((LONG64*)&_FreeNode, comp[1]+1, (LONG64)nextNode, comp) == 1)
					break;
			}

			//if (currentTop->_OverFlowGuard != this) CRASH();
			//if (currentTop->_UnderFlowGuard != this) CRASH();
			//if (currentTop->_UseFlag != 0) CRASH();
			currentTop->_UseFlag++;
			ObjectType* pObj = &currentTop->_Object;

			if (this->_PlacementNew)
				new  (pObj) ObjectType();

			InterlockedIncrement((unsigned long*)&this->_UseCount);
			return pObj;
		}
		bool Free(ObjectType* pData)
		{
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
			//if (removeNode->_OverFlowGuard != this) CRASH();
			//if (removeNode->_UnderFlowGuard != this) CRASH();
			//if (removeNode->_UseFlag == 0) CRASH();
			//if (this->_UseCount < 0) CRASH();

			if (this->_PlacementNew)
				removeNode->_Object.~ObjectType();

			DataNode* currentTop = nullptr;
			ULONG64 popCount = 0;
			alignas(16) LONG64 comp[2];

			//InterlockedDecrement(&removeNode->_UseFlag);
			removeNode->_UseFlag--;
			while (true)
			{
				currentTop = this->_FreeNode;
				popCount = this->_CheckCount;

				comp[0] = (LONG64)currentTop;
				comp[1] = (LONG64)popCount;

				removeNode->_NextNode = currentTop;
				if (InterlockedCompareExchange128((LONG64*)&this->_FreeNode, comp[1] + 1, (LONG64)removeNode, (LONG64*)comp) == 1)
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
			DataNode* newNode = (DataNode*)malloc(sizeof(DataNode));
			if (newNode == nullptr)
			{
				CRASH();
				return nullptr;
			}

			newNode->_UnderFlowGuard = this;
			newNode->_OverFlowGuard = this;
			newNode->_UseFlag = 0;
			newNode->_NextNode = nullptr;
			InterlockedIncrement((LONG*)&this->_UseCount);
			this->_Capacity++;

			ObjectType* pObj = &newNode->_Object;
			newNode->_UseFlag++;

			if (!this->_PlacementNew)
				new (pObj) ObjectType();

			return pObj;
		}


		int						_Capacity = 0;
		bool					_PlacementNew = false;
		alignas(64) int			_UseCount = 0;
		alignas(64)DataNode*	_FreeNode;
		__int64					_CheckCount;
	};
}




#endif // !__LOCK_FREE_MEMORY_POOL__
