#pragma once
#ifndef __CORE_BASE_DEF__
#define __CORE_BASE_DEF__
#define __UNIV_DEVELOPER_
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"Winmm.lib")
#include <WinSock2.h>
#include <Windows.h>

#include "ObjectFreeList.hpp"
#include "profiler.h"
#include "RingBuffer.h"
#include "SerializingBuffer.h"

#define CRASH() do{int*ptr =nullptr; *ptr =100;}while(0)


#endif // !__CORE_BASE_DEF__
