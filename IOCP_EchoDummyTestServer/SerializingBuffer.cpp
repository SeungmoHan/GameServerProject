#include "LockFreeMemoryPoolTLS.hpp"
#include "SerializingBuffer.h"
#include <string>

#define NET_HEADER_SIZE (short)sizeof(NetServerPacket)

namespace univ_dev
{


	static LockFreeMemoryPoolTLS<Packet> _PakcetPool;

	Packet::Packet() : _Begin(new char[PacketSize::DefaultSize]), _End(_Begin + PacketSize::DefaultSize), _WritePointer(_Begin), _ReadPointer(_Begin), _BufferSize(PacketSize::DefaultSize), _RefCount(0) {}


	Packet::Packet(int bufferSize) : _Begin(new char[bufferSize]), _End(_Begin + bufferSize), _WritePointer(_Begin), _ReadPointer(_Begin), _BufferSize(bufferSize), _RefCount(0) {}

	Packet::~Packet()
	{
		Release();
	}

	void Packet::Release()
	{
		delete[] _Begin;
	}

	void Packet::Clear()
	{
		_WritePointer = _Begin + 5;
		_ReadPointer = _Begin;
	}


	int Packet::GetBufferSize()
	{
		return _WritePointer - _ReadPointer;
	}

	char* Packet::GetWritePtr()
	{
		return _WritePointer;
	}

	char* Packet::GetBeginPtr()
	{
		return _Begin;
	}

	void Packet::AddRef()
	{
		int r = _InterlockedIncrement((long*)&_RefCount);
	}

	bool Packet::SubRef()
	{
		int r = _InterlockedDecrement((long*)&_RefCount);
		if (r == 0)
			return true;
		return false;
	}

	void Packet::SetLanHeader()
	{
		short* header = (short*)(_Begin + 3);
		_ReadPointer = (char*)header;
		*header = GetBufferSize() - LAN_HEADER_SIZE;
	}

	void Packet::SetNetHeader()
	{
		NetServerPacket packet{ 0 };
		packet._ByteCode = this->PACKET_CODE;
		packet._Len = GetBufferSize() - NET_HEADER_SIZE;

		char* temp = _Begin + 5;
		int checkSum = 0;

		while (temp < _WritePointer)
		{
			checkSum += *temp;
			temp++;
		}
		checkSum %= 256;
		packet._CheckSum = checkSum;
		packet._RandomKey = rand();
		memcpy_s(_Begin, NET_HEADER_SIZE, &packet, NET_HEADER_SIZE);
		Encode();
	}

	void Packet::Encode()
	{

		unsigned char* temp = (unsigned char*)this->_Begin;
		int bufferSize = GetBufferSize();

		unsigned char e = 0;
		unsigned char p = 0;

		int RandKey = temp[3];
		for (int i = NET_HEADER_SIZE - 1; i < bufferSize + NET_HEADER_SIZE; i++)
		{
			p = temp[i] ^ (p + RandKey + (unsigned char)(i - NET_HEADER_SIZE + 2));
			temp[i] = e = p ^ (e + this->FIXED_KEY + (unsigned char)(i - NET_HEADER_SIZE + 2));
		}
	}

	void Packet::Decode()
	{
		unsigned char* temp = (unsigned char*)this->_Begin;

		unsigned char RandKey = temp[3];

		int bufferSize = GetBufferSize() - NET_HEADER_SIZE;

		unsigned char p = temp[4] ^ (this->FIXED_KEY + 1);
		unsigned char prevP = p;
		unsigned char prevE = temp[4];
		temp[4] = p ^ (RandKey + 1);

		for (int i = NET_HEADER_SIZE; i < bufferSize + NET_HEADER_SIZE; i++)
		{
			prevP = p;
			p = temp[i] ^ (prevE + this->FIXED_KEY + i - NET_HEADER_SIZE + 2);
			prevE = temp[i];
			temp[i] = p ^ (prevP + RandKey + i - NET_HEADER_SIZE + 2);
		}
	}

	bool Packet::VerifyCheckSum()
	{
		char checksum = this->_Begin[4];
		char* temp = this->_Begin + 5;

		int compChecksum = 0;
		while (temp < _WritePointer)
		{
			compChecksum += *temp;
			temp++;
		}
		compChecksum %= 256;

		if (compChecksum != checksum)
			return false;
		return true;
	}

	char* Packet::GetReadPtr()
	{
		return _ReadPointer;
	}

	int Packet::MoveWritePtr(int size)
	{
		if (_WritePointer + size >= _End)
		{
			_WritePointer = _End - 1;
			return _End - _WritePointer;
		}
		_WritePointer += size;
		return size;
	}

	int Packet::MoveReadPtr(int size)
	{
		if (_ReadPointer + size >= _End)
		{
			_ReadPointer = _End - 1;
			return _End - _ReadPointer;
		}
		_ReadPointer += size;
		return size;
	}

	int Packet::GetUseCount()
	{
		return _PakcetPool.GetUseCount();
	}

	int Packet::GetTotalPacketCount()
	{
		return _PakcetPool.GetTotalUseCount();
	}

	int Packet::GetCapacityCount()
	{
		return _PakcetPool.GetCapacityCount();
	}

	Packet* Packet::Alloc()
	{
		Packet* packet = _PakcetPool.Alloc();
		packet->Clear();
		packet->AddRef();
		return packet;
	}

	void Packet::Free(Packet* packet)
	{
		if (packet->SubRef())
		{
			_PakcetPool.Free(packet);
		}
	}

	void Packet::PutString(const char* str, int size)
	{
		memcpy_s(_WritePointer, size, str, size);
		_WritePointer += size;
	}

	Packet& Packet::operator=(const Packet& other)
	{
		this->_BufferSize = other._BufferSize;
		_Begin = new char[_BufferSize];
		_End = _Begin + _BufferSize;
		memcpy_s(_Begin, other._WritePointer - other._ReadPointer, other._ReadPointer, other._WritePointer - other._ReadPointer);
		return *this;
	}

	Packet& Packet::operator>>(unsigned char& value)
	{
		if (_ReadPointer + sizeof(value) - 1 >= _End)
		{
			//읽기 불가능
			return *this;
		}
		value = *_ReadPointer;
		MoveReadPtr(sizeof(value));
		return *this;
	}

	Packet& Packet::operator>>(char& value)
	{
		if (_ReadPointer + sizeof(value) - 1 >= _End)
		{
			//읽기 불가능
			return *this;
		}
		value = *_ReadPointer;
		MoveReadPtr(sizeof(value));
		return *this;
	}

	Packet& Packet::operator>>(unsigned short& value)
	{
		if (_ReadPointer + sizeof(value) - 1 >= _End)
		{
			//읽기 불가능
			return *this;
		}
		unsigned short* tempPtr = (unsigned short*)_ReadPointer;
		value = *tempPtr;
		MoveReadPtr(sizeof(value));
		return *this;
	}

	Packet& Packet::operator>>(short& value)
	{
		if (_ReadPointer + sizeof(value) - 1 >= _End)
		{
			//읽기 불가능
			return *this;
		}
		short* tempPtr = (short*)_ReadPointer;
		value = *tempPtr;
		MoveReadPtr(sizeof(value));
		return *this;
	}

	Packet& Packet::operator>>(int& value)
	{
		if (_ReadPointer + sizeof(value) - 1 >= _End)
		{
			//읽기 불가능
			return *this;
		}
		int* tempPtr = (int*)_ReadPointer;
		value = *tempPtr;
		MoveReadPtr(sizeof(value));
		return *this;
	}
	Packet& Packet::operator>>(unsigned int& value)
	{
		if (_ReadPointer + sizeof(value) - 1 >= _End)
		{
			//읽기 불가능
			return *this;
		}
		unsigned int* tempPtr = (unsigned int*)_ReadPointer;
		value = *tempPtr;
		MoveReadPtr(sizeof(value));
		return *this;
	}
	Packet& Packet::operator>>(unsigned long& value)
	{
		if (_ReadPointer + sizeof(value) - 1 >= _End)
		{
			//읽기 불가능
			return *this;
		}
		unsigned long* tempPtr = (unsigned long*)_ReadPointer;
		value = *tempPtr;
		MoveReadPtr(sizeof(value));
		return *this;
	}
	Packet& Packet::operator>>(long& value)
	{
		if (_ReadPointer + sizeof(value) - 1 >= _End)
		{
			//읽기 불가능
			return *this;
		}
		long* tempPtr = (long*)_ReadPointer;
		value = *tempPtr;
		MoveReadPtr(sizeof(value));
		return *this;
	}

	Packet& Packet::operator>>(float& value)
	{
		if (_ReadPointer + sizeof(value) - 1 >= _End)
		{
			//읽기 불가능
			return *this;
		}
		float* tempPtr = (float*)_ReadPointer;
		value = *tempPtr;
		MoveReadPtr(sizeof(value));
		return *this;
	}

	Packet& Packet::operator>>(double& value)
	{
		if (_ReadPointer + sizeof(value) - 1 >= _End)
		{
			//읽기 불가능
			return *this;
		}
		double* tempPtr = (double*)_ReadPointer;
		value = *tempPtr;
		MoveReadPtr(sizeof(value));
		return *this;
	}

	Packet& Packet::operator>>(__int64& value)
	{
		if (_ReadPointer + sizeof(value) - 1 >= _End)
		{
			//읽기 불가능
			return *this;
		}
		__int64* tempPtr = (__int64*)_ReadPointer;
		value = *tempPtr;
		MoveReadPtr(sizeof(value));
		return *this;
	}

	Packet& Packet::operator>>(unsigned __int64& value)
	{
		if (_ReadPointer + sizeof(value) - 1 >= _End)
		{
			//읽기 불가능
			return *this;
		}
		unsigned __int64* tempPtr = (unsigned __int64*)_ReadPointer;
		value = *tempPtr;
		MoveReadPtr(sizeof(value));
		return *this;
	}

	Packet& Packet::operator<<(unsigned char value)
	{
		if (_WritePointer + (sizeof(value) - 1) >= _End)
		{
			//저장이 불가능한경우.
			return *this;
		}
		*_WritePointer = value;
		MoveWritePtr(sizeof(value));
		return *this;
	}

	Packet& Packet::operator<<(char value)
	{
		if (_WritePointer + (sizeof(value) - 1) >= _End)
		{
			//저장이 불가능한경우.
			return *this;
		}
		*_WritePointer = value;
		MoveWritePtr(sizeof(value));
		return *this;
	}

	Packet& Packet::operator<<(unsigned short value)
	{
		if (_WritePointer + (sizeof(value) - 1) >= _End)
		{
			//저장이 불가능한경우.
			return *this;
		}
		unsigned short* tempPtr = (unsigned short*)_WritePointer;
		*tempPtr = value;
		MoveWritePtr(sizeof(value));
		return *this;
	}

	Packet& Packet::operator<<(short value)
	{
		if (_WritePointer + (sizeof(value) - 1) >= _End)
		{
			//저장이 불가능한경우.
			return *this;
		}
		short* tempPtr = (short*)_WritePointer;
		*tempPtr = value;
		MoveWritePtr(sizeof(value));
		return *this;
	}

	Packet& Packet::operator<<(int value)
	{
		if (_WritePointer + (sizeof(value) - 1) >= _End)
		{
			//저장이 불가능한경우.
			return *this;
		}
		int* tempPtr = (int*)_WritePointer;
		*tempPtr = value;
		MoveWritePtr(sizeof(value));
		return *this;
	}
	Packet& Packet::operator<<(unsigned int value)
	{
		if (_WritePointer + (sizeof(value) - 1) >= _End)
		{
			//저장이 불가능한경우.
			return *this;
		}
		unsigned int* tempPtr = (unsigned int*)_WritePointer;
		*tempPtr = value;
		MoveWritePtr(sizeof(value));
		return *this;
	}
	Packet& Packet::operator<<(unsigned long value)
	{
		if (_WritePointer + (sizeof(value) - 1) >= _End)
		{
			//저장이 불가능한경우.
			return *this;
		}
		unsigned long* tempPtr = (unsigned long*)_WritePointer;
		*tempPtr = value;
		MoveWritePtr(sizeof(value));
		return *this;
	}
	Packet& Packet::operator<<(long value)
	{
		if (_WritePointer + (sizeof(value) - 1) >= _End)
		{
			//저장이 불가능한경우.
			return *this;
		}
		long* tempPtr = (long*)_WritePointer;
		*tempPtr = value;
		MoveWritePtr(sizeof(value));
		return *this;
	}

	Packet& Packet::operator<<(float value)
	{
		if (_WritePointer + (sizeof(value) - 1) >= _End)
		{
			//저장이 불가능한경우.
			return *this;
		}
		float* tempPtr = (float*)_WritePointer;
		*tempPtr = value;
		MoveWritePtr(sizeof(value));
		return *this;
	}

	Packet& Packet::operator<<(__int64 value)
	{
		if (_WritePointer + (sizeof(value) - 1) >= _End)
		{
			//저장이 불가능한경우.
			return *this;
		}
		__int64* tempPtr = (__int64*)_WritePointer;
		*tempPtr = value;
		MoveWritePtr(sizeof(value));
		return *this;
	}

	Packet& Packet::operator<<(unsigned __int64 value)
	{
		if (_WritePointer + (sizeof(value) - 1) >= _End)
		{
			//저장이 불가능한경우.
			return *this;
		}
		unsigned __int64* tempPtr = (unsigned __int64*)_WritePointer;
		*tempPtr = value;
		MoveWritePtr(sizeof(value));
		return *this;
	}

	Packet& Packet::operator<<(double value)
	{
		if (_WritePointer + (sizeof(value) - 1) >= _End)
		{
			//저장이 불가능한경우.
			return *this;
		}
		double* tempPtr = (double*)_WritePointer;
		*tempPtr = value;
		MoveWritePtr(sizeof(value));
		return *this;
	}


}