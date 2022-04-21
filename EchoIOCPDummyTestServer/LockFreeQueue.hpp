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

		LockFreeQueue() : _Size(0), _Head(nullptr), _Tail(nullptr) ,_Pool(new LockFreeMemoryPool<Node>),_HeadCheckCount(0),_TailCheckCount(0)
		{
			_Tail = _Head = this->_Pool->Alloc();
		}
		~LockFreeQueue()
		{
			for (int i = 0; i < _Size; i++)
			{
				T ret;
				if (!dequeue(ret))
					CRASH();
			}
			delete _Pool;
		}

		int size()
		{
			return _Size;
		}
		int GetUseCount()
		{
			return _Pool->GetUseCount();
		}
		int GetCapacityCount()
		{
			return _Pool->GetCapacityCount();
		}

		void enqueue(T val)
		{
			DWORD threadID = GetCurrentThreadId();

			Node* newNode = this->_Pool->Alloc();
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

			//InterlockedIncrement((unsigned long long*) & eq_dq_Counts);
		}

		bool dequeue(T& ret)
		{
			int curSize = InterlockedDecrement((unsigned long*)&this->_Size);
			if (curSize < 0)
			{
				CRASH();
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
			this->_Pool->Free(cmpHead._Node);
			//InterlockedIncrement((unsigned long long*) & eq_dq_Counts);
			return true;
		}

		//__int64 GetCountAndSetCountZero()
		//{
		//	return InterlockedExchange((unsigned long long*) & eq_dq_Counts, 0);
		//}


	private:
		alignas(64) Node			*_Head;
		__int64						_HeadCheckCount;

		alignas(64) Node			*_Tail;
		__int64						_TailCheckCount;
		
		LockFreeMemoryPool<Node>	*_Pool;
		alignas(64) int				_Size;



	private:
		//debugging
		//alignas(64) __int64 eq_dq_Counts;
	};
}



#endif // !__LOCK_FREE_QUEUE__





////#pragma once
////
////#include <Windows.h>
//////#include "CLFMemoryPool.h"
////#include "LockFreeMemoryPool.hpp"
////
////using namespace univ_dev;
////
////#define DEBUG_CNT   (10000)
//////#define DEBUGING
////
////template <typename T>
////class CLFQueue
////{
////private:
////    struct st_NODE
////    {
////        st_NODE* _pNext;
////        T           _tData;
////    };
////
////    struct st_CMP
////    {
////        st_NODE* _pNode;
////        UINT64      _uiID;
////    };
////
////    alignas(16)
////        st_NODE* m_pHead;
////    UINT64                      m_uiHeadID;
////
////    alignas(64)
////        st_NODE* m_pTail;
////    UINT64                      m_uiTailID;
////
////    alignas(64)
////        LockFreeMemoryPool<st_NODE>* m_Pool;
////    int                         m_iSize;
////
////#ifdef DEBUGING
////public:
////    enum class e_TYPE
////    {
////        ENQUEUE = 0,
////        DEQUEUE,
////        DEQUEUE_SIZE_CHECK
////    };
////
////    struct st_DEBUG
////    {
////        DWORD       dwThreadID;
////        UINT64      uiDebugCnt;
////        e_TYPE      eType;
////        st_NODE* pOrigin;
////        st_NODE* pOriginNext;
////        st_NODE* pCopy;
////        st_NODE* pCopyNext;
////        st_NODE* pChangeNode;
////        st_NODE* pChangeNodeNext;
////        int         iSize;
////    };
////
////    UINT64      m_uiDebugIndex;
////    UINT64      m_uiDebugCnt;
////    st_DEBUG* m_pDebug;
////#endif
////
////public:
////    CLFQueue()
////        : m_pHead()
////        , m_uiHeadID(0)
////        , m_pTail()
////        , m_uiTailID(0)
////        , m_Pool(new LockFreeMemoryPool<st_NODE>)
////        , m_iSize(0)
////    {
////        m_pHead = m_Pool->Alloc();
////        m_pHead->_pNext = nullptr;
////
////        m_pTail = m_pHead;
////
////#ifdef DEBUGING
////        m_uiDebugIndex = -1;
////        m_uiDebugCnt = 0;
////        m_pDebug = new st_DEBUG[DEBUG_CNT];
////#endif
////    }
////
////    ~CLFQueue()
////    {}
////
////    int size() const { return m_iSize; }
////
////    void enqueue(T tData)
////    {
////        st_CMP stCmp;
////        st_NODE* pNewNode = m_Pool->Alloc();
////        pNewNode->_tData = tData;
////        pNewNode->_pNext = nullptr;
////
////#ifdef DEBUGING
////        UINT64 uiIndex = InterlockedIncrement(&m_uiDebugIndex);
////        uiIndex %= DEBUG_CNT;
////
////        st_DEBUG* pDebug = &m_pDebug[uiIndex];
////        pDebug->uiDebugCnt = InterlockedIncrement(&m_uiDebugCnt);
////        pDebug->pChangeNode = pNewNode;
////        pDebug->pChangeNode = pNewNode->_pNext;
////#endif
////        while (1)
////        {
////            stCmp._pNode = m_pTail;
////            stCmp._uiID = m_uiTailID;
////
////            if (stCmp._pNode->_pNext != nullptr)
////            {
////                InterlockedCompareExchange128((LONG64*)&m_pTail, m_uiTailID + 1, (LONG64)stCmp._pNode->_pNext, (LONG64*)&stCmp);
////                //InterlockedCompareExchange128((LONG64*)&m_pTail, stCmp._uiID + 1, (LONG64)stCmp._pNode->_pNext, (LONG64*)&stCmp);
////
////                //InterlockedCompareExchangePointer((PVOID*)&m_pTail, stCmp._pNode->_pNext, stCmp._pNode);
////                continue;
////            }
////
////#ifdef DEBUGING
////            pDebug->dwThreadID = GetCurrentThreadId();
////            pDebug->eType = e_TYPE::ENQUEUE;
////            pDebug->pOrigin = m_pTail;
////            pDebug->pOriginNext = m_pTail->_pNext;
////            pDebug->pCopy = stCmp._pNode;
////            pDebug->pCopy = stCmp._pNode->_pNext;
////#endif
////            if (InterlockedCompareExchangePointer((PVOID*)&stCmp._pNode->_pNext, pNewNode, nullptr) == nullptr)
////            {
////                InterlockedCompareExchange128((LONG64*)&m_pTail, m_uiTailID + 1, (LONG64)pNewNode, (LONG64*)&stCmp);
////                //InterlockedCompareExchange128((LONG64*)&m_pTail, stCmp._uiID + 1, (LONG64)pNewNode, (LONG64*)&stCmp);
////
////                //InterlockedCompareExchangePointer((PVOID*)&m_pTail, pNewNode, stCmp._pNode);
////                break;
////            }
////        }
////
////        int iSize = InterlockedIncrement((long*)&m_iSize);
////
////#ifdef DEBUGING
////        pDebug->iSize = iSize;
////#endif
////    }
////
////    bool dequeue(volatile T& pOut)
////    {
////#ifdef DEBUGING
////        UINT64 uiIndex = InterlockedIncrement(&m_uiDebugIndex);
////        uiIndex %= DEBUG_CNT;
////
////        st_DEBUG* pDebug = &m_pDebug[uiIndex];
////        pDebug->uiDebugCnt = InterlockedIncrement(&m_uiDebugCnt);
////#endif
////
////        // size check
////        int iSize = InterlockedDecrement((long*)&m_iSize);
////
////        if (iSize < 0)
////        {
////            InterlockedIncrement((long*)&m_iSize);
////#ifdef DEBUGING
////            pDebug->dwThreadID = GetCurrentThreadId();
////            pDebug->eType = e_TYPE::DEQUEUE_SIZE_CHECK;
////            pDebug->pOrigin = nullptr;
////            pDebug->pOriginNext = nullptr;
////            pDebug->pCopy = nullptr;
////            pDebug->pCopyNext = nullptr;
////            pDebug->iSize = iSize;
////#endif
////            return false;
////        }
////
////        st_CMP stCmpTail;
////        st_CMP stCmpHead;
////        st_NODE* pHeadNext;
////
////        T tData;
////
////        while (1)
////        {
////            stCmpTail._pNode = m_pTail;
////            stCmpTail._uiID = m_uiTailID;
////
////            stCmpHead._pNode = m_pHead;
////            stCmpHead._uiID = m_uiHeadID;
////
////            pHeadNext = stCmpHead._pNode->_pNext;
////
////            // 지금 tail의 next가 null이 아니다 -> 현재 다른 쓰레드에서 Enqueue하면서 1st CAS만 성공했음..
////            // 그럴 경우 아무 데이터도 없었을 때(2nd CAS실패하면서 size로 체크해서 나타 날 수 있는 문제) 
////            // Enqueue 1st CAS만 성공했으면 head->next 는 null이 아닐것임..
////            // 이 상태에서 Dequeue가 완료되고 1st CAS 진행한 쓰레드에서 2nd CAS를 하려고 하면 tail은 이미 메모리 풀로 free 된 놈임
////            // 그 상태에서 다른 쓰레드에서 Enqueue할 때 free된 노드(현재 tail이 가르키고 있는)가 재할당되고(alloc) 노드의 next를 null로 밀면 그 뒤에 달려있던 노드는 분실되고 이어서 1st CAS하면 tail->next = tail이 되는 기이한 현상이 일어남
////
////            // tail 한번 뒤로 옮겨주고 진행....
////            if (stCmpTail._pNode->_pNext != nullptr)
////            {
////                InterlockedCompareExchange128((LONG64*)&m_pTail, (LONG64)m_uiTailID + 1, (LONG64)stCmpTail._pNode->_pNext, (LONG64*)&stCmpTail);
////                //InterlockedCompareExchange128((LONG64*)&m_pTail, (LONG64)stCmpTail._uiID + 1, (LONG64)stCmpTail._pNode->_pNext, (LONG64*)&stCmpTail);
////
////                //InterlockedCompareExchange128((LONG64*)&m_pTail, (LONG64)m_uiTailID + 1, (LONG64)pNext, (LONG64*)&stCmpTail);
////            }
////
////            // next가 null이 아닐때 진행
////            if (pHeadNext == nullptr)
////                continue;
////
////            tData = pHeadNext->_tData;
////
////#ifdef DEBUGING
////            pDebug->dwThreadID = GetCurrentThreadId();
////            pDebug->eType = e_TYPE::DEQUEUE;
////            pDebug->pOrigin = m_pHead;
////            pDebug->pOriginNext = m_pHead->_pNext;
////            pDebug->pCopy = stCmp._pNode;
////            pDebug->pCopyNext = stCmp._pNode->_pNext;
////#endif
////
////            if (InterlockedCompareExchange128((LONG64*)&m_pHead, (LONG64)m_uiHeadID + 1, (LONG64)m_pHead->_pNext, (LONG64*)&stCmpHead))
////                //if (InterlockedCompareExchange128((LONG64*)&m_pHead, (LONG64)m_uiHeadID + 1, (LONG64)pNext, (LONG64*)&stCmp) == TRUE)
////            {
////#ifdef DEBUGING
////                pDebug->iSize = iSize;
////                pDebug->pChangeNode = pDebug->pOrigin->_pNext;
////                pDebug->pChangeNodeNext = pDebug->pChangeNode->_pNext;
////#endif
////                pOut = tData;
////                m_Pool->Free(stCmpHead._pNode);
////                break;
////            }
////        }
////
////        return true;
////    }
////};