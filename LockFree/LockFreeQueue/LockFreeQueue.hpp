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

			dbg_TestSample = new Test[TEST_CASE_SIZE + 200];
			dbg_Count = 0;
			dbg_TestIdx = 0;
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
		struct Test
		{
			DWORD _ThreadID;

			Node* eq_CurrentTail;
			Node* eq_CurrentTailNext;

			Node* dq_CurrentHead;
			Node* dq_CurrentHeadNext;

			Node* _Head;
			Node* _Tail;

			unsigned long long _DebugCount;
			int _CurrentQueueSize;
		};

		int size()
		{
			return _Size;
		}
		int pool_size()
		{
			return _Pool->GetUseCount();
		}
		int pool_capacity()
		{
			return _Pool->GetCapacityCount();
		}

		void enqueue(T val)
		{
			int curDBGCount = InterlockedIncrement(&dbg_Count);
			if (this->dbg_TestIdx >= TEST_CASE_SIZE - 1)
				InterlockedExchange((unsigned long*)&this->dbg_TestIdx, 0);
			int curIdx = InterlockedIncrement((unsigned long*)&this->dbg_TestIdx);

			DWORD threadID = GetCurrentThreadId();

			ZeroMemory(&dbg_TestSample[curIdx], sizeof(Test));

			this->dbg_TestSample[curIdx]._ThreadID = threadID;
			this->dbg_TestSample[curIdx]._DebugCount = curDBGCount;


			Node* newNode = this->_Pool->Alloc();
			newNode->_Value = val;
			//newNode->_Next = (Node*)curDBGCount;
			newNode->_Next = nullptr;

			while (true)
			{
				cmpNode cmp;
				cmp._Node = this->_Tail;
				cmp._Count = this->_TailCheckCount;

				//Node* currentTailNext = cmp._Node->_Next;
				//Node* currentTail = this->_Tail;

				this->dbg_TestSample[curIdx].eq_CurrentTail = cmp._Node;
				this->dbg_TestSample[curIdx].eq_CurrentTailNext = cmp._Node->_Next;
				
				//LONG64 comp[2];
				//comp[0] = (LONG64)currentTail;
				//comp[1] = this->_TailCheckCount;

				//currentTailNext && currentTail->_Next
				if (cmp._Node->_Next == nullptr)
				{
					if (InterlockedCompareExchangePointer((PVOID*)&this->_Tail->_Next, newNode, nullptr) == nullptr)
					//if (InterlockedCompareExchangePointer((PVOID*)&currentTail->_Next, newNode, nullptr) == nullptr)
					{
						//InterlockedCompareExchange128((LONG64*)&this->_Tail, comp[1] + 1, (LONG64)newNode, comp);
						//InterlockedCompareExchange128((LONG64*)&this->_Tail, _TailCheckCount + 1, (LONG64)newNode, (LONG64*)&cmp);
						InterlockedCompareExchange128((LONG64*)&this->_Tail, cmp._Count + 1, (LONG64)newNode, (LONG64*)&cmp);
						break;
					}
				}
				else
				{
					//InterlockedCompareExchange128((LONG64*)&this->_Tail, _TailCheckCount + 1, (LONG64)cmp._Node->_Next, (LONG64*)&cmp);
					InterlockedCompareExchange128((LONG64*)&this->_Tail, cmp._Count + 1, (LONG64)cmp._Node->_Next, (LONG64*)&cmp);
					//InterlockedCompareExchange128((LONG64*)&this->_Tail, comp[1] + 1, (LONG64)currentTail->_Next, (LONG64*)cmp);
					//InterlockedCompareExchange128((LONG64*)&this->_Tail, _TailCheckCount + 1, (LONG64)currentTailNext, (LONG64*)comp);
					//InterlockedCompareExchange128((LONG64*)&this->_Tail, comp[1] + 1, (LONG64)currentTailNext, (LONG64*)cmp);
				}
			}
			int curSize = InterlockedIncrement((unsigned long*)&this->_Size);
			this->dbg_TestSample[curIdx]._CurrentQueueSize = curSize;

			InterlockedIncrement((unsigned long long*) & eq_dq_Counts);
		}

		bool dequeue(T& ret)
		{
			int curDBGCount = InterlockedIncrement(&this->dbg_Count);
			if (this->dbg_TestIdx >= TEST_CASE_SIZE - 1)
				InterlockedExchange((unsigned long*)&this->dbg_TestIdx, 0);
			int curIdx = InterlockedIncrement((unsigned long*)&this->dbg_TestIdx);

			DWORD threadID = GetCurrentThreadId();

			ZeroMemory(&dbg_TestSample[curIdx], sizeof(Test));

			this->dbg_TestSample[curIdx]._ThreadID = threadID;
			this->dbg_TestSample[curIdx]._DebugCount = curDBGCount;


			int curSize = InterlockedDecrement((unsigned long*)&this->_Size);
			this->dbg_TestSample[curIdx]._CurrentQueueSize = curSize + 1;
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


				this->dbg_TestSample[curIdx].dq_CurrentHead = cmpHead._Node;
				this->dbg_TestSample[curIdx].dq_CurrentHeadNext = headNext;

				//currentHead = this->_Head;
				//nextNode = currentHead->_Next;
				//LONG64 comp[2];
				//comp[0] = (LONG64)currentHead;
				//comp[1] = _HeadCheckCount;

				cmpNode cmpTail;
				cmpTail._Node = this->_Tail;
				cmpTail._Count = this->_TailCheckCount;

				//Node* currentTail = this->_Tail;
				//Node* currentTailNext = currentTail->_Next;
				//LONG64 tailComp[2];
				//tailComp[0] = (LONG64)currentTail;
				//tailComp[1] = _TailCheckCount;

				//if (currentTail->_Next!= nullptr)
				//currentTailNext
				if (cmpTail._Node->_Next != nullptr)
				{
					//InterlockedCompareExchange128((LONG64*)&this->_Tail, this->_TailCheckCount + 1, (LONG64)cmpTail._Node->_Next, (LONG64*)&cmpTail);
					InterlockedCompareExchange128((LONG64*)&this->_Tail, cmpTail._Count + 1, (LONG64)cmpTail._Node->_Next, (LONG64*)&cmpTail);
					//InterlockedCompareExchange128((LONG64*)&this->_Tail, this->_TailCheckCount + 1, (LONG64)currentTail->_Next, tailComp);
					//InterlockedCompareExchange128((LONG64*)&this->_Tail, this->_TailCheckCount + 1, (LONG64)currentTailNext, tailComp);
					//InterlockedCompareExchange128((LONG64*)&this->_Tail, tailComp[1] + 1, (LONG64)currentTail->_Next, tailComp);
					//InterlockedCompareExchange128((LONG64*)&this->_Tail, tailComp[1] + 1, (LONG64)currentTailNext, tailComp);
					continue;
				}

				if (headNext == nullptr)
				{
					//CRASH();
					//����1 ���⼭ ��� �����尡 �������Ͷ� ���ѷ����� ������.
					continue;
				}
				retData = headNext->_Value;

				if (InterlockedCompareExchange128((LONG64*)&this->_Head, cmpHead._Count + 1, (LONG64)this->_Head->_Next, (LONG64*)&cmpHead))
				//if (InterlockedCompareExchange128((LONG64*)&this->_Head, this->_HeadCheckCount + 1, (LONG64)currentHead->_Next,comp))
				//if (InterlockedCompareExchange128((LONG64*)&this->_Head, this->_HeadCheckCount + 1, (LONG64)nextNode,comp))
				//if (InterlockedCompareExchange128((LONG64*)&this->_Head, comp[1] + 1, (LONG64)this->_Head->_Next, comp))
				//if (InterlockedCompareExchange128((LONG64*)&this->_Head, comp[1] + 1, (LONG64)currentHead->_Next,comp))
				//if (InterlockedCompareExchange128((LONG64*)&this->_Head, comp[1] + 1, (LONG64)nextNode, comp))
					break;
			}
			//����2 �Ȱ��� currentHead�� ���� free�ι�
			//ret = nextNode->_Value;
			//ret = currentHead->_Next->_Value;
			ret = retData;
			this->_Pool->Free(cmpHead._Node);

			InterlockedIncrement((unsigned long long*) & eq_dq_Counts);

			return true;
		}

		__int64 GetCountAndSetCountZero()
		{
			return InterlockedExchange((unsigned long long*) & eq_dq_Counts, 0);
		}


	private:
		Node						*_Head;
		__int64						_HeadCheckCount;

		alignas(64) Node			*_Tail;
		__int64						_TailCheckCount;
		
		LockFreeMemoryPool<Node>	*_Pool;
		alignas(64) int				_Size;



	private:
		//debugging
		Test* dbg_TestSample;
		alignas(64) unsigned long long dbg_Count;
		alignas(64) int dbg_TestIdx;

		alignas(64) __int64 eq_dq_Counts;

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
////            // ���� tail�� next�� null�� �ƴϴ� -> ���� �ٸ� �����忡�� Enqueue�ϸ鼭 1st CAS�� ��������..
////            // �׷� ��� �ƹ� �����͵� ������ ��(2nd CAS�����ϸ鼭 size�� üũ�ؼ� ��Ÿ �� �� �ִ� ����) 
////            // Enqueue 1st CAS�� ���������� head->next �� null�� �ƴҰ���..
////            // �� ���¿��� Dequeue�� �Ϸ�ǰ� 1st CAS ������ �����忡�� 2nd CAS�� �Ϸ��� �ϸ� tail�� �̹� �޸� Ǯ�� free �� ����
////            // �� ���¿��� �ٸ� �����忡�� Enqueue�� �� free�� ���(���� tail�� ����Ű�� �ִ�)�� ���Ҵ�ǰ�(alloc) ����� next�� null�� �и� �� �ڿ� �޷��ִ� ���� �нǵǰ� �̾ 1st CAS�ϸ� tail->next = tail�� �Ǵ� ������ ������ �Ͼ
////
////            // tail �ѹ� �ڷ� �Ű��ְ� ����....
////            if (stCmpTail._pNode->_pNext != nullptr)
////            {
////                InterlockedCompareExchange128((LONG64*)&m_pTail, (LONG64)m_uiTailID + 1, (LONG64)stCmpTail._pNode->_pNext, (LONG64*)&stCmpTail);
////                //InterlockedCompareExchange128((LONG64*)&m_pTail, (LONG64)stCmpTail._uiID + 1, (LONG64)stCmpTail._pNode->_pNext, (LONG64*)&stCmpTail);
////
////                //InterlockedCompareExchange128((LONG64*)&m_pTail, (LONG64)m_uiTailID + 1, (LONG64)pNext, (LONG64*)&stCmpTail);
////            }
////
////            // next�� null�� �ƴҶ� ����
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