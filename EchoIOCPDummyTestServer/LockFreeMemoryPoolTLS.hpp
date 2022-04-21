#pragma once
#ifndef __LOCK_FREE_MEMORY_POOL_TLS__
#define __LOCK_FREE_MEMORY_POOL_TLS__
#define __UNIV_DEVELOPER_

#include <cstdlib>
#include <new>
#include <Windows.h>
#include "LockFreeMemoryPool.hpp"


namespace univ_dev
{
	template<typename ObjectType>
	class LockFreeMemoryPoolTLS
	{
	private:
		constexpr static int MAX_CHUNK_SIZE = 1000;
		struct Chunk;

		struct ChunkNode
		{
			ObjectType _Value;
			Chunk* _Chunk;
		};
		struct Chunk
		{
			ObjectType* Alloc()
			{
				ObjectType* ret = (ObjectType*)&_ChunkArr[_AllocCount]._Value;
				_ChunkArr[_AllocCount]._Chunk = this;
				_AllocCount++;
				return ret;
			}
			void Clear()
			{
				this->_ChunkSize = MAX_CHUNK_SIZE;
				this->_FreeCount = 0;
				this->_AllocCount = 0;
			}

			DWORD _FreeCount;
			DWORD _AllocCount;
			DWORD _ChunkSize;
			ChunkNode _ChunkArr[MAX_CHUNK_SIZE];
		};

	public:
		LockFreeMemoryPoolTLS(bool placementNew = false) : _TLS_PoolIdx(TlsAlloc()), _PlacementNew(placementNew),_TotalUseCount(0), _Pool(new LockFreeMemoryPool<Chunk>(placementNew)) {};
		~LockFreeMemoryPoolTLS()
		{
			TlsFree(_TLS_PoolIdx);
			delete _Pool;
		}

		ObjectType* Alloc()
		{
			Chunk* chunk = (Chunk*)TlsGetValue(_TLS_PoolIdx);
			if (chunk == nullptr)
			{
				chunk = ChunkAlloc();
				TlsSetValue(_TLS_PoolIdx, chunk);
			}

			ObjectType* ret = chunk->Alloc();
			if (chunk->_AllocCount >= MAX_CHUNK_SIZE)
				TlsSetValue(_TLS_PoolIdx, ChunkAlloc());

			if (_PlacementNew)
				new (ret) ObjectType();

			//InterlockedIncrement((unsigned long long*)&tps);
			InterlockedIncrement((unsigned long long*)&_TotalUseCount);

			return ret;
		}

		void Free(ObjectType* pObj)
		{
			ChunkNode* node = (ChunkNode*)pObj;
			if (_PlacementNew)
				pObj->~ObjectType();

			Chunk* chunk = node->_Chunk;

			if (InterlockedIncrement(&chunk->_FreeCount) == MAX_CHUNK_SIZE)
				ChunkFree(chunk);
			//InterlockedIncrement((unsigned long long*)&tps);
			InterlockedDecrement((unsigned long long*)&_TotalUseCount);
		}

		int GetUseCount() { return _Pool->GetUseCount(); }
		int GetCapacityCount() { return _Pool->GetCapacityCount(); }
		//int GetTps() { return InterlockedExchange((unsigned long long*) & tps, 0); }
		int GetTotalUseCount() { return _TotalUseCount; }

	private:
		Chunk* ChunkAlloc()
		{
			Chunk* newChunk = _Pool->Alloc();
			if (newChunk == nullptr)
			{
				CRASH();
				return nullptr;
			}
			newChunk->Clear();

			return newChunk;
		}
		void ChunkFree(Chunk* chunk)
		{
			chunk->Clear();
			_Pool->Free(chunk);
		}

	private:
		LockFreeMemoryPool<Chunk>* _Pool;
		DWORD _TLS_PoolIdx;
		bool _PlacementNew;

	public:
		//__int64 tps;
		alignas(64) __int64 _TotalUseCount;
	};
}




#endif // !__LOCK_FREE_MEMORY_POOL__
