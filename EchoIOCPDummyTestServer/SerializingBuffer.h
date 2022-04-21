#pragma once
#ifndef __SERIALIZEING_BUFFER_HEADER__
#define __SERIALIZEING_BUFFER_HEADER__
#define __UNIV_DEVELOPER_

namespace univ_dev
{
#pragma pack(push,1)
	struct LanServerPacket
	{
		unsigned short _Len;
	};
	struct NetServerPacket
	{
		unsigned char _ByteCode;
		unsigned short _Len;
		unsigned char _RandomKey;
		unsigned char _CheckSum;
	};
#pragma pack(pop)

#define LAN_HEADER_SIZE (short)sizeof(LanServerPacket)
#define NET_HEADER_SIZE (short)sizeof(NetServerPacket)

	class Packet
	{
	protected:
		static constexpr int MSS = 1460;
		static constexpr int PACKET_CODE = 0x77;
		static constexpr int FIXED_KEY = 0xa9;

		friend class CLanServer;
		friend class CNetServer;
		enum PacketSize
		{
			DefaultSize = MSS
		};
	public:
		Packet();
		Packet(int bufferSize);

		virtual ~Packet();
	protected:
		void Release();

		void Clear();

		int GetBufferSize();
		char* GetReadPtr();
		char* GetWritePtr();
		char* GetBeginPtr();
		void AddRef();
		bool SubRef();

		void SetLanHeader();
		void SetNetHeader();
		void Encode();
		void Decode();
		bool VerifyCheckSum();

		int GetRefCount() { return _RefCount; }

		int MoveWritePtr(int size);
		int MoveReadPtr(int size);
	public:

		static int GetUseCount();
		static int GetTotalPacketCount();
		static int GetCapacityCount();
		static Packet* Alloc();
		static void Free(Packet* packet);

		void PutString(const char* str, int size);
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
		char* _Begin;
		char* _End;
		char* _WritePointer;
		char* _ReadPointer;

		int _RefCount;

		int _BufferSize;
	};
}


#endif // !__SERIALIZEING_BUFFER_HEADER__