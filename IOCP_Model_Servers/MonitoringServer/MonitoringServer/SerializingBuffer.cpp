#include "LockFreeMemoryPoolTLS.hpp"
#include "SerializingBuffer.h"
#include <string>


namespace univ_dev
{


	static LockFreeMemoryPoolTLS<Packet> g_PakcetPool;

	Packet::Packet() : _EncodeFlag(0),_Begin(new unsigned char[PacketSize::DefaultSize]), _End(_Begin + PacketSize::DefaultSize), _WritePointer(_Begin), _ReadPointer(_Begin), _BufferSize(PacketSize::DefaultSize), _RefCount(0) {}


	Packet::Packet(int bufferSize) : _EncodeFlag(false),_Begin(new unsigned char[bufferSize]), _End(_Begin + bufferSize), _WritePointer(_Begin), _ReadPointer(_Begin), _BufferSize(bufferSize), _RefCount(0) {}

	Packet::~Packet()
	{
		this->Release();
	}

	void Packet::Release()
	{
		delete[] this->_Begin;
	}

	void Packet::Clear()
	{
		InterlockedExchange(&this->_EncodeFlag, false);
		this->_WritePointer = this->_Begin + 5;
		this->_ReadPointer = this->_Begin;
	}


	int Packet::GetBufferSize()
	{
		return this->_WritePointer - this->_ReadPointer;
	}

	unsigned char* Packet::GetWritePtr()
	{
		return this->_WritePointer;
	}

	unsigned char* Packet::GetBeginPtr()
	{
		return this->_Begin;
	}

	void Packet::AddRef()
	{
		_InterlockedIncrement((long*)&this->_RefCount);
	}

	bool Packet::SubRef()
	{
		int r = _InterlockedDecrement((long*)&this->_RefCount);
		if (r == 0)
			return true;
		return false;
	}

	void Packet::SetLanHeader()
	{
		short* header = (short*)(this->_Begin + 3);
		this->_ReadPointer = (unsigned char*)header;
		*header = this->GetBufferSize() - LAN_HEADER_SIZE;
	}

	void Packet::SetNetHeader()
	{
		if (this->_EncodeFlag == true)
			return;
		this->_EncodeFlag = true;

		NetServerHeader header{ 0 };
		header._ByteCode = this->PACKET_CODE;
		header._Len = this->GetBufferSize() - NET_HEADER_SIZE;

		unsigned char* temp = this->_Begin + 5;
		int checkSum = 0;

		while (temp < this->_WritePointer)
		{
			checkSum += *temp;
			temp++;
		}
		checkSum %= 256;
		header._CheckSum = checkSum;
		header._RandomKey = rand();
		memcpy_s(this->_Begin, NET_HEADER_SIZE, &header, NET_HEADER_SIZE);
		this->Encode();
	}

	void Packet::Encode()
	{
		unsigned char* temp = (unsigned char*)this->_Begin;
		int bufferSize = this->GetBufferSize();

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

		int bufferSize = this->GetBufferSize() - NET_HEADER_SIZE;

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
		unsigned char checksum = this->_Begin[4];
		unsigned char* temp = (unsigned char*)this->_Begin + 5;

		unsigned int compChecksum = 0;
		while (temp < (unsigned char*)this->_WritePointer)
		{
			compChecksum += *temp;
			temp++;
		}
		compChecksum %= 256;

		if (compChecksum != checksum)
			return false;
		return true;
	}

	unsigned char* Packet::GetReadPtr()
	{
		return this->_ReadPointer;
	}

	//int Packet::MoveWritePtr(int size)
	//{
	//	if (this->_WritePointer + size >= this->_End)
	//	{
	//		this->_WritePointer = this->_End - 1;
	//		return this->_End - this->_WritePointer;
	//	}
	//	this->_WritePointer += size;
	//	return size;
	//}

	//int Packet::MoveReadPtr(int size)
	//{
	//	if (this->_ReadPointer + size >= this->_End)
	//	{
	//		this->_ReadPointer = this->_End - 1;
	//		return this->_End - this->_ReadPointer;
	//	}
	//	this->_ReadPointer += size;
	//	return size;
	//}

	int Packet::GetUseCount()
	{
		return g_PakcetPool.GetUseCount();
	}

	int Packet::GetTotalPacketCount()
	{
		return g_PakcetPool.GetTotalUseCount();
	}

	int Packet::GetCapacityCount()
	{
		return g_PakcetPool.GetCapacityCount();
	}

	Packet* Packet::Alloc()
	{
		Packet* packet = g_PakcetPool.Alloc();
		packet->Clear();
		return packet;
	}

	void Packet::Free(Packet* packet)
	{
		if (packet->SubRef())
		{
			packet->Clear();
			g_PakcetPool.Free(packet);
		}
	}

	void Packet::PutString(const char* str, size_t stringLen)
	{
		strcpy_s((char*)this->_WritePointer, stringLen, str);
		this->_WritePointer += stringLen;
	}

	void Packet::PutWString(const WCHAR* str, size_t stringLen)
	{
		wcscpy_s((WCHAR*)this->_WritePointer, stringLen * 2, str);
		this->_WritePointer += stringLen * 2;
	}

	void Packet::GetString(char* dest, size_t stringLen)
	{
		strcpy_s(dest, stringLen, (char*)this->_ReadPointer);
		this->_ReadPointer += stringLen;
	}

	void Packet::GetWString(WCHAR* dest, size_t stringLen)
	{
		wcscpy_s(dest, stringLen * 2, (WCHAR*)this->_ReadPointer);
		this->_ReadPointer += stringLen * 2;
	}

	void Packet::PutBuffer(const char* buffer, size_t size)
	{
		memmove_s(this->_WritePointer, size, buffer, size);
		this->_WritePointer += size;
	}

	void Packet::GetBuffer(char* buffer, size_t size)
	{
		memmove_s(buffer, size, this->_ReadPointer, size);
		this->_ReadPointer += size;
	}

	Packet& Packet::operator=(const Packet& other)
	{
		this->_BufferSize = other._BufferSize;
		this->_Begin = new unsigned char[this->_BufferSize];
		this->_End = this->_Begin + this->_BufferSize;
		memcpy_s(this->_Begin, other._WritePointer - other._ReadPointer, other._ReadPointer, other._WritePointer - other._ReadPointer);
		return *this;
	}

	//Packet& Packet::operator>>(unsigned char& value)
	//{
	//	if (this->_ReadPointer + sizeof(value) - 1 >= this->_End)
	//	{
	//		//읽기 불가능
	//		return *this;
	//	}
	//	value = *this->_ReadPointer;
	//	this->MoveReadPtr(sizeof(value));
	//	return *this;
	//}

	//Packet& Packet::operator>>(char& value)
	//{
	//	if (this->_ReadPointer + sizeof(value) - 1 >= this->_End)
	//	{
	//		//읽기 불가능
	//		return *this;
	//	}
	//	value = *this->_ReadPointer;
	//	this->MoveReadPtr(sizeof(value));
	//	return *this;
	//}

	//Packet& Packet::operator>>(unsigned short& value)
	//{
	//	if (this->_ReadPointer + sizeof(value) - 1 >= this->_End)
	//	{
	//		//읽기 불가능
	//		return *this;
	//	}
	//	unsigned short* tempPtr = (unsigned short*)this->_ReadPointer;
	//	value = *tempPtr;
	//	this->MoveReadPtr(sizeof(value));
	//	return *this;
	//}

	//Packet& Packet::operator>>(short& value)
	//{
	//	if (this->_ReadPointer + sizeof(value) - 1 >= this->_End)
	//	{
	//		//읽기 불가능
	//		return *this;
	//	}
	//	short* tempPtr = (short*)this->_ReadPointer;
	//	value = *tempPtr;
	//	this->MoveReadPtr(sizeof(value));
	//	return *this;
	//}

	//Packet& Packet::operator>>(int& value)
	//{
	//	if (this->_ReadPointer + sizeof(value) - 1 >= this->_End)
	//	{
	//		//읽기 불가능
	//		return *this;
	//	}
	//	int* tempPtr = (int*)this->_ReadPointer;
	//	value = *tempPtr;
	//	this->MoveReadPtr(sizeof(value));
	//	return *this;
	//}
	//Packet& Packet::operator>>(unsigned int& value)
	//{
	//	if (this->_ReadPointer + sizeof(value) - 1 >= this->_End)
	//	{
	//		//읽기 불가능
	//		return *this;
	//	}
	//	unsigned int* tempPtr = (unsigned int*)this->_ReadPointer;
	//	value = *tempPtr;
	//	this->MoveReadPtr(sizeof(value));
	//	return *this;
	//}
	//Packet& Packet::operator>>(unsigned long& value)
	//{
	//	if (this->_ReadPointer + sizeof(value) - 1 >= this->_End)
	//	{
	//		//읽기 불가능
	//		return *this;
	//	}
	//	unsigned long* tempPtr = (unsigned long*)this->_ReadPointer;
	//	value = *tempPtr;
	//	this->MoveReadPtr(sizeof(value));
	//	return *this;
	//}
	//Packet& Packet::operator>>(long& value)
	//{
	//	if (this->_ReadPointer + sizeof(value) - 1 >= this->_End)
	//	{
	//		//읽기 불가능
	//		return *this;
	//	}
	//	long* tempPtr = (long*)this->_ReadPointer;
	//	value = *tempPtr;
	//	this->MoveReadPtr(sizeof(value));
	//	return *this;
	//}

	//Packet& Packet::operator>>(float& value)
	//{
	//	if (this->_ReadPointer + sizeof(value) - 1 >= this->_End)
	//	{
	//		//읽기 불가능
	//		return *this;
	//	}
	//	float* tempPtr = (float*)this->_ReadPointer;
	//	value = *tempPtr;
	//	this->MoveReadPtr(sizeof(value));
	//	return *this;
	//}

	//Packet& Packet::operator>>(double& value)
	//{
	//	if (this->_ReadPointer + sizeof(value) - 1 >= this->_End)
	//	{
	//		//읽기 불가능
	//		return *this;
	//	}
	//	double* tempPtr = (double*)_ReadPointer;
	//	value = *tempPtr;
	//	this->MoveReadPtr(sizeof(value));
	//	return *this;
	//}

	//Packet& Packet::operator>>(__int64& value)
	//{
	//	if (this->_ReadPointer + sizeof(value) - 1 >= this->_End)
	//	{
	//		//읽기 불가능
	//		return *this;
	//	}
	//	__int64* tempPtr = (__int64*)this->_ReadPointer;
	//	value = *tempPtr;
	//	this->MoveReadPtr(sizeof(value));
	//	return *this;
	//}

	//Packet& Packet::operator>>(unsigned __int64& value)
	//{
	//	if (this->_ReadPointer + sizeof(value) - 1 >= this->_End)
	//	{
	//		//읽기 불가능
	//		return *this;
	//	}
	//	unsigned __int64* tempPtr = (unsigned __int64*)this->_ReadPointer;
	//	value = *tempPtr;
	//	this->MoveReadPtr(sizeof(value));
	//	return *this;
	//}

	//Packet& Packet::operator<<(unsigned char value)
	//{
	//	if (this->_WritePointer + (sizeof(value) - 1) >= this->_End)
	//	{
	//		//저장이 불가능한경우.
	//		return *this;
	//	}
	//	*this->_WritePointer = value;
	//	this->MoveWritePtr(sizeof(value));
	//	return *this;
	//}

	//Packet& Packet::operator<<(char value)
	//{
	//	if (this->_WritePointer + (sizeof(value) - 1) >= this->_End)
	//	{
	//		//저장이 불가능한경우.
	//		return *this;
	//	}
	//	*this->_WritePointer = value;
	//	this->MoveWritePtr(sizeof(value));
	//	return *this;
	//}

	//Packet& Packet::operator<<(unsigned short value)
	//{
	//	if (this->_WritePointer + (sizeof(value) - 1) >= this->_End)
	//	{
	//		//저장이 불가능한경우.
	//		return *this;
	//	}
	//	unsigned short* tempPtr = (unsigned short*)this->_WritePointer;
	//	*tempPtr = value;
	//	this->MoveWritePtr(sizeof(value));
	//	return *this;
	//}

	//Packet& Packet::operator<<(short value)
	//{
	//	if (this->_WritePointer + (sizeof(value) - 1) >= this->_End)
	//	{
	//		//저장이 불가능한경우.
	//		return *this;
	//	}
	//	short* tempPtr = (short*)this->_WritePointer;
	//	*tempPtr = value;
	//	this->MoveWritePtr(sizeof(value));
	//	return *this;
	//}

	//Packet& Packet::operator<<(int value)
	//{
	//	if (this->_WritePointer + (sizeof(value) - 1) >= this->_End)
	//	{
	//		//저장이 불가능한경우.
	//		return *this;
	//	}
	//	int* tempPtr = (int*)this->_WritePointer;
	//	*tempPtr = value;
	//	this->MoveWritePtr(sizeof(value));
	//	return *this;
	//}
	//Packet& Packet::operator<<(unsigned int value)
	//{
	//	if (this->_WritePointer + (sizeof(value) - 1) >= this->_End)
	//	{
	//		//저장이 불가능한경우.
	//		return *this;
	//	}
	//	unsigned int* tempPtr = (unsigned int*)this->_WritePointer;
	//	*tempPtr = value;
	//	this->MoveWritePtr(sizeof(value));
	//	return *this;
	//}
	//Packet& Packet::operator<<(unsigned long value)
	//{
	//	if (this->_WritePointer + (sizeof(value) - 1) >= this->_End)
	//	{
	//		//저장이 불가능한경우.
	//		return *this;
	//	}
	//	unsigned long* tempPtr = (unsigned long*)this->_WritePointer;
	//	*tempPtr = value;
	//	this->MoveWritePtr(sizeof(value));
	//	return *this;
	//}
	//Packet& Packet::operator<<(long value)
	//{
	//	if (this->_WritePointer + (sizeof(value) - 1) >= this->_End)
	//	{
	//		//저장이 불가능한경우.
	//		return *this;
	//	}
	//	long* tempPtr = (long*)this->_WritePointer;
	//	*tempPtr = value;
	//	this->MoveWritePtr(sizeof(value));
	//	return *this;
	//}

	//Packet& Packet::operator<<(float value)
	//{
	//	if (this->_WritePointer + (sizeof(value) - 1) >= this->_End)
	//	{
	//		//저장이 불가능한경우.
	//		return *this;
	//	}
	//	float* tempPtr = (float*)this->_WritePointer;
	//	*tempPtr = value;
	//	this->MoveWritePtr(sizeof(value));
	//	return *this;
	//}

	//Packet& Packet::operator<<(__int64 value)
	//{
	//	if (this->_WritePointer + (sizeof(value) - 1) >= this->_End)
	//	{
	//		//저장이 불가능한경우.
	//		return *this;
	//	}
	//	__int64* tempPtr = (__int64*)this->_WritePointer;
	//	*tempPtr = value;
	//	this->MoveWritePtr(sizeof(value));
	//	return *this;
	//}

	//Packet& Packet::operator<<(unsigned __int64 value)
	//{
	//	if (this->_WritePointer + (sizeof(value) - 1) >= this->_End)
	//	{
	//		//저장이 불가능한경우.
	//		return *this;
	//	}
	//	unsigned __int64* tempPtr = (unsigned __int64*)this->_WritePointer;
	//	*tempPtr = value;
	//	this->MoveWritePtr(sizeof(value));
	//	return *this;
	//}

	//Packet& Packet::operator<<(double value)
	//{
	//	if (this->_WritePointer + (sizeof(value) - 1) >= this->_End)
	//	{
	//		//저장이 불가능한경우.
	//		return *this;
	//	}
	//	double* tempPtr = (double*)this->_WritePointer;
	//	*tempPtr = value;
	//	this->MoveWritePtr(sizeof(value));
	//	return *this;
	//}
}