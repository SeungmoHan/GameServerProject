#include <time.h>
#include <process.h>
#include <conio.h>
#include "CLanServer.h"


namespace univ_dev
{
	DWORD CLanServer::_ServerOnFlag = false;

	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	//Worker Thread Wrapping function
	// param CLanServer*
	unsigned __stdcall WorkerThread(void* param)
	{
		//WorkerThread -> NetCore::Run 이 호출될때까지 막혀있다가 NetCore::Run이 호출되어야 시작
		CLanServer* core = (CLanServer*)param;
		WaitForSingleObject(core->_RunningEvent, INFINITE);
		return core->CLanServerWorkerThread(param);
	}
	//---------------------------------------------------------------------------------------------------------------------------------

	//---------------------------------------------------------------------------------------------------------------------------------
	//Accept Thread Wrapping function
	// param CLanServer*
	unsigned __stdcall AcceptThread(void* param)
	{
		//AcceptThread -> NetCore::Run 이 호출될때까지 막혀있다가 NetCore::Run이 호출되어야 시작
		CLanServer* core = (CLanServer*)param;
		WaitForSingleObject(core->_RunningEvent, INFINITE);
		return core->CLanServerAcceptThread(param);
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------

	unsigned __stdcall TimeOutThread(void* param)
	{
		CLanServer* core = (CLanServer*)param;
		WaitForSingleObject(core->_RunningEvent, INFINITE);
		return core->CLanServerTimeOutThread(param);
	}

	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	//RecvPost -> WSARecv call
	void CLanServer::RecvPost(Session* session)
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
		recvRet = WSARecv(session->_Sock, recvWSABuf, 2, nullptr, &flag, &session->_RecvJob._Overlapped, nullptr);
		if (recvRet == SOCKET_ERROR)
		{
			int err = WSAGetLastError();
			if (err != WSA_IO_PENDING)
			{

				if (err != 10054 && err != 10063)
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
	void CLanServer::SendPost(Session* session)
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
		sendRet = WSASend(session->_Sock, sendWSABuf, cnt, nullptr, 0, &session->_SendJob._Overlapped, nullptr);
		if (sendRet == SOCKET_ERROR)
		{
			int err = WSAGetLastError();
			if (err != WSA_IO_PENDING)
			{
				if (err != 10054 && err != 10063)
					DispatchError(dfNCWORKER_WSASEND_SOCKET_ERROR_WAS_NOT_WSA_IO_PENDING, err, L"WSASend ret is SOCKET_ERROR and error code is not WSA_IO_PENDING");
				if (InterlockedDecrement(&session->_IOCounts) == 0)
					ReleaseSession(session);
			}
		}
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------

	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// SendPacket -> Enqueuing Send Ringbuffer
	void CLanServer::SendPacket(ULONGLONG sessionID, Packet* packet)
	{
		//올바른 세션인지 확인
		packet->AddRef();
		Session* session = this->AcquireSession(sessionID);
		if (session == nullptr)
		{
			Packet::Free(packet);
			return;
		}
		//그게 아니라면 정상 송신
		packet->SetLanHeader();
		InterlockedIncrement(&this->_SendPacketPerSec);
		session->_SendPacketQueue.enqueue(packet);
		this->SendPost(session);
		this->ReturnSession(session);
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------


	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// Run 호출전에는 절대로 다른 스레드들이 작동하지 않음 주의바람
	void CLanServer::Run()
	{
		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------
		// 작동중인 스레드들은 이 이벤트를 바라보고있고 메뉴얼 이벤트.
		// 이벤트를 셋하면 메뉴얼 이벤트이기 때문에 모든 스레드를 다깨운다.
		SetEvent(this->_RunningEvent);
		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------


		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------
		// 모든 스레드들이 작업이 끝날때까지 대기하는 메인스레드
		HANDLE* runningThreads = new HANDLE[(size_t)this->_ThreadPoolSize + ACCEPT_THREAD_COUNT];

		for (int i = 0; i < (int)this->_ThreadPoolSize; i++)
			runningThreads[i] = this->_WorkerThreads[i];

		runningThreads[this->_ThreadPoolSize] = this->_AcceptThread;

		WaitForMultipleObjects(this->_ThreadPoolSize + ACCEPT_THREAD_COUNT, runningThreads, true, INFINITE);

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



	void CLanServer::PostLanServerStop()
	{
		this->_ShutDownFlag = true;
		closesocket(this->_ListenSocket);
	}

	void CLanServer::PostOnClientLeave(ULONGLONG sessionID)
	{
		PostQueuedCompletionStatus(_IOCP, 0, sessionID, (LPOVERLAPPED)0xffffffff);
	}

	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// 에러났을때 에러코드 저장하면서 OnErrorOccured 호출
	void CLanServer::DispatchError(DWORD errorCode,DWORD APIErrorCode,const WCHAR* errorStr)
	{
		//라이브러리 자체 에러코드 등록 및 API에러 코드 등록
		this->_ErrorCode = errorCode;
		this->_APIErrorCode = APIErrorCode;
		// OnErrorOccured 함수는 라이브러리 에러코드를 전달하므로 GetLastAPIErrorCode 함수 호출해서 값을 얻어가야됨.
		OnErrorOccured(errorCode, errorStr);
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------


	
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// CLanServer 초기화
	void CLanServer::CLanServerStartup()
	{
		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------
		// 인터락으로 서버 플래그를 true로 만들었을때 내가 true로 만들어야지만
		DWORD ret = InterlockedExchange(&this->_ServerOnFlag, true);
		if (ret == true)
		{
			this->_ErrorCode = dfNCINIT_NET_CORE_ALREADY_EXIST;
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
		this->_RunningEvent = CreateEvent(nullptr, true, false, nullptr);
		if (this->_RunningEvent == nullptr)
		{
			InterlockedExchange(&this->_ServerOnFlag, false);
			this->_ErrorCode = dfNCINIT_RUNNING_EVENT_CREATE_FAILED;
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
			InterlockedExchange(&this->_ServerOnFlag, false);
			this->_ErrorCode = dfNCINIT_WSASTARTUP_FAILED;
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
		if (this->_ThreadPoolSize <= 0)
		{
			SYSTEM_INFO info;
			GetSystemInfo(&info);
			this->_ThreadPoolSize = info.dwNumberOfProcessors * 2;
		}
		printf("IOCP CreateBegin\n");
		fprintf(MainThreadLogFile, "IOCP CreateBegin\n");
		if (this->_RunningThreadCount > this->_ThreadPoolSize)
			this->_RunningThreadCount = this->_ThreadPoolSize;
		this->_IOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, _RunningThreadCount);
		if (this->_IOCP == NULL)
		{
			InterlockedExchange(&this->_ServerOnFlag, false);
			this->_ErrorCode = dfNCINIT_IOCOMPLETIONPORT_CREATE_FAILED;
			this->_APIErrorCode = GetLastError();
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
		printf("timeBeginPeriod, CSLocker Init, ProfilerSample Init Begin\n");
		fprintf(MainThreadLogFile, "timeBeginPeriod, CSLocker Init, ProfilerSample Init Begin\n");
		timeBeginPeriod(1);
		InitializeCriticalSection(&_SessionMapLock);
		//InitializeSRWLock(&sessionMapLock);
		univ_dev::InitProfile();
		fprintf(MainThreadLogFile, "timeBeginPeriod, CSLocker Init, ProfilerSample Init End\n");
		printf("timeBeginPeriod, CSLocker Init, ProfilerSample Init End\n");
		fflush(MainThreadLogFile);
		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------


		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------
		// 초단위 로깅용 스레드 생성
		//printf("PrintThread CreateBegin\n");
		//fprintf(MainThreadLogFile, "PrintThread CreateBegin\n");
		//this->_LogThread = (HANDLE)_beginthreadex(nullptr, 0, MoniteringThread, this, 0, nullptr);
		//if (this->_LogThread == nullptr)
		//{
		//	InterlockedExchange(&this->_ServerOnFlag, false);
		//	this->_ErrorCode = dfNCINIT_LOG_THREAD_CREATE_FAILED;
		//	this->_APIErrorCode = GetLastError();
		//	return;
		//}
		//fprintf(MainThreadLogFile, "PrintThread CreateEnd\n");
		//printf("PrintThread CreateEnd\n");
		//fflush(MainThreadLogFile);
		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------


		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------
		// 스레드풀 생성, 
		// 스레드풀 사이즈는 0이면 systeminfo에서 받아온 논리코어 * 2개만큼 위에서적용
		printf("ThreadPool CreateBegin\n");
		fprintf(MainThreadLogFile, "ThreadPool CreateBegin\n");
		this->_WorkerThreads = new HANDLE[this->_ThreadPoolSize];
		for (int i = 0; i < (int)this->_ThreadPoolSize; i++)
		{
			this->_WorkerThreads[i] = (HANDLE)_beginthreadex(NULL, 0, WorkerThread, this, 0, NULL);
			if (this->_WorkerThreads[i] == NULL)
			{
				fclose(MainThreadLogFile);
				InterlockedExchange(&this->_ServerOnFlag, false);
				this->_ErrorCode = dfNCINIT_WORKER_THREAD_CREATE_FAILED_0 + i;
				this->_APIErrorCode = GetLastError();
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
		this->_ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (this->_ListenSocket == INVALID_SOCKET)
		{
			int err = WSAGetLastError();
			printf("listen socket is INVALID_SOCKET : %d\n", err);
			fprintf(MainThreadLogFile, "listen socket is INVALID_SOCKET : %d\n", err);
			fclose(MainThreadLogFile);
			InterlockedExchange(&this->_ServerOnFlag, false);
			this->_ErrorCode = dfNCINIT_LISTEN_SOCKET_CREATE_FAILED;
			this->_APIErrorCode = err;
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
		serveraddr.sin_port = htons(this->_ServerPort);
		printf("Port : %d BindBegin\n", ntohs(serveraddr.sin_port));
		fprintf(MainThreadLogFile, "Port : %d BindBegin\n", ntohs(serveraddr.sin_port));
		int bindRet = bind(this->_ListenSocket, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
		if (bindRet == SOCKET_ERROR)
		{
			int err = WSAGetLastError();
			printf("bind retval is SOCKET_ERROR : %d\n", err);
			fprintf(MainThreadLogFile, "bind retval is SOCKET_ERROR : %d\n", err);
			fclose(MainThreadLogFile);
			InterlockedExchange(&this->_ServerOnFlag, false);
			this->_APIErrorCode = err;
			this->_ErrorCode = dfNCINIT_SOCKET_BIND_FAILED;
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
		setsockopt(this->_ListenSocket, SOL_SOCKET, SO_LINGER, (const char*)&l, sizeof(l));
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
		setsockopt(this->_ListenSocket, SOL_SOCKET, SO_SNDBUF, (const char*)&sendBufferSize, sizeof(sendBufferSize));
		fprintf(MainThreadLogFile, "setsockopt -> Async Send Active End\n");
		printf("setsockopt -> Async Send Active End\n");
		fflush(MainThreadLogFile);
		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------

		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------
		// Send IO 비동기 처리를 위한 버퍼 사이즈 0 세팅
		if (this->_NagleOff)
		{
			printf("setsockopt -> Nagle Algorithm Off Begin\n");
			fprintf(MainThreadLogFile, "setsockopt -> Nagle Algorithm Off Begin\n");
			setsockopt(this->_ListenSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&this->_NagleOff, sizeof(this->_NagleOff));
			fprintf(MainThreadLogFile, "setsockopt -> Nagle Algorithm Off End\n");
			printf("setsockopt -> Nagle Algorithm Off End\n");
			fflush(MainThreadLogFile);
		}
		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------



		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------
		// 리슨 콜해서 포트 열기
		printf("listen begin BACKLOG QUEUE Size : %d -> ReasonableValue 200\n", SOMAXCONN);
		fprintf(MainThreadLogFile, "listen begin BACKLOG QUEUE Size : %d -> ReasonableValue 200\n", SOMAXCONN);
		int listenRet = listen(this->_ListenSocket, SOMAXCONN);
		if (listenRet == SOCKET_ERROR)
		{
			int err = WSAGetLastError();
			fprintf(MainThreadLogFile, "listen retval is SOCKET_ERROR : %d\n", err);
			printf("listen retval is SOCKET_ERROR : %d\n", err);
			fclose(MainThreadLogFile);
			InterlockedExchange(&this->_ServerOnFlag, false);
			this->_APIErrorCode = err;
			this->_ErrorCode = dfNCINIT_SOCKET_LISTEN_FAILED;
			return;
		}
		fprintf(MainThreadLogFile, "listen end BACKLOG QUEUE Size : %d -> ReasonableValue 200\n", SOMAXCONN);
		printf("listen end BACKLOG QUEUE Size : %d -> ReasonableValue 200\n", SOMAXCONN);
		fflush(MainThreadLogFile);
		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------


		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------
		// 메모리 풀 대신 사용할 세션 배열 + 인덱스 스택 초기화
		this->_SessionArr = new Session[this->_MaxSessionCounts];
		for (int i = 0; i < this->_MaxSessionCounts; i++)
			this->_SessionIdx.push(i);
		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------


		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------
		// Accept Thread 생성
		printf("AcceptThread Create Begin\n");
		fprintf(MainThreadLogFile, "AcceptThread Create Begin\n");
		this->_AcceptThread = (HANDLE)_beginthreadex(nullptr, 0, AcceptThread, this, 0, nullptr);
		if (this->_AcceptThread == nullptr)
		{
			printf("AcceptThread is nullptr\n");
			fprintf(MainThreadLogFile, "AcceptThread is nullptr\n");
			fclose(MainThreadLogFile);
			InterlockedExchange(&this->_ServerOnFlag, false);
			this->_APIErrorCode = GetLastError();
			this->_ErrorCode = dfNCINIT_ACCEPT_THREAD_CREATE_FAILED;
			return;
		}
		fprintf(MainThreadLogFile, "AcceptThread Create End\n");
		printf("AcceptThread Create End\n");
		fclose(MainThreadLogFile);
		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	

	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// MoniteringThread에서 Post한 경우 FALSE반환 그외에는 TRUE반환 TRUE반환시 socket이 INVALID_SOCKET이라면 accept error
	BOOL CLanServer::TryAccept(SOCKET& clientSocket, sockaddr_in& clientAddr)
	{
		int addrlen = sizeof(clientAddr);
		SOCKET sock = accept(this->_ListenSocket, (SOCKADDR*)&clientAddr, &addrlen);
		if (sock == INVALID_SOCKET)
		{
			int err = WSAGetLastError();
			if (err == WSAEINTR || err == WSAENOTSOCK || err == WSAEINVAL)
			{
				ULONG_PTR completionKey = (ULONG_PTR)0xffffffff;
				DWORD byteTransfered = 0;
				PostQueuedCompletionStatus(this->_IOCP, byteTransfered, completionKey, nullptr);
				printf("Accept Thread End\n");
				return false;
			}
			this->DispatchError(dfNCACCEPT_CLIENT_SOCKET_IS_INVALID_SOCKET, err, L"accept returned and client socket was INVAILD_SOCKET");
		}
		clientSocket = sock;
		if (_SessionIdx.size() == 0)
		{
			closesocket(sock);
			DispatchError(dfNCACCEPT_SESSION_COUNTS_OVER, 0, L"session container size is over than max session counts");
		}

		return true;
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------


	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// if this function returns false there's no data in packet
	BOOL CLanServer::TryGetCompletedPacket(Session* session, Packet* packet, LanServerHeader& header)
	{
		//링버퍼의 현재 사이즈가 헤더의 길이보다 작으면 더이상 뽑을게 없다는 의미임.
		if (session->_RingBuffer.GetUseSize() < LAN_HEADER_SIZE) return false;
		
		//헤더하나 만든뒤 피크해서 2바이트가 제대로 피크됬는지 확인
		//EchoPacketHeader header;
		int pkRet1 = session->_RingBuffer.Peek((char*)&header, LAN_HEADER_SIZE);
		if (pkRet1 != LAN_HEADER_SIZE)
		{
			DispatchError(dfNCWORKER_USE_SIZE_OVER_HEADER_SIZE_AND_FIRST_PEEK_ZERO, 0, L"ringbuffer use size was over than header size but peek ret is zero");
			return false;
		}
		//recv링버퍼에서 피크 했을때 payloadSize랑 합한것보다 링버퍼 사이즈가 작으면 다음번으로 넘긴다.
		if (session->_RingBuffer.GetUseSize() < LAN_HEADER_SIZE + header._Len) return false;
			
		//recv 링버퍼에서 데이터를 꺼냈으니 ReadPointer를 증가시켜서 UseSize를 줄여야함.
		session->_RingBuffer.MoveReadPtr(pkRet1);
		
		int pkRet2 = session->_RingBuffer.Peek((char*)packet->GetWritePtr(), header._Len);
		if (pkRet2 != header._Len)
		{
			DispatchError(dfNCWORKER_USE_SIZE_OVER_PAYLOAD_SIZE_AND_SECOND_PEEK_ZERO, 0, L"ringbuffer use size was over than header + payloadsize but peek ret is zero");
			return false;
		}

		session->_RingBuffer.MoveReadPtr(pkRet2);
		packet->MoveWritePtr(pkRet2);
		return true;
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------



	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// SendProc
	void CLanServer::SendProc(Session* session, DWORD byteTransfered)
	{
		InterlockedAdd64(&_TotalProcessedBytes, byteTransfered);
		DWORD sendCount = InterlockedExchange(&session->_SendBufferCount, 0);
		Packet* packet = nullptr;
		for (int i = 0; i < (int)sendCount; i++)
		{
			packet = session->_SendPacketBuffer[i];
			if (packet != nullptr)
			{
				Packet::Free(packet);
			}
			else
				CRASH();
		}
		InterlockedExchange(&session->_IOFlag, false);
		if (session->_SendPacketQueue.size() > 0)
			this->SendPost(session);
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------



	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// RecvProc
	void CLanServer::RecvProc(Session* session, DWORD byteTransfered)
	{
		//완료된 사이즈만큼 writePointer를 증가시킨다.
		session->_RingBuffer.MoveWritePtr(byteTransfered);
		//링버퍼에 있는거 하나하나 전부다 직렬화 버퍼로 가져올거임.
		while (true)
		{
			InterlockedIncrement(&this->_TotalPacket);
			InterlockedIncrement(&this->_RecvPacketPerSec);

			Packet* recvPacket = Packet::Alloc();
			recvPacket->AddRef();
			recvPacket->MoveReadPtr(NET_HEADER_SIZE);
			LanServerHeader header;
			if (this->TryGetCompletedPacket(session, recvPacket, header))
				this->OnRecv(session->_SessionID, recvPacket);
			else
			{
				Packet::Free(recvPacket);
				break;
			}
			Packet::Free(recvPacket);
		}
		this->RecvPost(session);
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------



	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// helper function 아이피를 가지고 wide string함
	void CLanServer::GetStringIP(WCHAR* str,DWORD bufferLen, sockaddr_in& addr)
	{
		WCHAR temp[10]{ 0 };
		_itow_s(addr.sin_addr.S_un.S_un_b.s_b1, temp, 10);
		wcscat_s(str,bufferLen, temp);
		wcscat_s(str, bufferLen, L".");
		_itow_s(addr.sin_addr.S_un.S_un_b.s_b2, temp, 10);
		wcscat_s(str, bufferLen, temp);
		wcscat_s(str, bufferLen, L".");
		_itow_s(addr.sin_addr.S_un.S_un_b.s_b3, temp, 10);
		wcscat_s(str, bufferLen, temp);
		wcscat_s(str, bufferLen, L".");
		_itow_s(addr.sin_addr.S_un.S_un_b.s_b4, temp, 10);
		wcscat_s(str, bufferLen, temp);
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------



	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// Accept Thread
	// param : this
	unsigned int CLanServer::CLanServerAcceptThread(void* param)
	{
		ULONGLONG sessionID = 0;
		SOCKET client_sock;
		sockaddr_in clientaddr;
		for (;;)
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

			InterlockedIncrement(&_TotalAcceptSession);
			InterlockedIncrement(&_AcceptPerSec);
		}
		return -1;
	}
	unsigned int CLanServer::CLanServerTimeOutThread(void* param)
	{
		DWORD timeOutTimer;
		while (!_ShutDownFlag)
		{
			timeOutTimer = timeGetTime();
			Sleep(40000);
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
	unsigned int CLanServer::CLanServerWorkerThread(void* param)
	{
		DWORD thisThreadID = GetCurrentThreadId();
		DWORD byteTransfered = 0;
		ULONG_PTR completionKey = 0;
		OVERLAPPED* over = nullptr;
		OverlappedEx* job = nullptr;
		ULONGLONG sessionID = 0;
		Session* session = nullptr;
		int GQCSRet = 0;
		for (;;)
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
					this->DispatchError(dfNCWORKER_OVERLAPPED_IS_NULL, err, L"GQCS returned and overlapped is nullptr");
					continue;
				}
			}
			else if (over == (OVERLAPPED*)0xffffffff)
			{
				this->OnClientLeave(completionKey);
				//PushSessionIndex(completionKey & 0xffff);
				continue;
			}
			sessionID = (ULONGLONG)completionKey;
			job = (OverlappedEx*)over;

			session = AcquireSession(sessionID);
			if (session == nullptr)
				continue;
			//recv 완료처리라면
			else if (GQCSRet != 0 && byteTransfered != 0)
			{
				if (job->_IsRecv)
					this->RecvProc(session, byteTransfered);
				//send 완료 처리라면
				else
					this->SendProc(session, byteTransfered);
			}
			ReturnSession(session);

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
	// Monitering Thread
	//unsigned int CLanServer::CLanServerMoniteringThread(void* param)
	//{
	//	printf("NetCore::NetCoreMoniteringThread Running...\n");
	//	_BeginTime = GetTickCount64();
	//	DWORD prev = timeGetTime();
	//	while (true)
	//	{
	//		DWORD cur = timeGetTime();
	//		ULONGLONG now = GetTickCount64();
	//		if (_kbhit())
	//		{
	//			int key = _getch();
	//			key = tolower(key);
	//			if (key == 's' || key == 'q')
	//			{
	//				univ_dev::SaveProfiling();
	//				univ_dev::ResetProfiling();
	//			}
	//			if (key == 'q')
	//			{
	//				this->_ShutDownFlag = true;
	//				closesocket(this->_ListenSocket);
	//				printf("Monitering Thread End\n");
	//				return 0;
	//			}
	//			if (key == 'p')
	//			{
	//				switch (this->_ProfilingFlag)
	//				{
	//				case PROFILING_FLAG::OFF_FLAG:
	//					this->_ProfilingFlag = PROFILING_FLAG::MAIN_LOOP_FLAG;
	//					break;
	//				case PROFILING_FLAG::MAIN_LOOP_FLAG:
	//					this->_ProfilingFlag = PROFILING_FLAG::PACKET_PROCESS_LOOP_FLAG;
	//					break;
	//				case PROFILING_FLAG::PACKET_PROCESS_LOOP_FLAG:
	//					this->_ProfilingFlag = PROFILING_FLAG::OFF_FLAG;
	//					break;
	//				}
	//			}
	//		}
	//		if (cur - prev < 1000)
	//		{
	//			Sleep(50);
	//			continue;
	//		}
	//		prev = cur;
	//		//system("cls");
	//		const char* str = nullptr;
	//		switch (this->_ProfilingFlag)
	//		{
	//		case PROFILING_FLAG::OFF_FLAG:
	//			str = "PROFILING_OFF_FLAG";
	//			break;
	//		case PROFILING_FLAG::MAIN_LOOP_FLAG:
	//			str = "MAIN_LOOP_PROFILING_FLAG";
	//			break;
	//		case PROFILING_FLAG::PACKET_PROCESS_LOOP_FLAG:
	//			str = "MAIN_LOOP + PACKET_PROCESS_LOOP_FLAG";
	//			break;
	//		default:
	//			str = "DEFAULT";
	//			break;
	//		}
	//		unsigned long long r_TPS = InterlockedExchange(&this->_PacketPerSec, 0);
	//		unsigned long long r_AcceptPS = InterlockedExchange(&this->_AcceptPerSec, 0);
	//		printf("\n-------------------------------------------------------------------------\nServerControl\nSaveProfile = \'s\'\nQuitProgram = \'q\'\nChangeProfilingFlag = \'p\'\n-------------------------------------------------------------------------\nNumOfThread = %d\nRunningThread = %d\nCurrentProfilingFlag = %s\nExcuteTime : %llu\nTPS : %llu\nTotalProcessedBytes : %llu\nTotal Packet Process : %llu\ng_SendSuccessCount : %llu\ng_SendIOPendingCount : %llu\ng_RecvSuccessCount : %llu\ng_RecvIOPendingCount : %llu\nMEMORY_FREE_LIST\nPacketPool Capacity : %d\nPacketPool UseCount : %d\nTotal Packet Use Count : %d\nSessionPool Capacity : %llu\nSessionPool UseCount : %d\nTotalRemovedSessionCounts :%llu\nTotalAcceptSessionCounts :%llu\nAcceptPerSec : %llu\n-------------------------------------------------------------------------\n", this->_ThreadPoolSize, this->_RunningThreadCount, str, (now - _BeginTime) / 1000, r_TPS, this->_TotalProcessedBytes, this->_TotalPacket, this->_SendSuccessCount, this->_SendIOPendingCount, this->_RecvSuccessCount, this->_RecvIOPendingCount, Packet::GetCapacityCount(), Packet::GetUseCount(), Packet::GetTotalPacketCount(), this->_MaxSessionCounts, (int)(this->_MaxSessionCounts - this->_SessionIdx.GetUseCount()), this->_TotalReleasedSession, this->_TotalAcceptSession, r_AcceptPS/*, this->_SessionMap.size()*/);
	//		int cnt = 0;
	//		for (auto iter = this->_ThreadIdMap.begin(); iter != this->_ThreadIdMap.end(); ++iter)
	//		{
	//			printf("Thread : %d , Count : %d\t\t\t", iter->first, iter->second);
	//			if (++cnt % 2 == 0)
	//				printf("\n");
	//		}
	//		printf("\n");
	//	}
	//	return -1;
	//}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------

	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// Net Core Cleanup
	void CLanServer::CLanServerCleanup()
	{
		//정리 작업
		CloseHandle(this->_IOCP);
		CloseHandle(this->_RunningEvent);
		DeleteCriticalSection(&this->_SessionMapLock);
		//delete this->_TLSPacketPool;
		//delete this->_PacketPool;
		delete[] this->_WorkerThreads;
		InterlockedExchange(&this->_ServerOnFlag, false);
		WSACleanup();
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------


	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// 세션 찾기
	Session* CLanServer::FindSession(ULONGLONG sessionID)
	{
		return &_SessionArr[sessionID & 0xffff];
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------



	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// 동기화 함수
	void CLanServer::SessionMapLock()
	{
		EnterCriticalSection(&_SessionMapLock);
	}
	//---------------------------------------------------------------------------------------------------------------------------------

	//---------------------------------------------------------------------------------------------------------------------------------
	void CLanServer::SessionMapUnlock()
	{
		LeaveCriticalSection(&_SessionMapLock);
	}
	//---------------------------------------------------------------------------------------------------------------------------------


	Session* CLanServer::AcquireSession(ULONGLONG sessionID)
	{
		Session* session = FindSession(sessionID);

		DWORD IOCount = InterlockedIncrement(&session->_IOCounts);
		if ((IOCount & 0x80000000) == true)
		{
			DWORD ret = InterlockedDecrement(&session->_IOCounts);
			if (ret == 0)
			{
				ReleaseSession(session);
			}
			return nullptr;
		}
		if (session->_SessionID != sessionID)
		{
			DWORD ret = InterlockedDecrement(&session->_IOCounts);
			if (ret == 0)
			{
				ReleaseSession(session);
			}
			return nullptr;
		}
		return session;
	}

	void CLanServer::ReturnSession(Session* session)
	{
		if (session == nullptr) return;
		DWORD ret = InterlockedDecrement(&session->_IOCounts);
		if (ret == 0)
			ReleaseSession(session);
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------


	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// 세션 생성 함수
	Session* CLanServer::CreateSession(SOCKET sock, sockaddr_in clientaddr, ULONGLONG sessionID)
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
		newSession->_RingBuffer.ClearBuffer();
		newSession->_SessionID = (sessionID << 16) | idx;
		InterlockedExchange(&newSession->_IOFlag, false);
		InterlockedIncrement(&newSession->_IOCounts);
		InterlockedAnd((long*)&newSession->_IOCounts, 0x7fffffff);
		InterlockedExchange(&newSession->_Sock, sock);
		newSession->_Sock = sock;
		InterlockedIncrement(&this->_CurSessionCount);
		CreateIoCompletionPort((HANDLE)sock, this->_IOCP, (ULONG_PTR)newSession->_SessionID, 0);
		return newSession;
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------


	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	bool CLanServer::PopSessionIndex(DWORD & ret)
	{
		return _SessionIdx.pop(ret);
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	void CLanServer::PushSessionIndex(DWORD idx)
	{
		return _SessionIdx.push(idx);
	}
	CLanServer::MoniteringInfo CLanServer::GetMoniteringInfo()
	{
		MoniteringInfo ret;
		ret._WorkerThreadCount = _ThreadPoolSize;
		ret._RunningThreadCount = _RunningThreadCount;
		ret._AccpeptCount = InterlockedExchange(&_AcceptPerSec, 0);
		ret._RecvPacketCount = InterlockedExchange(&_RecvPacketPerSec, 0);
		ret._SendPacketCount = InterlockedExchange(&_SendPacketPerSec, 0);
		ret._TotalAcceptSession = _TotalAcceptSession;
		ret._TotalPacket = _TotalPacket;
		ret._TotalProecessedBytes = _TotalProcessedBytes;
		ret._TotalReleaseSession = _TotalReleasedSession;
		ret._SessionCnt = _CurSessionCount;
		ret._LockFreeStackCapacity = _SessionIdx.GetCapacityCount();
		ret._LockFreeStackSize = _SessionIdx.GetUseCount();
		ret._LockFreeQueueSize = 0;
		ret._LockFreeQueueCapacity = 0;
		ret._LockFreeMaxCapacity = 0;
		for (int i = 0; i < _MaxSessionCounts; i++)
			ret._LockFreeQueueSize += _SessionArr[i]._SendPacketQueue.size();
		for (int i = 0; i < _MaxSessionCounts; i++)
		{
			if (_SessionArr[i]._SendPacketQueue.GetCapacityCount() > ret._LockFreeMaxCapacity)
				ret._LockFreeMaxCapacity = _SessionArr[i]._SendPacketQueue.GetCapacityCount();
			ret._LockFreeQueueCapacity += _SessionArr[i]._SendPacketQueue.GetCapacityCount();
		}
		return ret;
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------

	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// 세션 제거함수
	void CLanServer::ReleaseSession(Session* session)
	{
		if (InterlockedCompareExchange(&session->_IOCounts, 0x80000000, 0) != 0)
			return;

		ULONGLONG sessionID = session->_SessionID;
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

		this->PostOnClientLeave(sessionID);
		this->PushSessionIndex(idx);
		InterlockedDecrement(&_CurSessionCount);
		InterlockedIncrement(&_TotalReleasedSession);
		return;
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------


	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// 연결 종료 함수
	void CLanServer::DisconnectSession(ULONGLONG sessionID)
	{
		Session* disconnectSession = this->AcquireSession(sessionID);
		if (disconnectSession == nullptr) return;
		SOCKET sock = InterlockedOr((long*)&disconnectSession->_Sock, 0x80000000);
		CancelIoEx((HANDLE)(sock & 0x7fffffff), nullptr);
		ReturnSession(disconnectSession);
	}
	//---------------------------------------------------------------------------------------------------------------------------------




	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// 생성자 소멸자
	CLanServer::CLanServer(USHORT port, DWORD backlogQueueSize, DWORD threadPoolSize, DWORD runningThread, DWORD nagleOff, ULONGLONG maxSessionCounts)
		: _ServerPort{ port }, _ShutDownFlag{ false }, _APIErrorCode{ 0 }, _ErrorCode{ 0 }, _IOCP{ nullptr }, _AcceptThread{ nullptr }, _RecvPacketPerSec{ 0 }, _SendPacketPerSec{ 0 },  _TotalPacket{ 0 }, _ListenSocket{ 0 }, _WorkerThreads{ nullptr }, _ThreadPoolSize{ threadPoolSize }, _RunningThreadCount{ runningThread }, _RunningEvent{ nullptr }, _TotalProcessedBytes{ 0 }, _SessionMapLock{ 0 }, _TotalReleasedSession{ 0 }, _NagleOff{ nagleOff }, _MaxSessionCounts{ maxSessionCounts }, _AcceptPerSec{ 0 }, _SessionArr{ nullptr }
	{
		this->CLanServerStartup();
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	CLanServer::~CLanServer()
	{
		this->CLanServerCleanup();
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
}