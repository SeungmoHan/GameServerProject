#include "BaseServer.h"
#include <process.h>
#include <conio.h>
#include <locale.h>

#define DEFAULT_THREAD_BLOCK_INDEX 0
#define SHARED_TRUE true
#define SHARED_FALSE false
#define MAX_THREAD_BLOCK_SIZE 100

namespace univ_dev
{
	unsigned __stdcall MoniteringThread(void* param);
	/// ---------------------------------------------------------------------------------------
	/// ---------------------------------------------------------------------------------------
	/// ---------------------------------------------------------------------------------------
	/// ---------------------------------------------------------------------------------------
	/// THREAD_BLOCK_DEF
	unsigned __stdcall BasicThread(void* threadBlockPtr)
	{
		if (threadBlockPtr == nullptr)
			return -1;
		BaseServer::BasicThreadBlock* block = (BaseServer::BasicThreadBlock*)threadBlockPtr;
		return block->BasicThreadProc();
	}
	unsigned int BaseServer::BasicThreadBlock::BasicThreadProc()
	{
		WCHAR* errStr = this->_BaseServer->GetErrString();
		wsprintf(errStr, L"BasicThreadProc Run %S", this->_ThreadBlockName.c_str());
		this->_BaseServer->_BaseServerLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
		WaitForSingleObject(this->_BaseServer->_ThreadBlockStartEvent, INFINITE);
		JobMessage job;
		DWORD lastFrame = timeGetTime();
		DWORD cur = lastFrame;
		DWORD MSPerFrame = 1000 / this->_FramePerSec;
		for(;;)
		{
			if (!this->_RunningFlag)
				break;
			WaitForSingleObject(this->_RunningEvent, MSPerFrame);
			int i = 0;
			for(;;)
			{
				if (!this->_JobQueue.dequeue(job))
					break;
			
				switch (job._Type)
				{
					case JobMessage::Type::THREAD_BLOCK_STOP:
					{
						this->_RunningFlag = false;
						this->OnThreadBlockStop();
						this->_BaseServer->PostServerStop();
						break;
					}
					case JobMessage::Type::CLIENT_ENTER:
					{
						this->OnPlayerJoined(job._SessionID,job._Player);
						break;
					}
					case JobMessage::Type::CLIENT_LEAVE:
					{
						this->OnPlayerLeaved(job._SessionID, job._Player);
						break;
					}
					case JobMessage::Type::CLIENT_MOVE_ENTER:
					{
						this->OnPlayerMoveJoin(job._SessionID, job._Player);
						break;
					}
					case JobMessage::Type::CLIENT_MOVE_LEAVE:
					{
						this->OnPlayerMoveLeave(job._SessionID, job._Player);
						break;
					}
					case JobMessage::Type::TIME_OUT:
					{
						this->OnTimeOut(job._SessionID);
						break;
					}
					case JobMessage::Type::MESSAGE:
					{
						this->OnMessage(job._SessionID, job._Packet);
						Packet::Free(job._Packet);
						break;
					}
					default:
					{
						WCHAR* tlsErr = (WCHAR*)TlsGetValue(this->_BaseServer->_ErrTlsIdx);
						wsprintf(tlsErr, L"Default Case SessionID : %llu Type : %d", job._SessionID, job._Type);
						this->_BaseServer->_BaseServerLog.LOG(tlsErr, LogClass::LogLevel::LOG_LEVEL_ERROR);
						this->Disconnect(job._SessionID);
						break;
					}
				}
				if (i++ >= 100000)
				{
					SetEvent(this->_RunningEvent);
					break;
				}

			}
			cur = timeGetTime();
			if ((cur - lastFrame) >= (MSPerFrame))
			{
				this->OnUpdate();
				lastFrame = cur;
			}
		}
		wsprintf(errStr, L"ThreadBlock : %S(block name) , %u(ThreadID) End\n", this->_ThreadBlockName.c_str(), GetCurrentThreadId());
		wprintf_s(errStr);
		this->_BaseServer->_BaseServerLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
		return (0-0);
	}
	BaseServer::BasicThreadBlock::BasicThreadBlock(DWORD framePerSec, BaseServer* server,std::string blockName)
		: _FramePerSec(framePerSec), _RunningEvent(CreateEvent(nullptr, false, false, nullptr)), _RunningFlag(false), _BaseServer(server), _ThreadBlockName(std::move(blockName))
	{
		Init();
	}
	void BaseServer::BasicThreadBlock::Init()
	{
		this->_RunningThread = (HANDLE)_beginthreadex(nullptr, 0, BasicThread, this, 0, nullptr);
		if (_RunningThread == nullptr)
		{
			CRASH();
			return;
		}
		this->_RunningFlag = true;
	}
	

	/// THREAD_BLOCK_DEF
	/// ---------------------------------------------------------------------------------------
	/// ---------------------------------------------------------------------------------------
	/// ---------------------------------------------------------------------------------------
	/// ---------------------------------------------------------------------------------------





	/// ---------------------------------------------------------------------------------------
	/// ---------------------------------------------------------------------------------------
	/// ---------------------------------------------------------------------------------------
	/// ---------------------------------------------------------------------------------------
	/// BASE_SERVER_DEF
	BaseServer::~BaseServer()
	{
		this->Close();
	}
	BaseServer::BaseServer() : _RunningFlag(false), _ThreadBlockStartEvent(CreateEvent(nullptr, true, false, nullptr)),_ErrTlsIdx(-1),_MoniteringThread(nullptr)
	{
		//this->InitNetServer(port, backlogQueueSize, threadPoolSize, runningThread, nagleOff, maxSessionCounts, timeOutTime);
	}
	void BaseServer::InitBaseServer(USHORT port, DWORD backlogQueueSize, DWORD threadPoolSize, DWORD runningThread, DWORD nagleOff, ULONGLONG maxSessionCounts, DWORD timeOutTime)
	{
		this->InitNetServer(port, backlogQueueSize, threadPoolSize, runningThread, nagleOff, maxSessionCounts, timeOutTime);
	};



	void BaseServer::Start()
	{
		auto _ = _wmkdir(L"ServerLog");
		_ = _wmkdir(L"ServerLog\\BaseServerLog");
		this->_BaseServerLog.LOG_SET_DIRECTORY(L"ServerLog\\BaseServerLog");
		this->_BaseServerLog.LOG_SET_LEVEL(LogClass::LogLevel::LOG_LEVEL_LIBRARY);

		this->_ErrTlsIdx = TlsAlloc();

		setlocale(LC_ALL, "korean");
		_wsetlocale(LC_ALL, L"korean");

		if (!this->GetNetCoreInitializeFlag())
		{
			DWORD coreErr = this->GetLastCoreErrno();
			DWORD lastErr = this->GetLastAPIErrno();
			this->_BaseServerLog.LOG(L"Network Library Initialize Failed API Err : %u, Core Err : %u", LogClass::LogLevel::LOG_LEVEL_LIBRARY);
			printf("API Error : %d\nCore Error : %d\n", lastErr, coreErr);
			this->_RunningFlag = false;
			return;
		}
	}

	void BaseServer::Attach(BaseServer::BasicThreadBlock* blockPtr)
	{
		if (this->_DefaultThreadBlock == nullptr)
			this->_DefaultThreadBlock = blockPtr;
		this->_BaseServerThreadBlockMap.emplace(std::make_pair(blockPtr->_ThreadBlockName, blockPtr));
	}

	void BaseServer::RunThreadBlock()
	{
		this->_RunningFlag = true;

		this->_MoniteringThread = (HANDLE)_beginthreadex(nullptr, 0, MoniteringThread, this, 0, nullptr);
		if (this->_MoniteringThread == nullptr)
		{
			CRASH();
			return;
		}

		HANDLE threadArr[100];
		int i = 0;
		for (auto iter = this->_BaseServerThreadBlockMap.begin(); iter != this->_BaseServerThreadBlockMap.end(); ++iter,i++)
			threadArr[i] = iter->second->_RunningThread;

		threadArr[_BaseServerThreadBlockMap.size()] = this->_MoniteringThread;
		SetEvent(this->_ThreadBlockStartEvent);
		this->Run(threadArr, _BaseServerThreadBlockMap.size() + 1);
	}

	void BaseServer::Close()
	{
		TlsFree(this->_ErrTlsIdx);
	}




	unsigned int BaseServer::MoniteringThreadProc()
	{
		DWORD begin = timeGetTime();
		DWORD prev;
		DWORD cur;
		DWORD simpleLogTimer;
		DWORD longLogTimer;
		longLogTimer = simpleLogTimer = cur = prev = begin;

		ULONGLONG lastTotalAccept = 0;
		ULONGLONG lastTotalRelease = 0;
		ULONGLONG lastTotalSend = 0;
		ULONGLONG lastTotalRecv = 0;

		while (this->_RunningFlag)
		{
			WaitForMoniteringSignal();
			if (_kbhit())
			{
				int key = _getch();
				if (toupper(key) == 'Q')
				{
					this->_RunningFlag = false;
					{
						JobMessage job;
						job._Type = JobMessage::Type::THREAD_BLOCK_STOP;
						HANDLE threads[50];
						int i = 0;
						for (auto iter = this->_BaseServerThreadBlockMap.begin(); iter != this->_BaseServerThreadBlockMap.end(); ++iter, i++)
						{
							iter->second->JobEnqueue(job);
							threads[i] = iter->second->_RunningThread;
						}
						WaitForMultipleObjects(this->_BaseServerThreadBlockMap.size(), threads, true, INFINITE);
					}
					this->CNetServer::PostServerStop();
					return 0;
				}
				else if (toupper(key) == 'S')
				{
					SaveProfiling();
					ResetProfiling();
				}
			}
			cur = timeGetTime();

			CNetServer::MoniteringInfo info;
			this->CNetServer::GetMoniteringInfo(info);
			this->_HardWareMoniter.UpdateHardWareTime();
			this->_ProcessMoniter.UpdateProcessTime();

			DWORD afterServerOn = (timeGetTime() - begin) / 1000;

			DWORD day = afterServerOn / 86400;
			DWORD hour = (afterServerOn % 86400) / 3600;
			DWORD minute = (afterServerOn % 3600) / 60;
			DWORD sec = afterServerOn % 60;

			prev = cur;

			//printf("\n-------------------------------------MONITERING----------------------------------------\n");
			//printf("| After Server On : %u day / %u h / %u m / %u s\n", day, hour, minute, sec);
			//printf("|----------------------------------------THREAD----------------------------------------\n");
			//printf("| Worker Thread / Running  thread : %u / %u\n", info._WorkerThreadCount, info._RunningThreadCount);
			//printf("|----------------------------------------TOTAL-----------------------------------------\n");
			//printf("| Total Accept / Release / : %llu /  %llu\n", info._TotalAcceptSessionCount, info._TotalReleaseSessionCount);
			//printf("| Total Processed Bytes : %llu\n", info._TotalProcessedBytes);
			//printf("|----------------------------------------TPS-------------------------------------------\n");
			//printf("| Accept Per Sec / Release Per Sec : %llu / %llu\n", info._TotalAcceptSessionCount - lastTotalAccept, info._TotalReleaseSessionCount - lastTotalRelease);
			printf("| TPS// Recv / Send / Total  : %llu / %llu / %llu\n", info._TotalRecvPacketCount - lastTotalRecv, info._TotalSendPacketCount - lastTotalSend, (info._TotalSendPacketCount - lastTotalSend) + (info._TotalRecvPacketCount - lastTotalRecv));
			printf("| LockFreeQueue Size / Capacity / Max : %u / %u / %u\n", info._SessionSendQueueSize, info._SessionSendQueueCapacity, info._SessionSendQueueMax);
			//printf("|----------------------------------------Library---------------------------------------\n");
			//printf("| Packet Chunk Capacity / UseCount : %d / %d\n", Packet::GetCapacityCount(), Packet::GetUseCount());
			//printf("| Packet ToTal UseCount : %d\n", Packet::GetTotalPacketCount());
			//printf("| Session Count / IDX Capacity / IDX Size : %u / %u / %u\n", info._CurrentSessionCount, info._SessionIndexStackCapacity, info._SessionIndexStackSize);

			int totalJobQueueSize = 0;

			for (auto iter = this->_BaseServerThreadBlockMap.begin(); iter != this->_BaseServerThreadBlockMap.end(); ++iter)
			{
				int queueSize = iter->second->_JobQueue.size();
				totalJobQueueSize += queueSize;
				//printf("| %s JobQueue Size : %d\n", iter->second->GetThreadBlockName().c_str(), queueSize);
			}




			for (auto iter = this->_BaseServerThreadBlockMap.begin(); iter != this->_BaseServerThreadBlockMap.end(); ++iter)
			{
				iter->second->RunMonitering(this->_HardWareMoniter, this->_ProcessMoniter);
			}
			printf("Alive\n");
			//printf("|-----------------------------------USAGE_MONITER--------------------------------------\n");
			//printf("| NIC Send / Recv (10KB) : %.3llf / %.3llf\n", this->_HardWareMoniter.EthernetSendKBytes(), this->_HardWareMoniter.EthernetRecvKBytes());
			//printf("| Available / NPPool / Private Mem : %lluMb / %lluMb / %lluKb\n", this->_HardWareMoniter.AvailableMemoryMBytes(), this->_HardWareMoniter.NonPagedPoolMBytes(), this->_ProcessMoniter.PrivateMemoryMBytes());
			//printf("| CPU / PROCESS : [T %.2llf%% K %.2llf%% U %.2llf%%] / [T %.2llf%% K %.2llf%% U %.2llf%%]   \n", this->_HardWareMoniter.ProcessorTotal(), this->_HardWareMoniter.ProcessorKernel(), this->_HardWareMoniter.ProcessorUser(), this->_ProcessMoniter.ProcessTotal(), this->_ProcessMoniter.ProcessKernel(), this->_ProcessMoniter.ProcessUser());

			Packet* packet[50];
			int packetCount = 0;
			int curTime = ::time(nullptr);

			packet[packetCount] = Packet::Alloc();
			this->MakePacketMoniteringInfo(packet[packetCount++],
				this->SERVER_TYPE, GAME_SERVER_MONITERING_TYPE::G_ON_OFF,
				true, curTime);
			packet[packetCount] = Packet::Alloc();
			this->MakePacketMoniteringInfo(packet[packetCount++],
				this->SERVER_TYPE, GAME_SERVER_MONITERING_TYPE::G_CPU_USAGE,
				this->_ProcessMoniter.ProcessTotal(), curTime);
			packet[packetCount] = Packet::Alloc();
			this->MakePacketMoniteringInfo(packet[packetCount++],
				this->SERVER_TYPE, GAME_SERVER_MONITERING_TYPE::G_PRIVATE_BYTES,
				this->_ProcessMoniter.PrivateMemoryMBytes(), curTime);
			packet[packetCount] = Packet::Alloc();
			this->MakePacketMoniteringInfo(packet[packetCount++],
				this->SERVER_TYPE, GAME_SERVER_MONITERING_TYPE::G_SESSION_COUNTS,
				this->_CurSessionCount, curTime);
			packet[packetCount] = Packet::Alloc();
			this->MakePacketMoniteringInfo(packet[packetCount++],
				this->SERVER_TYPE, GAME_SERVER_MONITERING_TYPE::G_ACCEPT_TPS,
				this->_TotalAcceptSessionCount - lastTotalAccept, curTime);
			packet[packetCount] = Packet::Alloc();
			this->MakePacketMoniteringInfo(packet[packetCount++],
				this->SERVER_TYPE, GAME_SERVER_MONITERING_TYPE::G_PACKET_RECV_TPS,
				this->_TotalRecvPacketCount - lastTotalRecv, curTime);
			packet[packetCount] = Packet::Alloc();
			this->MakePacketMoniteringInfo(packet[packetCount++],
				this->SERVER_TYPE, GAME_SERVER_MONITERING_TYPE::G_PACKET_SEND_TPS,
				this->_TotalSendPacketCount - lastTotalSend, curTime);
			packet[packetCount] = Packet::Alloc();
			this->MakePacketMoniteringInfo(packet[packetCount++],
				this->SERVER_TYPE, GAME_SERVER_MONITERING_TYPE::G_PACKET_POOL_USAGE,
				Packet::GetUseCount(), curTime);


			//이거 잡큐 사이즈 측정하려고 한건데 임시로 채팅서버용에다가 넣어서 보는중임
			packet[packetCount] = Packet::Alloc();
			this->MakePacketMoniteringInfo(packet[packetCount++],
				2, CHAT_SERVER_MONITERING_TYPE::C_UPDATE_MSG_QUEUE_SIZE,
				totalJobQueueSize, curTime);

			// 모조리 보내기
			for (int i = 0; i < packetCount; i++)
				this->SendToMoniteringSession(packet[i]);

			lastTotalRecv = info._TotalRecvPacketCount;
			lastTotalSend = info._TotalSendPacketCount;
			lastTotalAccept = info._TotalAcceptSessionCount;
			lastTotalRelease = info._TotalReleaseSessionCount;


			if (cur - simpleLogTimer <= 60000)
				continue;
			simpleLogTimer = cur;
			FILE* file = nullptr;
			while (file == nullptr)
				fopen_s(&file, "__HardWareMemoryLog.txt", "ab");

			//f
			(file, "Available / NPPool / Private Mem : %lluMb / %lluMb / %lluKb\n", this->_HardWareMoniter.AvailableMemoryMBytes(), this->_HardWareMoniter.NonPagedPoolMBytes(), this->_ProcessMoniter.PrivateMemoryKBytes());

			if (cur - longLogTimer < 300000)
			{
				fclose(file);
				continue;
			}
			longLogTimer = cur;
			fprintf(file, "\n-------------------------------------MONITERING----------------------------------------\n");
			fprintf(file, "| After Server On : %u day / %u h / %u m / %u s\n", day, hour, minute, sec);
			fprintf(file, "|----------------------------------------POOL------------------------------------------\n");
			fprintf(file, "| LockFreeQueue Size / Capacity / Max : %u / %u / %u\n", info._SessionSendQueueSize, info._SessionSendQueueCapacity, info._SessionSendQueueMax);
			fprintf(file, "| Packet Chunk Capacity / UseCount : %d / %d\n", Packet::GetCapacityCount(), Packet::GetUseCount());
			fprintf(file, "| \n");
			fprintf(file, "| Packet ToTal UseCount : %d\n", Packet::GetTotalPacketCount());
			fprintf(file, "| Available / NPPool / Private Mem : %lluMb / %lluMb / %lluKb\n", this->_HardWareMoniter.AvailableMemoryMBytes(), this->_HardWareMoniter.NonPagedPoolMBytes(), this->_ProcessMoniter.PrivateMemoryKBytes());
			fprintf(file, "---------------------------------------------------------------------------------------\n");
			fclose(file);
		}
		return -1;
	}
	unsigned __stdcall MoniteringThread(void* param)
	{
		BaseServer* server = (BaseServer*)param;
		if (server == nullptr)
			CRASH();
		return server->MoniteringThreadProc();
	}


	/// BASE_SERVER_DEF
	/// ---------------------------------------------------------------------------------------
	/// ---------------------------------------------------------------------------------------
	/// ---------------------------------------------------------------------------------------
	/// ---------------------------------------------------------------------------------------
}

