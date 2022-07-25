#pragma once
#ifndef __CHAT_SERVER__
#define __CHAT_SERVER__
#define __UNIV_DEVELOPER_

#include "CNetServer.h"
#include "JobMessage.h"
#include "Player.h"
#include "CommonProtocol.h"
#include "HardWareMoniteringClass.h"
#include "ProcessMoniteringClass.h"
#include "SS_MoniteringProtocol.h"


#include <unordered_map>
#include <unordered_set>
#include <list>

namespace univ_dev
{
	class ChatServer : public CNetServer
	{
	private:
		constexpr static int SECTOR_X_SIZE = 50;
		constexpr static int SECTOR_Y_SIZE = 50;
		constexpr static int ID_MAX_LEN = 20;
		constexpr static int ID_MAX_SIZE = 40;
		constexpr static int NICK_NAME_MAX_LEN = 20;
		constexpr static int NICK_NAME_MAX_SIZE = 40;
		constexpr static int TOKEN_KEY_SIZE = 64;

		constexpr static int ServerType = SERVER_TYPE::CHAT_SERVER;


		friend unsigned __stdcall UpdateThread(void* param);
		friend unsigned __stdcall MonitoringThread(void* param);


		unsigned int ChatServerUpdateThread(void* param);
		unsigned int ChatServerMonitoringThread(void* param);

		void OnRecv(ULONGLONG sessionID, Packet* recvPacket) final;
		void OnErrorOccured(DWORD errorCode, const WCHAR* error) final;
		bool OnConnectionRequest(WCHAR* ipStr, DWORD ip, USHORT port) final;
		void OnClientJoin(WCHAR* ipStr, DWORD ip, USHORT port, ULONGLONG sessionID) final;
		void OnClientLeave(ULONGLONG sessionID) final;
		void OnTimeOut(ULONGLONG sessionID) final;
		void OnSend(ULONGLONG sessionID) final {};



		void PacketProc(Packet* packet, ULONGLONG sessionID, WORD type);

		void PacketProcRequestLogin(Packet* packet, ULONGLONG sessionID);
		void PacketProcMoveSector(Packet* packet, ULONGLONG sessionID);
		void PacketProcChatRequire(Packet* packet, ULONGLONG sessionID);
		void PacketProcHeartBeating(Packet* packet, ULONGLONG sessionID);

		inline void MakePacketResponseLogin(Packet* packet, INT64 accountNo, BYTE status)
		{
			WORD type = PACKET_TYPE::en_PACKET_CS_CHAT_RES_LOGIN;
			(*packet) << type << status << accountNo;
		}
		inline void MakePacketResponseMoveSector(Packet* packet, INT64 accountNo, WORD sectorX, WORD sectorY)
		{
			WORD type = PACKET_TYPE::en_PACKET_CS_CHAT_RES_SECTOR_MOVE;
			(*packet) << type << accountNo << sectorX << sectorY;
		}
		inline void MakePacketResponseMessage(Packet* packet, INT64 accountNo, const WCHAR* ID, const WCHAR* nickName, WORD messageLen, const WCHAR* message)
		{
			WORD type = PACKET_TYPE::en_PACKET_CS_CHAT_RES_MESSAGE;
			(*packet) << type << accountNo;
			packet->PutBuffer((char*)ID, 40);
			packet->PutBuffer((char*)nickName, 40);
			(*packet) << messageLen;
			packet->PutBuffer((char*)message, messageLen);
		}
		//en_PACKET_CS_MONITOR_TOOL_DATA_UPDATE
		inline void MakePacketMoniteringInfo(Packet* packet, BYTE serverNo, BYTE dataType, int dataValue, int timeStamp)
		{
			WORD type = PACKET_TYPE::en_PACKET_SS_MONITOR_DATA_UPDATE;
			(*packet) << type << serverNo << dataType << dataValue << timeStamp;
		}


		inline void InsertPlayer(ULONGLONG sessionID, Player* player)
		{
			this->_PlayerMap.emplace(std::make_pair(sessionID, player));
		}
		void RemovePlayer(ULONGLONG sessionID);
		inline Player* FindPlayer(ULONGLONG sessionID)
		{
			auto iter = this->_PlayerMap.find(sessionID);
			if (iter == this->_PlayerMap.end())
				return nullptr;
			return iter->second;
		}


		LogClass								_ChatServerLog;

		void Start();
		void Close();
	public:
		ChatServer();
		~ChatServer();
		void ChatServerInit();
		bool GetRunningFlag() { return _RunningFlag; }
	private:
		volatile ULONGLONG						_TotalUpdateCount = 0;

		HardWareMoniter							_HardWareMoniter;
		ProcessMoniter							_ProcessMoniter;

		volatile ULONGLONG						_LeaveSessionCount;
		volatile DWORD							_RunningFlag;

		volatile HANDLE							_UpdateThread;
		volatile HANDLE							_MonitoringThread;

		LockFreeQueue<JobMessage>				_JobQueue;
		HANDLE									_DequeueEvent;
		LockFreeMemoryPool<Player>				_PlayerPool;
		LockFreeMemoryPoolTLS<JobMessage>		_JobMessagePool;

		std::unordered_set<Player*>**			_Sector;
		std::unordered_map<ULONGLONG,Player*>	_PlayerMap;

		ULONGLONG _SectorMoveTPS;
		ULONGLONG _ChatTPS;
		ULONGLONG _LoginTPS;
	};
}


#endif // !__CHAT_SERVER__
