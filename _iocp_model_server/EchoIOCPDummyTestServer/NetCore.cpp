#include <time.h>
#include <process.h>
#include <conio.h>
#include "NetCore.h"


namespace univ_dev
{
	DWORD NetCore::serverOnFlag = false;
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	//Worker Thread Wrapping function
	unsigned __stdcall WorkerThread(void* param)
	{
		//WorkerThread -> NetCore::Run 이 호출될때까지 막혀있다가 NetCore::Run이 호출되어야 시작
		NetCore* core = (NetCore*)param;
		core->threadIds[GetCurrentThreadId()]++;
		WaitForSingleObject(core->runningEvent, INFINITE);
		return core->NetCoreWorkerThread(param);
	}

	//Accept Thread Wrapping function
	unsigned __stdcall AcceptThread(void* param)
	{
		//AcceptThread -> NetCore::Run 이 호출될때까지 막혀있다가 NetCore::Run이 호출되어야 시작
		NetCore* core = (NetCore*)param;
		WaitForSingleObject(core->runningEvent, INFINITE);
		return core->NetCoreAcceptThread(param);
	}

	//Monitering Thread Warpping function
	unsigned __stdcall MoniteringThread(void* param)
	{
		//MoniteringThread -> NetCore::Run 이 호출될때까지 막혀있다가 NetCore::Run이 호출되어야 시작
		NetCore* core = (NetCore*)param;
		WaitForSingleObject(core->runningEvent, INFINITE);
		return core->NetCoreMoniteringThread(param);
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------



	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	//RecvPost -> WSARecv call
	void NetCore::RecvPost(Session* session)
	{
		int recvRet;
		WSABUF recvWSABuf[2];
		recvWSABuf[0].buf = session->recvJob.ringBuffer.GetWritePtr();
		recvWSABuf[0].len = session->recvJob.ringBuffer.DirectEnqueueSize();
		recvWSABuf[1].buf = session->recvJob.ringBuffer.GetBeginPtr();
		recvWSABuf[1].len = session->recvJob.ringBuffer.GetFreeSize() - session->recvJob.ringBuffer.DirectEnqueueSize();
		if (recvWSABuf[1].len > session->recvJob.ringBuffer.GetBufferSize())
			recvWSABuf[1].len = 0;
		DWORD byteRecvs;
		DWORD flag = 0;
		ZeroMemory(&session->recvJob.overlapped, sizeof(OVERLAPPED));
		//printf("        WSARecv Call\n");
		InterlockedIncrement(&session->ioCounts);
		recvRet = WSARecv(session->sock, recvWSABuf, 2, &byteRecvs, &flag, &session->recvJob.overlapped, nullptr);
		if (recvRet == 0)
		{
			InterlockedIncrement(&this->recvSuccessCount);
		}
		else if (recvRet == SOCKET_ERROR)
		{
			int err = WSAGetLastError();
			if (err != WSA_IO_PENDING)
			{
				if (err != 10054 && err != 10063)
					printf("WSARecv(), err is not WSA_IO_PENDING : %d\n", err);
				InterlockedDecrement(&session->ioCounts);
			}
			else
			{
				InterlockedIncrement(&this->recvIOPendingCount);
			}
		}
	}
	//---------------------------------------------------------------------------------------------------------------------------------

	//SendPost -> WSASend call
	void NetCore::SendPost(Session* session)
	{
		if (InterlockedExchange(&session->ioFlag, true) == false)
		{
			int useSize = session->sendJob.ringBuffer.GetUseSize();
			char* readPtr = session->sendJob.ringBuffer.GetReadPtr();
			int directDqSize = session->sendJob.ringBuffer.DirectDequeueSize();
			if (useSize > 0)
			{
				int sendRet = 0;
				DWORD byteSents = 0;
				ZeroMemory(&session->sendJob.overlapped, sizeof(OVERLAPPED));
				session->sendJob.isRecv = false;
				//printf("        WSASend Call Processed Bytes : %d\n", processBytes);
				WSABUF sendWSABuf[2];
				sendWSABuf[0].buf = readPtr;
				sendWSABuf[0].len = directDqSize;
				sendWSABuf[1].buf = session->sendJob.ringBuffer.GetBeginPtr();
				sendWSABuf[1].len = useSize - directDqSize;
				if (sendWSABuf[1].len > session->sendJob.ringBuffer.GetBufferSize())
					sendWSABuf[1].len = 0;
				InterlockedIncrement(&session->ioCounts);
				sendRet = WSASend(session->sock, sendWSABuf, 2, &byteSents, 0, &session->sendJob.overlapped, nullptr);
				if (sendRet == 0)
				{
					InterlockedIncrement(&this->sendSuccessCount);
					//printf("Session -> %llu send completion\n", session->sessionID);
				}
				else if (sendRet == SOCKET_ERROR)
				{
					int err = WSAGetLastError();
					if (err != WSA_IO_PENDING)
					{
						if (err != 10054 && err != 10063)
							printf("WSASend(), err is not WSA_IO_PENDING : %d\n", err);
						InterlockedDecrement(&session->ioCounts);
					}
					else
					{
						InterlockedIncrement(&this->sendIOPendingCount);
						//printf("Session -> %llu send completion\n", session->sessionID);
					}
				}
			}
			else
			{
				InterlockedExchange(&session->ioFlag, false);
			}
		}
	}
	//---------------------------------------------------------------------------------------------------------------------------------

	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// SendPacket -> Enqueuing Send Ringbuffer
	void NetCore::SendPacket(ULONGLONG sessionID, Packet* packet)
	{
		Session* session = this->FindSession(sessionID);
		//Session* session = this->FindAndLockSession(sessionID);
		if (session == nullptr)
		{
			printf("SendPacket : Session is nullptr\n");
			return;
		}
		int eqRet = session->sendJob.ringBuffer.Enqueue(packet->GetReadPtr(), packet->GetBufferSize());
		if (eqRet != packet->GetBufferSize())
		{
			//printf("eqRet is not header.payloadSize + dfECHO_PACKET_HEADER_LENGTH : %d EQRET : %d\n", packet->GetBufferSize() + dfECHO_PACKET_HEADER_LENGTH, eqRet);
			//this->UnlockSession(session);
			return;
		}
		this->SendPost(session);
		//this->UnlockSession(session);
	}
	//---------------------------------------------------------------------------------------------------------------------------------

	void NetCore::SendPacket(Session* session, Packet* packet)
	{
		if (session == nullptr)
		{
			printf("SendPacket : Session is nullptr\n");
			return;
		}
		int eqRet = session->sendJob.ringBuffer.Enqueue(packet->GetReadPtr(), packet->GetBufferSize());
		if (eqRet != packet->GetBufferSize())
		{
			//printf("eqRet is not header.payloadSize + dfECHO_PACKET_HEADER_LENGTH : %d EQRET : %d\n", packet->GetBufferSize() + dfECHO_PACKET_HEADER_LENGTH, eqRet);
			//this->UnlockSession(session);
			return;
		}
		this->SendPost(session);
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------



	// Override this function
	void NetCore::OnRecv(Session* session, Packet* recvPacket, Packet* sendPacket, EchoPacketHeader header)
	{
		ULONGLONG data;
		(*recvPacket) >> data;
		//뭐... 중간에 데이터 가지고 어떤 작업을 했다고 가정하고
		(*sendPacket) << header.payloadSize << data;
		SendPacket(session->sessionID, sendPacket);
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------

	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// Run 호출전에는 절대로 다른 스레드들이 작동하지 않음 주의바람
	void NetCore::Run()
	{
		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------
		// 작동중인 스레드들은 이 이벤트를 바라보고있고 메뉴얼 이벤트.
		// 이벤트를 셋하면 메뉴얼 이벤트이기 때문에 모든 스레드를 다깨운다.
		SetEvent(this->runningEvent);
		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------


		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------
		// 모든 스레드들이 작업이 끝날때까지 대기하는 메인스레드
		HANDLE* runningThreads = new HANDLE[(size_t)this->threadPoolSize + ACCEPT_THREAD_COUNT + MONITERING_THREAD_COUNT];

		for (int i = 0; i < this->threadPoolSize; i++)
			runningThreads[i] = this->workerThread[i];

		runningThreads[this->threadPoolSize] = this->acceptThread;
		runningThreads[this->threadPoolSize + 1] = this->logThread;

		WaitForMultipleObjects(this->threadPoolSize + ACCEPT_THREAD_COUNT + MONITERING_THREAD_COUNT, runningThreads, true, INFINITE);

		delete[] runningThreads;
		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------


		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------
		//종료 작업을 위한 로그 파일 오픈
		FILE* MainThreadLogFile = nullptr;
		char logFileName[128]{ 0 };
		char tempStr[20]{ 0 };
		time_t currentTime = time(nullptr);
		tm time;
		localtime_s(&time, &currentTime);
		_itoa_s(time.tm_year + 1900, tempStr, 10);
		strcat_s(logFileName, tempStr);
		strcat_s(logFileName, "_");
		_itoa_s(time.tm_mon + 1, tempStr, 10);
		strcat_s(logFileName, tempStr);
		strcat_s(logFileName, "_");
		_itoa_s(time.tm_mday, tempStr, 10);
		strcat_s(logFileName, tempStr);
		strcat_s(logFileName, "_NetCoreInitialize.txt");


		while (MainThreadLogFile == nullptr)
		{
			printf("MainThreadLogFile == nullptr\n");
			fopen_s(&MainThreadLogFile, logFileName, "ab");
		}

		printf("WaitForMultipleObjects returned -> All Thread Stopped -> MainThread Stop\n");
		fprintf(MainThreadLogFile, "WaitForMultipleObjects returned->All Thread Stopped->MainThread Stop\n");

		fprintf(MainThreadLogFile, "-------------------------------------------------------\n");
		fprintf(MainThreadLogFile, "-------------------------------------------------------\n");
		fclose(MainThreadLogFile);
		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------

	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------

	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// MoniteringThread에서 Post한 경우 FALSE반환 그외에는 TRUE반환 TRUE반환시 socket이 INVALID_SOCKET이라면 accept error
	BOOL NetCore::TryAccept(SOCKET& clientSocket, sockaddr_in& clientAddr)
	{
		int addrlen = sizeof(clientAddr);
		clientSocket = accept(this->listenSocket, (SOCKADDR*)&clientAddr, &addrlen);
		if (clientSocket == INVALID_SOCKET)
		{
			int err = WSAGetLastError();
			if (err == WSAEINTR || err == WSAENOTSOCK || err == WSAEINVAL)
			{
				ULONG_PTR completionKey = (ULONG_PTR)0xffffffff;
				DWORD byteTransfered = 0;
				PostQueuedCompletionStatus(this->IOCP, byteTransfered, completionKey, nullptr);
				printf("Accept Thread End\n");
				return false;
			}
			printf("client_sock == INVALID_SOCKET : %d\n", err);
		}
		return true;
	}

	// if this function returns false there's no data in packet
	BOOL NetCore::TryGetCompletedPacket(Session* session, Packet* packet, EchoPacketHeader& header)
	{
		//EchoPacketHeader header;
		//GetCompletedPacket(session, recvPacket, header);
		//printf("Cycle %d\n", ++i);
		//링버퍼의 현재 사이즈가 헤더의 길이보다 작으면 더이상 뽑을게 없다는 의미임.
		if (session->recvJob.ringBuffer.GetUseSize() < dfECHO_PACKET_HEADER_LENGTH)
		{
			//printf("this point RingBuffer.GetUseSize() -> %d\n",session->recvJob.ringBuffer.GetUseSize());
			return false;
		}
		//헤더하나 만든뒤 피크해서 2바이트가 제대로 피크됬는지 확인
		//EchoPacketHeader header;
		int pkRet1 = session->recvJob.ringBuffer.Peek((char*)&header, dfECHO_PACKET_HEADER_LENGTH);
		if (pkRet1 != dfECHO_PACKET_HEADER_LENGTH)
		{
			printf("pkRet is not dfECHO_PACKET_HEADER_LENGTH : %d\n", pkRet1);
			return false;
		}
		//printf("payloadSize : %d\n", header.payloadSize);
		//recv링버퍼에서 피크 했을때 payloadSize랑 합한것보다 링버퍼 사이즈가 작으면 다음번으로 넘긴다.
		if (session->recvJob.ringBuffer.GetUseSize() < dfECHO_PACKET_HEADER_LENGTH + header.payloadSize)
			//if (session->recvJob.ringBuffer.GetUseSize() < dfECHO_PACKET_HEADER_LENGTH + 8)
		{
			printf("job->ringBuffer.GetUseSize() < dfECHO_PACKET_HEADER_LENGTH + header.payloadSize :%d, %d PKRET = %d\n", session->recvJob.ringBuffer.GetUseSize(), dfECHO_PACKET_HEADER_LENGTH + header.payloadSize, pkRet1);
			return false;
		}
		//recv 링버퍼에서 데이터를 꺼냈으니 ReadPointer를 증가시켜서 UseSize를 줄여야함.
		session->recvJob.ringBuffer.MoveReadPtr(pkRet1);
		//payloadSize만큼 recv 링버퍼에서 다시 꺼내온다
		int pkRet2 = session->recvJob.ringBuffer.Peek(packet->GetWritePtr(), header.payloadSize);
		if (pkRet2 != header.payloadSize)
		{
			printf("pkRet2 is not header.payloadSize : %d PKRET : %d\n", header.payloadSize, pkRet2);
			return false;
		}

		session->recvJob.ringBuffer.MoveReadPtr(pkRet2);
		packet->MoveWritePtr(pkRet2);
		return true;
	}



	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// SendProc
	void NetCore::SendProc(Session* session, DWORD byteTransfered)
	{
		int sendRet = 0;
		session->sendJob.ringBuffer.MoveReadPtr(byteTransfered);
		InterlockedAdd64(&totalProcessedBytes, byteTransfered);
		InterlockedExchange(&session->ioFlag, false);
		SendPost(session);

	}

	// RecvProc
	void NetCore::RecvProc(Session* session, DWORD byteTransfered)
	{
		if (this->profilingFlag >= PROFILING_FLAG::MAIN_LOOP_FLAG)
		{
			Profiler recvLoopProfile("recv loop", GetCurrentThreadId());
			//printf("RecvJob GQCS return\n");
			//완료된 사이즈만큼 writePointer를 증가시킨다.
			session->recvJob.ringBuffer.MoveWritePtr(byteTransfered);
			//링버퍼에 있는거 하나하나 전부다 직렬화 버퍼로 가져올거임.
			//Packet* recvPacket = this->packetPool.Alloc();
			//Packet* sendPacket = this->packetPool.Alloc();
			Packet recvPacket;
			Packet sendPacket;

			while (true)
			{
				InterlockedIncrement(&this->totalPacket);
				InterlockedIncrement(&this->packetPerSec);
				recvPacket.Clear();
				sendPacket.Clear();
				if (this->profilingFlag >= PROFILING_FLAG::PACKET_PROCESS_LOOP_FLAG)
				{
					univ_dev::Profiler packetProcessProfiler("Packet Process Profiler", GetCurrentThreadId());
					EchoPacketHeader header;
					//recvPacket->Clear();
					//sendPacket->Clear();
					//if (TryGetCompletedPacket(session, recvPacket, header))
						//OnRecv(session, recvPacket, sendPacket, header);
					if (this->TryGetCompletedPacket(session, &recvPacket, header))
					{
						this->OnRecv(session, &recvPacket, &sendPacket, header);
					}
					else break;
				}
				else
				{
					EchoPacketHeader header;
					//recvPacket->Clear();
					//sendPacket->Clear();
					//if (TryGetCompletedPacket(session, recvPacket, header))
						//OnRecv(session, recvPacket, sendPacket, header);
					if (this->TryGetCompletedPacket(session, &recvPacket, header))
					{
						//printf("Session -> %llu Get 1 Packet Completion\n", session->sessionID);
						this->OnRecv(session, &recvPacket, &sendPacket, header);
					}
					else break;
				}
			}
			//recvPacket->Clear();
			//this->packetPool.Free(recvPacket);
			//this->packetPool.Free(sendPacket);
			this->RecvPost(session);
			return;
		}
		//printf("RecvJob GQCS return\n");
		//완료된 사이즈만큼 writePointer를 증가시킨다.
		session->recvJob.ringBuffer.MoveWritePtr(byteTransfered);
		//링버퍼에 있는거 하나하나 전부다 직렬화 버퍼로 가져올거임.
		//Packet* recvPacket = this->packetPool.Alloc();
		//Packet* sendPacket = this->packetPool.Alloc();
		Packet recvPacket;
		Packet sendPacket;
		while (true)
		{
			InterlockedIncrement(&this->totalPacket);
			InterlockedIncrement(&this->packetPerSec);
			recvPacket.Clear();
			sendPacket.Clear();
			if (this->profilingFlag >= PROFILING_FLAG::PACKET_PROCESS_LOOP_FLAG)
			{
				Profiler packetProcessProfiler("Packet Process Profiler", GetCurrentThreadId());
				EchoPacketHeader header;
				//recvPacket->Clear();
				//sendPacket->Clear();
				//if (TryGetCompletedPacket(session, recvPacket, header))
					//OnRecv(session, recvPacket, sendPacket, header);
				if (this->TryGetCompletedPacket(session, &recvPacket, header))
					this->OnRecv(session, &recvPacket, &sendPacket, header);
				else break;
			}
			else
			{
				EchoPacketHeader header;
				//recvPacket->Clear();
				//sendPacket->Clear();
				//if (TryGetCompletedPacket(session, recvPacket, header))
					//OnRecv(session, recvPacket, sendPacket, header);
				if (this->TryGetCompletedPacket(session, &recvPacket, header))
					this->OnRecv(session, &recvPacket, &sendPacket, header);
				else break;
			}
		}
		//recvPacket->Clear();
		//this->packetPool.Free(recvPacket);
		//this->packetPool.Free(sendPacket);
		this->RecvPost(session);
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------


	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// Accept Thread
	unsigned int NetCore::NetCoreAcceptThread(void* param)
	{
		ULONGLONG sessionID = 0;
		SOCKET client_sock;
		sockaddr_in clientaddr;
		int addrlen;
		DWORD recvbytes, flags;
		DWORD retval;
		while (true)
		{
			if (!this->TryAccept(client_sock, clientaddr)) return 0;
			else if (client_sock == INVALID_SOCKET) continue;

			Session* newSession = this->CreateSession(client_sock, clientaddr, ++sessionID);
			this->RecvPost(newSession);
		}
		return -1;
	}
	//---------------------------------------------------------------------------------------------------------------------------------

	// Worker Thread
	unsigned int NetCore::NetCoreWorkerThread(void* param)
	{
		DWORD byteTransfered = 0;
		ULONG_PTR completionKey = 0;
		OVERLAPPED* over = nullptr;
		JobInfo* job = nullptr;
		Session* session = nullptr;
		int GQCSRet = 0;
		while (true)
		{
			byteTransfered = 0;
			completionKey = 0;
			over = nullptr;
			job = nullptr;
			session = nullptr;
			GQCSRet = 0;

			GQCSRet = GetQueuedCompletionStatus(this->IOCP, &byteTransfered, &completionKey, &over, INFINITE);
			_InterlockedIncrement((LONG*)&this->threadIds[GetCurrentThreadId()]);
			if (over == nullptr)
			{
				if (completionKey == 0xffffffff)
				{
					printf("ThreadID : %d\nCompletionKey : %llu     Thread End\n", GetCurrentThreadId(), completionKey);
					ULONG_PTR completionKey = (ULONG_PTR)0xffffffff;
					DWORD byteTransfered = 0;
					PostQueuedCompletionStatus(this->IOCP, byteTransfered, completionKey, nullptr);
					return 0;
				}
				int err = WSAGetLastError();
				printf("over is nullptr Error code : %d\n", err);
				continue;
			}
			session = (Session*)completionKey;		

			job = (JobInfo*)over;
			//recv 완료처리라면
			this->LockSession(session);
			if (GQCSRet != 0 && byteTransfered != 0 && job->isRecv)
			{
				//printf("RECVPROC -> %llu Session Process Byte Transfered -> %d\n", session->sessionID, byteTransfered);
				this->RecvProc(session, byteTransfered);
			}
			//send 완료 처리라면
			else if (GQCSRet != 0 && byteTransfered != 0)
			{
				//printf("SENDPROC -> %llu Session Process\n", session->sessionID);
				SendProc(session, byteTransfered);
			}
			this->UnlockSession(session);
			int ioCounts = InterlockedDecrement(&session->ioCounts);
			if (ioCounts == 0)
			{
				//printf("ioCount == 0  and current Session->ioCount : %d\n", session->ioCounts);
				this->ReleaseSession(session->sessionID);
			}
		}
		return -1;
	}
	//---------------------------------------------------------------------------------------------------------------------------------

	// Monitering Thread
	unsigned int NetCore::NetCoreMoniteringThread(void* param)
	{
		printf("NetCore::NetCoreMoniteringThread Running...\n");
		ULONGLONG beginTime = GetTickCount64();
		DWORD prev = timeGetTime();
		while (true)
		{
			DWORD cur = timeGetTime();
			ULONGLONG now = GetTickCount64();
			if (_kbhit())
			{
				int key = _getch();
				key = tolower(key);
				if (key == 's' || key == 'q')
				{
					univ_dev::SaveProfiling();
					univ_dev::ResetProfiling();
				}
				if (key == 'q')
				{
					this->shutDownFlag = true;
					closesocket(this->listenSocket);
					printf("Monitering Thread End\n");
					return 0;
				}
				if (key == 'p')
				{
					switch (this->profilingFlag)
					{
					case PROFILING_FLAG::OFF_FLAG:
						this->profilingFlag = PROFILING_FLAG::MAIN_LOOP_FLAG;
						break;
					case PROFILING_FLAG::MAIN_LOOP_FLAG:
						this->profilingFlag = PROFILING_FLAG::PACKET_PROCESS_LOOP_FLAG;
						break;
					case PROFILING_FLAG::PACKET_PROCESS_LOOP_FLAG:
						this->profilingFlag = PROFILING_FLAG::OFF_FLAG;
						break;
					}
				}
			}
			if (cur - prev < 1000)
			{
				Sleep(50);
				continue;
			}
			prev = cur;
			system("cls");
			const char* str = nullptr;
			switch (this->profilingFlag)
			{
			case PROFILING_FLAG::OFF_FLAG:
				str = "PROFILING_OFF_FLAG";
				break;
			case PROFILING_FLAG::MAIN_LOOP_FLAG:
				str = "MAIN_LOOP_PROFILING_FLAG";
				break;
			case PROFILING_FLAG::PACKET_PROCESS_LOOP_FLAG:
				str = "MAIN_LOOP + PACKET_PROCESS_LOOP_FLAG";
				break;
			default:
				str = "DEFAULT";
				break;
			}
			unsigned long long r_TPS = InterlockedExchange(&this->packetPerSec, 0);
			printf("\n-------------------------------------------------------------------------\nServerControl\nSaveProfile = \'s\'\nQuitProgram = \'q\'\nChangeProfilingFlag = \'p\'\n-------------------------------------------------------------------------\nNumOfThread = %d\nRunningThread = %d\nCurrentProfilingFlag = %s\nExcuteTime : %llu\nTPS : %llu\nTotalProcessedBytes : %llu\nTotal Packet Process : %llu\ng_SendSuccessCount : %llu\ng_SendIOPendingCount : %llu\ng_RecvSuccessCount : %llu\ng_RecvIOPendingCount : %llu\nMEMORY_FREE_LIST\nPacketPool Capacity : %d\nPacketPool UseCount : %d\nSessionPool Capacity : %d\nSessionPool UseCount : %d\nTotalRemovedSessionCounts :%llu\n\n-------------------------------------------------------------------------\n",this->threadPoolSize,this->runningThreadCount, str, (now - beginTime) / 1000, r_TPS, this->totalProcessedBytes, this->totalPacket, this->sendSuccessCount, this->sendIOPendingCount, this->recvSuccessCount, this->recvIOPendingCount, this->packetPool.GetCapacityCount(), this->packetPool.GetUseCount(), this->sessionPool.GetCapacityCount(), this->sessionPool.GetUseCount(),this->totalReleasedSession);
			int cnt = 0;
			for (auto iter = this->threadIds.begin(); iter != this->threadIds.end(); ++iter)
			{
				printf("Thread : %d , Count : %d\t\t\t", iter->first, iter->second);
				if (++cnt % 2 == 0)
					printf("\n");
			}
		}
		return -1;
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------

	// Net Core Cleanup
	void NetCore::NetCoreCleanup()
	{
		WSACleanup();
		CloseHandle(this->IOCP);
		CloseHandle(this->runningEvent);
		for (auto iter = this->sessionMap.begin(); iter != this->sessionMap.end(); ++iter)
		{
			closesocket(iter->second->sock);
			this->sessionPool.Free(iter->second);
		}
		this->sessionMap.clear();
		delete[] this->workerThread;
		InterlockedExchange(&this->serverOnFlag, false);
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------



	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// 세션관리 함수
	Session* NetCore::FindAndLockSession(ULONGLONG key)
	{
		this->SessionMapLock();
		auto iter = this->sessionMap.find(key);
		if (iter == this->sessionMap.end())
		{
			this->SessionMapUnlock();
			return nullptr;
		}
		Session* session = iter->second;
		this->LockSession(session);
		this->SessionMapUnlock();
		return session;
	}

	Session* NetCore::FindSession(ULONGLONG sessionID)
	{
		this->SessionMapLock();
		auto iter = this->sessionMap.find(sessionID);
		if (iter == this->sessionMap.end())
		{
			this->SessionMapUnlock();
			return nullptr;
		}
		Session* session = iter->second;
		this->SessionMapUnlock();
		return session;
	}

	void NetCore::SessionMapLock()
	{
		AcquireSRWLockExclusive(&sessionMapLock);
	}

	void NetCore::SessionMapUnlock()
	{
		ReleaseSRWLockExclusive(&sessionMapLock);
	}

	void NetCore::LockSession(Session* session)
	{
		if (session == nullptr)return;
		AcquireSRWLockExclusive(&session->lock);
	}

	void NetCore::UnlockSession(Session* session)
	{
		if (session == nullptr) return;
		ReleaseSRWLockExclusive(&session->lock);
	}


	//---------------------------------------------------------------------------------------------------------------------------------
	// 세션 생성 함수
	Session* NetCore::CreateSession(SOCKET key, sockaddr_in clientaddr,ULONGLONG sessionId)
	{
		Session* newSession = new Session();
		//Session* newSession = this->sessionPool.Alloc();
		newSession->ip = clientaddr.sin_addr.S_un.S_addr;
		newSession->port = clientaddr.sin_port;
		newSession->ioCounts = 0;
		newSession->sock = key;
		newSession->recvJob.isRecv = true;
		newSession->sendJob.isRecv = false;
		newSession->sessionID = sessionId;
		ZeroMemory(&newSession->recvJob.overlapped, sizeof(OVERLAPPED));
		ZeroMemory(&newSession->sendJob.overlapped, sizeof(OVERLAPPED));
		InitializeSRWLock(&newSession->lock);
		this->SessionMapLock();
		this->sessionMap.emplace(std::make_pair(newSession->sessionID, newSession));
		CreateIoCompletionPort((HANDLE)key, this->IOCP, (ULONG_PTR)newSession, 0);
		this->SessionMapUnlock();
		return newSession;
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	// 세션 제거함수
	void NetCore::ReleaseSession(ULONGLONG sessionID)
	{
		this->SessionMapLock();
		auto iter = this->sessionMap.find(sessionID);
		if (iter == this->sessionMap.end())
		{
			printf("ReleaseSession -> Iter is sessionMap.end()  -> session id : %llu\n", sessionID);
			return;
		}
		Session* removeSession = iter->second;
		this->LockSession(removeSession);
		//removeSession->ip = 0;
		//removeSession->port = 0;
		//removeSession->ioCounts = 0;
		//removeSession->sock = 0;
		//removeSession->recvJob.isRecv = true;
		//removeSession->sendJob.isRecv = false;
		//removeSession->sessionID = 0;
		//ZeroMemory(&removeSession->recvJob.overlapped, sizeof(OVERLAPPED));
		//ZeroMemory(&removeSession->sendJob.overlapped, sizeof(OVERLAPPED));
		this->sessionMap.erase(sessionID);
		this->SessionMapUnlock();
		this->UnlockSession(removeSession);
		delete removeSession;
		InterlockedIncrement(&totalReleasedSession);
		//this->sessionPool.Free(removeSession);
		return;
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	// 연결 종료 함수
	void NetCore::DisconnectSession(ULONGLONG sessionID)
	{
		Session* disconnectSession = this->FindSession(sessionID);
		if (disconnectSession == nullptr)
		{
			printf("DisconnectSession -> Session is nullptr\n");
			return;
		}
		closesocket(disconnectSession->sock);
		//this->UnlockSession(disconnectSession);
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------




	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// 생성자 소멸자
	NetCore::NetCore(USHORT port, DWORD backlogQueueSize, DWORD threadPoolSize, DWORD runningThread)
		: serverPort{ port }, shutDownFlag{ false }, APIErrorCode{ 0 }, errorCode{ 0 }, IOCP{ nullptr }, acceptThread{ nullptr }, packetPerSec{ 0 }, recvIOPendingCount{ 0 }, recvSuccessCount{ 0 }, sendIOPendingCount{ 0 }, sendSuccessCount{ 0 }, totalPacket{ 0 }, listenSocket{ 0 }, logThread{ nullptr }, workerThread{ nullptr }, threadPoolSize{ threadPoolSize }, runningThreadCount{ runningThread }, runningEvent{ nullptr }, profilingFlag{ PROFILING_FLAG::OFF_FLAG }, totalProcessedBytes{ 0 }, sessionMapLock{ 0 },totalReleasedSession{0}
	{
		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------
		// 인터락으로 서버 플래그를 true로 만들었을때 내가 true로 만들어야지만
		DWORD ret = InterlockedExchange(&this->serverOnFlag, true);
		if (ret == true)
		{
			this->errorCode = dfNCINIT_NET_CORE_ALREADY_EXIST;
			return;
		}
		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------


		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------
		// NetCore 로깅용 파일 오픈
		FILE* MainThreadLogFile = nullptr;
		char logFileName[128]{ 0 };
		char tempStr[20]{ 0 };
		time_t currentTime = time(nullptr);
		tm time;
		localtime_s(&time, &currentTime);
		_itoa_s(time.tm_year + 1900, tempStr, 10);
		strcat_s(logFileName, tempStr);
		strcat_s(logFileName, "_");
		_itoa_s(time.tm_mon + 1, tempStr, 10);
		strcat_s(logFileName, tempStr);
		strcat_s(logFileName, "_");
		_itoa_s(time.tm_mday, tempStr, 10);
		strcat_s(logFileName, tempStr);
		strcat_s(logFileName, "_NetCoreInitialize.txt");

		while (MainThreadLogFile == nullptr)
		{
			printf("MainThreadLogFile == nullptr\n");
			fopen_s(&MainThreadLogFile, logFileName, "ab");
		}
		fprintf(MainThreadLogFile, "-------------------------------------------------------\n");
		fprintf(MainThreadLogFile, "-------------------------------------------------------\n");
		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------


		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------
		// 작업자 스레드들이 메인 스레드에서 NetCore::Run이라는 함수를 호출하기 전까지는 블락되어있음
		this->runningEvent = CreateEvent(nullptr, true, false, nullptr);
		if (this->runningEvent == nullptr)
		{
			InterlockedExchange(&this->serverOnFlag, false);
			this->errorCode = dfNCINIT_RUNNING_EVENT_CREATE_FAILED;
			return;
		}
		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------


		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------
		// 윈속 초기화
		WSADATA wsa;
		printf("WSAStartUp Begin\n");
		fprintf(MainThreadLogFile, "WSAStartUp Begin\n");
		if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		{
			InterlockedExchange(&this->serverOnFlag, false);
			this->errorCode = dfNCINIT_WSASTARTUP_FAILED;
			return;
		}
		fprintf(MainThreadLogFile, "WSAStartUp End\n");
		printf("WSAStartUp End\n");
		fflush(MainThreadLogFile);
		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------


		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------
		// IOCP 생성
		if (this->threadPoolSize <= 0)
		{
			SYSTEM_INFO info;
			GetSystemInfo(&info);
			this->threadPoolSize = info.dwNumberOfProcessors * 2;
		}
		printf("IOCP CreateBegin\n");
		fprintf(MainThreadLogFile, "IOCP CreateBegin\n");
		if (this->runningThreadCount > this->threadPoolSize)
			this->runningThreadCount = this->threadPoolSize;
		this->IOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, runningThreadCount);
		if (this->IOCP == NULL)
		{
			InterlockedExchange(&this->serverOnFlag, false);
			this->errorCode = dfNCINIT_IOCOMPLETIONPORT_CREATE_FAILED;
			this->APIErrorCode = GetLastError();
			return;
		}
		fprintf(MainThreadLogFile, "IOCP CreateEnd\n");
		printf("IOCP CreateEnd\n");
		fflush(MainThreadLogFile);
		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------


		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------
		// 타이머 인터럽트 해상도 상향
		// 세션 컨테이너에 대한 SRWLock객체 초기화
		// 프로파일러 및 샘플들 초기화
		printf("timeBeginPeriod, SRWLock Init, ProfilerSample Init Begin\n");
		fprintf(MainThreadLogFile, "timeBeginPeriod, SRWLock Init, ProfilerSample Init Begin\n");
		timeBeginPeriod(1);
		InitializeSRWLock(&sessionMapLock);
		univ_dev::InitializeProfilerAndSamples();
		fprintf(MainThreadLogFile, "timeBeginPeriod, SRWLock Init, ProfilerSample Init End\n");
		printf("timeBeginPeriod, SRWLock Init, ProfilerSample Init End\n");
		fflush(MainThreadLogFile);
		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------


		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------
		// 초단위 로깅용 스레드 생성
		printf("PrintThread CreateBegin\n");
		fprintf(MainThreadLogFile, "PrintThread CreateBegin\n");
		this->logThread = (HANDLE)_beginthreadex(nullptr, 0, MoniteringThread, this, 0, nullptr);
		if (this->logThread == nullptr)
		{
			InterlockedExchange(&this->serverOnFlag, false);
			this->errorCode = dfNCINIT_LOG_THREAD_CREATE_FAILED;
			this->APIErrorCode = GetLastError();
			return;
		}
		fprintf(MainThreadLogFile, "PrintThread CreateEnd\n");
		printf("PrintThread CreateEnd\n");
		fflush(MainThreadLogFile);
		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------


		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------
		// 스레드풀 생성, 
		// 스레드풀 사이즈는 0이면 systeminfo에서 받아온 논리코어 * 2개만큼 위에서적용
		printf("ThreadPool CreateBegin\n");
		fprintf(MainThreadLogFile, "ThreadPool CreateBegin\n");
		this->workerThread = new HANDLE[this->threadPoolSize];
		for (int i = 0; i < (int)this->threadPoolSize; i++)
		{
			this->workerThread[i] = (HANDLE)_beginthreadex(NULL, 0, WorkerThread, this, 0, NULL);
			if (this->workerThread[i] == NULL)
			{
				fclose(MainThreadLogFile);
				InterlockedExchange(&this->serverOnFlag, false);
				this->errorCode = dfNCINIT_WORKER_THREAD_CREATE_FAILED_0 + i;
				this->APIErrorCode = GetLastError();
				return;
			}
		}
		fprintf(MainThreadLogFile, "ThreadPool CreateEnd\n");
		printf("ThreadPool CreateEnd\n");
		fflush(MainThreadLogFile);
		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------


		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------
		// 리슨소켓 생성
		printf("listen socket CreateBegin\n");
		fprintf(MainThreadLogFile, "listen socket CreateBegin\n");
		this->listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (this->listenSocket == INVALID_SOCKET)
		{
			int err = WSAGetLastError();
			printf("listen socket is INVALID_SOCKET : %d\n", err);
			fprintf(MainThreadLogFile, "listen socket is INVALID_SOCKET : %d\n", err);
			fclose(MainThreadLogFile);
			InterlockedExchange(&this->serverOnFlag, false);
			this->errorCode = dfNCINIT_LISTEN_SOCKET_CREATE_FAILED;
			this->APIErrorCode = err;
			return;
		}
		fprintf(MainThreadLogFile, "listen socket CreateEnd\n");
		printf("listen socket CreateEnd\n");
		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------


		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------
		// 리슨소켓에 IP Port 바인딩
		SOCKADDR_IN serveraddr;
		ZeroMemory(&serveraddr, sizeof(serveraddr));
		serveraddr.sin_family = AF_INET;
		serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
		serveraddr.sin_port = htons(this->serverPort);
		printf("Port : %d BindBegin\n", ntohs(serveraddr.sin_port));
		fprintf(MainThreadLogFile, "Port : %d BindBegin\n", ntohs(serveraddr.sin_port));
		int bindRet = bind(this->listenSocket, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
		if (bindRet == SOCKET_ERROR)
		{
			int err = WSAGetLastError();
			printf("bind retval is SOCKET_ERROR : %d\n", err);
			fprintf(MainThreadLogFile, "bind retval is SOCKET_ERROR : %d\n", err);
			fclose(MainThreadLogFile);
			InterlockedExchange(&this->serverOnFlag, false);
			this->APIErrorCode = err;
			this->errorCode = dfNCINIT_SOCKET_BIND_FAILED;
			return;
		}
		fprintf(MainThreadLogFile, "Port : %d BindEnd\n", ntohs(serveraddr.sin_port));
		printf("Port : %d BindEnd\n", ntohs(serveraddr.sin_port));
		fflush(MainThreadLogFile);
		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------


		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------
		// 링거 옵션 적용으로 TIME_WAIT 삭제
		LINGER l;
		l.l_onoff = 1;
		l.l_linger = 0;
		printf("Linger Option Setting Begin\n");
		fprintf(MainThreadLogFile, "Linger Option Setting Begin\n");
		setsockopt(this->listenSocket, SOL_SOCKET, SO_LINGER, (const char*)&l, sizeof(l));
		fprintf(MainThreadLogFile, "Linger Option Setting End\n");
		printf("Linger Option Setting End\n");
		fflush(MainThreadLogFile);
		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------


		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------
		// Send IO 비동기 처리를 위한 버퍼 사이즈 0 세팅
		DWORD sendBufferSize = 0;
		printf("setsockopt -> Async Send Active Begin\n");
		fprintf(MainThreadLogFile, "setsockopt -> Async Send Active Begin\n");
		setsockopt(this->listenSocket, SOL_SOCKET, SO_SNDBUF, (const char*)&sendBufferSize, sizeof(sendBufferSize));
		fprintf(MainThreadLogFile, "setsockopt -> Async Send Active End\n");
		printf("setsockopt -> Async Send Active End\n");
		fflush(MainThreadLogFile);
		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------


		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------
		// 리슨 콜해서 포트 열기
		printf("listen begin BACKLOG QUEUE Size : %d -> ReasonableValue 200\n", SOMAXCONN);
		fprintf(MainThreadLogFile, "listen begin BACKLOG QUEUE Size : %d -> ReasonableValue 200\n", SOMAXCONN);
		int listenRet = listen(this->listenSocket, SOMAXCONN);
		if (listenRet == SOCKET_ERROR)
		{
			int err = WSAGetLastError();
			fprintf(MainThreadLogFile, "listen retval is SOCKET_ERROR : %d\n", err);
			printf("listen retval is SOCKET_ERROR : %d\n", err);
			fclose(MainThreadLogFile);
			InterlockedExchange(&this->serverOnFlag, false);
			this->APIErrorCode = err;
			this->errorCode = dfNCINIT_SOCKET_LISTEN_FAILED;
			return;
		}
		fprintf(MainThreadLogFile, "listen end BACKLOG QUEUE Size : %d -> ReasonableValue 200\n", SOMAXCONN);
		printf("listen end BACKLOG QUEUE Size : %d -> ReasonableValue 200\n", SOMAXCONN);
		fflush(MainThreadLogFile);
		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------

		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------
		// Accept Thread 생성
		printf("AcceptThread Create Begin\n");
		fprintf(MainThreadLogFile, "AcceptThread Create Begin\n");
		this->acceptThread = (HANDLE)_beginthreadex(nullptr, 0, AcceptThread, this, 0, nullptr);
		if (this->acceptThread == nullptr)
		{
			printf("AcceptThread is nullptr\n");
			fprintf(MainThreadLogFile, "AcceptThread is nullptr\n");
			fclose(MainThreadLogFile);
			InterlockedExchange(&this->serverOnFlag, false);
			this->APIErrorCode = GetLastError();
			this->errorCode = dfNCINIT_ACCEPT_THREAD_CREATE_FAILED;
			return;
		}
		fprintf(MainThreadLogFile, "AcceptThread Create End\n");
		printf("AcceptThread Create End\n");
		fclose(MainThreadLogFile);
		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	NetCore::~NetCore()
	{
		NetCoreCleanup();
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
}