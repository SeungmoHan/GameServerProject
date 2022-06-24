#pragma once
#ifndef __GAME_SERVER_DEF__
#define __GAME_SERVER_DEF__
#define __UNIV_DEVELOPER_

#include "BaseServer.h"
#include "ConfigReader.h"

// 실제 실행시에는 주석처리하고 가면됨
//#define IN_VISUAL_STUDIO_DEBUG_FLAG

namespace univ_dev
{
	class GameServer : public BaseServer
	{
	public:
		inline GameServer() :
			_ServerPort(0),
			_BackLogQueueSize(0),
			_WorkerThreadCount(0),
			_RunningThreadCount(0),
			_NagleOffOption(0),
			_MaxSessionCount(0),
			_TimeOutClock(0)
		{
#ifdef IN_VISUAL_STUDIO_DEBUG_FLAG
			// VS에서 디버깅 하려고 임시로 적어넣은거임
			this->_ServerPort = 10445;
			this->_BackLogQueueSize = 1000;
			this->_WorkerThreadCount = 4;
			this->_RunningThreadCount = 4;
			this->_NagleOffOption = false;
			this->_MaxSessionCount = 20000;
			this->_TimeOutClock = 30000;
#endif
		};

		void AttachGameBlock();
		void AttachAuthBlock();

		friend class GameThreadBlock;
		friend class AuthThreadBlock;
		//void AttachChattingBlock();

		inline void WaitForServerEnd()
		{
			this->RunThreadBlock();
		}
		bool SetServerConfig();

		void	OnRecv(ULONGLONG sessionID, Packet* pakcet) final;
		void	OnErrorOccured(DWORD errorCode, const WCHAR* error, LogClass::LogLevel level) final;
		bool	OnConnectionRequest(WCHAR* ipStr, DWORD ip, USHORT port) final;
		void	OnClientJoin(WCHAR* ipStr, DWORD ip, USHORT port, ULONGLONG sessionID) final;
		void	OnClientLeave(ULONGLONG sessionID) final;
		void	OnTimeOut(ULONGLONG sessionID) final;
		void	OnSend(ULONGLONG sessionID) final;
	private:
		Player* CreatePlayer(ULONGLONG sessionID);
		Player* FindPlayer(ULONGLONG sessionID);
		void				RemovePlayer(ULONGLONG sessionID);

		inline void	LockPlayerMap(bool shared = false)
		{
			if (shared)
				return AcquireSRWLockShared(&this->_PlayerMapLock);
			AcquireSRWLockExclusive(&this->_PlayerMapLock);
		};
		inline void	UnlockPlayerMap(bool shared = false)
		{
			if (shared)
				return ReleaseSRWLockShared(&this->_PlayerMapLock);
			ReleaseSRWLockExclusive(&this->_PlayerMapLock);
		};

		inline void LockSessionThreadBlock(bool shared = false)
		{
			if (shared)
				return AcquireSRWLockShared(&this->_SessionThreadBlockLock);
			return AcquireSRWLockExclusive(&this->_SessionThreadBlockLock);
		}
		inline void	UnlockSessionThreadBlock(bool shared = false)
		{
			if (shared)
				return ReleaseSRWLockShared(&this->_SessionThreadBlockLock);
			return ReleaseSRWLockExclusive(&this->_SessionThreadBlockLock);
		};

		inline void LockThreadBlockMap(bool shared = false)
		{
			if (shared)
				return AcquireSRWLockShared(&this->_ThreadBlockMapLock);
			return AcquireSRWLockExclusive(&this->_ThreadBlockMapLock);
		}
		inline void UnlockThreadBlockMap(bool shared = false)
		{
			if (shared)
				return ReleaseSRWLockShared(&this->_ThreadBlockMapLock);
			return ReleaseSRWLockExclusive(&this->_ThreadBlockMapLock);
		}

		void		SetPlayerThreadBlock(ULONGLONG sessionID,std::string blockName);

		void		MoveRequest(ULONGLONG sessionID, std::string blockName);

		BasicThreadBlock* FindThreadBlock(ULONGLONG sessionID);
		BasicThreadBlock* FindThreadBlock(std::string blockName);


		inline BasicThreadBlock* GetDefaultThreadBlock() { return this->_DefaultThreadBlock; }

	private:
		LockFreeMemoryPool<Player>							_PlayerPool;

		std::unordered_map<std::string, BasicThreadBlock*>	_ThreadBlockMap;
		SRWLOCK												_ThreadBlockMapLock;
		
		SRWLOCK												_SessionThreadBlockLock;
		std::unordered_map<ULONGLONG, BasicThreadBlock*>	_SessionThreadBlockMap;

		SRWLOCK												_PlayerMapLock;
		std::unordered_map<ULONGLONG, Player*>				_PlayerMap;

		LogClass											_GameServerLog;

	private: // SERVER_CONFIG
		WORD _ServerPort;
		DWORD _BackLogQueueSize;
		DWORD _WorkerThreadCount;
		DWORD _RunningThreadCount;
		DWORD _NagleOffOption;
		ULONGLONG _MaxSessionCount;
		DWORD _TimeOutClock;
	};
}





#endif // !__GAME_SERVER_DEF__

