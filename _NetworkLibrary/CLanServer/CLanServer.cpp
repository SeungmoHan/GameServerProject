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
		//WorkerThread -> NetCore::Run �� ȣ��ɶ����� �����ִٰ� NetCore::Run�� ȣ��Ǿ�� ����
		CLanServer* core = (CLanServer*)param;
		core->_ThreadIdMap[GetCurrentThreadId()]++;
		WaitForSingleObject(core->_RunningEvent, INFINITE);
		return core->CLanServerWorkerThread(param);
	}
	//---------------------------------------------------------------------------------------------------------------------------------

	//---------------------------------------------------------------------------------------------------------------------------------
	//Accept Thread Wrapping function
	// param CLanServer*
	unsigned __stdcall AcceptThread(void* param)
	{
		//AcceptThread -> NetCore::Run �� ȣ��ɶ����� �����ִٰ� NetCore::Run�� ȣ��Ǿ�� ����
		CLanServer* core = (CLanServer*)param;
		WaitForSingleObject(core->_RunningEvent, INFINITE);
		return core->CLanServerAcceptThread(param);
	}
	//---------------------------------------------------------------------------------------------------------------------------------

	//---------------------------------------------------------------------------------------------------------------------------------
	//Monitering Thread Warpping function
	// param CLanServer*
	unsigned __stdcall MoniteringThread(void* param)
	{
		//MoniteringThread -> NetCore::Run �� ȣ��ɶ����� �����ִٰ� NetCore::Run�� ȣ��Ǿ�� ����
		CLanServer* core = (CLanServer*)param;
		WaitForSingleObject(core->_RunningEvent, INFINITE);
		return core->CLanServerMoniteringThread(param);
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------



	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	//RecvPost -> WSARecv call
	void CLanServer::RecvPost(Session* session)
	{
		int recvRet;
		WSABUF recvWSABuf[2];
		recvWSABuf[0].buf = session->_RecvJob._RingBuffer.GetWritePtr();
		recvWSABuf[0].len = session->_RecvJob._RingBuffer.DirectEnqueueSize();
		recvWSABuf[1].buf = session->_RecvJob._RingBuffer.GetBeginPtr();
		recvWSABuf[1].len = session->_RecvJob._RingBuffer.GetFreeSize() - session->_RecvJob._RingBuffer.DirectEnqueueSize();
		if (recvWSABuf[1].len > session->_RecvJob._RingBuffer.GetBufferSize())
			recvWSABuf[1].len = 0;
		DWORD flag = 0;
		ZeroMemory(&session->_RecvJob._Overlapped, sizeof(OVERLAPPED));
		//printf("        WSARecv Call\n");
		InterlockedIncrement(&session->_IOCounts);
		recvRet = WSARecv(session->_Sock, recvWSABuf, 2, nullptr, &flag, &session->_RecvJob._Overlapped, nullptr);
		if (recvRet == 0)
		{
			InterlockedIncrement(&this->_RecvSuccessCount);
		}
		else if (recvRet == SOCKET_ERROR)
		{
			int err = WSAGetLastError();
			if (err != WSA_IO_PENDING)
			{
				if (err != 10054 && err != 10063)
				{
					DispatchError(dfNCWORKER_WSARECV_SOCKET_ERROR_WAS_NOT_WSA_IO_PENDING, err, L"WSARecv ret is SOCKET_ERROR and error code is not WSA_IO_PENDING");
				}
				DWORD IOCount = InterlockedDecrement(&session->_IOCounts);
				if (IOCount == 0)
					ReleaseSession(session->_SessionID);
			}
			else
			{
				InterlockedIncrement(&this->_RecvIOPendingCount);
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
		if (InterlockedExchange(&session->_IOFlag, true) == false)
		{
			int useSize = session->_SendJob._RingBuffer.GetUseSize();
			char* readPtr = session->_SendJob._RingBuffer.GetReadPtr();
			int directDqSize = session->_SendJob._RingBuffer.DirectDequeueSize();
			if (useSize > 0)
			{
				int sendRet = 0;
				ZeroMemory(&session->_SendJob._Overlapped, sizeof(OVERLAPPED));
				session->_SendJob._IsRecv = false;
				int cnt = min(200, useSize / sizeof(Packet*));
				WSABUF sendWSABuf[200]{ 0 };
				Packet* packetArr[200]{ 0 };
				
				int peekRet = session->_SendJob._RingBuffer.Peek((char*)packetArr, cnt * sizeof(Packet*));
				for (int i = 0; i < cnt; i++)
				{
					sendWSABuf[i].buf = packetArr[i]->GetReadPtr();
					sendWSABuf[i].len = packetArr[i]->GetBufferSize();
				}
				if (cnt < 200)
					sendWSABuf[cnt].len = 0;
				InterlockedExchange(&session->_SendBufferCount, cnt);
				InterlockedIncrement(&session->_IOCounts);
				sendRet = WSASend(session->_Sock, sendWSABuf, cnt, nullptr, 0, &session->_SendJob._Overlapped, nullptr);
				if (sendRet == 0)
				{
					InterlockedIncrement(&this->_SendSuccessCount);
				}
				else if (sendRet == SOCKET_ERROR)
				{
					int err = WSAGetLastError();
					if (err != WSA_IO_PENDING)
					{
						if (err != 10054 && err != 10063)
						{
							DispatchError(dfNCWORKER_WSASEND_SOCKET_ERROR_WAS_NOT_WSA_IO_PENDING, err, L"WSASend ret is SOCKET_ERROR and error code is not WSA_IO_PENDING");
						}
						InterlockedDecrement(&session->_IOCounts);
						DWORD IOCount = InterlockedExchange(&session->_IOFlag, false);
						if (IOCount == 0)
							ReleaseSession(session->_SessionID);
					}
					else
					{
						InterlockedIncrement(&this->_SendIOPendingCount);
					}
				}
			}
			else
			{
				InterlockedExchange(&session->_IOFlag, false);
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
		//�ùٸ� �������� Ȯ��
		Session* session = this->FindSession(sessionID);
		if (session == nullptr)
		{
			DispatchError(dfNCWORKER_INVALID_SESSION_ID, sessionID, L"SendPacket -> Invalid SessionID in param API err is session ID");
			_PacketPool.Free(packet);
			return;
		}
		//��û��̿� ���� ID�� �ٲ�Ŷ�� �� ������ �̹� Release�Ǿ��� �����̶�� �ǹ�.
		if (session->_SessionID != sessionID)
		{
			DispatchError(dfNCWORKER_INVALID_SESSION_ID, sessionID, L"SendPacket -> session->_SessioID != sessionID(param) API err is session ID");
			_PacketPool.Free(packet);
			return;
		}
		//�װ� �ƴ϶�� ���� �۽�
		int eqRet = session->_SendJob._RingBuffer.Enqueue((const char*)&packet, sizeof(Packet*));
		if (eqRet != sizeof(Packet*))
		{
			printf("eqRet is sizeof(Packet*): %llu EQRET : %d\n", sizeof(Packet), eqRet);
			DispatchError(dfNCWORKER_SENDQ_IS_FULL, 0, L"send ringbuffer is full");
			_PacketPool.Free(packet);
			DisconnectSession(session);
			return;
		}
		if (_ProfilingFlag >= PROFILING_FLAG::PACKET_PROCESS_LOOP_FLAG)
		{
			Profiler profiler("SendPost2", GetCurrentThreadId());
			this->SendPost(session);
		}
		else
			this->SendPost(session);
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------


	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// Run ȣ�������� ����� �ٸ� ��������� �۵����� ���� ���ǹٶ�
	void CLanServer::Run()
	{
		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------
		// �۵����� ��������� �� �̺�Ʈ�� �ٶ󺸰��ְ� �޴��� �̺�Ʈ.
		// �̺�Ʈ�� ���ϸ� �޴��� �̺�Ʈ�̱� ������ ��� �����带 �ٱ����.
		SetEvent(this->_RunningEvent);
		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------


		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------
		// ��� ��������� �۾��� ���������� ����ϴ� ���ν�����
		HANDLE* runningThreads = new HANDLE[(size_t)this->_ThreadPoolSize + ACCEPT_THREAD_COUNT + MONITERING_THREAD_COUNT];

		for (int i = 0; i < this->_ThreadPoolSize; i++)
			runningThreads[i] = this->_WorkerThreads[i];

		runningThreads[this->_ThreadPoolSize] = this->_AcceptThread;
		runningThreads[this->_ThreadPoolSize + 1] = this->_LogThread;

		WaitForMultipleObjects(this->_ThreadPoolSize + ACCEPT_THREAD_COUNT + MONITERING_THREAD_COUNT, runningThreads, true, INFINITE);

		delete[] runningThreads;
		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------


		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------
		//���� �۾��� ���� �α� ���� ����
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
	// ���������� �����ڵ� �����ϸ鼭 OnErrorOccured ȣ��
	void CLanServer::DispatchError(DWORD errorCode,DWORD APIErrorCode,const WCHAR* errorStr)
	{
		//���̺귯�� ��ü �����ڵ� ��� �� API���� �ڵ� ���
		this->_ErrorCode = errorCode;
		this->_APIErrorCode = APIErrorCode;
		// OnErrorOccured �Լ��� ���̺귯�� �����ڵ带 �����ϹǷ� GetLastAPIErrorCode �Լ� ȣ���ؼ� ���� ���ߵ�.
		OnErrorOccured(errorCode, errorStr);
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------


	
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// CLanServer �ʱ�ȭ
	void CLanServer::CLanServerStartup()
	{
		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------
		// ���Ͷ����� ���� �÷��׸� true�� ��������� ���� true�� ����������
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
		// NetCore �α�� ���� ����
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
		// �۾��� ��������� ���� �����忡�� NetCore::Run�̶�� �Լ��� ȣ���ϱ� �������� ����Ǿ�����
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
		// ���� �ʱ�ȭ
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
		// IOCP ����
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
		// Ÿ�̸� ���ͷ�Ʈ �ػ� ����
		// ���� �����̳ʿ� ���� SRWLock��ü �ʱ�ȭ
		// �������Ϸ� �� ���õ� �ʱ�ȭ
		printf("timeBeginPeriod, CSLocker Init, ProfilerSample Init Begin\n");
		fprintf(MainThreadLogFile, "timeBeginPeriod, CSLocker Init, ProfilerSample Init Begin\n");
		timeBeginPeriod(1);
		InitializeCriticalSection(&_SessionMapLock);
		//InitializeSRWLock(&sessionMapLock);
		univ_dev::InitializeProfilerAndSamples();
		fprintf(MainThreadLogFile, "timeBeginPeriod, CSLocker Init, ProfilerSample Init End\n");
		printf("timeBeginPeriod, CSLocker Init, ProfilerSample Init End\n");
		fflush(MainThreadLogFile);
		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------


		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------
		// �ʴ��� �α�� ������ ����
		printf("PrintThread CreateBegin\n");
		fprintf(MainThreadLogFile, "PrintThread CreateBegin\n");
		this->_LogThread = (HANDLE)_beginthreadex(nullptr, 0, MoniteringThread, this, 0, nullptr);
		if (this->_LogThread == nullptr)
		{
			InterlockedExchange(&this->_ServerOnFlag, false);
			this->_ErrorCode = dfNCINIT_LOG_THREAD_CREATE_FAILED;
			this->_APIErrorCode = GetLastError();
			return;
		}
		fprintf(MainThreadLogFile, "PrintThread CreateEnd\n");
		printf("PrintThread CreateEnd\n");
		fflush(MainThreadLogFile);
		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------


		// --------------------------------------------------------------------------------------------------------------
		// --------------------------------------------------------------------------------------------------------------
		// ������Ǯ ����, 
		// ������Ǯ ������� 0�̸� systeminfo���� �޾ƿ� ���ھ� * 2����ŭ ����������
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
		// �������� ����
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
		// �������Ͽ� IP Port ���ε�
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
		// ���� �ɼ� �������� TIME_WAIT ����
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
		// Send IO �񵿱� ó���� ���� ���� ������ 0 ����
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
		// Send IO �񵿱� ó���� ���� ���� ������ 0 ����
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
		// ���� ���ؼ� ��Ʈ ����
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
		// Accept Thread ����
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
	// MoniteringThread���� Post�� ��� FALSE��ȯ �׿ܿ��� TRUE��ȯ TRUE��ȯ�� socket�� INVALID_SOCKET�̶�� accept error
	BOOL CLanServer::TryAccept(SOCKET& clientSocket, sockaddr_in& clientAddr)
	{
		int addrlen = sizeof(clientAddr);
		clientSocket = accept(this->_ListenSocket, (SOCKADDR*)&clientAddr, &addrlen);
		if (clientSocket == INVALID_SOCKET)
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
			DispatchError(dfNCACCEPT_CLIENT_SOCKET_IS_INVALID_SOCKET, err, L"accept returned and client socket was INVAILD_SOCKET");
		}
		if (_MaxSessionCounts <= _SessionMap.size())
		{
			closesocket(clientSocket);
			DispatchError(dfNCACCEPT_SESSION_COUNTS_OVER, 0, L"session container size is over than max session counts");
		}
		return true;
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------


	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// if this function returns false there's no data in packet
	BOOL CLanServer::TryGetCompletedPacket(Session* session, Packet* packet, LANPacketHeader& header)
	{
		//�������� ���� ����� ����� ���̺��� ������ ���̻� ������ ���ٴ� �ǹ���.
		if (session->_RecvJob._RingBuffer.GetUseSize() < dfECHO_PACKET_HEADER_LENGTH)
		{
			return false;
		}
		//����ϳ� ����� ��ũ�ؼ� 2����Ʈ�� ����� ��ũ����� Ȯ��
		//EchoPacketHeader header;
		int pkRet1 = session->_RecvJob._RingBuffer.Peek((char*)&header, dfECHO_PACKET_HEADER_LENGTH);
		if (pkRet1 != dfECHO_PACKET_HEADER_LENGTH)
		{
			DispatchError(dfNCWORKER_USE_SIZE_OVER_HEADER_SIZE_AND_FIRST_PEEK_ZERO, 0, L"ringbuffer use size was over than header size but peek ret is zero");
			return false;
		}
		//recv�����ۿ��� ��ũ ������ payloadSize�� ���Ѱͺ��� ������ ����� ������ ���������� �ѱ��.
		if (session->_RecvJob._RingBuffer.GetUseSize() < dfECHO_PACKET_HEADER_LENGTH + header._payloadSize)
		{
			return false;
		}
		//recv �����ۿ��� �����͸� �������� ReadPointer�� �������Ѽ� UseSize�� �ٿ�����.
		session->_RecvJob._RingBuffer.MoveReadPtr(pkRet1);
		//payloadSize��ŭ recv �����ۿ��� �ٽ� �����´�
		int pkRet2 = session->_RecvJob._RingBuffer.Peek(packet->GetWritePtr(), header._payloadSize);
		if (pkRet2 != header._payloadSize)
		{
			DispatchError(dfNCWORKER_USE_SIZE_OVER_PAYLOAD_SIZE_AND_SECOND_PEEK_ZERO, 0, L"ringbuffer use size was over than header + payloadsize but peek ret is zero");
			return false;
		}

		session->_RecvJob._RingBuffer.MoveReadPtr(pkRet2);
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
		ULONGLONG sessionID = session->_SessionID;

		InterlockedAdd64(&_TotalProcessedBytes, byteTransfered);

		DWORD sendCount = InterlockedExchange(&session->_SendBufferCount, 0);
		Packet* packet = nullptr;
		for (int i = 0; i < sendCount; i++)
		{
			packet = nullptr;
			int dqRet = session->_SendJob._RingBuffer.Dequeue((char*)&packet, sizeof(Packet*));
			if (dqRet != sizeof(Packet*))
			{
				printf("DQ Ret is not sizeof(Packet*)\n");
			}
			packet->Clear();
			this->_PacketPool.Free(packet);
			//delete packet;
		}
		InterlockedExchange(&session->_IOFlag, false);

		if (session->_SessionID != sessionID) return;

		if (session->_SendJob._RingBuffer.GetUseSize() > 0)
		{
			if (_ProfilingFlag >= PROFILING_FLAG::PACKET_PROCESS_LOOP_FLAG)
			{
				Profiler profiler("SendPost", GetCurrentThreadId());
				this->SendPost(session);
			}
			else
				this->SendPost(session);
		}
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------



	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// RecvProc
	void CLanServer::RecvProc(Session* session, DWORD byteTransfered)
	{
		//�Ϸ�� �����ŭ writePointer�� ������Ų��.
		session->_RecvJob._RingBuffer.MoveWritePtr(byteTransfered);
		//�����ۿ� �ִ°� �ϳ��ϳ� ���δ� ����ȭ ���۷� �����ð���.
		while (true)
		{
			InterlockedIncrement(&this->_TotalPacket);
			InterlockedIncrement(&this->_PacketPerSec);
			Packet* recvPacket = this->_PacketPool.Alloc();
			recvPacket->Clear();
			LANPacketHeader header;
			if (this->_ProfilingFlag >= PROFILING_FLAG::PACKET_PROCESS_LOOP_FLAG)
			{
				Profiler packetProcessProfiler("Packet Process Profiler", GetCurrentThreadId());
				if (this->TryGetCompletedPacket(session, recvPacket, header))
					this->OnRecv(session->_SessionID, recvPacket);
				else
				{
					recvPacket->Clear();
					this->_PacketPool.Free(recvPacket);
					break;
				}
			}
			else
			{
				if (this->TryGetCompletedPacket(session, recvPacket, header))
					this->OnRecv(session->_SessionID, recvPacket);
				else
				{
					recvPacket->Clear();
					this->_PacketPool.Free(recvPacket);
					break;
				}

			}
			recvPacket->Clear();
			this->_PacketPool.Free(recvPacket);
		}
		this->RecvPost(session);
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------

	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// helper function �����Ǹ� ������ wide string��
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
		while (true)
		{
			client_sock = INVALID_SOCKET;
			if (!this->TryAccept(client_sock, clientaddr)) return 0;
			else if (client_sock == INVALID_SOCKET) continue;
			//Sleep(1000);
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
			InterlockedIncrement(&_TotalAcceptSession);
			InterlockedIncrement(&_AcceptPerSec);
		}
		return -1;
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
			GQCSRet = GetQueuedCompletionStatus(this->_IOCP, &byteTransfered, &completionKey, &over, INFINITE);
			_InterlockedIncrement((LONG*)&this->_ThreadIdMap[thisThreadID]);
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
				int err = WSAGetLastError();
				DispatchError(dfNCWORKER_OVERLAPPED_IS_NULL, err, L"gqcs returned and overlapped is nullptr");
				continue;
			}
			session = (Session*)completionKey;
			job = (JobInfo*)over;
			ULONGLONG sessionID = session->_SessionID;
			//recv �Ϸ�ó�����
			if (GQCSRet != 0 && byteTransfered != 0 && job->_IsRecv)
			{
				if (session->_SessionID != sessionID)
				{
					continue;
				}
				if (_ProfilingFlag >= PROFILING_FLAG::MAIN_LOOP_FLAG)
				{
					Profiler profile("RecvProc", thisThreadID);
					this->RecvProc(session, byteTransfered);
				}
				else
					this->RecvProc(session, byteTransfered);
			}
			//send �Ϸ� ó�����
			else if (GQCSRet != 0 && byteTransfered != 0)
			{
				if (session->_SessionID != sessionID)
				{
					continue;
				}
				if (_ProfilingFlag >= PROFILING_FLAG::MAIN_LOOP_FLAG)
				{
					Profiler profile("SendProc", thisThreadID);
					this->SendProc(session, byteTransfered);
				}
				else
				{
					this->SendProc(session, byteTransfered);
				}
			}
			int ioCounts = InterlockedDecrement(&session->_IOCounts);
			if (ioCounts == 0)
			{
				this->ReleaseSession(session->_SessionID);
			}
		}
		return -1;
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------

	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// Monitering Thread
	unsigned int CLanServer::CLanServerMoniteringThread(void* param)
	{
		printf("NetCore::NetCoreMoniteringThread Running...\n");
		_BeginTime = GetTickCount64();
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
					this->_ShutDownFlag = true;
					closesocket(this->_ListenSocket);
					printf("Monitering Thread End\n");
					return 0;
				}
				if (key == 'p')
				{
					switch (this->_ProfilingFlag)
					{
					case PROFILING_FLAG::OFF_FLAG:
						this->_ProfilingFlag = PROFILING_FLAG::MAIN_LOOP_FLAG;
						break;
					case PROFILING_FLAG::MAIN_LOOP_FLAG:
						this->_ProfilingFlag = PROFILING_FLAG::PACKET_PROCESS_LOOP_FLAG;
						break;
					case PROFILING_FLAG::PACKET_PROCESS_LOOP_FLAG:
						this->_ProfilingFlag = PROFILING_FLAG::OFF_FLAG;
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
			//system("cls");
			const char* str = nullptr;
			switch (this->_ProfilingFlag)
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
			unsigned long long r_TPS = InterlockedExchange(&this->_PacketPerSec, 0);
			unsigned long long r_AcceptPS = InterlockedExchange(&this->_AcceptPerSec, 0);
			printf("\n-------------------------------------------------------------------------\nServerControl\nSaveProfile = \'s\'\nQuitProgram = \'q\'\nChangeProfilingFlag = \'p\'\n-------------------------------------------------------------------------\nNumOfThread = %d\nRunningThread = %d\nCurrentProfilingFlag = %s\nExcuteTime : %llu\nTPS : %llu\nTotalProcessedBytes : %llu\nTotal Packet Process : %llu\ng_SendSuccessCount : %llu\ng_SendIOPendingCount : %llu\ng_RecvSuccessCount : %llu\ng_RecvIOPendingCount : %llu\nMEMORY_FREE_LIST\nPacketPool Capacity : %d\nPacketPool UseCount : %d\nSessionPool Capacity : %d\nSessionPool UseCount : %d\nTotalRemovedSessionCounts :%llu\nTotalAcceptSessionCounts :%llu\nAcceptPerSec : %llu\nSessionMapSize : %llu\n-------------------------------------------------------------------------\n", this->_ThreadPoolSize, this->_RunningThreadCount, str, (now - _BeginTime) / 1000, r_TPS, this->_TotalProcessedBytes, this->_TotalPacket, this->_SendSuccessCount, this->_SendIOPendingCount, this->_RecvSuccessCount, this->_RecvIOPendingCount, this->_PacketPool.GetCapacityCount(), this->_PacketPool.GetUseCount(), this->_SessionPool.GetCapacityCount(), this->_SessionPool.GetUseCount(), this->_TotalReleasedSession, this->_TotalAcceptSession,r_AcceptPS,this->_SessionMap.size());
			int cnt = 0;
			for (auto iter = this->_ThreadIdMap.begin(); iter != this->_ThreadIdMap.end(); ++iter)
			{
				printf("Thread : %d , Count : %d\t\t\t", iter->first, iter->second);
				if (++cnt % 2 == 0)
					printf("\n");
			}
			printf("\n");
		}
		return -1;
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------

	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// Net Core Cleanup
	void CLanServer::CLanServerCleanup()
	{
		//���� �۾�
		CloseHandle(this->_IOCP);
		CloseHandle(this->_RunningEvent);
		for (auto iter = this->_SessionMap.begin(); iter != this->_SessionMap.end(); ++iter)
		{
			closesocket(iter->second->_Sock);
			this->_SessionPool.Free(iter->second);
		}
		this->_SessionMap.clear();
		DeleteCriticalSection(&this->_SessionMapLock);
		delete[] this->_WorkerThreads;
		InterlockedExchange(&this->_ServerOnFlag, false);
		WSACleanup();
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------


	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// ���� ã��
	Session* CLanServer::FindAndLockSession(ULONGLONG key)
	{
		this->SessionMapLock();
		auto iter = this->_SessionMap.find(key);
		if (iter == this->_SessionMap.end())
		{
			this->SessionMapUnlock();
			return nullptr;
		}
		Session* session = iter->second;
		this->LockSession(session);
		this->SessionMapUnlock();
		return session;
	}
	//---------------------------------------------------------------------------------------------------------------------------------

	//---------------------------------------------------------------------------------------------------------------------------------
	Session* CLanServer::FindSession(ULONGLONG sessionID)
	{
		this->SessionMapLock();
		auto iter = this->_SessionMap.find(sessionID);
		if (iter == this->_SessionMap.end())
		{
			this->SessionMapUnlock();
			return nullptr;
		}
		Session* session = iter->second;
		this->SessionMapUnlock();
		return session;
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------



	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// ����ȭ �Լ�
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

	//---------------------------------------------------------------------------------------------------------------------------------
	void CLanServer::LockSession(Session* session)
	{
		if (session == nullptr) return;
		EnterCriticalSection(&session->_Lock);
	}
	//---------------------------------------------------------------------------------------------------------------------------------

	//---------------------------------------------------------------------------------------------------------------------------------
	void CLanServer::UnlockSession(Session* session)
	{
		if (session == nullptr) return;
		LeaveCriticalSection(&session->_Lock);
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------


	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// ���� ���� �Լ�
	Session* CLanServer::CreateSession(SOCKET sock, sockaddr_in clientaddr, ULONGLONG sessionID)
	{
		//Session* newSession = new Session();
		Session* newSession = this->_SessionPool.Alloc();
		if (newSession == nullptr)
		{
			this->DispatchError(dfNCACCEPT_SESSION_POOL_NO_MEMORY, 0, L"session pool can not alloc no memory");
			return nullptr;
		}
		InitializeCriticalSection(&newSession->_Lock);
		newSession->_SessionIP = ntohl(clientaddr.sin_addr.S_un.S_addr);
		newSession->_SessionPort = ntohs(clientaddr.sin_port);
		ZeroMemory(newSession->_SessionIPStr, sizeof(newSession->_SessionIPStr));
		this->GetStringIP(newSession->_SessionIPStr, 20, clientaddr);
		newSession->_IOCounts = 0;
		newSession->_IOFlag = false;
		newSession->_Sock = sock;

		if (newSession->_SessionID != 0)
			this->DispatchError(dfNCACCEPT_SESSION_ID_NOT_CLEANUP, 0, L"last free session was not cleaned up");
		newSession->_SessionID = sessionID;

		newSession->_RecvJob._IsRecv = true;
		newSession->_SendJob._IsRecv = false;
		newSession->_RecvJob._RingBuffer.ClearBuffer();
		newSession->_SendJob._RingBuffer.ClearBuffer();
		ZeroMemory(&newSession->_RecvJob._Overlapped, sizeof(OVERLAPPED));
		ZeroMemory(&newSession->_SendJob._Overlapped, sizeof(OVERLAPPED));
		this->SessionMapLock();
		this->_SessionMap.emplace(std::make_pair(newSession->_SessionID, newSession));
		this->SessionMapUnlock();
		CreateIoCompletionPort((HANDLE)sock, this->_IOCP, (ULONG_PTR)newSession, 0);
		return newSession;
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------


	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// ���� �����Լ�
	void CLanServer::ReleaseSession(ULONGLONG sessionID)
	{
		if (sessionID == 0)
		{
			this->DispatchError(dfNCWORKER_INVALID_SESSION_ID, sessionID, L"ReleaseSession -> Invalid Session ID API err is session ID");
			return;
		}
		this->SessionMapLock();
		auto iter = this->_SessionMap.find(sessionID);
		if (iter == this->_SessionMap.end())
		{
			this->DispatchError(dfNCWORKER_INVALID_SESSION_ID, sessionID, L"ReleaseSession -> Invalid Session Iter API err is session id");
			this->DispatchError(sessionID, GetCurrentThreadId(), L"Session, ThreadID");
			this->SessionMapUnlock();
			return;
		}
		Session* removeSession = iter->second;
		this->_SessionMap.erase(sessionID);
		this->SessionMapUnlock();

		if (removeSession->_SessionID != sessionID)
		{
			this->DispatchError(dfNCWORKER_INVALID_SESSION_ID, sessionID, L"ReleaseSession -> Invalid SessionID in param API err is session id");
			return;
		}
		DWORD sendCount = InterlockedExchange(&removeSession->_SendBufferCount, 0);
		if (removeSession->_SendJob._RingBuffer.GetUseSize() / sizeof(Packet*) > sendCount)
			sendCount = removeSession->_SendJob._RingBuffer.GetUseSize() / sizeof(Packet*);

		Packet* packet = nullptr;
		for (int i = 0; i < sendCount; i++)
		{
			packet = nullptr;
			int dqRet = removeSession->_SendJob._RingBuffer.Dequeue((char*)&packet, sizeof(Packet*));
			if (dqRet != sizeof(Packet*))
			{
				printf("DQ Ret is not sizeof(Packet*)\n");
			}
			if (packet != nullptr)
			{
				packet->Clear();
				this->_PacketPool.Free(packet);
			}
			else
				CRASH();
		}
		SOCKET tempsock = removeSession->_Sock;
		if (removeSession->_IOCounts != 0)
		{
			CRASH();
		}
		this->DisconnectSession(removeSession);
		SOCKET sock = InterlockedExchange(&removeSession->_Sock, 0);
		if (sock != 0)
			closesocket(sock);
		InterlockedExchange(&removeSession->_SessionID, 0);
		removeSession->_IOFlag = false;
		removeSession->_SendBufferCount = 0;
		removeSession->_SendJob._RingBuffer.ClearBuffer();
		removeSession->_RecvJob._RingBuffer.ClearBuffer();
		DeleteCriticalSection(&removeSession->_Lock);
		this->_SessionPool.Free(removeSession);
		this->OnClientLeave(sessionID);
		InterlockedIncrement(&_TotalReleasedSession);
		return;
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------


	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// ���� ���� �Լ�
	void CLanServer::DisconnectSession(ULONGLONG sessionID)
	{
		Session* disconnectSession = this->FindSession(sessionID);
		if (disconnectSession == nullptr)
		{
			this->DispatchError(dfNCWORKER_INVALID_SESSION_ID, sessionID, L"DisconnectSession -> Invalid Session Iter API err is session id");
			return;
		}
		if (disconnectSession->_SessionID != sessionID)
		{
			this->DispatchError(dfNCWORKER_INVALID_SESSION_ID, sessionID, L"DisconnectSession -> Invalid SessionID in param API err is session id");
			return;
		}
		CancelIo((HANDLE)disconnectSession->_Sock);
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	void CLanServer::DisconnectSession(Session* session)
	{
		if (session == nullptr)
		{
			this->DispatchError(dfNCWORKER_INVALID_SESSION_ID, 0, L"DisconnectSession -> Session(param) is nullptr");
			return;
		}
		if (session->_SessionID == 0)
		{
			this->DispatchError(dfNCWORKER_INVALID_SESSION_ID, session->_SessionID, L"DisconnectSession -> Session->_SessionID == 0");
			return;
		}
		CancelIo((HANDLE)session->_Sock);
	}
	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------




	//---------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------------
	// ������ �Ҹ���
	CLanServer::CLanServer(USHORT port, DWORD backlogQueueSize, DWORD threadPoolSize, DWORD runningThread, DWORD nagleOff, ULONGLONG maxSessionCounts)
		: _ServerPort{ port }, _ShutDownFlag{ false }, _APIErrorCode{ 0 }, _ErrorCode{ 0 }, _IOCP{ nullptr }, _AcceptThread{ nullptr }, _PacketPerSec{ 0 }, _RecvIOPendingCount{ 0 }, _RecvSuccessCount{ 0 }, _SendIOPendingCount{ 0 }, _SendSuccessCount{ 0 }, _TotalPacket{ 0 }, _ListenSocket{ 0 }, _LogThread{ nullptr }, _WorkerThreads{ nullptr }, _ThreadPoolSize{ threadPoolSize }, _RunningThreadCount{ runningThread }, _RunningEvent{ nullptr }, _ProfilingFlag{ PROFILING_FLAG::OFF_FLAG }, _TotalProcessedBytes{ 0 }, _SessionMapLock{ 0 }, _TotalReleasedSession{ 0 },_NagleOff(nagleOff),_MaxSessionCounts(maxSessionCounts),_AcceptPerSec(0)
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