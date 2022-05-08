#pragma once
#ifndef __MEMORY_POOL__
#define __MEMORY_POOL__

#include <cstdlib>
#include <new>
#include <stdio.h>
namespace univ_dev
{
#define CRASH() do{int*ptr =nullptr; *ptr =100;}while(0)
	template <typename ObjectType>
	class ObjectFreeList
	{
	private:
		//�ܺο��� ���ٺҰ����� ���
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
		//������ �׳� alloc�� n�� ȣ���ѰͰ� ������ȿ�� �� ���Ŀ� �߰��Ǵ°͵� free listó��
		ObjectFreeList(int blockNum = 0, bool placementNew = false) : capacity(blockNum), useCount(0), placementNew(placementNew), pFreeNode(nullptr)
		{
			//ó������ free list��� �ƹ��͵� �����ʿ����.
			if (capacity == 0) return;
			//�װ� �ƴ϶�� n�� allocȣ��� �����ϰ� �������ߵ� �� capacity�� �����Ǿ�ߵ�.
			DataNode<ObjectType>* temp;
			//�迭������ ����Ʈó�� ������� ����
			pFreeNode = (DataNode<ObjectType>*)malloc(sizeof(DataNode<ObjectType>));
			if (pFreeNode == nullptr) { CRASH(); return; }
			pFreeNode->underFlowGuard = this;
			pFreeNode->next = nullptr;
			pFreeNode->overFlowGuard = this;
			pFreeNode->useFlag = false;
			for (int i = 0; i < blockNum - 1; i++)
			{
				temp = (DataNode<ObjectType>*)malloc(sizeof(DataNode<ObjectType>));
				if (temp == nullptr) { CRASH(); return; }
				temp->next = pFreeNode;
				temp->underFlowGuard = this;
				temp->overFlowGuard = this;
				temp->useFlag = false;
				pFreeNode = temp;
				temp++;
			}
		}
		virtual ~ObjectFreeList()
		{
			DataNode<ObjectType>* temp = pFreeNode;
			while (temp != nullptr)
			{
				DataNode<ObjectType>* next = temp->next;
				free(temp);
				temp = next;
			}
			capacity = 0;
			useCount = 0;
			//printf("capacity : %d", capacity);
		};

		ObjectType* Alloc()
		{
			if (pFreeNode != nullptr)
			{
				pFreeNode->useFlag = true;
				ObjectType* pObj = &pFreeNode->object;
				pFreeNode = pFreeNode->next;
				useCount++;
				if (placementNew)
				{
					new(pObj) ObjectType;
				}
				return pObj;
			}
			DataNode<ObjectType>* newNode = new DataNode<ObjectType>();
			newNode->overFlowGuard = this;
			newNode->underFlowGuard = this;
			newNode->useFlag = true;
			ObjectType* pObj = &newNode->object;
			useCount++;
			capacity++;
			return pObj;
		}
		bool Free(ObjectType* pData)
		{
			char* temp = (char*)pData;
			temp -= sizeof(ObjectFreeList<ObjectType>*);
			DataNode<ObjectType>* tempNode = (DataNode<ObjectType>*)temp;
			if (tempNode->overFlowGuard != this) CRASH();
			if (tempNode->underFlowGuard != this) CRASH();
			if (!tempNode->useFlag) CRASH();
			if (useCount < 0) CRASH();
			useCount--;

			tempNode->next = pFreeNode;
			pFreeNode = tempNode;
			pFreeNode->useFlag = false;
			return true;
		}

		int GetCapacityCount() { return capacity; }
		int GetUseCount() { return useCount; }

	private:
		int capacity = 0;
		int useCount = 0;
		bool placementNew;
		DataNode<ObjectType>* pFreeNode;
	};
}



#endif // !__MEMORY_POOL__
