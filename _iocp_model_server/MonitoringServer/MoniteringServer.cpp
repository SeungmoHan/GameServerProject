#include "MonitoringServer.h"
#include <time.h>
#include <process.h>

namespace univ_dev
{
	unsigned __stdcall MonitoringThread(void* param)
	{
		MonitoringServer* server = (MonitoringServer*)param;
		if (server == nullptr)
			CRASH();
		return server->MonitoringThreadProc();
	}

	MonitoringServer::MonitoringServer() : _MonitoringSessionID(0), _RunningFlag(0)
	{
	}


	void MonitoringServer::InitMonitoringServer()
	{
		this->Start();
	}




	unsigned int MonitoringServer::MonitoringThreadProc()
	{
		this->_LastRecvBytes = 0;
		this->_LastSendBytes = 0;

		DWORD prev;
		DWORD cur;
		cur = prev = timeGetTime();

		while (this->_RunningFlag)
		{
			Sleep(999);
			this->_HardWareMonitor.UpdateHardWareTime();
			this->_ProcessMonitor.UpdateProcessTime();
			cur = timeGetTime();

			this->UpdateHardwareData();
			//if(false)
			if ((cur - prev) >= 60000)
			{
				prev = cur;
				this->QueryHardWareData();
				if (this->_G_RunningFlag)
					this->QueryGameServerData();
				if (this->_L_RunningFlag)
					this->QueryLoginServerData();
				if (this->_C_RunningFlag)
					this->QueryChatServerData();

				this->_C_RunningFlag = this->_G_RunningFlag = this->_L_RunningFlag = false;
			}
		}
		return 0;
	}

	void MonitoringServer::QueryDataBase(int serverNo, const char* dataType, int min, int max, int avr)
	{
		time_t now = time(nullptr);
		tm t;
		localtime_s(&t, &now);
		char query[512];
		sprintf_s(query, QUERY_FORMAT[(serverNo << 1)], t.tm_year % 100, t.tm_mon + 1, serverNo, dataType, min, max, avr);
		if (!this->_Database->Query(query))
		{
			char newTableQuery[256];
			sprintf_s(newTableQuery, QUERY_FORMAT[(serverNo << 1) + 1], t.tm_year % 100, t.tm_mon + 1);
			this->_Database->Query(newTableQuery);
			this->_Database->Query(query);
		}
	}

	void MonitoringServer::QueryHardWareData()
	{
		WCHAR errStr[512];
		if (this->_H_CPU[this->TICK] > 30)
		{
			int cpuAvr = this->_H_CPU[this->TOTAL] / this->_H_CPU[this->TICK];
			this->QueryDataBase(SERVER_TYPE::MonitorING_SERVER, "Server CPU",
				this->_H_CPU[this->MIN], this->_H_CPU[this->MAX], cpuAvr);
		}
		else
		{
			wsprintf(errStr, L"Hardware CPU : Query Tick under 30, %I64u", this->_H_CPU[this->TICK]);
			this->_MonitoringLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
		}
		if (this->_H_AvailableMemory[this->TICK] > 30)
		{
			int availableMemAvr = this->_H_AvailableMemory[this->TOTAL] / this->_H_AvailableMemory[this->TICK];
			this->QueryDataBase(SERVER_TYPE::MonitorING_SERVER, "Server Private MBytes",
				this->_H_AvailableMemory[this->MIN], this->_H_AvailableMemory[this->MAX], availableMemAvr);
		}
		else
		{
			wsprintf(errStr, L"Hardware Available Memory MBytes : Query Tick under 30, %I64u", this->_H_AvailableMemory[this->TICK]);
			this->_MonitoringLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
		}
		if (this->_H_NonpagedMemory[this->TICK])
		{
			int npPoolAvr = this->_H_NonpagedMemory[this->TOTAL] / this->_H_NonpagedMemory[this->TICK];
			this->QueryDataBase(SERVER_TYPE::MonitorING_SERVER, "Server Nonpaged MBytes",
				this->_H_NonpagedMemory[this->MIN], this->_H_NonpagedMemory[this->MAX], npPoolAvr);
		}
		else
		{
			wsprintf(errStr, L"Hardware NPPool MBytes : Query Tick under 30, %I64u", this->_H_NonpagedMemory[this->TICK]);
			this->_MonitoringLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
		}
		if (this->_H_NetworkSend[this->TICK] > 30)
		{
			int netSendAvr = this->_H_NetworkSend[this->TOTAL] / this->_H_NetworkSend[this->TICK];
			this->QueryDataBase(SERVER_TYPE::MonitorING_SERVER, "Network Send KBytes",
				this->_H_NetworkSend[this->MIN], this->_H_NetworkSend[this->MAX], netSendAvr);
		}
		else
		{
			wsprintf(errStr, L"Hardware Network Send KBytes : Query Tick under 30, %I64u", this->_H_NetworkSend[this->TICK]);
			this->_MonitoringLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
		}
		if (this->_H_NetworkRecv[this->TICK] > 30)
		{
			int netRecvAvr = this->_H_NetworkRecv[this->TOTAL] / this->_H_NetworkRecv[this->TICK];
			this->QueryDataBase(SERVER_TYPE::MonitorING_SERVER, "Network Recv KBytes",
				this->_H_NetworkRecv[this->MIN], this->_H_NetworkRecv[this->MAX], netRecvAvr);
		}
		else
		{
			wsprintf(errStr, L"Hardware Network Recv KBytes : Query Tick under 30, %I64u", this->_H_NetworkRecv[this->TICK]);
			this->_MonitoringLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
		}
		ResetHardWareData();
	}

	void MonitoringServer::QueryGameServerData()
	{
		WCHAR errStr[512];
		int avr = 0;
		if (this->_G_CPU[this->TICK] > 30)
		{
			avr = this->_G_CPU[this->TOTAL] / this->_G_CPU[this->TICK];
			this->QueryDataBase(SERVER_TYPE::GAME_SERVER, "CPU",
				this->_G_CPU[this->MIN], this->_G_CPU[this->MAX], avr);
		}
		else
		{
			wsprintf(errStr, L"Game CPU : Query Tick under 30, %I64u", this->_G_CPU[this->TICK]);
			this->_MonitoringLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
		}

		if (this->_G_PrivateBytes[this->TICK] > 30)
		{
			avr = this->_G_PrivateBytes[this->TOTAL] / this->_G_PrivateBytes[this->TICK];
			this->QueryDataBase(SERVER_TYPE::GAME_SERVER, "Private MBytes",
				this->_G_PrivateBytes[this->MIN], this->_G_PrivateBytes[this->MAX], avr);
		}
		else
		{
			wsprintf(errStr, L"Game Private MBytes : Query Tick under 30, %I64u", this->_G_PrivateBytes[this->TICK]);
			this->_MonitoringLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
		}

		if (this->_G_SessionCount[this->TICK] > 30)
		{
			int avr = this->_G_SessionCount[this->TOTAL] / this->_G_SessionCount[this->TICK];
			this->QueryDataBase(SERVER_TYPE::GAME_SERVER, "Session Count",
				this->_G_SessionCount[this->MIN], this->_G_SessionCount[this->MAX], avr);
		}
		else
		{
			wsprintf(errStr, L"Game Session Counts : Query Tick under 30, %I64u", this->_G_SessionCount[this->TICK]);
			this->_MonitoringLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
		}

		if (this->_G_AuthCount[this->TICK] > 30)
		{
			int avr = this->_G_AuthCount[this->TOTAL] / this->_G_AuthCount[this->TICK];
			this->QueryDataBase(SERVER_TYPE::GAME_SERVER, "Auth Count",
				this->_G_AuthCount[this->MIN], this->_G_AuthCount[this->MAX], avr);
		}
		else
		{
			wsprintf(errStr, L"Game Auth Count : Query Tick under 30, %I64u", this->_G_AuthCount[this->TICK]);
			this->_MonitoringLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
		}

		if (this->_G_PlayerCount[this->TICK] > 30)
		{
			avr = this->_G_PlayerCount[this->TOTAL] / this->_G_PlayerCount[this->TICK];
			this->QueryDataBase(SERVER_TYPE::GAME_SERVER, "Player Count",
				this->_G_PlayerCount[this->MIN], this->_G_PlayerCount[this->MAX], avr);
		}
		else
		{
			wsprintf(errStr, L"Game Player Count : Query Tick under 30, %I64u", this->_G_PlayerCount[this->TICK]);
			this->_MonitoringLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
		}

		if (this->_G_AcceptTPS[this->TICK] > 30)
		{
			avr = this->_G_AcceptTPS[this->TOTAL] / this->_G_AcceptTPS[this->TICK];
			this->QueryDataBase(SERVER_TYPE::GAME_SERVER, "Accept TPS",
				this->_G_AcceptTPS[this->MIN], this->_G_AcceptTPS[this->MAX], avr);
		}
		else
		{
			wsprintf(errStr, L"Game Accept TPS : Query Tick under 30, %I64u", this->_G_AcceptTPS[this->TICK]);
			this->_MonitoringLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
		}

		if (this->_G_RecvTPS[this->TICK] > 30)
		{
			avr = this->_G_RecvTPS[this->TOTAL] / this->_G_RecvTPS[this->TICK];
			this->QueryDataBase(SERVER_TYPE::GAME_SERVER, "Recv TPS",
				this->_G_RecvTPS[this->MIN], this->_G_RecvTPS[this->MAX], avr);
		}
		else
		{
			wsprintf(errStr, L"Game Recv TPS : Query Tick under 30, %I64u", this->_G_RecvTPS[this->TICK]);
			this->_MonitoringLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
		}

		if (this->_G_SendTPS[this->TICK] > 30)
		{
			avr = this->_G_SendTPS[this->TOTAL] / this->_G_SendTPS[this->TICK];
			this->QueryDataBase(SERVER_TYPE::GAME_SERVER, "Send TPS",
				this->_G_SendTPS[this->MIN], this->_G_SendTPS[this->MAX], avr);
		}
		else
		{
			wsprintf(errStr, L"Game Send TPS : Query Tick under 30, %I64u", this->_G_SendTPS[this->TICK]);
			this->_MonitoringLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
		}

		if (this->_G_DBWrite[this->TICK] > 30)
		{
			avr = this->_G_DBWrite[this->TOTAL] / this->_G_DBWrite[this->TICK];
			this->QueryDataBase(SERVER_TYPE::GAME_SERVER, "DB Wirte TPS",
				this->_G_DBWrite[this->MIN], this->_G_DBWrite[this->MAX], avr);
		}
		else
		{
			wsprintf(errStr, L"Game DBWrite TPS : Query Tick under 30, %I64u", this->_G_DBWrite[this->TICK]);
			this->_MonitoringLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
		}

		if (this->_G_DBQueue[this->TICK] > 30)
		{
			avr = this->_G_DBQueue[this->TOTAL] / this->_G_DBQueue[this->TICK];
			this->QueryDataBase(SERVER_TYPE::GAME_SERVER, "DB Queue Size",
				this->_G_DBQueue[this->MIN], this->_G_DBQueue[this->MAX], avr);
		}
		else
		{
			wsprintf(errStr, L"Game DBQueue Size TPS : Query Tick under 30, %I64u", this->_G_DBQueue[this->TICK]);
			this->_MonitoringLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
		}

		if (this->_G_AuthFPS[this->TICK] > 30)
		{
			avr = this->_G_AuthFPS[this->TOTAL] / this->_G_AuthFPS[this->TICK];
			this->QueryDataBase(SERVER_TYPE::GAME_SERVER, "Auth FPS",
				this->_G_AuthFPS[this->MIN], this->_G_AuthFPS[this->MAX], avr);
		}
		else
		{
			wsprintf(errStr, L"Game Auth FPS : Query Tick under 30, %I64u", this->_G_AuthFPS[this->TICK]);
			this->_MonitoringLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
		}

		if (this->_G_GameFPS[this->TICK] > 30)
		{
			avr = this->_G_GameFPS[this->TOTAL] / this->_G_GameFPS[this->TICK];
			this->QueryDataBase(SERVER_TYPE::GAME_SERVER, "Game Update FPS",
				this->_G_GameFPS[this->MIN], this->_G_GameFPS[this->MAX], avr);
		}
		else
		{
			wsprintf(errStr, L"Game Game FPS : Query Tick under 30, %I64u", this->_G_GameFPS[this->TICK]);
			this->_MonitoringLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
		}

		if (this->_G_PacketPoolSize[this->TICK] > 30)
		{
			avr = this->_G_PacketPoolSize[this->TOTAL] / this->_G_PacketPoolSize[this->TICK];
			this->QueryDataBase(SERVER_TYPE::GAME_SERVER, "Packet Pool Size",
				this->_G_PacketPoolSize[this->MIN], this->_G_PacketPoolSize[this->MAX], avr);
		}
		else
		{
			wsprintf(errStr, L"Game Packet Pool Size : Query Tick under 30, %I64u", this->_G_PacketPoolSize[this->TICK]);
			this->_MonitoringLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
		}
		this->ResetGameServerData();
	}

	void MonitoringServer::QueryChatServerData()
	{
		WCHAR errStr[512];
		int avr = 0;
		if (this->_C_CPU[this->TICK] > 30)
		{
			avr = this->_C_CPU[this->TOTAL] / this->_C_CPU[this->TICK];
			this->QueryDataBase(SERVER_TYPE::CHAT_SERVER, "CPU",
				this->_C_CPU[this->MIN], this->_C_CPU[this->MAX], avr);
		}
		else
		{
			wsprintf(errStr, L"Chat CPU : Query Tick under 30, %I64u", this->_C_CPU[this->TICK]);
			this->_MonitoringLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
		}

		if (this->_C_PrivateBytes[this->TICK] > 30)
		{
			avr = this->_C_PrivateBytes[this->TOTAL] / this->_C_PrivateBytes[this->TICK];
			this->QueryDataBase(SERVER_TYPE::CHAT_SERVER, "Private MBytes",
				this->_C_PrivateBytes[this->MIN], this->_C_PrivateBytes[this->MAX], avr);
		}
		else
		{
			wsprintf(errStr, L"Chat Private MBytes : Query Tick under 30, %I64u", this->_C_PrivateBytes[this->TICK]);
			this->_MonitoringLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
		}

		if (this->_C_SessionCount[this->TICK] > 30)
		{
			avr = this->_C_SessionCount[this->TOTAL] / this->_C_SessionCount[this->TICK];
			this->QueryDataBase(SERVER_TYPE::CHAT_SERVER, "Session Count",
				this->_C_SessionCount[this->MIN], this->_C_SessionCount[this->MAX], avr);
		}
		else
		{
			wsprintf(errStr, L"Chat Session Counts : Query Tick under 30, %I64u", this->_C_SessionCount[this->TICK]);
			this->_MonitoringLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
		}

		if (this->_C_PlayerCount[this->TICK] > 30)
		{
			avr = this->_C_PlayerCount[this->TOTAL] / this->_C_PlayerCount[this->TICK];
			this->QueryDataBase(SERVER_TYPE::CHAT_SERVER, "Player Count",
				this->_C_PlayerCount[this->MIN], this->_C_PlayerCount[this->MAX], avr);
		}
		else
		{
			wsprintf(errStr, L"Chat Player Counts : Query Tick under 30, %I64u", this->_C_PlayerCount[this->TICK]);
			this->_MonitoringLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
		}

		if (this->_C_UpdateTPS[this->TICK] > 30)
		{
			avr = this->_C_UpdateTPS[this->TOTAL] / this->_C_UpdateTPS[this->TICK];
			this->QueryDataBase(SERVER_TYPE::CHAT_SERVER, "Chat Update TPS",
				this->_C_UpdateTPS[this->MIN], this->_C_UpdateTPS[this->MAX], avr);
		}
		else
		{
			wsprintf(errStr, L"Chat Update TPS : Query Tick under 30, %I64u", this->_C_UpdateTPS[this->TICK]);
			this->_MonitoringLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
		}

		if (this->_C_PacketPoolSize[this->TICK] > 30)
		{
			avr = this->_C_PacketPoolSize[this->TOTAL] / this->_C_PacketPoolSize[this->TICK];
			this->QueryDataBase(SERVER_TYPE::CHAT_SERVER, "Packet Pool Size",
				this->_C_PacketPoolSize[this->MIN], this->_C_PacketPoolSize[this->MAX], avr);
		}
		else
		{
			wsprintf(errStr, L"Chat Packet Pool Size : Query Tick under 30, %I64u", this->_C_PacketPoolSize[this->TICK]);
			this->_MonitoringLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
		}

		if (this->_C_JobQueue[this->TICK] > 30)
		{
			avr = this->_C_JobQueue[this->TOTAL] / this->_C_JobQueue[this->TICK];
			this->QueryDataBase(SERVER_TYPE::CHAT_SERVER, "Job Queue Size",
				this->_C_JobQueue[this->MIN], this->_C_JobQueue[this->MAX], avr);
		}
		else
		{
			wsprintf(errStr, L"Chat Job Queue Size : Query Tick under 30, %I64u", this->_C_JobQueue[this->TICK]);
			this->_MonitoringLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
		}

		this->ResetChatServerData();
	}

	void MonitoringServer::QueryLoginServerData()
	{
		WCHAR errStr[512];
		int avr;
		if (this->_L_CPU[this->TICK] > 30)
		{
			avr = this->_L_CPU[this->TOTAL] / this->_L_CPU[this->TICK];
			this->QueryDataBase(SERVER_TYPE::LOGIN_SERVER, "CPU",
				this->_L_CPU[this->MIN], this->_L_CPU[this->MAX], avr);
		}
		else
		{
			wsprintf(errStr, L"Login CPU : Query Tick under 30, %I64u", this->_L_CPU[this->TICK]);
			this->_MonitoringLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
		}

		if (this->_L_PrivateBytes[this->TICK] > 30)
		{
			avr = this->_L_PrivateBytes[this->TOTAL] / this->_L_PrivateBytes[this->TICK];
			this->QueryDataBase(SERVER_TYPE::LOGIN_SERVER, "Private MBytes",
				this->_L_PrivateBytes[this->MIN], this->_L_PrivateBytes[this->MAX], avr);
		}
		else
		{
			wsprintf(errStr, L"Login Private MBytes : Query Tick under 30, %I64u", this->_L_PrivateBytes[this->TICK]);
			this->_MonitoringLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
		}

		if (this->_L_SessionCount[this->TICK] > 30)
		{
			avr = this->_L_SessionCount[this->TOTAL] / this->_L_SessionCount[this->TICK];
			this->QueryDataBase(SERVER_TYPE::LOGIN_SERVER, "Session Count",
				this->_L_SessionCount[this->MIN], this->_L_SessionCount[this->MAX], avr);
		}
		else
		{
			wsprintf(errStr, L"Login Session Counts : Query Tick under 30, %I64u", this->_L_SessionCount[this->TICK]);
			this->_MonitoringLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
		}

		if (this->_L_AuthTPS[this->TICK] > 30)
		{
			avr = this->_L_AuthTPS[this->TOTAL] / this->_L_AuthTPS[this->TICK];
			this->QueryDataBase(SERVER_TYPE::LOGIN_SERVER, "Auth TPS",
				this->_L_AuthTPS[this->MIN], this->_L_AuthTPS[this->MAX], avr);
		}
		else
		{
			wsprintf(errStr, L"Login Auth TPS : Query Tick under 30, %I64u", this->_L_AuthTPS[this->TICK]);
			this->_MonitoringLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
		}

		if (this->_L_PacketPoolSize[this->TICK] > 30)
		{
			avr = this->_L_PacketPoolSize[this->TOTAL] / this->_L_PacketPoolSize[this->TICK];
			this->QueryDataBase(SERVER_TYPE::LOGIN_SERVER, "Packet Pool Size",
				this->_L_PacketPoolSize[this->MIN], this->_L_PacketPoolSize[this->MAX], avr);
		}
		else
		{
			wsprintf(errStr, L"Login Packet Pool Size : Query Tick under 30, %I64u", this->_L_PacketPoolSize[this->TICK]);
			this->_MonitoringLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
		}

		ResetLoginServerData();
	}

	void MonitoringServer::ResetHardWareData()
	{
		memmove_s(this->_H_CPU, sizeof(this->_Template), this->_Template, sizeof(this->_Template));
		memmove_s(this->_H_AvailableMemory, sizeof(this->_Template), this->_Template, sizeof(this->_Template));
		memmove_s(this->_H_NonpagedMemory, sizeof(this->_Template), this->_Template, sizeof(this->_Template));
		memmove_s(this->_H_NetworkSend, sizeof(this->_Template), this->_Template, sizeof(this->_Template));
		memmove_s(this->_H_NetworkRecv, sizeof(this->_Template), this->_Template, sizeof(this->_Template));
	}

	void MonitoringServer::ResetGameServerData()
	{
		this->_G_RunningFlag = false;
		memmove_s(this->_G_CPU, sizeof(this->_Template), this->_Template, sizeof(this->_Template));
		memmove_s(this->_G_PrivateBytes, sizeof(this->_Template), this->_Template, sizeof(this->_Template));
		memmove_s(this->_G_PacketPoolSize, sizeof(this->_Template), this->_Template, sizeof(this->_Template));
		memmove_s(this->_G_AcceptTPS, sizeof(this->_Template), this->_Template, sizeof(this->_Template));
		memmove_s(this->_G_PlayerCount, sizeof(this->_Template), this->_Template, sizeof(this->_Template));
		memmove_s(this->_G_AuthCount, sizeof(this->_Template), this->_Template, sizeof(this->_Template));
		memmove_s(this->_G_AuthFPS, sizeof(this->_Template), this->_Template, sizeof(this->_Template));
		memmove_s(this->_G_GameFPS, sizeof(this->_Template), this->_Template, sizeof(this->_Template));
		memmove_s(this->_G_DBWrite, sizeof(this->_Template), this->_Template, sizeof(this->_Template));
		memmove_s(this->_G_DBQueue, sizeof(this->_Template), this->_Template, sizeof(this->_Template));
	}

	void MonitoringServer::ResetChatServerData()
	{
		this->_C_RunningFlag = false;
		memmove_s(this->_C_CPU, sizeof(this->_Template), this->_Template, sizeof(this->_Template));
		memmove_s(this->_C_PrivateBytes, sizeof(this->_Template), this->_Template, sizeof(this->_Template));
		memmove_s(this->_C_PacketPoolSize, sizeof(this->_Template), this->_Template, sizeof(this->_Template));
		memmove_s(this->_C_SessionCount, sizeof(this->_Template), this->_Template, sizeof(this->_Template));
		memmove_s(this->_C_PlayerCount, sizeof(this->_Template), this->_Template, sizeof(this->_Template));
		memmove_s(this->_C_UpdateTPS, sizeof(this->_Template), this->_Template, sizeof(this->_Template));
		memmove_s(this->_C_JobQueue, sizeof(this->_Template), this->_Template, sizeof(this->_Template));
	}

	void MonitoringServer::ResetLoginServerData()
	{
		this->_L_RunningFlag = false;
		memmove_s(this->_L_CPU, sizeof(this->_Template), this->_Template, sizeof(this->_Template));
		memmove_s(this->_L_PrivateBytes, sizeof(this->_Template), this->_Template, sizeof(this->_Template));
		memmove_s(this->_L_PacketPoolSize, sizeof(this->_Template), this->_Template, sizeof(this->_Template));
		memmove_s(this->_L_SessionCount, sizeof(this->_Template), this->_Template, sizeof(this->_Template));
		memmove_s(this->_L_AuthTPS, sizeof(this->_Template), this->_Template, sizeof(this->_Template));
	}

	void MonitoringServer::UpdateServerData(BYTE serverType, BYTE messageType, int data, int timeStamp)
	{
		switch (serverType)
		{
			case SERVER_TYPE::CHAT_SERVER:
			{
				UpdateChatServerData(messageType,data,timeStamp);
				break;
			}
			case SERVER_TYPE::GAME_SERVER:
			{
				UpdateGameServerData(messageType, data, timeStamp);
				break;
			}
			case SERVER_TYPE::LOGIN_SERVER:
			{
				UpdateLoginServerData(messageType, data, timeStamp);
				break;
			}
		}
	}

	void MonitoringServer::UpdateLoginServerData(BYTE messageType, int data, int timeStamp)
	{
		switch (messageType)
		{
			case LOGIN_SERVER_MonitorING_TYPE::LOGIN_SERVER_ON_OFF:	this->_L_RunningFlag = true; break;
			case LOGIN_SERVER_MonitorING_TYPE::LOGIN_SERVER_CPU_USAGE:
			{
				this->_L_CPU[this->TOTAL] += data;
				this->_L_CPU[this->MAX] = max(this->_L_CPU[this->MAX], data);
				this->_L_CPU[this->MIN] = min(this->_L_CPU[this->MIN], data);
				this->_L_CPU[this->TICK]++;
				break;
			}
			case LOGIN_SERVER_MonitorING_TYPE::LOGIN_SERVER_PRIVATE_BYTES:
			{
				this->_L_PrivateBytes[this->TOTAL] += data;
				this->_L_PrivateBytes[this->MAX] = max(this->_L_PrivateBytes[this->MAX], data);
				this->_L_PrivateBytes[this->MIN] = min(this->_L_PrivateBytes[this->MIN], data);
				this->_L_PrivateBytes[this->TICK]++;
				break;
			}
			case LOGIN_SERVER_MonitorING_TYPE::LOGIN_SERVER_SESSION_COUNTS:
			{
				this->_L_SessionCount[this->TOTAL] += data;
				this->_L_SessionCount[this->MAX] = max(this->_L_SessionCount[this->MAX], data);
				this->_L_SessionCount[this->MIN] = min(this->_L_SessionCount[this->MIN], data);
				this->_L_SessionCount[this->TICK]++;
				break;
			}
			case LOGIN_SERVER_MonitorING_TYPE::LOGIN_SERVER_AUTH_TPS:
			{
				this->_L_AuthTPS[this->TOTAL] += data;
				this->_L_AuthTPS[this->MAX] = max(this->_L_AuthTPS[this->MAX], data);
				this->_L_AuthTPS[this->MIN] = min(this->_L_AuthTPS[this->MIN], data);
				this->_L_AuthTPS[this->TICK]++;
				break;
			}
			case LOGIN_SERVER_MonitorING_TYPE::LOGIN_SERVER_PACKET_POOL_USAGE:
			{
				this->_L_PacketPoolSize[this->TOTAL] += data;
				this->_L_PacketPoolSize[this->MAX] = max(this->_L_PacketPoolSize[this->MAX], data);
				this->_L_PacketPoolSize[this->MIN] = min(this->_L_PacketPoolSize[this->MIN], data);
				this->_L_PacketPoolSize[this->TICK]++;
				break;
			}
		}
	}

	void MonitoringServer::UpdateChatServerData(BYTE messageType, int data, int timeStamp)
	{
		switch (messageType)
		{
			case CHAT_SERVER_MonitorING_TYPE::CHAT_SERVER_ON_OFF: this->_C_RunningFlag = true; break;
			case CHAT_SERVER_MonitorING_TYPE::CHAT_SERVER_CPU_USAGE:
			{
				this->_C_CPU[this->TOTAL] += data;
				this->_C_CPU[this->MAX] = max(this->_C_CPU[this->MAX], data);
				this->_C_CPU[this->MIN] = min(this->_C_CPU[this->MIN], data);
				this->_C_CPU[this->TICK]++;
				break;
			}
			case CHAT_SERVER_MonitorING_TYPE::CHAT_SERVER_PRIVATE_BYTES:
			{
				this->_C_PrivateBytes[this->TOTAL] += data;
				this->_C_PrivateBytes[this->MAX] = max(this->_C_PrivateBytes[this->MAX], data);
				this->_C_PrivateBytes[this->MIN] = min(this->_C_PrivateBytes[this->MIN], data);
				this->_C_PrivateBytes[this->TICK]++;
				break;
			}
			case CHAT_SERVER_MonitorING_TYPE::CHAT_SERVER_SESSION_COUNTS:
			{
				this->_C_SessionCount[this->TOTAL] += data;
				this->_C_SessionCount[this->MAX] = max(this->_C_SessionCount[this->MAX], data);
				this->_C_SessionCount[this->MIN] = min(this->_C_SessionCount[this->MIN], data);
				this->_C_SessionCount[this->TICK]++;
				break;
			}
			case CHAT_SERVER_MonitorING_TYPE::CHAT_SERVER_PLAYER_COUNTS:
			{
				this->_C_PlayerCount[this->TOTAL] += data;
				this->_C_PlayerCount[this->MAX] = max(this->_C_PlayerCount[this->MAX], data);
				this->_C_PlayerCount[this->MIN] = min(this->_C_PlayerCount[this->MIN], data);
				this->_C_PlayerCount[this->TICK]++;
				break;
			}
			case CHAT_SERVER_MonitorING_TYPE::CHAT_SERVER_UPDATE_TPS:
			{
				this->_C_UpdateTPS[this->TOTAL] += data;
				this->_C_UpdateTPS[this->MAX] = max(this->_C_UpdateTPS[this->MAX], data);
				this->_C_UpdateTPS[this->MIN] = min(this->_C_UpdateTPS[this->MIN], data);
				this->_C_UpdateTPS[this->TICK]++;
				break;
			}
			case CHAT_SERVER_MonitorING_TYPE::CHAT_SERVER_PACKET_POOL_USAGE:
			{
				this->_C_PacketPoolSize[this->TOTAL] += data;
				this->_C_PacketPoolSize[this->MAX] = max(this->_C_PacketPoolSize[MAX], data);
				this->_C_PacketPoolSize[this->MIN] = min(this->_C_PacketPoolSize[MIN], data);
				this->_C_PacketPoolSize[this->TICK]++;
				break;
			}
			case CHAT_SERVER_MonitorING_TYPE::CHAT_SERVER_UPDATE_MSG_QUEUE_SIZE:
			{
				this->_C_JobQueue[this->TOTAL] += data;
				this->_C_JobQueue[this->MAX] = max(this->_C_JobQueue[MAX], data);
				this->_C_JobQueue[this->MIN] = min(this->_C_JobQueue[MIN], data);
				this->_C_JobQueue[this->TICK]++;
				break;
			}
		}
	}

	void MonitoringServer::UpdateGameServerData(BYTE messageType, int data, int timeStamp)
	{
		switch (messageType)
		{
			case GAME_SERVER_MonitorING_TYPE::GAME_SERVER_ON_OFF:this->_G_RunningFlag = true; break;
			case GAME_SERVER_MonitorING_TYPE::GAME_SERVER_CPU_USAGE:
			{
				this->_G_CPU[this->TOTAL] += data;
				this->_G_CPU[this->MAX] = max(this->_G_CPU[this->MAX], data);
				this->_G_CPU[this->MIN] = min(this->_G_CPU[this->MAX], data);
				this->_G_CPU[this->TICK]++;
				break;
			}
			case GAME_SERVER_MonitorING_TYPE::GAME_SERVER_PRIVATE_BYTES:
			{
				this->_G_PrivateBytes[this->TOTAL] += data;
				this->_G_PrivateBytes[this->MAX] = max(this->_G_PrivateBytes[this->MAX], data);
				this->_G_PrivateBytes[this->MIN] = min(this->_G_PrivateBytes[this->MIN], data);
				this->_G_PrivateBytes[this->TICK]++;
				break;
			}
			case GAME_SERVER_MonitorING_TYPE::GAME_SERVER_SESSION_COUNTS:
			{
				this->_G_SessionCount[this->TOTAL] += data;
				this->_G_SessionCount[this->MAX] = max(this->_G_SessionCount[this->MAX], data);
				this->_G_SessionCount[this->MIN] = min(this->_G_SessionCount[this->MIN], data);
				this->_G_SessionCount[this->TICK]++;
				break;
			}
			case GAME_SERVER_MonitorING_TYPE::GAME_SERVER_AUTH_PLAYER:
			{
				this->_G_AuthCount[this->TOTAL] += data;
				this->_G_AuthCount[this->MAX] = max(this->_G_AuthCount[this->MAX], data);
				this->_G_AuthCount[this->MIN] = min(this->_G_AuthCount[this->MIN], data);
				this->_G_AuthCount[this->TICK]++;
				break;
			}
			case GAME_SERVER_MonitorING_TYPE::GAME_SERVER_GAME_PLAYER:
			{
				this->_G_PlayerCount[this->TOTAL] += data;
				this->_G_PlayerCount[this->MAX] = max(this->_G_PlayerCount[this->MAX], data);
				this->_G_PlayerCount[this->MIN] = min(this->_G_PlayerCount[this->MIN], data);
				this->_G_PlayerCount[this->TICK]++;
				break;
			}
			case GAME_SERVER_MonitorING_TYPE::GAME_SERVER_ACCEPT_TPS:
			{
				this->_G_AcceptTPS[this->TOTAL] += data;
				this->_G_AcceptTPS[this->MAX] = max(this->_G_AcceptTPS[this->MAX], data);
				this->_G_AcceptTPS[this->MIN] = min(this->_G_AcceptTPS[this->MIN], data);
				this->_G_AcceptTPS[this->TICK]++;
				break;
			}
			case GAME_SERVER_MonitorING_TYPE::GAME_SERVER_PACKET_RECV_TPS:
			{
				this->_G_RecvTPS[this->TOTAL] += data;
				this->_G_RecvTPS[this->MAX] = max(this->_G_RecvTPS[this->MAX], data);
				this->_G_RecvTPS[this->MIN] = min(this->_G_RecvTPS[this->MIN], data);
				this->_G_RecvTPS[this->TICK]++;
				break;
			}
			case GAME_SERVER_MonitorING_TYPE::GAME_SERVER_PACKET_SEND_TPS:
			{
				this->_G_SendTPS[this->TOTAL] += data;
				this->_G_SendTPS[this->MAX] = max(this->_G_SendTPS[this->MAX], data);
				this->_G_SendTPS[this->MIN] = min(this->_G_SendTPS[this->MIN], data);
				this->_G_SendTPS[this->TICK]++;
				break;
			}
			case GAME_SERVER_MonitorING_TYPE::GAME_SERVER_DB_WRITE_TPS:
			{
				this->_G_DBWrite[this->TOTAL] += data;
				this->_G_DBWrite[this->MAX] = max(this->_G_DBWrite[this->MAX], data);
				this->_G_DBWrite[this->MIN] = min(this->_G_DBWrite[this->MIN], data);
				this->_G_DBWrite[this->TICK]++;
				break;
			}
			case GAME_SERVER_MonitorING_TYPE::GAME_SERVER_DB_MSG_QUEUE_SIZE:
			{
				this->_G_DBQueue[this->TOTAL] += data;
				this->_G_DBQueue[this->MAX] = max(this->_G_DBQueue[this->MAX], data);
				this->_G_DBQueue[this->MIN] = min(this->_G_DBQueue[this->MIN], data);
				this->_G_DBQueue[this->TICK]++;
				break;
			}
			case GAME_SERVER_MonitorING_TYPE::GAME_SERVER_AUTH_THREAD_FPS:
			{
				this->_G_AuthFPS[this->TOTAL] += data;
				this->_G_AuthFPS[this->MAX] = max(this->_G_AuthFPS[this->MAX], data);
				this->_G_AuthFPS[this->MIN] = min(this->_G_AuthFPS[this->MIN], data);
				this->_G_AuthFPS[this->TICK]++;
				break;
			}
			case GAME_SERVER_MonitorING_TYPE::GAME_SERVER_GAME_THREAD_FPS:
			{
				this->_G_GameFPS[this->TOTAL] += data;
				this->_G_GameFPS[this->MAX] = max(this->_G_GameFPS[this->MAX], data);
				this->_G_GameFPS[this->MIN] = min(this->_G_GameFPS[this->MIN], data);
				this->_G_GameFPS[this->TICK]++;
				break;
			}
			case GAME_SERVER_MonitorING_TYPE::GAME_SERVER_PACKET_POOL_USAGE:
			{
				this->_G_PacketPoolSize[this->TOTAL] += data;
				this->_G_PacketPoolSize[this->MAX] = max(this->_G_PacketPoolSize[this->MAX], data);
				this->_G_PacketPoolSize[this->MIN] = min(this->_G_PacketPoolSize[this->MIN], data);
				this->_G_PacketPoolSize[this->TICK]++;
				break;
			}
		}
	}


	void MonitoringServer::PacketProc(Packet* packet, ULONGLONG sessionID, WORD type)
	{
		WCHAR errStr[512];
		switch (type)
		{
		case PACKET_TYPE::en_PACKET_CS_MONITOR_TOOL_REQ_LOGIN:
		{
			this->PacketProcMonitorToolLogin(packet, sessionID);
			break;
		}
		case PACKET_TYPE::en_PACKET_SS_MONITOR_DATA_UPDATE:
		{
			this->PacketProcMonitorDataUpdate(packet, sessionID);
			break;
		}
		default:
			wsprintf(errStr, L"Default Case SessionID : %I64d, Type : %u", sessionID, type);
			this->_MonitoringLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_ERROR);
			this->Disconnect(sessionID);
			break;
		}
	}

	void MonitoringServer::PacketProcMonitorToolLogin(Packet* packet, ULONGLONG sessionID)
	{
		char sessionKey[33];
		if (packet->GetBufferSize() != 32)
		{
			this->CNetServer::DisconnectSession(sessionID);
			return;
		}
		packet->GetBuffer(sessionKey, this->SESSION_KEY_SIZE);

		if (memcmp(sessionKey, this->_MonitoringSessionKey, this->SESSION_KEY_SIZE) != 0)
		{
			this->CNetServer::DisconnectSession(sessionID);
			return;
		}
		this->_MonitoringSessionID = sessionID;
		this->_MonitoringLog.LOG(L"MonitoringTools Login", LogClass::LogLevel::LOG_LEVEL_SYSTEM);
		printf("Monitoring Client Connect\n");
		this->_MoniteringSessionIDList.insert(sessionID);
	}

	void MonitoringServer::PacketProcMonitorDataUpdate(Packet* packet, ULONGLONG sessionID)
	{
		BYTE serverNo;
		BYTE dataType;
		int dataValue;
		int timeStamp;
		(*packet) >> serverNo >> dataType >> dataValue >> timeStamp;
		BYTE csDataType = this->MESSAGE_TYPE_TABLE[serverNo][dataType];
		Packet* sendPacket = Packet::Alloc();
		this->MakePacketMonitorDataUpdate(sendPacket, serverNo, csDataType, dataValue, timeStamp);
		this->UpdateServerData(serverNo, dataType, dataValue, timeStamp);
		for (auto iter = this->_MoniteringSessionIDList.begin(); iter != this->_MoniteringSessionIDList.end(); ++iter)
		{
			this->CNetServer::SendPacket(*iter, sendPacket);
		}
	}

	void MonitoringServer::MakePacketMonitorDataUpdate(Packet* packet, BYTE serverNo, BYTE dataType, int dataValue, int timeStamp)
	{
		WORD type = PACKET_TYPE::en_PACKET_CS_MONITOR_TOOL_DATA_UPDATE;
		(*packet) << type << serverNo << dataType << dataValue << timeStamp;
	}


	void MonitoringServer::InitNetServer()
	{
		if (!g_ConfigReader.SetCurrentSection(L"NetServerConfig"))CRASH();
		if (!g_ConfigReader.Find(L"Port")) CRASH();
		if (!g_ConfigReader.Find(L"BacklogQueue")) CRASH();
		if (!g_ConfigReader.Find(L"WorkerThread")) CRASH();
		if (!g_ConfigReader.Find(L"RunningThread")) CRASH();
		if (!g_ConfigReader.Find(L"NagleOff")) CRASH();
		if (!g_ConfigReader.Find(L"MaxSessionCount")) CRASH();
		if (!g_ConfigReader.Find(L"TimeOutClock")) CRASH();

		USHORT	NET_port = g_ConfigReader.Get(L"Port", 1);
		DWORD	NET_backlogQueueSize = g_ConfigReader.Get(L"BacklogQueue", 1);
		DWORD	NET_workerThreadCount = g_ConfigReader.Get(L"WorkerThread", 1);
		DWORD	NET_runningThreadCount = g_ConfigReader.Get(L"RunningThread", 1);
		DWORD	NET_nagleOff = g_ConfigReader.Get(L"NagleOff", 1);
		DWORD	NET_maxSessionCount = g_ConfigReader.Get(L"MaxSessionCount", 1);
		DWORD	NET_timeOutClock = g_ConfigReader.Get(L"TimeOutClock", 1);
		if (NET_timeOutClock == 0)
			NET_timeOutClock = INFINITE;

		printf("Net Server Config\n");
		printf("port : %u\n", NET_port);
		printf("backlogQueueSize : %u\n", NET_backlogQueueSize);
		printf("workerThreadCount : %u\n", NET_workerThreadCount);
		printf("runningThreadCount : %u\n", NET_runningThreadCount);
		printf("nagleOff : %u\n", NET_nagleOff);
		printf("maxSessionCount : %u\n", NET_maxSessionCount);
		printf("timeOutClock : %u\n", NET_timeOutClock);
		system("pause");
		this->CNetServer::InitServer(NET_port, NET_backlogQueueSize, NET_workerThreadCount, NET_runningThreadCount, NET_nagleOff, NET_maxSessionCount, NET_timeOutClock);
	}

	void MonitoringServer::InitLanServer()
	{
		if (!g_ConfigReader.SetCurrentSection(L"LanServerConfig"))CRASH();
		if (!g_ConfigReader.Find(L"Port")) CRASH();
		if (!g_ConfigReader.Find(L"BacklogQueue")) CRASH();
		if (!g_ConfigReader.Find(L"WorkerThread")) CRASH();
		if (!g_ConfigReader.Find(L"RunningThread")) CRASH();
		if (!g_ConfigReader.Find(L"NagleOff")) CRASH();
		if (!g_ConfigReader.Find(L"MaxSessionCount")) CRASH();
		if (!g_ConfigReader.Find(L"TimeOutClock")) CRASH();
		//USHORT NET_port = g_ConfigReader.Find()

		USHORT	LAN_port = g_ConfigReader.Get(L"Port", 1);
		DWORD	LAN_backlogQueueSize = g_ConfigReader.Get(L"BacklogQueue", 1);
		DWORD	LAN_workerThreadCount = g_ConfigReader.Get(L"WorkerThread", 1);
		DWORD	LAN_runningThreadCount = g_ConfigReader.Get(L"RunningThread", 1);
		DWORD	LAN_nagleOff = g_ConfigReader.Get(L"NagleOff", 1);
		DWORD	LAN_maxSessionCount = g_ConfigReader.Get(L"MaxSessionCount", 1);
		DWORD	LAN_timeOutClock = g_ConfigReader.Get(L"TimeOutClock", 1);
		if (LAN_timeOutClock == 0)
			LAN_timeOutClock = INFINITE;

		printf("Lan Server Config\n");
		printf("port : %u\n", LAN_port);
		printf("backlogQueueSize : %u\n", LAN_backlogQueueSize);
		printf("workerThreadCount : %u\n", LAN_workerThreadCount);
		printf("runningThreadCount : %u\n", LAN_runningThreadCount);
		printf("nagleOff : %u\n", LAN_nagleOff);
		printf("maxSessionCount : %u\n", LAN_maxSessionCount);
		printf("timeOutClock : %u\n", LAN_timeOutClock);
		system("pause");

		this->CLanServer::InitServer(LAN_port, LAN_backlogQueueSize, LAN_workerThreadCount, LAN_runningThreadCount, LAN_nagleOff, LAN_maxSessionCount, LAN_timeOutClock);
	}

	void MonitoringServer::InitDataBase()
	{
		if (!g_ConfigReader.SetCurrentSection(L"DataBase"))CRASH();
		if (!g_ConfigReader.Find(L"IPAddr")) CRASH();
		if (!g_ConfigReader.Find(L"RootName")) CRASH();
		if (!g_ConfigReader.Find(L"RootPassword")) CRASH();
		if (!g_ConfigReader.Find(L"InitSchema")) CRASH();
		if (!g_ConfigReader.Find(L"SlowQuery")) CRASH();

		std::wstring tempDB_ip, tempDB_root, tempDB_password, tempDB_initSchema;

		tempDB_ip = g_ConfigReader.Get(L"IPAddr");
		tempDB_root = g_ConfigReader.Get(L"RootName");
		tempDB_password = g_ConfigReader.Get(L"RootPassword");
		tempDB_initSchema = g_ConfigReader.Get(L"InitSchema");
		std::string DB_ip = { tempDB_ip.begin(),tempDB_ip.end() };
		std::string DB_root = { tempDB_root.begin(),tempDB_root.end() };
		std::string DB_password = { tempDB_password.begin(), tempDB_password.end() };
		std::string DB_initSchema = { tempDB_initSchema.begin(), tempDB_initSchema.end() };
		int DB_slowQuery = g_ConfigReader.Get(L"SlowQuery", 1);

		this->_Database = new DBConnector(DB_ip.c_str(), DB_root.c_str(), DB_password.c_str(), DB_initSchema.c_str(), DB_slowQuery);
		if (!this->_Database->DBConnect())
		{
			CRASH();
			return;
		}
		this->_Database->SetDBLogDirectory(L"ServerLog\\DBLog");
	}

	void MonitoringServer::Disconnect(ULONGLONG sessionID)
	{
		for (auto iter = this->_MoniteringSessionIDList.begin(); iter != this->_MoniteringSessionIDList.end(); ++iter)
		{
			if (*iter == sessionID)
			{
				CLanServer::DisconnectSession(sessionID);
				this->_MoniteringSessionIDList.erase(iter);
				return;
			}
		}
		CNetServer::DisconnectSession(sessionID);
	}

	unsigned __stdcall NetServerRun(void* param)
	{
		MonitoringServer* server = (MonitoringServer*)param;
		if (server == nullptr)
			CRASH();
		server->CNetServer::Run(nullptr, 0);
		return 0;
	}
	unsigned __stdcall LanServerRun(void* param)
	{
		MonitoringServer* server = (MonitoringServer*)param;
		if (server == nullptr)
			CRASH();
		server->CLanServer::Run(nullptr, 0);
		return 0;
	}

	void MonitoringServer::Start()
	{
		int _ =_wmkdir(L"ServerLog");
		int __ = _wmkdir(L"ServerLog\\MonitoringServerLog");

		this->_MonitoringLog.LOG_SET_DIRECTORY(L"ServerLog\\MonitoringServerLog");
		this->_MonitoringLog.LOG_SET_LEVEL(LogClass::LogLevel::LOG_LEVEL_SYSTEM);

		this->InitNetServer();
		this->InitLanServer();
		this->InitDataBase();

		this->_MonitoringThread = (HANDLE)_beginthreadex(nullptr, 0, MonitoringThread, this, 0, nullptr);
		if (this->_MonitoringThread == nullptr)
		{
			CRASH();
			return;
		}
		this->_RunningFlag = true;
		HANDLE lanServer = (HANDLE)_beginthreadex(nullptr, 0, LanServerRun, this, 0, nullptr);
		HANDLE netServer = (HANDLE)_beginthreadex(nullptr, 0, NetServerRun, this, 0, nullptr);

		HANDLE threads[3]{ this->_MonitoringThread,lanServer,netServer };
		WaitForMultipleObjects(3, threads, true, INFINITE);
	}

	void MonitoringServer::Close()
	{
		this->PostLanServerStop();
		this->_RunningFlag = false;
		return;
	}

	void MonitoringServer::UpdateHardwareData()
	{
		ULONGLONG cpuUsage = this->_HardWareMonitor.ProcessorTotal();
		ULONGLONG availableMemory = this->_HardWareMonitor.AvailableMemoryMBytes();
		ULONGLONG nonpagedMemory = this->_HardWareMonitor.NonPagedPoolMBytes();
		ULONGLONG networkSend = this->_HardWareMonitor.EthernetSendKBytes() - this->_LastSendBytes;
		ULONGLONG networkRecv = this->_HardWareMonitor.EthernetRecvKBytes() - this->_LastRecvBytes;

		this->_LastSendBytes = this->_HardWareMonitor.EthernetSendKBytes();
		this->_LastRecvBytes = this->_HardWareMonitor.EthernetRecvKBytes();

		this->_H_CPU[this->TOTAL] += cpuUsage;
		this->_H_CPU[this->MIN] = min(this->_H_CPU[this->MIN], cpuUsage);
		this->_H_CPU[this->MAX] = max(this->_H_CPU[this->MAX], cpuUsage);
		this->_H_CPU[this->TICK]++;

		this->_H_AvailableMemory[this->TOTAL] += availableMemory;
		this->_H_AvailableMemory[this->MAX] = max(this->_H_AvailableMemory[this->MAX], availableMemory);
		this->_H_AvailableMemory[this->MIN] = min(this->_H_AvailableMemory[this->MIN], availableMemory);
		this->_H_AvailableMemory[this->TICK]++;

		this->_H_NonpagedMemory[this->TOTAL] += nonpagedMemory;
		this->_H_NonpagedMemory[this->MAX] = max(this->_H_NonpagedMemory[this->MAX], availableMemory);
		this->_H_NonpagedMemory[this->MIN] = min(this->_H_NonpagedMemory[this->MIN], availableMemory);
		this->_H_NonpagedMemory[this->TICK]++;

		this->_H_NetworkSend[this->TOTAL] += networkSend;
		this->_H_NetworkSend[this->MAX] = max(this->_H_NetworkSend[this->MAX], networkSend);
		this->_H_NetworkSend[this->MIN] = min(this->_H_NetworkSend[this->MIN], networkSend);
		this->_H_NetworkSend[this->TICK]++;

		this->_H_NetworkRecv[this->TOTAL] += networkRecv;
		this->_H_NetworkRecv[this->MAX] = max(this->_H_NetworkRecv[this->MAX], networkRecv);
		this->_H_NetworkRecv[this->MIN] = min(this->_H_NetworkRecv[this->MIN], networkRecv);
		this->_H_NetworkRecv[this->TICK]++;

		Packet* packet[10];
		int i = 0;
		int curTime = time(nullptr);
		int processorTotal = this->_HardWareMonitor.ProcessorTotal();
		packet[i] = Packet::Alloc(); MakePacketMonitorDataUpdate(packet[i++], SERVER_TYPE::MonitorING_SERVER, dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL, cpuUsage, curTime);
		packet[i] = Packet::Alloc(); MakePacketMonitorDataUpdate(packet[i++], SERVER_TYPE::MonitorING_SERVER, dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY, nonpagedMemory, curTime);
		packet[i] = Packet::Alloc(); MakePacketMonitorDataUpdate(packet[i++], SERVER_TYPE::MonitorING_SERVER, dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV, networkRecv, curTime);
		packet[i] = Packet::Alloc(); MakePacketMonitorDataUpdate(packet[i++], SERVER_TYPE::MonitorING_SERVER, dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND, networkSend, curTime);
		packet[i] = Packet::Alloc(); MakePacketMonitorDataUpdate(packet[i++], SERVER_TYPE::MonitorING_SERVER, dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY, availableMemory, curTime);

		for (int j = 0; j < i; j++)
		{
			for (auto iter = this->_MoniteringSessionIDList.begin(); iter != this->_MoniteringSessionIDList.end(); ++iter)
			{
				CNetServer::SendPacket(*iter, packet[j]);
			}
		}
	}

	void MonitoringServer::OnRecv(ULONGLONG sessionID, Packet* recvPacket)
	{
		WORD type;
		(*recvPacket) >> type;
		this->PacketProc(recvPacket, sessionID, type);
	}

	void MonitoringServer::OnErrorOccured(DWORD errorCode, const WCHAR* error)
	{
		
	}

	bool MonitoringServer::OnConnectionRequest(WCHAR* ipStr, DWORD ip, USHORT port)
	{
		return this->_RunningFlag;
	}

	void MonitoringServer::OnClientJoin(WCHAR* ipStr, DWORD ip, USHORT port, ULONGLONG sessionID)
	{
		time_t cur = time(nullptr);
		tm t;
		localtime_s(&t, &cur);
		printf("%.2d%.2d%.2d//%.2d:%.2d:%.2d//OnClientJoin -> ", (t.tm_year + 1900) % 100, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
		wprintf_s(L"IP : %s / PORT : %u / SessionID : %I64u\n", ipStr, port, sessionID);
		this->_MonitoringLog.LOG(L"Server Join", LogClass::LogLevel::LOG_LEVEL_SYSTEM);
	}

	void MonitoringServer::OnClientLeave(ULONGLONG sessionID)
	{
		bool find = false;
		for (auto iter = this->_MoniteringSessionIDList.begin(); iter != this->_MoniteringSessionIDList.end(); ++iter)
		{
			if (sessionID == *iter)
			{
				printf("Monitoring Client Out\n");
				this->_MoniteringSessionIDList.erase(iter);
				find = true;
				break;
			}
		}
		if (!find)
		{
			printf("%llu", sessionID);
			printf("Server Out\n");
		}
	}

	void MonitoringServer::OnTimeOut(ULONGLONG sessionID)
	{
		auto iter = this->_MoniteringSessionIDList.begin();
		for (; iter != this->_MoniteringSessionIDList.end(); ++iter)
		{
			if (*iter == sessionID);
			return;
		}
		CNetServer::DisconnectSession(sessionID);
	}

}