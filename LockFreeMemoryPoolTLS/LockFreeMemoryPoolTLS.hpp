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
		class Chunk;

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
			bool IsEmpty() const
			{
				return _AllocCount >= MAX_CHUNK_SIZE;
			}
			ChunkNode _ChunkArr[MAX_CHUNK_SIZE];
			DWORD _ChunkSize;
			DWORD _AllocCount;
			DWORD _FreeCount;
		};

	public:
		LockFreeMemoryPoolTLS(bool placementNew = false) : _TLS_PoolIdx(TlsAlloc()), _PlacementNew(placementNew), _Pool(new LockFreeMemoryPool<Chunk>(placementNew)) {};
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
			if (chunk->IsEmpty())
				TlsSetValue(_TLS_PoolIdx, ChunkAlloc());

			if (_PlacementNew)
				new (ret) ObjectType();

			//InterlockedIncrement((unsigned long long*)&tps);

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
		}

		int GetUseCount() { return _Pool->GetUseCount(); }
		int GetCapacityCount() { return _Pool->GetCapacityCount(); }
		//__int64 GetTps() { return InterlockedExchange((unsigned long long*) & tps, 0); }


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
		DWORD _TLS_PoolIdx;
		bool _PlacementNew;
		LockFreeMemoryPool<Chunk>* _Pool;
		
	public:
		//__int64 tps;


	};
}




#endif // !__LOCK_FREE_MEMORY_POOL__
