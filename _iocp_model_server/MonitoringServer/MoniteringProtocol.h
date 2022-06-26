#pragma once
#ifndef __MONITERING_PROTOCOL_DEF__
#define __MONITERING_PROTOCOL_DEF__
#define __UNIV_DEVELOPER_


enum ServerType
{
	GameServer = 1,
	ChatServer = 2,
	LoginServer = 3,
	ServerMax = 4,
};

enum GameServerMoniteringType
{
	OnOffFlag = 1,
	CPUUsage = 2,
	PrivateBytes = 3,
	SessionCounts = 4,
	AuthPlayer = 5,
	GamePlayer = 6,
	AcceptTPS = 7,
	PacketRecvTPS = 8,
	PacketSendTPS = 9,
	DBWriteTPS = 10,
	DBMessageQueueSize = 11,
	AuthThreadFPS = 12,
	GameThreadFPS = 13,
	PacketPoolUsage = 14,
};

enum ChatServerMoniteringType
{
	OnOffFlag = 1,
	CPUUsage = 2,
	PrivateBytes = 3,
	SessionCounts = 4,
	PlayerCounts = 5,
	UpdateTPS = 6,
	PacketPoolUsage = 7,
	MSGQueueSize = 8,
};

enum LoginServerMoniteringType
{
	OnOffFlag = 1,
	CPUUsage = 2,
	PrivateBytes = 3,
	SessionCounts = 4,
	AuthTPS = 5,
	PacketPoolUsage = 6,
};

#endif // !__MONITERING_PROTOCOL_DEF__
