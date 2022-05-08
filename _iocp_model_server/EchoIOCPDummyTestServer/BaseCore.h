#pragma once
#ifndef __BASE_CORE_DEF__
#define __BASE_CORE_DEF__
#define __UNIV_DEVELOPER_
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"Winmm.lib")
#include <WinSock2.h>
#include <Windows.h>

#include "ObjectFreeList.hpp"
#include "profiler.h"
#include "RingBuffer.h"
#include "SerializingBuffer.h"



#endif // !__BASE_CORE_DEF__
