#pragma once
#ifndef __CHAT_SESSION_HEADER__
#define __CHAT_SESSION_HEADER__
#include "RingBuffer.h"

namespace univ_dev
{
	struct Session
	{
		DWORD sessionNo;
		DWORD currentRoomNumber;
		bool logined;
		WCHAR sessionNickName[dfNICK_MAX_LEN];
		SOCKET sock;
		RingBuffer RQ;
		RingBuffer SQ;
	};
}


#endif // !__CHAT_SESSION_HEADER__
