#pragma once
#ifndef __PACKET_FUNCTION_HEADER__
#define __PACKET_FUNCTION_HEADER__
#define __UNIV_DEVELOPER_
#include "PacketDefine.h"

namespace univ_dev
{
	using BYTE = unsigned char;

	class Packet;

	void MakePacketCreateNewPlayer(Packet& packet,unsigned int playerID, BYTE direction, unsigned short x, unsigned short y, BYTE HP);

	void MakePacketCreatePlayer(Packet& packet, unsigned int playerID, BYTE direction, unsigned short x, unsigned short y, BYTE HP);

	void MakePacketDeletePlayer(Packet& packet, unsigned int playerID);

	void MakePacketPlayerMoveStart(Packet& packet, unsigned int playerID, BYTE moveDirection, unsigned short x, unsigned short y);

	void MakePacketPlayerMoveStop(Packet& packet, unsigned int playerID, BYTE direction, unsigned short x, unsigned short y);

	void MakePacketPlayerAttack1(Packet& packet, unsigned int playerID, BYTE direction, unsigned short x, unsigned short y);

	void MakePacketPlayerAttack2(Packet& packet, unsigned int playerID, BYTE direction, unsigned short x, unsigned short y);

	void MakePacketPlayerAttack3(Packet& packet, unsigned int playerID, BYTE direction, unsigned short x, unsigned short y);

	void MakePacketPlayerHitDamage(Packet& packet, unsigned int attackPlayerID, unsigned int damagedPlayerID, BYTE damageHP);

	void MakePacketPositionSync(Packet& packet, unsigned int playerID, unsigned short x, unsigned short y);

	void MakePacketEcho(Packet& packet, int time);

}

#endif // !__PACKET_FUNCTION_HEADER__
