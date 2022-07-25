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
				ObjectType* ret = (ObjectType*)&this->_ChunkArr[this->_AllocCount]._Value;
				this->_ChunkArr[this->_AllocCount]._Chunk = this;
				this->_AllocCount++;
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
		LockFreeMemoryPoolTLS(bool placementNew = false) : _TLS_PoolIdx(TlsAlloc()), _PlacementNew(placementNew), _TotalUseCount(0)/*, _Pool(new LockFreeMemoryPool<Chunk>(placementNew))*/ {};
		~LockFreeMemoryPoolTLS()
		{
			TlsFree(this->_TLS_PoolIdx);
			//delete _Pool;
		}

		ObjectType* Alloc()
		{
			Chunk* chunk = (Chunk*)TlsGetValue(_TLS_PoolIdx);
			if (chunk == nullptr)
			{
				chunk = this->ChunkAlloc();
				TlsSetValue(this->_TLS_PoolIdx, chunk);
			}

			ObjectType* ret = chunk->Alloc();
			if (chunk->_AllocCount >= this->MAX_CHUNK_SIZE)
				TlsSetValue(this->_TLS_PoolIdx, this->ChunkAlloc());

			if (this->_PlacementNew)
				new (ret) ObjectType();
			InterlockedIncrement((unsigned long long*) & this->_TotalUseCount);
			return ret;
		}

		void Free(ObjectType* pObj)
		{
			ChunkNode* node = (ChunkNode*)pObj;
			if (this->_PlacementNew)
				pObj->~ObjectType();

			Chunk* chunk = node->_Chunk;

			if (InterlockedIncrement(&chunk->_FreeCount) == this->MAX_CHUNK_SIZE)
				this->ChunkFree(chunk);
			InterlockedDecrement((unsigned long long*) & this->_TotalUseCount);
		}

		int GetUseCount() { return this->_Pool.GetUseCount(); }
		int GetCapacityCount() { return this->_Pool.GetCapacityCount(); }
		int GetTotalUseCount() { return this->_TotalUseCount; }

	private:
		Chunk* ChunkAlloc()
		{
			Chunk* newChunk = this->_Pool.Alloc();
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
			this->_Pool.Free(chunk);
		}

	private:
		DWORD						_TLS_PoolIdx;
		bool						_PlacementNew;
		LockFreeMemoryPool<Chunk>	_Pool;
		alignas(64) __int64			_TotalUseCount;
	};
}




#endif // !__LOCK_FREE_MEMORY_POOL__
