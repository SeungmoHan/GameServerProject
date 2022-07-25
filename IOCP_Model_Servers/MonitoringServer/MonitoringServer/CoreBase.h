#pragma once
#ifndef __CORE_BASE_DEF__
#define __CORE_BASE_DEF__
#define __UNIV_DEVELOPER_
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"Winmm.lib")
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>

#include "DumpClass.h"
#include "profiler.h"
#include "RingBuffer.h"
#include "SerializingBuffer.h"
#include "LockFreeQueue.hpp"
#include "LockFreeStack.hpp"
#include "LockFreeMemoryPoolTLS.hpp"
#include "ConfigReader.h"
#include "LogClass.h"
#include "HardWareMoniteringClass.h"
#include "ProcessMoniteringClass.h"

#define CRASH() do{int*ptr =nullptr; *ptr =100;}while(0)


#endif // !__CORE_BASE_DEF__
