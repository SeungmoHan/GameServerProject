#pragma once
#ifndef __MonitorING_SERVER__
#define __MonitorING_SERVER__
#define __UNIV_DEVELOPER_
#include "CNetServer.h"
#include "CLanServer.h"
#include "CommonProtocol.h"
#include "SS_MoniteringProtocol.h"
#include "DBConnector.h"
#include <vector>
#include <set>

namespace univ_dev
{
	class MonitoringServer final : public CLanServer , public CNetServer
	{
	public:
		MonitoringServer();
		~MonitoringServer()
		{
			Close();
		}
		void InitMonitoringServer();
		
		// test해보려고 만든거

	private:

		void InitNetServer();
		void InitLanServer();
		void InitDataBase();
		void Disconnect(ULONGLONG sessionID);

		constexpr static int MAX_VALUE = 0x7fffffff;
		constexpr static int MIN_VALUE = 0;
		BYTE MESSAGE_TYPE_TABLE[10][20]
		{
			{0}, // 아무것도 없는구간임
			{
				0,
				dfMONITOR_DATA_TYPE_GAME_SERVER_RUN,
				dfMONITOR_DATA_TYPE_GAME_SERVER_CPU,
				dfMONITOR_DATA_TYPE_GAME_SERVER_MEM,
				dfMONITOR_DATA_TYPE_GAME_SESSION,
				dfMONITOR_DATA_TYPE_GAME_AUTH_PLAYER,
				dfMONITOR_DATA_TYPE_GAME_GAME_PLAYER,
				dfMONITOR_DATA_TYPE_GAME_ACCEPT_TPS,
				dfMONITOR_DATA_TYPE_GAME_PACKET_RECV_TPS,
				dfMONITOR_DATA_TYPE_GAME_PACKET_SEND_TPS,
				dfMONITOR_DATA_TYPE_GAME_DB_WRITE_TPS,
				dfMONITOR_DATA_TYPE_GAME_DB_WRITE_MSG,
				dfMONITOR_DATA_TYPE_GAME_AUTH_THREAD_FPS,
				dfMONITOR_DATA_TYPE_GAME_GAME_THREAD_FPS,
				dfMONITOR_DATA_TYPE_GAME_PACKET_POOL
			}, // game server
			{
				0,
				dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN,
				dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU,
				dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM,
				dfMONITOR_DATA_TYPE_CHAT_SESSION,
				dfMONITOR_DATA_TYPE_CHAT_PLAYER,
				dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS,
				dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL,
				dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL
			}, // Chatting Server
			{
				0,
				dfMONITOR_DATA_TYPE_LOGIN_SERVER_RUN,
				dfMONITOR_DATA_TYPE_LOGIN_SERVER_CPU,
				dfMONITOR_DATA_TYPE_LOGIN_SERVER_MEM,
				dfMONITOR_DATA_TYPE_LOGIN_SESSION,
				dfMONITOR_DATA_TYPE_LOGIN_AUTH_TPS,
				dfMONITOR_DATA_TYPE_LOGIN_PACKET_POOL, 
			}, // Login Server
		};
		friend unsigned __stdcall MonitoringThread(void *param);
		unsigned int MonitoringThreadProc();

		HANDLE									_MonitoringThread;

		void PacketProc(Packet* packet, ULONGLONG sessionID,WORD type);

		
		void PacketProcMonitorToolLogin(Packet* packet, ULONGLONG sessionID);
		void PacketProcMonitorDataUpdate(Packet* packet, ULONGLONG sessionID);

		void MakePacketMonitorDataUpdate(Packet* packet, BYTE serverNo, BYTE dataType, int dataValue, int timeStamp);

		void QueryHardWareData();
		void QueryGameServerData();
		void QueryChatServerData();
		void QueryLoginServerData();

		void ResetHardWareData();
		void ResetGameServerData();
		void ResetChatServerData();
		void ResetLoginServerData();

		void UpdateServerData(BYTE serverType, BYTE messageType, int data,int timeStamp);
		void UpdateLoginServerData(BYTE messageType, int data, int timeStamp);
		void UpdateChatServerData(BYTE messageType, int data, int timeStamp);
		void UpdateGameServerData(BYTE messageType, int data, int timeStamp);

		void Start();
		void Close();

		std::set<ULONGLONG>						_MoniteringSessionIDList;

		constexpr static int					SESSION_KEY_SIZE = 32;
		char									_MonitoringSessionKey[33]{ "ajfw@!cv980dSZ[fje#@fdj123948djf" };
		ULONGLONG								_MonitoringSessionID;

		HardWareMonitor							_HardWareMonitor;
		ProcessMonitor							_ProcessMonitor;
		DWORD									_RunningFlag;
		LogClass								_MonitoringLog;

		ULONGLONG								_LastRecvBytes = 0;
		ULONGLONG								_LastSendBytes = 0;

		void UpdateHardwareData();
		void QueryDataBase(int serverNo, const char* dataType, int min, int max, int avr);

		constexpr static int					TOTAL = 0;
		constexpr static int					MIN = 1;
		constexpr static int					MAX = 2;
		constexpr static int					TICK = 3;
		char QUERY_FORMAT[50][256]{
			""
			"INSERT INTO `LOGDB`.`M_MONITORLOG_%.2d_%.2d` (`logtime`,`serverno`,`type`,`min`,`max`,`avr`) values(now(),%d,\"%s\",%d,%d,%d)",
			"CREATE TABLE `LOGDB`.`M_MONITORLOG_%.2d_%.2d` LIKE `logdb`.`monitorlog_template`",

			"INSERT INTO `LOGDB`.`G_MONITORLOG_%.2d_%.2d` (`logtime`,`serverno`,`type`,`min`,`max`,`avr`) values(now(),%d,\"%s\",%d,%d,%d)",
			"CREATE TABLE `LOGDB`.`G_MONITORLOG_%.2d_%.2d` LIKE `logdb`.`monitorlog_template`",
			
			"INSERT INTO `LOGDB`.`C_MONITORLOG_%.2d_%.2d` (`logtime`,`serverno`,`type`,`min`,`max`,`avr`) values(now(),%d,\"%s\",%d,%d,%d)",
			"CREATE TABLE `LOGDB`.`C_MONITORLOG_%.2d_%.2d` LIKE `logdb`.`monitorlog_template`",
			
			"INSERT INTO `LOGDB`.`L_MONITORLOG_%.2d_%.2d` (`logtime`,`serverno`,`type`,`min`,`max`,`avr`) values(now(),%d,\"%s\",%d,%d,%d)",
			"CREATE TABLE `LOGDB`.`L_MONITORLOG_%.2d_%.2d` LIKE `logdb`.`monitorlog_template`"
			""
		};

		//---------------------------------------------------------------------
		//HARD WARE DATA

		ULONGLONG								_Template[4]			= { 0,MAX_VALUE,MIN_VALUE,0 };

		ULONGLONG								_H_CPU[4]				= { 0,MAX_VALUE,MIN_VALUE,0 };
		ULONGLONG								_H_AvailableMemory[4]	= { 0,MAX_VALUE,MIN_VALUE,0 };
		ULONGLONG								_H_NonpagedMemory[4]	= { 0,MAX_VALUE,MIN_VALUE,0 };
		ULONGLONG								_H_NetworkSend[4]		= { 0,MAX_VALUE,MIN_VALUE,0 };
		ULONGLONG								_H_NetworkRecv[4]		= { 0,MAX_VALUE,MIN_VALUE,0 };
		//---------------------------------------------------------------------

		//---------------------------------------------------------------------
		//GAME SERVER DATA
		DWORD									_G_RunningFlag				= false;
		ULONGLONG								_G_CPU[4]				= { 0,MAX_VALUE,MIN_VALUE,0 };
		ULONGLONG								_G_PrivateBytes[4]		= { 0,MAX_VALUE,MIN_VALUE,0 };
		ULONGLONG								_G_SessionCount[4]		= { 0,MAX_VALUE,MIN_VALUE,0 };
		ULONGLONG								_G_AuthCount[4]			= { 0,MAX_VALUE,MIN_VALUE,0 };
		ULONGLONG								_G_PlayerCount[4]		= { 0,MAX_VALUE,MIN_VALUE,0 };
		ULONGLONG								_G_AcceptTPS[4]			= { 0,MAX_VALUE,MIN_VALUE,0 };
		ULONGLONG								_G_RecvTPS[4]			= { 0,MAX_VALUE,MIN_VALUE,0 };
		ULONGLONG								_G_SendTPS[4]			= { 0,MAX_VALUE,MIN_VALUE,0 };
		ULONGLONG								_G_DBWrite[4]			= { 0,MAX_VALUE,MIN_VALUE,0 };
		ULONGLONG								_G_DBQueue[4]			= { 0,MAX_VALUE,MIN_VALUE,0 };
		ULONGLONG								_G_AuthFPS[4]			= { 0,MAX_VALUE,MIN_VALUE,0 };
		ULONGLONG								_G_GameFPS[4]			= { 0,MAX_VALUE,MIN_VALUE,0 };
		ULONGLONG								_G_PacketPoolSize[4]	= { 0,MAX_VALUE,MIN_VALUE,0 };
		//---------------------------------------------------------------------

		//---------------------------------------------------------------------
		//CHAT SERVER DATA
		DWORD									_C_RunningFlag				= false;
		ULONGLONG								_C_CPU[4]				= { 0,MAX_VALUE,MIN_VALUE,0 };
		ULONGLONG								_C_PrivateBytes[4]		= { 0,MAX_VALUE,MIN_VALUE,0 };
		ULONGLONG								_C_PacketPoolSize[4]	= { 0,MAX_VALUE,MIN_VALUE,0 };
		ULONGLONG								_C_SessionCount[4]		= { 0,MAX_VALUE,MIN_VALUE,0 };
		ULONGLONG								_C_PlayerCount[4]		= { 0,MAX_VALUE,MIN_VALUE,0 };
		ULONGLONG								_C_UpdateTPS[4]			= { 0,MAX_VALUE,MIN_VALUE,0 };
		ULONGLONG								_C_JobQueue[4]			= { 0,MAX_VALUE,MIN_VALUE,0 };
		//---------------------------------------------------------------------

		//---------------------------------------------------------------------
		//CHAT SERVER DATA
		DWORD									_L_RunningFlag				= false;
		ULONGLONG								_L_CPU[4]				= { 0,MAX_VALUE,MIN_VALUE,0 };
		ULONGLONG								_L_PrivateBytes[4]		= { 0,MAX_VALUE,MIN_VALUE,0 };
		ULONGLONG								_L_PacketPoolSize[4]	= { 0,MAX_VALUE,MIN_VALUE,0 };
		ULONGLONG								_L_SessionCount[4]		= { 0,MAX_VALUE,MIN_VALUE,0 };
		ULONGLONG								_L_AuthTPS[4]			= { 0,MAX_VALUE,MIN_VALUE,0 };
		//---------------------------------------------------------------------


		DBConnector*							_Database;


		void OnRecv(ULONGLONG sessionID, Packet* recvPacket) override;
		void OnErrorOccured(DWORD errorCode, const WCHAR* error) override;
		bool OnConnectionRequest(WCHAR* ipStr, DWORD ip, USHORT port) override;
		void OnClientJoin(WCHAR* ipStr, DWORD ip, USHORT port, ULONGLONG sessionID) override;
		void OnClientLeave(ULONGLONG sessionID) override; // Release후 호출
		void OnTimeOut(ULONGLONG sessionID) override;
	};
}



#endif // !__MonitorING_SERVER__
