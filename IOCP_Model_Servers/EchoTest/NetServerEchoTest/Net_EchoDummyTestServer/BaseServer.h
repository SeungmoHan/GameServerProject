#pragma once
#ifndef __BASE_SERVER__
#define __BASE_SERVER__
#define __UNIV_DEVELOPER_
#include "CNetServer.h"
#include <JobMessage.h>
#include <unordered_set>
#include "Player.h"
#include "CommonProtocol.h"
#include "SS_MoniteringProtocol.h"

namespace univ_dev
{
	class BaseServer : public CNetServer
	{
	public:
		/// -----------------------------------------------------------------
		/// -----------------------------------------------------------------
		/// -----------------------------------------------------------------
		/// ThreadBlock
		class BasicThreadBlock
		{
		public:
			friend class BaseServer;

			BasicThreadBlock(DWORD framePerSec, BaseServer* server, std::string _ThreadBlockName);

			void Init();
			inline void JobEnqueue(JobMessage job)
			{
				this->_JobQueue.enqueue(job);
				SetEvent(this->_RunningEvent);
			}
			inline void Disconnect(ULONGLONG sessionID)
			{
				this->_BaseServer->DisconnectSession(sessionID);
			}

			inline std::string GetThreadBlockName() const
			{
				return this->_ThreadBlockName;
			}

		protected:
			constexpr static DWORD INVALID_THREAD_BLOCK_INDEX = -1;

			inline void SendPacket(ULONGLONG sessionID, Packet* packet)
			{
				return this->_BaseServer->SendPacket(sessionID, packet);
			}

			inline void SendToMoniteringServer(Packet*packet)
			{
				return this->_BaseServer->SendToMoniteringSession(packet);
			}

			inline DWORD GetTlsErrIdx() { return this->_BaseServer->_ErrTlsIdx; }
			//inline MoniteringInfo GetMoniteringInfo() { return this->_BaseServer->GetMoniteringInfo(); };
		private:
			virtual void OnUpdate() = 0;
			virtual void OnMessage(ULONGLONG sessionID, Packet* packet) = 0;

			virtual void OnPlayerJoined(ULONGLONG sessionID,Player* player) = 0;
			virtual void OnPlayerLeaved(ULONGLONG sessionID,Player* player) = 0;

			virtual void OnPlayerMoveJoin(ULONGLONG sessionID, Player* player) = 0;
			virtual void OnPlayerMoveLeave(ULONGLONG sessionID, Player* player) = 0;

			virtual void OnTimeOut(ULONGLONG sessionID) = 0;

			virtual void RunMonitering(HardWareMoniter& h, ProcessMoniter& p) = 0;
			virtual void OnThreadBlockStop() = 0;

			friend unsigned __stdcall BasicThread(void* threadBlockPtr);
			unsigned int BasicThreadProc();
		public:

		private:
			LockFreeQueue<JobMessage> _JobQueue;
			DWORD _FramePerSec;
			BaseServer* _BaseServer;
			std::string _ThreadBlockName;

			HANDLE _RunningEvent;
			DWORD _RunningFlag;
			HANDLE _RunningThread;
			
			// SessionId <-> AccountNo
			std::unordered_map<ULONGLONG, Player*> _BlockPlayerMap;
		public:
			inline int GetJobQueueSize() { return this->_JobQueue.size(); }
			inline int GetJobQueueCapacity() { return this->_JobQueue.GetCapacityCount(); }
			inline bool GetBlockRunningFlag() { return this->_RunningFlag; }

			inline void GetLibraryMoniteringInfo(CNetServer::MoniteringInfo& info) 
			{
				return this->_BaseServer->GetMoniteringInfo(info);
			}
		};
		friend class BasicThreadBlock;


		/// ThreadBlock
		/// -----------------------------------------------------------------
		/// -----------------------------------------------------------------
		/// -----------------------------------------------------------------
		
		
		/// -----------------------------------------------------------------
		/// -----------------------------------------------------------------
		/// -----------------------------------------------------------------
		/// BaseServer
		constexpr static int SERVER_TYPE = SERVER_TYPE::GAME_SERVER;
		~BaseServer();
		BaseServer();
		void	InitBaseServer(USHORT port, DWORD backlogQueueSize, DWORD threadPoolSize, DWORD runningThread, DWORD nagleOff, ULONGLONG maxSessionCounts, DWORD timeOutTime);

		//void	SendPacket(ULONGLONG sessionID, Packet* packet);
		//void	DisconnectSession(ULONGLONG sessionID);

		void	Start();
		void	Close();

		void	BeginAttach() {};
		void	Attach(BasicThreadBlock* blockPtr);
		void	EndAttach() {};
		void	RunThreadBlock();
		inline void MakePacketMoniteringInfo(Packet* packet, BYTE serverNo, BYTE dataType, int dataValue, int timeStamp)
		{
			WORD type = PACKET_TYPE::en_PACKET_SS_MONITOR_DATA_UPDATE;
			(*packet) << type << serverNo << dataType << dataValue << timeStamp;
		}
		inline WCHAR* GetErrString() 
		{
			WCHAR* errStr = (WCHAR*)TlsGetValue(this->_ErrTlsIdx);
			if (errStr == nullptr)
			{
				errStr = new WCHAR[512]{ 0 };
				TlsSetValue(this->_ErrTlsIdx, errStr);
			}
			return errStr;
		}

	private:
		virtual void	OnRecv(ULONGLONG sessionID, Packet* pakcet) = 0;
		virtual void	OnErrorOccured(DWORD errorCode, const WCHAR* error, LogClass::LogLevel level) = 0;
		virtual bool	OnConnectionRequest(WCHAR* ipStr, DWORD ip, USHORT port) = 0;
		virtual void	OnClientJoin(WCHAR* ipStr, DWORD ip, USHORT port, ULONGLONG sessionID) = 0;
		virtual void	OnClientLeave(ULONGLONG sessionID) = 0;
		virtual void	OnTimeOut(ULONGLONG sessionID) = 0;
		virtual void	OnSend(ULONGLONG sessionID) = 0;

		friend unsigned __stdcall MonitoringThread(void* param);
		unsigned int MonitoringThreadProc();


		LogClass											_BaseServerLog;
		HardWareMoniter										_HardWareMoniter;
		ProcessMoniter										_ProcessMoniter;

		HANDLE												_MonitoringThread;
		HANDLE												_ThreadBlockStartEvent;
		DWORD												_ErrTlsIdx = -1;

		std::unordered_map<std::string, BasicThreadBlock*>	_BaseServerThreadBlockMap;
	protected:
		BasicThreadBlock*									_DefaultThreadBlock = nullptr;
	public:
		DWORD												_RunningFlag = 0;
		/// BaseServer
		/// -----------------------------------------------------------------
		/// -----------------------------------------------------------------
		/// -----------------------------------------------------------------
	};
}




#endif // !__BASE_SERVER__
