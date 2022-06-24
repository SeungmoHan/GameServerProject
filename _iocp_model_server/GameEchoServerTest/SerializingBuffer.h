#pragma once
#ifndef __SERIALIZEING_BUFFER_HEADER__
#define __SERIALIZEING_BUFFER_HEADER__
#define __UNIV_DEVELOPER_

namespace univ_dev
{
#pragma pack(push,1)
	struct LanServerHeader
	{
		unsigned short _Len;
	};
	struct NetServerHeader
	{
		unsigned char _ByteCode;
		unsigned short _Len;
		unsigned char _RandomKey;
		unsigned char _CheckSum;
	};
#pragma pack(pop)

#define LAN_HEADER_SIZE (short)sizeof(LanServerHeader)
#define NET_HEADER_SIZE (short)sizeof(NetServerHeader)

	class Packet
	{
	protected:
		static constexpr int MSS = 1460;
		static constexpr int PACKET_CODE = 0x77;
		static constexpr int FIXED_KEY = 0x32;

		friend class CLanServer;
		friend class CNetServer;
		enum PacketSize
		{
			DefaultSize = MSS
		};
	public:
		Packet();
		Packet(int bufferSize);

		void AddRef();
		bool SubRef();
		virtual ~Packet();
		int GetBufferSize();
	protected:
		void Release();

		void Clear();

		unsigned char* GetReadPtr();
		unsigned char* GetWritePtr();
		unsigned char* GetBeginPtr();

		void SetLanHeader();
		void SetNetHeader();
		void Encode();
		void Decode();
		bool VerifyCheckSum();

		int GetRefCount() { return _RefCount; }

		inline int MoveWritePtr(int size)
		{
			if (this->_WritePointer + size >= this->_End)
			{
				this->_WritePointer = this->_End - 1;
				return this->_End - this->_WritePointer;
			}
			this->_WritePointer += size;
			return size;
		}
		inline int MoveReadPtr(int size)
		{
			if (this->_ReadPointer + size >= this->_End)
			{
				this->_ReadPointer = this->_End - 1;
				return this->_End - this->_ReadPointer;
			}
			this->_ReadPointer += size;
			return size;
		}
	public:

		static int GetUseCount();
		static int GetTotalPacketCount();
		static int GetCapacityCount();
		static Packet* Alloc();
		static void Free(Packet* packet);

		inline void PutString(const char* str, size_t stringLen)
		{
			strcpy_s((char*)this->_WritePointer, stringLen, str);
			this->_WritePointer += stringLen;
		}

		inline void PutWString(const WCHAR* str, size_t stringLen)
		{
			wcscpy_s((WCHAR*)this->_WritePointer, stringLen * 2, str);
			this->_WritePointer += stringLen * 2;
		}

		inline void GetString(char* dest, size_t stringLen)
		{
			strcpy_s(dest, stringLen, (char*)this->_ReadPointer);
			this->_ReadPointer += stringLen;
		}

		inline void GetWString(WCHAR* dest, size_t stringLen)
		{
			wcscpy_s(dest, stringLen * 2, (WCHAR*)this->_ReadPointer);
			this->_ReadPointer += stringLen * 2;
		}

		inline void PutBuffer(const char* buffer, size_t size)
		{
			memmove_s(this->_WritePointer, size, buffer, size);
			this->_WritePointer += size;
		}

		inline void GetBuffer(char* buffer, size_t size)
		{
			memmove_s(buffer, size, this->_ReadPointer, size);
			this->_ReadPointer += size;
		}


		Packet& operator=(const Packet& other);
		inline Packet& operator>>(unsigned char& value)
		{
			if (this->_ReadPointer + sizeof(value) - 1 >= this->_End)
			{
				//읽기 불가능
				return *this;
			}
			value = *this->_ReadPointer;
			this->MoveReadPtr(sizeof(value));
			return *this;
		}
		inline Packet& operator>>(char& value)
		{
			if (this->_ReadPointer + sizeof(value) - 1 >= this->_End)
			{
				//읽기 불가능
				return *this;
			}
			value = *this->_ReadPointer;
			this->MoveReadPtr(sizeof(value));
			return *this;
		}
		inline Packet& operator>>(unsigned short& value)
		{
			if (this->_ReadPointer + sizeof(value) - 1 >= this->_End)
			{
				//읽기 불가능
				return *this;
			}
			unsigned short* tempPtr = (unsigned short*)this->_ReadPointer;
			value = *tempPtr;
			this->MoveReadPtr(sizeof(value));
			return *this;
		}
		inline Packet& operator>>(short& value)
		{
			if (this->_ReadPointer + sizeof(value) - 1 >= this->_End)
			{
				//읽기 불가능
				return *this;
			}
			short* tempPtr = (short*)this->_ReadPointer;
			value = *tempPtr;
			this->MoveReadPtr(sizeof(value));
			return *this;
		}
		inline Packet& operator>>(unsigned int& value)
		{
			if (this->_ReadPointer + sizeof(value) - 1 >= this->_End)
			{
				//읽기 불가능
				return *this;
			}
			unsigned int* tempPtr = (unsigned int*)this->_ReadPointer;
			value = *tempPtr;
			this->MoveReadPtr(sizeof(value));
			return *this;
		}
		inline Packet& operator>>(int& value)
		{
			if (this->_ReadPointer + sizeof(value) - 1 >= this->_End)
			{
				//읽기 불가능
				return *this;
			}
			int* tempPtr = (int*)this->_ReadPointer;
			value = *tempPtr;
			this->MoveReadPtr(sizeof(value));
			return *this;
		}
		inline Packet& operator>>(long& value)
		{
			if (this->_ReadPointer + sizeof(value) - 1 >= this->_End)
			{
				//읽기 불가능
				return *this;
			}
			long* tempPtr = (long*)this->_ReadPointer;
			value = *tempPtr;
			this->MoveReadPtr(sizeof(value));
			return *this;
		}
		inline Packet& operator>>(unsigned long& value)
		{
			if (this->_ReadPointer + sizeof(value) - 1 >= this->_End)
			{
				//읽기 불가능
				return *this;
			}
			unsigned long* tempPtr = (unsigned long*)this->_ReadPointer;
			value = *tempPtr;
			this->MoveReadPtr(sizeof(value));
			return *this;
		}
		inline Packet& operator>>(float& value)
		{
			if (this->_ReadPointer + sizeof(value) - 1 >= this->_End)
			{
				//읽기 불가능
				return *this;
			}
			float* tempPtr = (float*)this->_ReadPointer;
			value = *tempPtr;
			this->MoveReadPtr(sizeof(value));
			return *this;
		}
		inline Packet& operator>>(double& value)
		{
			if (this->_ReadPointer + sizeof(value) - 1 >= this->_End)
			{
				//읽기 불가능
				return *this;
			}
			double* tempPtr = (double*)_ReadPointer;
			value = *tempPtr;
			this->MoveReadPtr(sizeof(value));
			return *this;
		}
		inline Packet& operator>>(__int64& value)
		{
			if (this->_ReadPointer + sizeof(value) - 1 >= this->_End)
			{
				//읽기 불가능
				return *this;
			}
			__int64* tempPtr = (__int64*)this->_ReadPointer;
			value = *tempPtr;
			this->MoveReadPtr(sizeof(value));
			return *this;
		}
		inline Packet& operator>>(unsigned __int64& value)
		{
			if (this->_ReadPointer + sizeof(value) - 1 >= this->_End)
			{
				//읽기 불가능
				return *this;
			}
			unsigned __int64* tempPtr = (unsigned __int64*)this->_ReadPointer;
			value = *tempPtr;
			this->MoveReadPtr(sizeof(value));
			return *this;
		}

		inline Packet& operator<<(char value)
		{
			if (this->_WritePointer + (sizeof(value) - 1) >= this->_End)
			{
				//저장이 불가능한경우.
				return *this;
			}
			*this->_WritePointer = value;
			this->MoveWritePtr(sizeof(value));
			return *this;
		}
		inline Packet& operator<<(unsigned char value)
		{
			if (this->_WritePointer + (sizeof(value) - 1) >= this->_End)
			{
				//저장이 불가능한경우.
				return *this;
			}
			*this->_WritePointer = value;
			this->MoveWritePtr(sizeof(value));
			return *this;
		}
		inline Packet& operator<<(unsigned short value)
		{
			if (this->_WritePointer + (sizeof(value) - 1) >= this->_End)
			{
				//저장이 불가능한경우.
				return *this;
			}
			unsigned short* tempPtr = (unsigned short*)this->_WritePointer;
			*tempPtr = value;
			this->MoveWritePtr(sizeof(value));
			return *this;
		}
		inline Packet& operator<<(short value)
		{
			if (this->_WritePointer + (sizeof(value) - 1) >= this->_End)
			{
				//저장이 불가능한경우.
				return *this;
			}
			short* tempPtr = (short*)this->_WritePointer;
			*tempPtr = value;
			this->MoveWritePtr(sizeof(value));
			return *this;
		}
		inline Packet& operator<<(unsigned int value)
		{
			if (this->_WritePointer + (sizeof(value) - 1) >= this->_End)
			{
				//저장이 불가능한경우.
				return *this;
			}
			unsigned int* tempPtr = (unsigned int*)this->_WritePointer;
			*tempPtr = value;
			this->MoveWritePtr(sizeof(value));
			return *this;
		}
		inline Packet& operator<<(int value)
		{
			if (this->_WritePointer + (sizeof(value) - 1) >= this->_End)
			{
				//저장이 불가능한경우.
				return *this;
			}
			int* tempPtr = (int*)this->_WritePointer;
			*tempPtr = value;
			this->MoveWritePtr(sizeof(value));
			return *this;
		}
		inline Packet& operator<<(unsigned long value)
		{
			if (this->_WritePointer + (sizeof(value) - 1) >= this->_End)
			{
				//저장이 불가능한경우.
				return *this;
			}
			unsigned long* tempPtr = (unsigned long*)this->_WritePointer;
			*tempPtr = value;
			this->MoveWritePtr(sizeof(value));
			return *this;
		}
		inline Packet& operator<<(long value)
		{
			if (this->_WritePointer + (sizeof(value) - 1) >= this->_End)
			{
				//저장이 불가능한경우.
				return *this;
			}
			long* tempPtr = (long*)this->_WritePointer;
			*tempPtr = value;
			this->MoveWritePtr(sizeof(value));
			return *this;
		}
		inline Packet& operator<<(float value)
		{
			if (this->_WritePointer + (sizeof(value) - 1) >= this->_End)
			{
				//저장이 불가능한경우.
				return *this;
			}
			float* tempPtr = (float*)this->_WritePointer;
			*tempPtr = value;
			this->MoveWritePtr(sizeof(value));
			return *this;
		}
		inline Packet& operator<<(__int64 value)
		{
			if (this->_WritePointer + (sizeof(value) - 1) >= this->_End)
			{
				//저장이 불가능한경우.
				return *this;
			}
			__int64* tempPtr = (__int64*)this->_WritePointer;
			*tempPtr = value;
			this->MoveWritePtr(sizeof(value));
			return *this;
		}
		inline Packet& operator<<(unsigned __int64 value)
		{
			if (this->_WritePointer + (sizeof(value) - 1) >= this->_End)
			{
				//저장이 불가능한경우.
				return *this;
			}
			unsigned __int64* tempPtr = (unsigned __int64*)this->_WritePointer;
			*tempPtr = value;
			this->MoveWritePtr(sizeof(value));
			return *this;
		}
		inline Packet& operator<<(double value)
		{
			if (this->_WritePointer + (sizeof(value) - 1) >= this->_End)
			{
				//저장이 불가능한경우.
				return *this;
			}
			double* tempPtr = (double*)this->_WritePointer;
			*tempPtr = value;
			this->MoveWritePtr(sizeof(value));
			return *this;
		}
	protected:
		DWORD _EncodeFlag;
		
		unsigned char* _Begin;
		unsigned char* _End;
		unsigned char* _WritePointer;
		unsigned char* _ReadPointer;

		int _RefCount;

		int _BufferSize;
	};
}


#endif // !__SERIALIZEING_BUFFER_HEADER__