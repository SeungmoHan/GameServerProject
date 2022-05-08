#pragma once
#ifndef __JOB_HEADER__
#define __JOB_HEADER__
#define __UNIV_DEVELOPER_

#include "CoreBase.h"


namespace univ_dev
{
	struct JobMessage
	{
		ULONGLONG _SessionID;
		WORD _Type;
		Packet* _Packet;
	};
}



#endif // !__JOB_HEADER__
