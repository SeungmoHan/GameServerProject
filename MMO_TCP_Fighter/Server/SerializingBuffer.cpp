#pragma once
#include "SerializingBuffer.h"
#include <string>

namespace univ_dev
{
	ObjectFreeList<Packet> g_PacketObjectPool;
	Packet::Packet() : begin(new char[PacketSize::DefaultSize]), end(begin + PacketSize::DefaultSize), writePointer(begin), readPointer(begin), bufferSize(PacketSize::DefaultSize) {}
	//Packet::Packet() : begin((char*)bufferFreeList.Alloc()), end(begin + PacketSize::DefaultSize), writePointer(begin), readPointer(begin), bufferSize(PacketSize::DefaultSize) {}

	Packet::Packet(int bufferSize) : begin(new char[bufferSize]), end(begin + bufferSize), writePointer(begin), readPointer(begin), bufferSize(bufferSize) {}

	Packet::~Packet()
	{
		Release();
	}

	void Packet::Release()
	{
		delete[] begin;
	}

	void Packet::Clear()
	{
		writePointer = readPointer = begin;
	}

	int Packet::GetBufferSize()
	{
		return writePointer - readPointer;
	}

	char* Packet::GetWritePtr()
	{
		return writePointer;
	}

	char* Packet::GetReadPtr()
	{
		return readPointer;
	}
	int Packet::MoveWritePtr(int size)
	{
		if (writePointer + size >= end)
		{
			writePointer = end - 1;
			return end - writePointer;
		}
		writePointer += size;
		return size;
	}

	int Packet::MoveReadPtr(int size)
	{
		if (readPointer + size >= end)
		{
			readPointer = end - 1;
			return end - readPointer;
		}
		readPointer += size;
		return size;
	}

	Packet& Packet::operator=(const Packet& other)
	{
		this->bufferSize = other.bufferSize;
		begin = new char[bufferSize];
		end = begin + bufferSize;
		memcpy_s(begin, other.writePointer - other.readPointer, other.readPointer, other.writePointer - other.readPointer);
		return *this;
	}

	Packet& Packet::operator>>(unsigned char& value)
	{
		if (readPointer + sizeof(value) - 1 >= end)
		{
			//�б� �Ұ���
			return *this;
		}
		value = *readPointer;
		MoveReadPtr(sizeof(value));
		return *this;
	}

	Packet& Packet::operator>>(char& value)
	{
		if (readPointer + sizeof(value) - 1 >= end)
		{
			//�б� �Ұ���
			return *this;
		}
		value = *readPointer;
		MoveReadPtr(sizeof(value));
		return *this;
	}

	Packet& Packet::operator>>(unsigned short& value)
	{
		if (readPointer + sizeof(value) - 1 >= end)
		{
			//�б� �Ұ���
			return *this;
		}
		unsigned short* tempPtr = (unsigned short*)readPointer;
		value = *tempPtr;
		MoveReadPtr(sizeof(value));
		return *this;
	}

	Packet& Packet::operator>>(short& value)
	{
		if (readPointer + sizeof(value) - 1 >= end)
		{
			//�б� �Ұ���
			return *this;
		}
		short* tempPtr = (short*)readPointer;
		value = *tempPtr;
		MoveReadPtr(sizeof(value));
		return *this;
	}

	Packet& Packet::operator>>(int& value)
	{
		if (readPointer + sizeof(value) - 1 >= end)
		{
			//�б� �Ұ���
			return *this;
		}
		int* tempPtr = (int*)readPointer;
		value = *tempPtr;
		MoveReadPtr(sizeof(value));
		return *this;
	}
	Packet& Packet::operator>>(unsigned int& value)
	{
		if (readPointer + sizeof(value) - 1 >= end)
		{
			//�б� �Ұ���
			return *this;
		}
		unsigned int* tempPtr = (unsigned int*)readPointer;
		value = *tempPtr;
		MoveReadPtr(sizeof(value));
		return *this;
	}
	Packet& Packet::operator>>(unsigned long& value)
	{
		if (readPointer + sizeof(value) - 1 >= end)
		{
			//�б� �Ұ���
			return *this;
		}
		unsigned long* tempPtr = (unsigned long*)readPointer;
		value = *tempPtr;
		MoveReadPtr(sizeof(value));
		return *this;
	}
	Packet& Packet::operator>>(long& value)
	{
		if (readPointer + sizeof(value) - 1 >= end)
		{
			//�б� �Ұ���
			return *this;
		}
		long* tempPtr = (long*)readPointer;
		value = *tempPtr;
		MoveReadPtr(sizeof(value));
		return *this;
	}

	Packet& Packet::operator>>(float& value)
	{
		if (readPointer + sizeof(value) - 1 >= end)
		{
			//�б� �Ұ���
			return *this;
		}
		float* tempPtr = (float*)readPointer;
		value = *tempPtr;
		MoveReadPtr(sizeof(value));
		return *this;
	}

	Packet& Packet::operator>>(double& value)
	{
		if (readPointer + sizeof(value) - 1 >= end)
		{
			//�б� �Ұ���
			return *this;
		}
		double* tempPtr = (double*)readPointer;
		value = *tempPtr;
		MoveReadPtr(sizeof(value));
		return *this;
	}

	Packet& Packet::operator>>(__int64& value)
	{
		if (readPointer + sizeof(value) - 1 >= end)
		{
			//�б� �Ұ���
			return *this;
		}
		__int64* tempPtr = (__int64*)readPointer;
		value = *tempPtr;
		MoveReadPtr(sizeof(value));
		return *this;
	}

	Packet& Packet::operator<<(unsigned char value)
	{
		if (writePointer + (sizeof(value) - 1) >= end)
		{
			//������ �Ұ����Ѱ��.
			return *this;
		}
		*writePointer = value;
		MoveWritePtr(sizeof(value));
		return *this;
	}

	Packet& Packet::operator<<(char value)
	{
		if (writePointer + (sizeof(value) - 1) >= end)
		{
			//������ �Ұ����Ѱ��.
			return *this;
		}
		*writePointer = value;
		MoveWritePtr(sizeof(value));
		return *this;
	}

	Packet& Packet::operator<<(unsigned short value)
	{
		if (writePointer + (sizeof(value) - 1) >= end)
		{
			//������ �Ұ����Ѱ��.
			return *this;
		}
		unsigned short* tempPtr = (unsigned short*)writePointer;
		*tempPtr = value;
		MoveWritePtr(sizeof(value));
		return *this;
	}

	Packet& Packet::operator<<(short value)
	{
		if (writePointer + (sizeof(value) - 1) >= end)
		{
			//������ �Ұ����Ѱ��.
			return *this;
		}
		short* tempPtr = (short*)writePointer;
		*tempPtr = value;
		MoveWritePtr(sizeof(value));
		return *this;
	}

	Packet& Packet::operator<<(int value)
	{
		if (writePointer + (sizeof(value) - 1) >= end)
		{
			//������ �Ұ����Ѱ��.
			return *this;
		}
		int* tempPtr = (int*)writePointer;
		*tempPtr = value;
		MoveWritePtr(sizeof(value));
		return *this;
	}
	Packet& Packet::operator<<(unsigned int value)
	{
		if (writePointer + (sizeof(value) - 1) >= end)
		{
			//������ �Ұ����Ѱ��.
			return *this;
		}
		unsigned int* tempPtr = (unsigned int*)writePointer;
		*tempPtr = value;
		MoveWritePtr(sizeof(value));
		return *this;
	}
	Packet& Packet::operator<<(unsigned long value)
	{
		if (writePointer + (sizeof(value) - 1) >= end)
		{
			//������ �Ұ����Ѱ��.
			return *this;
		}
		unsigned long* tempPtr = (unsigned long*)writePointer;
		*tempPtr = value;
		MoveWritePtr(sizeof(value));
		return *this;
	}
	Packet& Packet::operator<<(long value)
	{
		if (writePointer + (sizeof(value) - 1) >= end)
		{
			//������ �Ұ����Ѱ��.
			return *this;
		}
		long* tempPtr = (long*)writePointer;
		*tempPtr = value;
		MoveWritePtr(sizeof(value));
		return *this;
	}

	Packet& Packet::operator<<(float value)
	{
		if (writePointer + (sizeof(value) - 1) >= end)
		{
			//������ �Ұ����Ѱ��.
			return *this;
		}
		float* tempPtr = (float*)writePointer;
		*tempPtr = value;
		MoveWritePtr(sizeof(value));
		return *this;
	}

	Packet& Packet::operator<<(__int64 value)
	{
		if (writePointer + (sizeof(value) - 1) >= end)
		{
			//������ �Ұ����Ѱ��.
			return *this;
		}
		__int64* tempPtr = (__int64*)writePointer;
		*tempPtr = value;
		MoveWritePtr(sizeof(value));
		return *this;
	}

	Packet& Packet::operator<<(double value)
	{
		if (writePointer + (sizeof(value) - 1) >= end)
		{
			//������ �Ұ����Ѱ��.
			return *this;
		}
		double* tempPtr = (double*)writePointer;
		*tempPtr = value;
		MoveWritePtr(sizeof(value));
		return *this;
	}


}
