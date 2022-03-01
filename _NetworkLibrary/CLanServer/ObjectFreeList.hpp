#pragma once
#ifndef __OBJECT_FREE_LIST__
#define __OBJECT_FREE_LIST__
#define __UNIV_DEVELOPER_
#include <new>
#include <synchapi.h>
#include <stdio.h>


namespace univ_dev
{
#define CRASH() do{int*ptr =nullptr; *ptr =100;}while(0)
	template <typename ObjectType>
	class ObjectFreeList
	{
	private:
		//외부에서 접근불가능한 노드
		template<typename ObjectType>
		struct DataNode
		{
			ObjectFreeList<ObjectType>* underFlowGuard = nullptr;
			ObjectType object;
			ObjectFreeList<ObjectType>* overFlowGuard = nullptr;
			DataNode<ObjectType>* next = nullptr;
			bool useFlag = false;
		};
	public:
		//생성자 그냥 alloc을 n번 호출한것과 동일한효과 그 이후에 추가되는것도 free list처럼
		ObjectFreeList(int blockNum = 0, bool placementNew = false) : _Capacity(blockNum), _UseCount(0), _PlacementNewFlag(placementNew), _pFreeNode(nullptr)
		{
			InitializeCriticalSection(&_ObjectPoolLock);
			//처음부터 free list라면 아무것도 해줄필요없음.
			if (_Capacity == 0) return;
			//그게 아니라면 n번 alloc호출과 동일하게 만들어줘야됨 단 capacity만 증가되어야됨.
			DataNode<ObjectType>* temp;
			//배열이지만 리스트처럼 다음노드 연결
			_pFreeNode = (DataNode<ObjectType>*)malloc(sizeof(DataNode<ObjectType>));
			if (_pFreeNode == nullptr) { CRASH(); return; }
			_pFreeNode->underFlowGuard = this;
			_pFreeNode->next = nullptr;
			_pFreeNode->overFlowGuard = this;
			_pFreeNode->useFlag = false;
			for (int i = 0; i < blockNum - 1; i++)
			{
				temp = (DataNode<ObjectType>*)malloc(sizeof(DataNode<ObjectType>));
				if (temp == nullptr) { CRASH(); return; }
				temp->next = _pFreeNode;
				temp->underFlowGuard = this;
				temp->overFlowGuard = this;
				temp->useFlag = false;
				_pFreeNode = temp;
				temp++;
			}
		}
		virtual ~ObjectFreeList()
		{
			DataNode<ObjectType>* temp = _pFreeNode;
			while (temp != nullptr)
			{
				DataNode<ObjectType>* next = temp->next;
				free(temp);
				temp = next;
			}
			_Capacity = 0;
			_UseCount = 0;
			DeleteCriticalSection(&_ObjectPoolLock);
		};

		ObjectType* Alloc()
		{
			EnterCriticalSection(&_ObjectPoolLock);
			if (_pFreeNode != nullptr)
			{
				_pFreeNode->overFlowGuard = this;
				_pFreeNode->underFlowGuard = this;
				_pFreeNode->useFlag = true;
				ObjectType* pObj = &_pFreeNode->object;
				_pFreeNode = _pFreeNode->next;
				_UseCount++;
				if (_PlacementNewFlag)
				{
					new(pObj) ObjectType;
				}
				LeaveCriticalSection(&_ObjectPoolLock);
				return pObj;
			}
			DataNode<ObjectType>* newNode = new DataNode<ObjectType>();
			newNode->overFlowGuard = this;
			newNode->underFlowGuard = this;
			newNode->useFlag = true;
			ObjectType* pObj = &newNode->object;
			_UseCount++;
			_Capacity++;
			LeaveCriticalSection(&_ObjectPoolLock);
			return pObj;
		}
		bool Free(ObjectType* pData)
		{
			EnterCriticalSection(&_ObjectPoolLock);
			char* temp = (char*)pData;
			temp -= sizeof(ObjectFreeList<ObjectType>*);
			DataNode<ObjectType>* tempNode = (DataNode<ObjectType>*)temp;
			if (tempNode->underFlowGuard == nullptr)
			{
				temp -= sizeof(ObjectFreeList<ObjectType>*);
				tempNode = (DataNode<ObjectType>*)temp;
			}
			if (tempNode->overFlowGuard != this) CRASH();
			if (tempNode->underFlowGuard != this) CRASH();
			if (!tempNode->useFlag) CRASH();
			if (_UseCount <= 0) CRASH();
			_UseCount--;

			tempNode->next = _pFreeNode;
			_pFreeNode = tempNode;
			_pFreeNode->useFlag = false;
			LeaveCriticalSection(&_ObjectPoolLock);
			return true;
		}

		int GetCapacityCount() { return _Capacity; }
		int GetUseCount() { return _UseCount; }

	private:
		int _Capacity = 0;
		int _UseCount = 0;
		bool _PlacementNewFlag;
		DataNode<ObjectType>* _pFreeNode;
		CRITICAL_SECTION _ObjectPoolLock;
	};
}



#endif // !__OBJECT_FREE_LIST__
