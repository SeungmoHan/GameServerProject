#include "MakePacketMessage.h"

///__univ_developer_packet_functions_

namespace univ_dev
{
	/*
	BYTE	byCode;			// 패킷코드 0x89 고정.
	BYTE	bySize;			// 패킷 사이즈.
	BYTE	byType;			// 패킷타입.
*/
//void MakePacketStartMove(PacketHeader& header, CS_PacketMoveStart& packet, BYTE direction, unsigned short x, unsigned short y)
//{
//	header.code = 0x89;
//	header.packetType = dfPACKET_CS_MOVE_START;
//	header.payloadSize = sizeof(CS_PacketMoveStart);
//	
//	packet.direction = direction;
//	packet.x = x;
//	packet.y = y;
//}
//
//void MakePacketStopMove(PacketHeader& header, CS_PacketMoveStop& packet, BYTE direction, unsigned short x, unsigned short y)
//{
//	header.code = 0x89;
//	header.packetType = dfPACKET_CS_MOVE_STOP;
//	header.payloadSize = sizeof(CS_PacketMoveStop);
//
//	packet.direction = direction;
//	packet.x = x;
//	packet.y = y;
//}
//
//void MakePacketAttack1(PacketHeader& header, CS_PacketAttack1& packet, BYTE direction, unsigned short x, unsigned short y)
//{
//	header.code = 0x89;
//	header.packetType = dfPACKET_CS_ATTACK1;
//	header.payloadSize = sizeof(CS_PacketAttack1);
//
//	packet.direction = direction;
//	packet.x = x;
//	packet.y = y;
//}
//
//void MakePacketAttack2(PacketHeader& header, CS_PacketAttack2& packet, BYTE direction, unsigned short x, unsigned short y)
//{
//	header.code = 0x89;
//	header.packetType = dfPACKET_CS_ATTACK2;
//	header.payloadSize = sizeof(CS_PacketAttack2);
//
//	packet.direction = direction;
//	packet.x = x;
//	packet.y = y;
//}
//
//void MakePacketAttack3(PacketHeader& header, CS_PacketAttack3& packet, BYTE direction, unsigned short x, unsigned short y)
//{
//	header.code = 0x89;
//	header.packetType = dfPACKET_CS_ATTACK3;
//	header.payloadSize = sizeof(CS_PacketAttack3);
//
//	packet.direction = direction;
//	packet.x = x;
//	packet.y = y;
//}

	void MakePacketStartMove(Packet& packet, BYTE direction, unsigned short x, unsigned short y)
	{
		BYTE code = 0x89;
		BYTE payloadSize = 5;
		BYTE packetType = dfPACKET_CS_MOVE_START;

		packet << code << payloadSize << packetType << direction << x << y;
	}

	void MakePacketStopMove(Packet& packet, BYTE direction, unsigned short x, unsigned short y)
	{
		BYTE code = 0x89;
		BYTE payloadSize = 5;
		BYTE packetType = dfPACKET_CS_MOVE_STOP;

		packet << code << payloadSize << packetType << direction << x << y;
	}

	void MakePacketAttack1(Packet& packet, BYTE direction, unsigned short x, unsigned short y)
	{
		BYTE code = 0x89;
		BYTE payloadSize = 5;
		BYTE packetType = dfPACKET_CS_ATTACK1;

		packet << code << payloadSize << packetType << direction << x << y;
	}

	void MakePacketAttack2(Packet& packet, BYTE direction, unsigned short x, unsigned short y)
	{
		BYTE code = 0x89;
		BYTE packetType = dfPACKET_CS_ATTACK2;
		BYTE payloadSize = 5;

		packet << code << payloadSize << packetType << direction << x << y;
	}

	void MakePacketAttack3(Packet& packet, BYTE direction, unsigned short x, unsigned short y)
	{
		BYTE code = 0x89;
		BYTE payloadSize = 5;
		BYTE packetType = dfPACKET_CS_ATTACK3;

		packet << code << payloadSize << packetType << direction << x << y;
	}

}
