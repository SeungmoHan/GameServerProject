#pragma once 
#ifndef __CREATE_PACKET_MESSAGE_HEADER__
#define __CREATE_PACKET_MESSAGE_HEADER__
#define __UNIV_DEVELOPER_
#include "SerializingPacket.h"
#include "PacketDefine.h"
namespace univ_dev
{
	void MakePacketStartMove(PacketHeader& header, CS_PacketMoveStart& packet, BYTE direction, unsigned short x, unsigned short y);
	void MakePacketStopMove(PacketHeader& header, CS_PacketMoveStop& packet, BYTE direction, unsigned short x, unsigned short y);
	void MakePacketAttack1(PacketHeader& header, CS_PacketAttack1& packet, BYTE direction, unsigned short x, unsigned short y);
	void MakePacketAttack2(PacketHeader& header, CS_PacketAttack2& packet, BYTE direction, unsigned short x, unsigned short y);
	void MakePacketAttack3(PacketHeader& header, CS_PacketAttack3& packet, BYTE direction, unsigned short x, unsigned short y);

	void MakePacketStartMove(Packet& packet, BYTE direction, unsigned short x, unsigned short y);
	void MakePacketStopMove(Packet& packet, BYTE direction, unsigned short x, unsigned short y);
	void MakePacketAttack1(Packet& packet, BYTE direction, unsigned short x, unsigned short y);
	void MakePacketAttack2(Packet& packet, BYTE direction, unsigned short x, unsigned short y);
	void MakePacketAttack3(Packet& packet, BYTE direction, unsigned short x, unsigned short y);
}

#endif // !__CREATE_PACKET_MESSAGE_HEADER__