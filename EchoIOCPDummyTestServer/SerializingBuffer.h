#pragma once
#ifndef __SERIALIZEING_BUFFER_HEADER__
#define __SERIALIZEING_BUFFER_HEADER__
#include "BaseCore.h"

namespace univ_dev
{
	class Packet
	{
	private:
		static constexpr int MSS = 1460;
	public:
		enum PacketSize
		{
			DefaultSize = MSS
		};
		Packet();
		Packet(int bufferSize);

		virtual ~Packet();
		void Release();

		void Clear();
		int GetBufferSize();
		char* GetReadPtr();
		char* GetWritePtr();

		int MoveWritePtr(int size);
		int MoveReadPtr(int size);

		//template<typename T>
		//Packet& operator<<(T value)
		//{
		//	if (writePointer + (sizeof(value) - 1) >= end)
		//	{
		//		throw std::exception("need more memory");
		//	}
		//	T* tempPtr = (T*)writePointer;
		//	*tempPtr = value;
		//	MoveWritePtr(sizeof(value));
		//	return *this;
		//}
		//template<typename T>
		//Packet& operator>>(T& value)
		//{
		//	if (readPointer + sizeof(value) - 1 >= end)
		//	{
		//		throw std::exception("cant read");
		//	}
		//	T* tempPtr = (T*)readPointer;
		//	value = *tempPtr;
		//	MoveReadPtr(sizeof(value));
		//	return *this;
		//}

		Packet& operator=(const Packet& other);
		Packet& operator>>(unsigned char& value);
		Packet& operator>>(char& value);

		Packet& operator>>(unsigned short& value);
		Packet& operator>>(short& value);

		Packet& operator>>(unsigned int& value);
		Packet& operator>>(int& value);
		Packet& operator>>(long& value);
		Packet& operator>>(unsigned long& value);
		Packet& operator>>(float& value);
		Packet& operator>>(double& value);
		Packet& operator>>(__int64& value);
		Packet& operator>>(unsigned __int64& value);

		Packet& operator<<(char value);
		Packet& operator<<(unsigned char value);
		Packet& operator<<(unsigned short value);
		Packet& operator<<(short value);
		Packet& operator<<(unsigned int value);

		Packet& operator<<(int value);
		Packet& operator<<(unsigned long value);
		Packet& operator<<(long value);
		Packet& operator<<(float value);
		Packet& operator<<(__int64 value);
		Packet& operator<<(unsigned __int64 value);
		Packet& operator<<(double value);
	protected:
		char* begin;
		char* end;
		char* writePointer;
		char* readPointer;

		int bufferSize;
	};
	extern ObjectFreeList<Packet> g_PacketObjectPool;
}


#endif // !__SERIALIZEING_BUFFER_HEADER__