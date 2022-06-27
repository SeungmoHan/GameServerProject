#pragma once
#ifndef __JOB_HEADER__
#define __JOB_HEADER__
#define __UNIV_DEVELOPER_

#include "CoreBase.h"
#include <string>


namespace univ_dev
{
	class Player;
	struct JobMessage
	{
		enum Type
		{
			THREAD_BLOCK_RUN, THREAD_BLOCK_STOP, THREAD_BLOCK_RESUME, THREAD_BLOCK_PAUSE,
			CLIENT_LEAVE, CLIENT_ENTER, MESSAGE, TIME_OUT, CLIENT_MOVE_LEAVE, CLIENT_MOVE_ENTER,
		};

		ULONGLONG _SessionID;
		Player* _Player;
		WORD _Type;
		Packet* _Packet;
		std::string _ThreadBlockName;
	};
}



#endif // !__JOB_HEADER__
