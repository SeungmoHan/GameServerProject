#include <time.h>
#include <process.h>
#include <conio.h>
#include "CNetServer.h"

#define SYSTEM LogClass::LogLevel::LOG_LEVEL_SYSTEM
#define LIBRARY LogClass::LogLevel::LOG_LEVEL_LIBRARY
namespace univ_dev
{
	DWORD CNetServer::_ServerOnFlag = false;

	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	//Worker Thread Wrapping function
	// param CNetServer*
	unsigned __stdcall _NET_WorkerThread(void* param)
	{
		//TlsGetValue(TlsCountingIdx);
		//WorkerThread -> NetCore::Run 이 호출될때까지 막혀있다가 NetCore::Run이 호출되어야 시작
		CNetServer* core = (CNetServer*)param;
		WaitForSingleObject(core->_ThreadStartEvent, INFINITE);
		return core->CNetServerWorkerThread(param);
	}
	//---------------------------------------------------------------------------------------------------------------------------------

	//---------------------------------------------------------------------------------------------------------------------------------
	//Accept Thread Wrapping function
	// param CNetServer*
	unsigned __stdcall _NET_AcceptThread(void* param)
	{
		//AcceptThread -> NetCore::Run 이 호출될때까지 막혀있다가 NetCore::Run이 호출되어야 시작
		CNetServer* server = (CNetServer*)param;
		WaitForSingleObject(server->_ThreadStartEvent, INFINITE);
		return server->CNetServerAcceptThread(param);
	}
	//---------------------------------------------------------------------------------------------------------------------------------


	//---------------------------------------------------------------------------------------------------------------------------------
	//TimeOut Thread Wrapping function
	// param CNetServer*
	unsigned __stdcall _NET_TimeOutThread(void* param)
	{
		if (param == nullptr)
			return -1;
		CNetServer* server = (CNetServer*)param;
		WaitForSingleObject(server->_ThreadStartEvent, INFINITE);
		return server->CNetServerTimeOutThread(server);
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------



	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	//RecvPost -> WSARecv call
	void CNetServer::RecvPost(Session* session)
	{

		int recvRet;
		WSABUF recvWSABuf[2];
		recvWSABuf[0].buf = session->_RingBuffer.GetWritePtr();
		recvWSABuf[0].len = session->_RingBuffer.DirectEnqueueSize();
		recvWSABuf[1].buf = session->_RingBuffer.GetBeginPtr();
		recvWSABuf[1].len = session->_RingBuffer.GetFreeSize() - session->_RingBuffer.DirectEnqueueSize();
		if (recvWSABuf[1].len > session->_RingBuffer.GetBufferSize())
			recvWSABuf[1].len = 0;
		DWORD flag = 0;
		ZeroMemory(&session->_RecvJob._Overlapped, sizeof(OVERLAPPED));

		InterlockedIncrement(&session->_IOCounts);
		SOCKET sock = InterlockedOr64((LONG64*)&session->_Sock, 0);
		recvRet = WSARecv(sock, recvWSABuf, 2, nullptr, &flag, &session->_RecvJob._Overlapped, nullptr);
		if (recvRet == SOCKET_ERROR)
		{
			int err = WSAGetLastError();
			if (err != WSA_IO_PENDING)
			{
				CancelIoEx((HANDLE)session->_Sock, nullptr);
				if (err != 10053 && err != 10054 && err != 10064 && err != 10038)
				{
					WCHAR errorStr[512] = { L"WSARecv ret is SOCKET_ERROR and error code is not WSA_IO_PENDING" };
					this->DispatchError(dfNCWORKER_WSARECV_SOCKET_ERROR_WAS_NOT_WSA_IO_PENDING, err, errorStr);
				}
				if (InterlockedDecrement(&session->_IOCounts) == 0)
					this->ReleaseSession(session);
			}
		}
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------


	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	//SendPost -> WSASend call
	void CNetServer::SendPost(Session* session)
	{
		if (InterlockedExchange(&session->_IOFlag, true) == true) return;

		int numOfPacket = session->_SendPacketQueue.size();
		int sendRet = 0;

		if (numOfPacket == 0)
		{
			InterlockedExchange(&session->_IOFlag, false);
			return;
		}
		ZeroMemory(&session->_SendJob._Overlapped, sizeof(OVERLAPPED));
		session->_SendJob._IsRecv = false;

		int cnt = min(SESSION_SEND_PACKER_BUFFER_SIZE, numOfPacket + session->_SendBufferCount);
		WSABUF sendWSABuf[SESSION_SEND_PACKER_BUFFER_SIZE]{ 0 };

		Packet* packet = nullptr;
		for (int i = session->_SendBufferCount; i < cnt; i++)
		{
			packet = nullptr;
			session->_SendPacketQueue.dequeue(packet);
			session->_SendPacketBuffer[i] = packet;
			sendWSABuf[i].buf = (char*)packet->GetReadPtr();
			sendWSABuf[i].len = packet->GetBufferSize();
		}

		InterlockedExchange(&session->_SendBufferCount, cnt);

		InterlockedIncrement(&session->_IOCounts);

		SOCKET sock = InterlockedOr64((LONG64*)&session->_Sock, 0);
		sendRet = WSASend(sock, sendWSABuf, cnt, nullptr, 0, &session->_SendJob._Overlapped, nullptr);
		if (sendRet == SOCKET_ERROR)
		{
			int err = WSAGetLastError();
			if (err != WSA_IO_PENDING)
			{
				CancelIoEx((HANDLE)session->_Sock, nullptr);
				if (err != 10053 && err != 10054 && err != 10064 && err != 10038)
				{
					WCHAR errStr[512];
					wsprintf(errStr, L"SendPost::WSAGetLastError return value is %u sessionID is %llu", err, session->_SessionID);
					DispatchError(dfNCWORKER_WSASEND_SOCKET_ERROR_WAS_NOT_WSA_IO_PENDING, err, errStr);
				}
				if (InterlockedDecrement(&session->_IOCounts) == 0)
					ReleaseSession(session);
			}
		}
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------


	void CNetServer::InitServer(USHORT port, DWORD backlogQueueSize, DWORD threadPoolSize, DWORD runningThread, DWORD nagleOff, ULONGLONG maxSessionCounts,DWORD timeOutClock)
	{
		this->_ServerPort = port;
		this->_BackLogQueueSize = backlogQueueSize;
		this->_ThreadPoolSize = threadPoolSize;
		this->_RunningThreadCount = runningThread;
		this->_NagleOff = nagleOff;
		this->_MaxSessionCounts = maxSessionCounts;
		this->_TimeOutClock = timeOutClock;
		this->StartUp();
	}

	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// SendPacket -> Enqueuing Send Ringbuffer
	void CNetServer::SendPacket(ULONGLONG sessionID, Packet* packet)
	{
		//Profiler p("SendPacket");
		//올바른 세션인지 확인
		packet->AddRef(); 
		Session* session = this->AcquireSession(sessionID);
		if (session == nullptr)
		{
			Packet::Free(packet);
			return;
		}
		//그게 아니라면 정상 송신
		packet->SetNetHeader();
		InterlockedIncrement(&this->_TotalSendPacketCount);
		session->_SendPacketQueue.enqueue(packet);
		this->SendPost(session);
		this->ReturnSession(session);
	}


	void CNetServer::Run(HANDLE* threadArr, size_t size)
	{
		SetEvent(this->_ThreadStartEvent);
		
		HANDLE* runningThreads = new HANDLE[(size_t)this->_ThreadPoolSize + ACCEPT_THREAD_COUNT + size];

		int i = 0;
		for (; i < (int)this->_ThreadPoolSize; i++)
			runningThreads[i] = this->_WorkerThreads[i];

		runningThreads[i++] = this->_AcceptThread;

		int j = i;

		for (int k = 0; k < size; k++, j++)
			runningThreads[j] = threadArr[k];

		WaitForMultipleObjects(this->_ThreadPoolSize + ACCEPT_THREAD_COUNT + size, runningThreads, true, INFINITE);

		delete[] runningThreads;
		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------

		auto _ = _wmkdir(L"ServerLog");
		auto __ = _wmkdir(L"ServerLog\\InitLog");
		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------
		//종료 작업을 위한 로그 파일 오픈
		FILE* MainThreadLogFile = nullptr;
		char logFileName[128]{ 0 };
		char tempStr[20]{ 0 };
		time_t currentTime = time(nullptr);
		tm time;
		localtime_s(&time, &currentTime);
		sprintf_s(logFileName, "ServerLog\\InitLog\\%d_%d_%d_ServerInitialize.txt", time.tm_year + 1900, time.tm_mon + 1, time.tm_mday);

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


	void CNetServer::PostOnClientLeave(ULONGLONG sessionID)
	{
		PostQueuedCompletionStatus(_IOCP, 0, sessionID, (LPOVERLAPPED)0xffffffff);
	}

	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// 에러났을때 에러코드 저장하면서 OnErrorOccured 호출
	void CNetServer::DispatchError(DWORD errorCode, DWORD APIErrorCode, const WCHAR* errorStr)
	{
		//라이브러리 자체 에러코드 등록 및 API에러 코드 등록
		this->_ErrorCode = errorCode;
		this->_APIErrorCode = APIErrorCode;
		// OnErrorOccured 함수는 라이브러리 에러코드를 전달하므로 GetLastAPIErrorCode 함수 호출해서 값을 얻어가야됨.
		this->OnErrorOccured(errorCode, errorStr);
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------



	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// CLanServer 초기화
	void CNetServer::StartUp()
	{
		WCHAR logStr[512];
		int _ = _wmkdir(L"ServerLog");
		_ = _wmkdir(L"ServerLog\\InitLog");
		
		timeBeginPeriod(1);

		this->_LibraryLog.LOG_SET_DIRECTORY(L"ServerLog\\InitLog");
		this->_LibraryLog.LOG_SET_LEVEL(LogClass::LogLevel::LOG_LEVEL_SYSTEM);

		if (this->_ServerOnFlag == true)
		{
			this->_ErrorCode = dfNCINIT_NET_CORE_ALREADY_EXIST;
			this->_ServerOnFlag = false;
			this->_LibraryLog.LOG(L"NetServer Already Running", LogClass::LogLevel::LOG_LEVEL_ERROR);
			return;
		}
		this->_ServerOnFlag = true;

		this->_LibraryLog.LOG(L"NetServer Flag On", LogClass::LogLevel::LOG_LEVEL_SYSTEM);

		this->_MonitoringSignal = CreateEvent(nullptr, false, false, nullptr);
		if (this->_MonitoringSignal == nullptr)
		{
			this->_ErrorCode = dfNCINIT_MONITORING_EVENT_CREATE_FAILED;
			this->_APIErrorCode = WSAGetLastError();
			this->_LibraryLog.LOG(L"Monitoring Signal Event Create Failed", LogClass::LogLevel::LOG_LEVEL_ERROR);
			this->_ServerOnFlag = false;
			return;
		}
		this->_LibraryLog.LOG(L"Monitoring Signal Event Created", LogClass::LogLevel::LOG_LEVEL_SYSTEM);

		this->_ThreadStartEvent = CreateEvent(nullptr, true, false, nullptr);
		if (this->_ThreadStartEvent == nullptr)
		{
			this->_ErrorCode = dfNCINIT_RUNNING_EVENT_CREATE_FAILED;
			this->_APIErrorCode = WSAGetLastError();
			this->_LibraryLog.LOG(L"Thread Start Event Create Failed", LogClass::LogLevel::LOG_LEVEL_ERROR);
			this->_ServerOnFlag = false;
			return;
		}
		this->_LibraryLog.LOG(L"Thread Start Event Created", LogClass::LogLevel::LOG_LEVEL_SYSTEM);
		
		
		WSADATA wsa;
		if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		{
			this->_ErrorCode = dfNCINIT_WSASTARTUP_FAILED;
			this->_APIErrorCode = WSAGetLastError();
			this->_LibraryLog.LOG(L"WSAStartup Failed", LogClass::LogLevel::LOG_LEVEL_ERROR);
			this->_ServerOnFlag = false;
			return;
		}
		this->_LibraryLog.LOG(L"WSAStartup Clear", LogClass::LogLevel::LOG_LEVEL_SYSTEM);


		if (this->_ThreadPoolSize <= 0)
		{
			SYSTEM_INFO info;
			GetSystemInfo(&info);
			this->_ThreadPoolSize = info.dwNumberOfProcessors * 2;
		}
		if (this->_RunningThreadCount > this->_ThreadPoolSize)
			this->_RunningThreadCount = this->_ThreadPoolSize;

		this->_IOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, _RunningThreadCount);
		if (this->_IOCP == NULL)
		{
			this->_ErrorCode = dfNCINIT_IOCOMPLETIONPORT_CREATE_FAILED;
			this->_APIErrorCode = GetLastError();
			this->_LibraryLog.LOG(L"IOCP Create Failed", LogClass::LogLevel::LOG_LEVEL_ERROR);
			this->_ServerOnFlag = false;
			return;
		}
		this->_LibraryLog.LOG(L"IOCP Created", LogClass::LogLevel::LOG_LEVEL_SYSTEM);


		if (this->_TimeOutClock != INFINITE)
		{
			this->_TimeOutThread = (HANDLE)_beginthreadex(nullptr, 0, _NET_TimeOutThread, this, 0, nullptr);
			if (this->_TimeOutThread == nullptr)
			{
				this->_ErrorCode = dfNCINIT_LOG_THREAD_CREATE_FAILED;
				this->_APIErrorCode = GetLastError();
				this->_LibraryLog.LOG(L"TimeOut Thread Create Failed", LogClass::LogLevel::LOG_LEVEL_ERROR);
				this->_ServerOnFlag = false;
				return;
			}
			this->_LibraryLog.LOG(L"TimeOut Thread Created", LogClass::LogLevel::LOG_LEVEL_SYSTEM);
		}



		this->_LibraryLog.LOG(L"WorkerThread Create Begin", LogClass::LogLevel::LOG_LEVEL_SYSTEM);
		this->_WorkerThreads = new HANDLE[this->_ThreadPoolSize];
		for (int i = 0; i < (int)this->_ThreadPoolSize; i++)
		{
			this->_WorkerThreads[i] = (HANDLE)_beginthreadex(NULL, 0, _NET_WorkerThread, this, 0, NULL);
			if (this->_WorkerThreads[i] == NULL)
			{
				this->_ErrorCode = dfNCINIT_WORKER_THREAD_CREATE_FAILED_0 + i;
				this->_APIErrorCode = GetLastError();
				this->_LibraryLog.LOG(L"WorkerThread Create Failed", LogClass::LogLevel::LOG_LEVEL_ERROR);
				this->_ServerOnFlag = false;
				return;
			}
			this->_LibraryLog.LOG(L"WorkerThread Created", LogClass::LogLevel::LOG_LEVEL_SYSTEM);
		}
		this->_LibraryLog.LOG(L"WorkerThread Create End", LogClass::LogLevel::LOG_LEVEL_SYSTEM);


		this->_ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (this->_ListenSocket == INVALID_SOCKET)
		{
			int err = WSAGetLastError();
			this->_ErrorCode = dfNCINIT_LISTEN_SOCKET_CREATE_FAILED;
			this->_APIErrorCode = err;
			this->_LibraryLog.LOG(L"Listen Socket Create Failed", LogClass::LogLevel::LOG_LEVEL_ERROR);
			this->_ServerOnFlag = false;
			return;
		}
		this->_LibraryLog.LOG(L"Listen Socket Created", LogClass::LogLevel::LOG_LEVEL_SYSTEM);

		
		SOCKADDR_IN serveraddr;
		ZeroMemory(&serveraddr, sizeof(serveraddr));
		serveraddr.sin_family = AF_INET;
		serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
		serveraddr.sin_port = htons(this->_ServerPort);
		int bindRet = bind(this->_ListenSocket, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
		if (bindRet == SOCKET_ERROR)
		{
			int err = WSAGetLastError();
			this->_APIErrorCode = err;
			this->_ErrorCode = dfNCINIT_SOCKET_BIND_FAILED;
			this->_ServerOnFlag = false;
			return;
		}
		
		
		LINGER l;
		l.l_onoff = 1;
		l.l_linger = 0;
		setsockopt(this->_ListenSocket, SOL_SOCKET, SO_LINGER, (const char*)&l, sizeof(l));
		this->_LibraryLog.LOG(L"Linger Option Off", LogClass::LogLevel::LOG_LEVEL_SYSTEM);


		BOOL keppAliveFlag = 0;
		setsockopt(this->_ListenSocket, SOL_SOCKET, SO_KEEPALIVE, (const char*)&keppAliveFlag, sizeof(BOOL));
		this->_LibraryLog.LOG(L"Keep Alive Option Off", LogClass::LogLevel::LOG_LEVEL_SYSTEM);
		
		
		DWORD sendBufferSize = 0;
		setsockopt(this->_ListenSocket, SOL_SOCKET, SO_SNDBUF, (const char*)&sendBufferSize, sizeof(sendBufferSize));
		this->_LibraryLog.LOG(L"Send Buffer Zero", LogClass::LogLevel::LOG_LEVEL_SYSTEM);
		
		
		setsockopt(this->_ListenSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&this->_NagleOff, sizeof(this->_NagleOff));
		this->_LibraryLog.LOG(L"Nagle Option Set", LogClass::LogLevel::LOG_LEVEL_SYSTEM);

		
		int listenRet = listen(this->_ListenSocket, this->_BackLogQueueSize);
		if (listenRet == SOCKET_ERROR)
		{
			int err = WSAGetLastError();
			this->_APIErrorCode = err;
			this->_ErrorCode = dfNCINIT_SOCKET_LISTEN_FAILED;
			this->_ServerOnFlag = false;
			this->_LibraryLog.LOG(L"Listen Failed", LogClass::LogLevel::LOG_LEVEL_ERROR);
			return;
		}
		
		
		this->_SessionArr = new Session[this->_MaxSessionCounts];
		for (int i = 0; i < this->_MaxSessionCounts; i++)
			this->_SessionIdx.push(i);
		this->_LibraryLog.LOG(L"Session Alloc, Session Index Stack Alloc", LogClass::LogLevel::LOG_LEVEL_SYSTEM);

		
 		this->_AcceptThread = (HANDLE)_beginthreadex(nullptr, 0, _NET_AcceptThread, this, 0, nullptr);
		if (this->_AcceptThread == nullptr)
		{
			this->_APIErrorCode = GetLastError();
			this->_ErrorCode = dfNCINIT_ACCEPT_THREAD_CREATE_FAILED;
			this->_ServerOnFlag = false;
			this->_LibraryLog.LOG(L"Accept Thread Create Failed", LogClass::LogLevel::LOG_LEVEL_ERROR);
			return;
		}
		this->_LibraryLog.LOG(L"Accept Thread Created", LogClass::LogLevel::LOG_LEVEL_SYSTEM);

		_ = _wmkdir(L"ServerLog\\Library_Log");
		this->_LibraryLog.LOG_SET_DIRECTORY(L"ServerLog\\Library_Log");
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------


	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// MonitoringThread에서 Post한 경우 FALSE반환 그외에는 TRUE반환 TRUE반환시 socket이 INVALID_SOCKET이라면 accept error
	BOOL CNetServer::TryAccept(SOCKET& clientSocket, sockaddr_in& clientAddr)
	{
		int addrlen = sizeof(clientAddr);
		SOCKET sock = accept(this->_ListenSocket, (SOCKADDR*)&clientAddr, &addrlen);
		WCHAR errStr[512];
		if (sock == INVALID_SOCKET)
		{
			int err = WSAGetLastError();
			if (err == WSAEINTR || err == WSAENOTSOCK || err == WSAEINVAL)
			{
				ULONG_PTR completionKey = (ULONG_PTR)0xffffffff;
				DWORD byteTransfered = 0;
				PostQueuedCompletionStatus(this->_IOCP, byteTransfered, completionKey, nullptr);
				this->_LibraryLog.LOG(L"Accept Thread End", LogClass::LogLevel::LOG_LEVEL_SYSTEM);
				printf("Accept Thread End\n");
				return false;
			}
			wsprintf(errStr, L"TryAccept::WSAGetLastError return value is %u", err);
			this->_LibraryLog.LOG(errStr, LogClass::LogLevel::LOG_LEVEL_ERROR);
		}
		clientSocket = sock;
		if (this->_SessionIdx.size() == 0)
		{
			closesocket(sock);
			this->_LibraryLog.LOG(L"RunOutOf Session IDX", LogClass::LogLevel::LOG_LEVEL_ERROR);
			clientSocket = INVALID_SOCKET;
		}
		return true;
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------


	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// if this function returns false there's no data in packet
	BOOL CNetServer::TryGetCompletedPacket(Session* session, Packet* packet, NetServerHeader& header)
	{
		if (session->_RingBuffer.GetUseSize() < NET_HEADER_SIZE) return false;

		//헤더하나 만든뒤 피크해서 5바이트가 제대로 피크됬는지 확인
		WCHAR errStr[512];
		int pkRet1 = session->_RingBuffer.Peek((char*)&header, NET_HEADER_SIZE);
		if (pkRet1 != NET_HEADER_SIZE)
		{
			wsprintf(errStr, L"TryGetCompletedPacket::RB first Peek return value is NET_HEADER_SIZE sessionID is %llu",session->_SessionID);
			this->DispatchError(dfNCWORKER_USE_SIZE_OVER_HEADER_SIZE_AND_FIRST_PEEK_ZERO, 0, errStr);
			this->DisconnectSession(session->_SessionID);
			return false;
		}
		if (header._ByteCode != 0x77)
		{
			this->DisconnectSession(session->_SessionID);
			return false;
		}
		//recv링버퍼에서 피크 했을때 payloadSize랑 합한것보다 링버퍼 사이즈가 작으면 다음번으로 넘긴다.
		if (session->_RingBuffer.GetUseSize() < NET_HEADER_SIZE + header._Len)
		{
			if (header._Len > 10000)
			{
				this->DispatchError(dfNCWORKER_HEADER_LEN_OVER_RINGBUFFER_SIZE, header._Len, L"TryGetCompletedPacket::Header_Len Over RingBufferSize");
				DisconnectSession(session->_SessionID);
			}
			return false;
		}

		//recv 링버퍼에서 데이터를 꺼냈으니 ReadPointer를 증가시켜서 UseSize를 줄여야함.
		session->_RingBuffer.MoveReadPtr(pkRet1);

		int pkRet2 = session->_RingBuffer.Peek((char*)packet->GetWritePtr(), header._Len);
		if (pkRet2 != header._Len)
		{
			wsprintf(errStr, L"TryGetCompletedPacket::RB second Peek return value is header._Len sessionID is %llu", session->_SessionID);
			this->DispatchError(dfNCWORKER_USE_SIZE_OVER_PAYLOAD_SIZE_AND_SECOND_PEEK_ZERO, 0, errStr);
			this->DisconnectSession(session->_SessionID);
			return false;
		}
		packet->MoveWritePtr(pkRet2);

		unsigned char* originWritePointer = packet->_WritePointer;
		packet->_WritePointer = packet->_Begin;
		(*packet) << header._ByteCode << header._Len << header._RandomKey << header._CheckSum;
		packet->_WritePointer = originWritePointer;

		packet->Decode();
		if (!packet->VerifyCheckSum())
		{
			this->DispatchError(dfNCWORKER_CHECKSUM_WRONG, header._CheckSum, L"TryGetCompletedPacket::Header_Len Over RingBufferSize");
			this->DisconnectSession(session->_SessionID);
			return false;
		}
		session->_RingBuffer.MoveReadPtr(pkRet2);
		packet->MoveReadPtr(NET_HEADER_SIZE);
		return true;
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------



	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// SendProc
	void CNetServer::SendProc(Session* session, DWORD byteTransfered)
	{
		InterlockedAdd64((LONG64*)&this->_TotalProcessedBytes, byteTransfered + 20);
		DWORD sendCount = InterlockedExchange(&session->_SendBufferCount, 0);
		Packet* packet = nullptr;
		for (int i = 0; i < (int)sendCount; i++)
		{
			packet = session->_SendPacketBuffer[i];
			Packet::Free(packet);
		}
		InterlockedExchange(&session->_IOFlag, false);
		if (session->_SendPacketQueue.size() > 0)
		{
			this->SendPost(session);
		}
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------



	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// RecvProc
	void CNetServer::RecvProc(Session* session, DWORD byteTransfered)
	{
		//완료된 사이즈만큼 writePointer를 증가시킨다.
		session->_RingBuffer.MoveWritePtr(byteTransfered);
		session->_LastRecvdTime = timeGetTime();
		//링버퍼에 있는거 하나하나 전부다 직렬화 버퍼로 가져올거임.
		for(;;)
		{
			Packet* recvPacket = Packet::Alloc();
			recvPacket->AddRef();
			NetServerHeader header;
			if (this->TryGetCompletedPacket(session, recvPacket, header))
			{
				InterlockedIncrement(&this->_TotalRecvPacketCount);
				this->OnRecv(session->_SessionID, recvPacket);
			}
			else
			{
				Packet::Free(recvPacket);
				break;
			}
		}
		this->RecvPost(session);
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------



	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// helper function 아이피를 가지고 wide string함
	void CNetServer::GetStringIP(WCHAR* str, DWORD bufferLen, sockaddr_in& addr)
	{
		wsprintf(str, L"%d.%d.%d.%d", addr.sin_addr.S_un.S_un_b.s_b1, addr.sin_addr.S_un.S_un_b.s_b2, addr.sin_addr.S_un.S_un_b.s_b3, addr.sin_addr.S_un.S_un_b.s_b4);
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------



	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// Accept Thread
	// param : this
	unsigned int CNetServer::CNetServerAcceptThread(void* param)
	{
		ULONGLONG sessionID = 1;
		SOCKET client_sock;
		sockaddr_in clientaddr;
		for(;;)
		{
			client_sock = INVALID_SOCKET;

			if (!this->TryAccept(client_sock, clientaddr)) return 0;

			if (client_sock == INVALID_SOCKET) continue;

			WCHAR ipStr[20]{ 0 };
			this->GetStringIP(ipStr, 20, clientaddr);
			if (!OnConnectionRequest(ipStr, ntohl(clientaddr.sin_addr.S_un.S_addr), ntohs(clientaddr.sin_port)))
			{
				closesocket(client_sock);
				continue;
			}
			Session* newSession = this->CreateSession(client_sock, clientaddr, ++sessionID);
			if (newSession == nullptr)
			{
				closesocket(client_sock);
				continue;
			}
			this->OnClientJoin(newSession->_SessionIPStr, newSession->_SessionIP, newSession->_SessionPort, newSession->_SessionID);
			this->RecvPost(newSession);

			if (InterlockedDecrement(&newSession->_IOCounts) == 0)
				ReleaseSession(newSession);

			InterlockedIncrement(&this->_TotalAcceptSessionCount);
		}
		return -1;
	}

	unsigned int CNetServer::CNetServerTimeOutThread(void* param)
	{
		DWORD timeOutTimer;

		while (!this->_ShutDownFlag)
		{
			timeOutTimer = timeGetTime();
			Sleep(this->_TimeOutClock);
			for (int i = 0; i < _MaxSessionCounts; i++)
			{
				__faststorefence();
				if ((this->_SessionArr[i]._IOCounts & 0x80000000) != 0) continue;
				if (this->_SessionArr[i]._LastRecvdTime >= timeOutTimer) continue;
				this->OnTimeOut(_SessionArr[i]._SessionID);
			}
		}
		return 0;
	}


	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------


	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// Worker Thread
	// param : this
	unsigned int CNetServer::CNetServerWorkerThread(void* param)
	{
		DWORD thisThreadID = GetCurrentThreadId();
		DWORD byteTransfered = 0;
		ULONG_PTR completionKey = 0;
		OVERLAPPED* over = nullptr;
		OverlappedEx* job = nullptr;
		ULONGLONG sessionID = 0;
		Session* session = nullptr;
		int GQCSRet = 0;

		WCHAR errStr[512];
		for(;;)
		{
			byteTransfered = 0;
			completionKey = 0;
			over = nullptr;
			job = nullptr;
			session = nullptr;
			GQCSRet = 0;
			sessionID = 0;
			GQCSRet = GetQueuedCompletionStatus(this->_IOCP, &byteTransfered, &completionKey, &over, INFINITE);
			if (over == nullptr)
			{
				if (completionKey == 0xffffffff)
				{
					printf("ThreadID : %d\nCompletionKey : %llu     Thread End\n", thisThreadID, completionKey);
					ULONG_PTR completionKey = (ULONG_PTR)0xffffffff;
					DWORD byteTransfered = 0;
					PostQueuedCompletionStatus(this->_IOCP, byteTransfered, completionKey, nullptr);
					return 0;
				}
				else if (completionKey == 0)
				{
					int err = WSAGetLastError();
					wsprintf(errStr, L"GetQueuedCompletionStatus Overlapped is nullptr");
					this->DispatchError(dfNCWORKER_OVERLAPPED_IS_NULL, err, errStr);
					continue;
				}
			}
			else if (over == (OVERLAPPED*)0xffffffff)
			{
				this->OnClientLeave(completionKey);
				continue;
			}
			sessionID = (ULONGLONG)completionKey;
			job = (OverlappedEx*)over;

			session = FindSession(sessionID);
			//recv 완료처리라면
			if (GQCSRet != 0 && byteTransfered != 0)
			{
				if (job->_IsRecv)
					this->RecvProc(session, byteTransfered);
				//send 완료 처리라면
				else
					this->SendProc(session, byteTransfered);
			}

			if (InterlockedDecrement(&session->_IOCounts) == 0)
			{
				this->ReleaseSession(session);
			}
		}
		return -1;
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------


	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// Net Core Cleanup
	void CNetServer::CleanUp()
	{
		//정리 작업
		CloseHandle(this->_IOCP);
		CloseHandle(this->_ThreadStartEvent);
		DeleteCriticalSection(&this->_SessionMapLock);
		delete[] this->_WorkerThreads;
		InterlockedExchange(&this->_ServerOnFlag, false);
		WSACleanup();
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------


	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// 세션 찾기
	Session* CNetServer::FindAndLockSession(ULONGLONG sessionID)
	{
		return &_SessionArr[sessionID & 0xffff];
	}
	//---------------------------------------------------------------------------------------------------------------------------------

	//---------------------------------------------------------------------------------------------------------------------------------
	Session* CNetServer::FindSession(ULONGLONG sessionID)
	{
		return &_SessionArr[sessionID & 0xffff];
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------



	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// 동기화 함수
	void CNetServer::SessionMapLock()
	{
		EnterCriticalSection(&_SessionMapLock);
	}
	//---------------------------------------------------------------------------------------------------------------------------------

	//---------------------------------------------------------------------------------------------------------------------------------
	void CNetServer::SessionMapUnlock()
	{
		LeaveCriticalSection(&_SessionMapLock);
	}
	//---------------------------------------------------------------------------------------------------------------------------------




	Session* CNetServer::AcquireSession(ULONGLONG sessionID)
	{
		if ((sessionID & 0xffff) == 0) return nullptr;
		Session* session = FindSession(sessionID);
		if ((InterlockedIncrement(&session->_IOCounts) & 0x80000000) != 0)
		{
			if (InterlockedDecrement(&session->_IOCounts) == 0)
				ReleaseSession(session);
			return nullptr;
		}
		if (session->_SessionID != sessionID)
		{
			if (InterlockedDecrement(&session->_IOCounts) == 0)
				ReleaseSession(session);
			return nullptr;
		}
		if ((session->_Sock & 0x80000000) != 0)
		{
			if (InterlockedDecrement(&session->_IOCounts) == 0)
				ReleaseSession(session);
			return nullptr;
		}
		return session;
	}

	void CNetServer::ReturnSession(Session* session)
	{
		if (InterlockedDecrement(&session->_IOCounts) == 0)
			ReleaseSession(session);
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------


	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// 세션 생성 함수
	Session* CNetServer::CreateSession(SOCKET sock, sockaddr_in clientaddr, ULONGLONG sessionID)
	{
		DWORD idx;
		if (!this->PopSessionIndex(idx)) return nullptr;
		Session* newSession = &this->_SessionArr[idx];

		newSession->_SessionIP = ntohl(clientaddr.sin_addr.S_un.S_addr);
		newSession->_SessionPort = ntohs(clientaddr.sin_port);
		ZeroMemory(newSession->_SessionIPStr, sizeof(newSession->_SessionIPStr));
		this->GetStringIP(newSession->_SessionIPStr, 20, clientaddr);

		WCHAR err[512];
		if (newSession->_SendBufferCount > 0)
		{
			wsprintf(err, L"Session : %llu SendBufferCount is not Cleaned up size : %d", (sessionID << 16) | idx, newSession->_SendBufferCount);
			DispatchError(dfNCACCEPT_SESSION_ID_NOT_CLEANUP, 0, err);
			for (int i = 0; i < newSession->_SendBufferCount; i++)
				Packet::Free(newSession->_SendPacketBuffer[i]);
		}
		Packet* packet;
		if (newSession->_SendPacketQueue.size() > 0)
		{
			wsprintf(err, L"Session : %llu SendBufferQueue is not Cleaned up size : %d", (sessionID << 16) | idx, newSession->_SendPacketQueue.size());
			DispatchError(dfNCACCEPT_SESSION_ID_NOT_CLEANUP, 0, err);
			while (newSession->_SendPacketQueue.dequeue(packet))
				Packet::Free(packet);
		}


		newSession->_RecvJob._IsRecv = true;
		newSession->_SendJob._IsRecv = false;
		newSession->_LastRecvdTime = timeGetTime();
		newSession->_SessionID = (sessionID << 16) | idx;
		newSession->_RingBuffer.ClearBuffer();
		InterlockedExchange(&newSession->_IOFlag, false);
		InterlockedIncrement(&newSession->_IOCounts);
		InterlockedAnd((long*)&newSession->_IOCounts, 0x7fffffff);
		InterlockedExchange(&newSession->_Sock, sock);
		InterlockedIncrement(&this->_CurSessionCount);
		CreateIoCompletionPort((HANDLE)sock, this->_IOCP, (ULONG_PTR)newSession->_SessionID, 0);
		return newSession;
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------


	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	bool CNetServer::PopSessionIndex(DWORD& ret)
	{
		return this->_SessionIdx.pop(ret);
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	void CNetServer::PushSessionIndex(DWORD idx)
	{
		return this->_SessionIdx.push(idx);
	}
	void CNetServer::PostServerStop()
	{
		this->_ShutDownFlag = true;
		closesocket(this->_ListenSocket);
	}

	void CNetServer::GetMonitoringInfo(MonitoringInfo& ret)
	{
		ret._WorkerThreadCount = _ThreadPoolSize;
		ret._RunningThreadCount = _RunningThreadCount;
		ret._TotalAcceptSessionCount = this->_TotalAcceptSessionCount;
		ret._TotalReleaseSessionCount = this->_TotalReleaseSessionCount;
		ret._TotalRecvPacketCount = this->_TotalRecvPacketCount;
		ret._TotalSendPacketCount = this->_TotalSendPacketCount;
		ret._CurrentSessionCount = this->_CurSessionCount;
		ret._TotalProcessedBytes = this->_TotalProcessedBytes;
		ret._CurrentSessionCount = this->_CurSessionCount;
		ret._SessionIndexStackCapacity = this->_SessionIdx.GetCapacityCount();
		ret._SessionIndexStackSize = this->_SessionIdx.GetUseCount();
		ret._SessionSendQueueSize = 0;
		ret._SessionSendQueueCapacity = 0;
		ret._SessionSendQueueMax = 0;
		for (int i = 0; i < _MaxSessionCounts; i++)
			ret._SessionSendQueueSize += this->_SessionArr[i]._SendPacketQueue.size();
		for (int i = 0; i < _MaxSessionCounts; i++)
		{
			if (this->_SessionArr[i]._SendPacketQueue.GetCapacityCount() > ret._SessionSendQueueMax)
				ret._SessionSendQueueMax = this->_SessionArr[i]._SendPacketQueue.GetCapacityCount();
			ret._SessionSendQueueCapacity += this->_SessionArr[i]._SendPacketQueue.GetCapacityCount();
		}
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------

	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// 세션 제거함수
	void CNetServer::ReleaseSession(Session* session)
	{
		ULONGLONG sessionID = session->_SessionID;
		if (InterlockedCompareExchange(&session->_IOCounts, 0x80000000, 0) != 0)
			return;
		if (session->_SessionID != sessionID)
			return;

		DWORD idx = sessionID & 0xffff;
		closesocket(session->_Sock & 0x7fffffff);

		DWORD sendCount = InterlockedExchange(&session->_SendBufferCount, 0);
		Packet* packet = nullptr;
		for (int i = 0; i < (int)sendCount; i++)
		{
			packet = session->_SendPacketBuffer[i];
			Packet::Free(packet);
		}
		while (session->_SendPacketQueue.dequeue(packet))
			Packet::Free(packet);
		session->_IOFlag = false;
		this->PushSessionIndex(idx);
		this->PostOnClientLeave(sessionID);
		InterlockedDecrement(&_CurSessionCount);
		InterlockedIncrement(&_TotalReleaseSessionCount);
		return;
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------


	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// 연결 종료 함수
	void CNetServer::DisconnectSession(ULONGLONG sessionID)
	{
		Session* disconnectSession = this->AcquireSession(sessionID);
		if (disconnectSession == nullptr) return;
		SOCKET sock = InterlockedOr64((LONG64*)&disconnectSession->_Sock, 0x80000000);
		if ((sock & 0x80000000) != 0)
		{
			ReturnSession(disconnectSession);
			return;
		}
		
		CancelIoEx((HANDLE)(sock & 0x7fffffff), nullptr);
		ReturnSession(disconnectSession);
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------




	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// 생성자 소멸자
	CNetServer::CNetServer()
		: _ServerPort{ 0 }, _ShutDownFlag{ false }, _APIErrorCode{ 0 }, _ErrorCode{ 0 }, _IOCP{ nullptr }, _AcceptThread{ nullptr }, _ListenSocket{ 0 }, _WorkerThreads{ nullptr }, _ThreadPoolSize{ 0 }, _RunningThreadCount{ 0 }, _ThreadStartEvent{ nullptr }, _TotalProcessedBytes{ 0 }, _SessionMapLock{ 0 }, _NagleOff{ 0 }, _MaxSessionCounts{ 0 }, _SessionArr{ nullptr }, _BackLogQueueSize{ 0 }
	{
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	CNetServer::~CNetServer()
	{
		this->CleanUp();
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
}