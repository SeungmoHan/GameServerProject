#pragma once
#ifndef __MEMORY_POOL__
#define __MEMORY_POOL__

#define CRASH() do{int*ptr =nullptr; *ptr =100;}while(0)

template <typename ObjectType>
class ObjectFreeList
{
private:
	//외부에서 접근불가능한 노드
	template<typename ObjectType>
	struct DataNode
	{
		ObjectType object;
		DataNode<ObjectType>* next;
		ObjectFreeList<ObjectType>* parent;
		bool useFlag;
	};
public:
	//생성자 프리리스트처럼 사용할수도있고 베이스를 오브젝트풀로두고 그 이후에 resizing해야될 경우 free list처럼 구현
	ObjectFreeList(int blockNum = 0, bool placementNew = false) : capacity(blockNum), useCount(0), placementNew(placementNew), pFreeNode(nullptr), isMemoryPool(false), poolSize(blockNum)
	{
		//처음부터 free list라면 아무것도 해줄필요없음.
		if (capacity == 0) return;

		//그게 아니라면 메모리풀 베이스
		isMemoryPool = true;
		pFreeNode = new DataNode<ObjectType>[blockNum];
		DataNode<ObjectType>* temp = pFreeNode;
		//배열이지만 리스트처럼 다음노드 연결
		for (int i = 0; i < blockNum - 1; i++)
		{
			temp->next = temp;
			temp->parent = this;
			temp->useFlag = false;
			temp++;
		}
		//마지막노드만 따로 널포인터 처리
		temp->next = nullptr;
		temp->parent = this;
		temp->useFlag = false;
	}
	virtual ~ObjectFreeList() 
	{
		//DataNode<ObjectType>* tempNode = pFreeNode;
		//if (isMemoryPool)
		//{
		//	tempNode += poolSize - 1;
		//	delete pFreeNode;
		//}
		//while (pFreeNode != nullptr)
		//{
		//	delete pFreeNode;
		//}
	};

	ObjectType* Alloc()
	{
		if (pFreeNode != nullptr)
		{
			pFreeNode->useFlag = true;
			ObjectType* pObj = &pFreeNode->object;
			pFreeNode = pFreeNode->next;
			useCount++;
			return pObj;
		}
		DataNode<ObjectType>* newNode = new DataNode<ObjectType>();
		newNode->parent = this;
		newNode->useFlag = true;
		ObjectType* pObj = &newNode->object;
		useCount++;
		capacity++;
		return pObj;
	}
	bool Free(ObjectType* pData)
	{
		DataNode<ObjectType>* temp = (DataNode<ObjectType>*)pData;
		if (temp->parent != this) CRASH();
		if (!temp->useFlag) CRASH();
		if (useCount < 0) CRASH();
		if (pFreeNode == nullptr)
		{
			pFreeNode = temp;
			pFreeNode->useFlag = false;
			useCount--;
			return true;
		}

		temp->next = pFreeNode;
		pFreeNode = temp;
		pFreeNode->useFlag = false;
		useCount--;
		return true;
	}

	int GetCapacityCount() { return capacity; }
	int GetUseCount() { return useCount; }

private:

	bool isMemoryPool;
	int poolSize;
	int capacity = 0;
	int useCount = 0;
	bool placementNew;
	DataNode<ObjectType>* pFreeNode;
};


#endif // !__MEMORY_POOL__
